
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "uthash.h"

#include "kitsune_internal.h"
#include "addresscheck_internal.h"
#include "ktthreads_internal.h"
#include "vmareas_internal.h"
#include "bench_internal.h"
#include "alloctrack_internal.h"

typedef struct
{
	char *old_name;
	char *new_name;
	UT_hash_handle hh;
} rename_hash_entry;

rename_hash_entry* rename_hash = NULL;

static void transform_perform_free(void * old) {
//  kitsune_log("performing free");
  vmarea *area = vmareas_lookup(old);
  alarea *alint = alloctrack_lookup(old);
  if(alint){
    kitsune_free(old);
    return;
  }
  if (vmareas_get_type(area) == HEAP) {
    free(old);
  } else {
    char *printable = vmareas_to_str(area);
    kitsune_log("free for non-heap pointer skipped (%s)", printable);
    free(printable); 
  }
}

void transform_register_renaming(const char *old_key, const char *new_key)
{
#ifdef ENABLE_THREADING
  //assert(ktthread_is_main());
#endif
  rename_hash_entry *r;
  r = malloc(sizeof(rename_hash_entry));
  r->old_name = strdup(old_key);
  r->new_name = strdup(new_key);
  HASH_ADD_KEYPTR(hh, rename_hash, r->old_name, strlen(r->old_name), r);
}

char *transform_mapped_name(const char *old_key)
{
  rename_hash_entry *r;
  HASH_FIND_STR(rename_hash, old_key, r);
  if (r) 
    return r->new_name;
  else
    return NULL;
}

struct closure {
  xf f;
  int deep_copy;
  size_t size_old;
  size_t size_new;
  int nargs;
  void **args;
};

struct closure_list {
  closure *c;
  struct closure_list *next;
} *allocated_closures = NULL;

void transform_init(void) {
  vmareas_init();
}

void delete_hm_entries();
void transform_free(void) {
#ifdef ENABLE_THREADING
  ktthread_lock();
#endif
  struct closure_list *next, *cur = allocated_closures;
  while (cur) {
    next = cur->next;
    free(cur->c->args);
    free(cur->c);
    free(cur);
    cur = next;
  }
  allocated_closures = NULL;

  delete_hm_entries();

  /* release all the memory freed during transformation */
  vmareas_free();

#ifdef ENABLE_THREADING
  ktthread_unlock();
#endif
}

closure *transform_make_closure(xf f, copy_opt copy_opt, 
                                size_t size_old, size_t size_new,
                                int nargs, ...) {

  void **arg_list = malloc(sizeof(void *) * nargs);
  va_list argp;
  int i;

  va_start(argp, nargs);
  for(i=0; i<nargs; i++)
    arg_list[i] = va_arg(argp, void *);
  va_end(argp);

  closure *c = malloc(sizeof(closure));
  c->f = f;
  c->size_old = size_old;
  c->size_new = size_new;
  c->nargs = nargs;
  c->args = arg_list;

  /* if the copy method is set to XF_TARGET, then we need to look inside
     the argument closure to determine whether we need to copy */
  if (copy_opt == XF_TARGET) {
    assert(c->nargs > 0);
    closure *target_xf = c->args[c->nargs-1];
    c->deep_copy = target_xf->deep_copy;
  } else {
    c->deep_copy = (copy_opt == XF_DEEP);
  }
  
  struct closure_list *new_top = malloc(sizeof(struct closure_list));
  new_top->c = c;

  /* Accesses to shared kitsune resources requires mutual exclusion */
#ifdef ENABLE_THREADING
  ktthread_lock();
#endif
  new_top->next = allocated_closures;
  allocated_closures = new_top;  
#ifdef ENABLE_THREADING
  ktthread_unlock();
#endif
  return c;
}

void transform_invoke_closure(closure *c, void *in, void *out) {
  c->f(in, out, c->nargs, c->args);
}

typedef struct
{
	void* key;
	void* addr;
	UT_hash_handle hh;
} xform_mapping_entry;

xform_mapping_entry *xform_mappings_head = NULL;

void delete_hm_entries(){
  /*delete hm entries!*/
  xform_mapping_entry *current, *tmp;
    HASH_ITER(hh, xform_mappings_head, current, tmp) {
    HASH_DEL(xform_mappings_head, current); /* delete; users advances to next */
    free(current); /* optional- if you want to free */
  }
}

void *transform_find_mapping(void *from) {
  xform_mapping_entry *entry;	
#ifdef ENABLE_THREADING
  ktthread_lock();
#endif
	HASH_FIND_PTR(xform_mappings_head, &from, entry);	
#ifdef ENABLE_THREADING
  ktthread_unlock();
#endif
	if (entry)
		return entry->addr;
	else
		return NULL;
}

void transform_add_mapping(void *from, void *to) {
  xform_mapping_entry* new_entry = malloc(sizeof(xform_mapping_entry));
	new_entry->key = from;
	new_entry->addr = to;
#ifdef ENABLE_THREADING
  ktthread_lock();
#endif
	HASH_ADD_PTR(xform_mappings_head, key, new_entry);
#ifdef ENABLE_THREADING
  ktthread_unlock();
#endif
}

void transform_ptr(void *in, void *out, int num_gen_args, void **args) {
  assert(num_gen_args == 1);

  if (*(void **)in == 0) {
    *(void **)out = 0;
    return;
  }

  closure *target_xf = args[0];

  void *lookup;
  if ((lookup = transform_find_mapping(*(void **)in))) {
    *(void **)out = lookup;
  } else {
#ifdef ENABLE_DEBUG
    /* Check to make sure the pointer that we're dealing with does not overlap
       with some other pointer that we've seen */
    addresscheck(NULL, *(void **)in, target_xf->size_old);
#endif

    char *symbol;
    void *in_elem = *(void **)in;
    void *out_elem = NULL;
    int needtofree = 0;
    if ((symbol = kitsune_lookup_addr_old(in_elem))) {
      kitsune_log("transform_ptr: pointer to non-heap data found [%s]", symbol);

      if (!target_xf->deep_copy) {
        kitsune_log("WARN: transform_ptr: shallow copy requested for non-heap data.");
      }

      char *mapped_symbol = transform_mapped_name(symbol);
      if (mapped_symbol) {
        kitsune_log("transform_ptr: symbol [%s] was mapped to [%s]", symbol, mapped_symbol);
        symbol = mapped_symbol;
      }
      lookup = kitsune_lookup_key_new(symbol);
      kitsune_assert(lookup, "transform_ptr: no mapping found for %s\n", symbol);
      out_elem = lookup;
    } else {
      if (target_xf->deep_copy) {
        bench_xform_alloc(((closure *)args[0])->size_new);
        out_elem = malloc(((closure *)args[0])->size_new);
        needtofree=1;
      } else { /* use the same memory */
        out_elem = in_elem;
      }
    }
    *(void **)out = out_elem;
    transform_add_mapping(in_elem, out_elem);
    XF_INVOKE(target_xf, in_elem, out_elem);
    if(needtofree){
      transform_perform_free(in_elem);
    }
  }
}

void transform_fptr(void *in, void *out, int num_gen_args, void **args) {
  assert(num_gen_args == 0);

  void *in_elem = *(void **)in;
  void *lookup;
  if (!in_elem) {
    *(void **)out = 0;
  } else if ((lookup = transform_find_mapping(in_elem))) {
    *(void **)out = lookup;
  } else {
    char *symbol;
    if ((symbol = kitsune_lookup_addr_old(in_elem))) {
      kitsune_log("transform_fptr: pointer to non-heap data found [%s]", symbol);

      char *mapped_symbol = transform_mapped_name(symbol);
      if (mapped_symbol) {
        kitsune_log("transform_fptr: symbol [%s] was mapped to [%s]", symbol, mapped_symbol);
        symbol = mapped_symbol;
      }

      kitsune_assert((lookup = kitsune_lookup_key_new(symbol)), 
                     "transform_fptr: no mapping found for %s\n", symbol);
      kitsune_log("transform_fptr: mapping address %p", lookup);
      *(void **)out = lookup;
      kitsune_log("transform_fptr: mapping %p -> %p", in_elem, lookup);
      transform_add_mapping(in_elem, lookup);
    } else {
      kitsune_log("transform_fptr: could not find function corresponding to address %x\n", in_elem);
      assert(0);
    }
  }
}

void transform_array(void *in, void *out, int num_gen_args, void **args) {
  assert(num_gen_args == 4);

  size_t count = (size_t)args[0];
  size_t sz_in = (size_t)args[1];
  size_t sz_out = (size_t)args[2];
  
  char *in_ptr = (char *)in;
  char *out_ptr = (char *)out;

  int i;
  for(i=0; i<count; i++, in_ptr += sz_in, out_ptr += sz_out) {
    XF_INVOKE(args[3], in_ptr, out_ptr);
  }
}

int check_all_zero(char *start, size_t size) {
  assert(start != NULL);
  int i, c = size / sizeof(char);
  for(i=0; i<c; i++, start++)
    if (*start != 0)
      return 0;
  return 1;
}

void transform_ntarray(void *in, void *out, int num_args, void **args) {
  assert(num_args == 3);
  size_t sz_in = (size_t)args[0];
  size_t sz_out = (size_t)args[1];
  
  assert(sz_in % sizeof(char) == 0);

  char *in_ptr = *(char **)in;
  if (in_ptr == NULL) {
    *(char **)out = NULL;
    return;
  }

  /* General way to produce the number of elements encountered before
     a "zero" element is encountered. */
  int count = 1; /* start at 1 to include the null elem */
  while (!check_all_zero(in_ptr, sz_in)) {
    count++;
    in_ptr += sz_in;
  }
  XF_INVOKE(XF_PTR(XF_ARRAY(count, sz_in, sz_out, args[2])), in, out);
}

void transform_raw(void *in, void *out, int num_gen_args, void **args) {
  assert(num_gen_args == 1);
  size_t sz = (size_t)args[0];
  if (in != out)
    memcpy(out, in, sz);
}
