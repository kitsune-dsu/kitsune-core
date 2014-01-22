
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "uthash.h"

#include "kitsune_internal.h"

#include "registervars_internal.h"
#include "ktthreads_internal.h"

/* At the time the generated registration constructors are called, the kitsune
   library will not yet be prepared to lookup the appropriate transformers for
   auto_migrate variables.  This list queues up those registration requests. */
typedef struct loading_register_node {
  const char *var_name;
  const char *funcname;
  const char *filename; 
  const char *namespace;
  void* var_addr; 
  size_t size;
  int auto_migrate;  
  struct loading_register_node *next;
} loading_register_node;
loading_register_node *register_queue = NULL;

typedef struct
{
  char *name;
  void *addr;
  size_t size;
  int auto_migrate;
  xform_fn_t xf_fn;
  UT_hash_handle hh_name;
  UT_hash_handle hh_addr;
  const char *var_name; 
  const char *funcname; 
  const char *filename;  
  const char *namespace;
} hash_entry;

hash_entry* name_to_addr_hash = NULL;
hash_entry* addr_to_name_hash = NULL;

hash_entry* old_name_to_addr_hash = NULL;
hash_entry* old_addr_to_name_hash = NULL;

/**
 * \ingroup internal
 * puts the address into the new hash address hash table 
 */
static void kitsune_register_key(const char *key, void* var_addr, 
                                 size_t size, int auto_migrate,
                                 const char *var_name, const char *funcname, 
                                 const char *filename, const char *namespace)
{
	hash_entry* exists_key;
	hash_entry* exists_addr;

	HASH_FIND(hh_addr, addr_to_name_hash, &var_addr, sizeof(var_addr), exists_key);
	HASH_FIND(hh_name, name_to_addr_hash, key, strlen(key), exists_addr);
	
	if(exists_key || exists_addr) {
		/* For existing registrations, ensure that forward and backward mappings
       exist and match. */
    kitsune_assert(exists_key, 
                   "Address[%p] but not key[%s] found when registering var.\n", 
                   var_addr,
                   key);
    kitsune_assert(exists_addr,
                   "Key[%s] but not address[%p] found when registering var.\n",
                   key,
                   var_addr);
	  assert(exists_addr);
	} else {
    /* Allocate a new entry and add it to the forward and backward maps */
      hash_entry* new_entry = malloc(sizeof(hash_entry));
      new_entry->name = strdup(key);
      new_entry->addr = var_addr;
      new_entry->size = size;
      new_entry->auto_migrate = auto_migrate;
      new_entry->var_name = var_name; 
      new_entry->funcname = funcname; 
      new_entry->filename = filename;  
      new_entry->namespace = namespace;
      HASH_ADD_KEYPTR(hh_name, name_to_addr_hash, new_entry->name, strlen(new_entry->name), new_entry);
      HASH_ADD(hh_addr, addr_to_name_hash, addr, sizeof(var_addr), new_entry);
  }
}

/**
 * \ingroup internal
 * 
 * Register a variable with the Kitsune runtime symbol table.
 * 
 * This function is mostly called from compiler-generated code in shared-library
 * constructor functions. 
 */ 
void kitsune_register_var(const char *var_name, const char *funcname, 
                         const char *filename, const char *namespace,
                         void* var_addr, size_t size, int auto_migrate) 
{
#ifdef ENABLE_THREADING
  ktthread_lock();
#endif  
  char *key = kitsune_get_symbol_key(var_name, funcname, filename, namespace);
  kitsune_register_key(key, var_addr, size, auto_migrate, var_name, funcname, filename, namespace);
  free(key);
#ifdef ENABLE_THREADING
  ktthread_unlock();
#endif
}


/**
 * \ingroup public
 * Initiate automigration of all auto-migration-registered variables.
 * 
 * By default, the Kitsune compiler registers all global variables for
 * automigration. This function iterates over these variables, calls any
 * relevant transformation code, and migrates the old state into the
 * corresponding new version variables. When it returns, all variables marked
 * for automigration will contain their old-version values.
 */ 
void kitsune_do_automigrate(void) {
	hash_entry* cur;
	hash_entry* tmp;
    xform_fn_t xf = NULL;

#ifdef ENABLE_THREADING
  assert(ktthread_is_main());
  ktthread_lock();
#endif
  if (kitsune_is_updating()) {
    HASH_ITER(hh_addr, name_to_addr_hash, cur, tmp)	{
      if (cur->auto_migrate){
        xf = kitsune_get_xform(cur->var_name, cur->funcname, cur->filename, cur->namespace);
        kitsune_automigrate_key(cur->name, cur->addr, cur->size, xf);
      }
    }
	}
#ifdef ENABLE_THREADING
  ktthread_unlock();
#endif
}


/**
 * \ingroup internal
 * Gets an address from the old hash address hash table.
 */ 
void* kitsune_lookup_key_old(const char *key)
{
	hash_entry *entry;	
#ifdef ENABLE_THREADING
  ktthread_lock();
#endif
	HASH_FIND(hh_name, old_name_to_addr_hash, key, strlen(key), entry);
#ifdef ENABLE_THREADING
  ktthread_unlock();
#endif
	if (entry)
		return entry->addr;
	else
		return NULL;
}

/**
 * \ingroup internal
 *  Gets an address from the new hash address hash table.
 */
void* kitsune_lookup_key_new(const char *key)
{
	hash_entry *entry;
#ifdef ENABLE_THREADING
  ktthread_lock();
#endif
	HASH_FIND(hh_name, name_to_addr_hash, key, strlen(key), entry);
#ifdef ENABLE_THREADING
  ktthread_unlock();
#endif
	if(entry)
		return entry->addr;
	else
		return NULL;
}

/**
 * \ingroup internal 
 * 
 * Given the old address of a variable, look up the key it was registered under.
 * 
 */ 
char *kitsune_lookup_addr_old(void *addr) {
	hash_entry *entry;	
#ifdef ENABLE_THREADING
  ktthread_lock();
#endif
	HASH_FIND(hh_addr, old_addr_to_name_hash, &addr, sizeof(addr), entry);	
#ifdef ENABLE_THREADING
  ktthread_unlock();
#endif
	if(entry)
		return entry->name;
	else
		return NULL;
}

/**
 * \ingroup internal
 * 
 * Given the new/current version address, return the key the address is mapped
 * to in the Kitsune internal symbol table.
 */
char *kitsune_lookup_addr_new(void *addr) {
	hash_entry *entry;	
#ifdef ENABLE_THREADING
  ktthread_lock();
#endif
	HASH_FIND(hh_addr, addr_to_name_hash, &addr, sizeof(addr), entry);	
#ifdef ENABLE_THREADING
  ktthread_unlock();
#endif
	if(entry)
		return entry->name;
	else
		return NULL;
}

/**
 * \ingroup internal
 * 
 * Delete all entries in the Kitsune symbol table.
 */ 
void registervars_free(void) 
{
	hash_entry* cur;
	hash_entry* tmp;

#ifdef ENABLE_THREADING
  ktthread_lock();
#endif
	HASH_ITER(hh_name, old_name_to_addr_hash, cur, tmp) {
		HASH_DELETE(hh_name, old_name_to_addr_hash, cur);		
	}
	HASH_ITER(hh_addr, old_addr_to_name_hash, cur, tmp)	{
		HASH_DELETE(hh_addr, old_addr_to_name_hash, cur);
		free(cur->name);
		free(cur);
	}
	old_name_to_addr_hash = old_addr_to_name_hash = NULL;
#ifdef ENABLE_THREADING
  ktthread_unlock();
#endif
}

//remember the old hash from the previous version
/**
 * \ingroup internal
 * 
 * Called the initialization of the Kitsune runtime in the udpated version, this
 * function copies over the symbol table constructed by the previous version.
 * 
 */
void registervars_migrate(void)
{
#ifdef ENABLE_THREADING
  ktthread_lock();
#endif

  hash_entry** lookup_name_hash = kitsune_get_val("name_to_addr_hash");
  hash_entry** lookup_addr_hash = kitsune_get_val("addr_to_name_hash");
  
  assert(lookup_name_hash && lookup_addr_hash);
  
  old_name_to_addr_hash = *lookup_name_hash;
  old_addr_to_name_hash = *lookup_addr_hash;
    
#ifdef ENABLE_THREADING
  ktthread_unlock();
#endif
}
