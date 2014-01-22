#ifndef EKIDEN_H_
#define EKIDEN_H_

#include <string.h>
#include <stackvars.h>
#include <registervars.h>
#include <ktlog.h>
#include <kitsunemisc.h>
#include <transform.h>

/**
 * \ingroup manual 
 * Type signature for a transformer function. If you manually * write
 * transformer functions, they must conform to this type signature, * returning
 * a true value on successful transformation.
 */
typedef int (*xform_fn_t)(void *);
typedef void (*state_xform_fn_t)(void);

void kitsune_register_lib(void *);
void kitsune_update(const char *);
char *kitsune_new_code(void);
int kitsune_setup_val(void *state, xform_fn_t xform_fn);
void *kitsune_get_val(const char *var_name);
void *kitsune_get_cur_val(const char *var_name);
int kitsune_old_address_matches(const char *name, void *ptr);
int kitsune_is_updating(void);
int kitsune_is_updating_from(const char *pt);
void kitsune_signal_update(void);
void kitsune_clear_request(void);
void kitsune_set_next_version(char *code);

char *kitsune_get_symbol_key(const char *name, const char *funcname, 
                            const char *filename, const char *namespace);

void *kitsune_get_symbol_addr_old(const char *name, const char *funcname, 
                                 const char *filename, const char *namespace);
void *kitsune_get_symbol_addr_new(const char *name, const char *funcname, 
                                 const char *filename, const char *namespace);

xform_fn_t kitsune_get_xform(const char *name, const char *func, 
                            const char *file, const char *namespace);

int kitsune_migrate_var(const char *var_name, const char *funcname,
                       const char *filename, const char *namespace,
                       void *var_addr, size_t var_size, xform_fn_t xform_fun);

void kitsune_note_var(const char *var_name, const char *funcname, 
                     const char *filename, const char *namespace,
                     void *addr, size_t size);

void kitsune_note_heap(const char* name, void* addr);

/**
 * \addtogroup manual
 * Macro that will define a function xform_func_ptr that will return the new
 * address of a function given it's old address.  The macro DEF_XFORM_FUNC_PTR
 * is given a list of function pointers that xform_func_ptr should consider.
 * The function is declared with name def_name.  To make the function static
 * preceed the macro with static.
 * 
 * This is only really useful if you aren't using xfgen or the Kitsune compiler.
 * @{
 */
#define DEF_XFORM_FUNC_PTR(def_name, ...)                               \
  void* def_name(void* func)                                            \
  {                                                                     \
    void* func_addrs[] = {__VA_ARGS__, NULL};                           \
    char all_func_names[] = #__VA_ARGS__;                               \
    char* func_name = strtok(all_func_names, ", ");                     \
    int i = 0;                                                          \
    for(; func_name != ((void *)0); i++, func_name = strtok(((void *)0), ", ")) \
      {                                                                 \
        if(kitsune_old_address_matches(func_name, func))                 \
          {                                                             \
            return(func_addrs[i]);                                      \
          }                                                             \
      }                                                                 \
    return func;                                                        \
  }

/**
 * Manually migrate a global variable.
 * 
 * If the variable has a transformer defined for it, call that transformer.
 * Otherwise, use memcpy to transfer the state from the old version. 
 * \return True if the variable was migrated.
 */
#define MIGRATE_GLOBAL(global)                                          \
  (kitsune_migrate_var(#global, 0, 0, 0, &global, sizeof(global),        \
                      kitsune_get_xform(#global, 0, 0, 0)))
/**
 * Manually migrate a local variable.
 * 
 * Since the Kitsune compiler doesn't insert instrumentation to automatically
 * migrate locals, to migrate local variables you MUST call this macro at the
 * point where the variable should be updated.
 */
#define MIGRATE_LOCAL(local)                                            \
  (kitsune_migrate_var(#local, __func__, 0, 0, &local, sizeof(local),    \
                      kitsune_get_xform(#local, __func__, 0, 0)))
 

/**
 * Manually migrate a static variable.
 * 
 * If the variable has a transformer defined for it, call that transformer.
 * Otherwise, use memcpy to transfer the state from the old version. 
 * \return True if the variable was migrated.
 */
#define MIGRATE_STATIC(var)                                             \
  (kitsune_migrate_var(#var, 0, __FILE__, 0, &var, sizeof(var),          \
                      kitsune_get_xform(#var, 0, __FILE__, 0)))

/**
 * Manually migrate a local variable.
 * 
 * Since the Kitsune compiler doesn't insert instrumentation to automatically
 * migrate locals, to migrate local variables you MUST call this macro at the
 * point where the variable should be updated.
 */
#define MIGRATE_LOCAL_STATIC(funcname, var)                             \
  (kitsune_migrate_var(#var, #funcname, __FILE__, 0, &var, sizeof(var),  \
                      kitsune_get_xform(#var, #funcname, __FILE__, 0)))
/**@}*/

/** 
 * \addtogroup internal
 * These macros are used below and also within the implementation of kitsune.c
 * to compute transformation function names 
 * @{
 */
#define XFORM_NAME_BASE _kitsune_transform_
#define NS_PREFIX ___ns___
#define FILE_PREFIX ___file___
#define FUNC_PREFIX ___func___
/**@}*/

/**
 * \defgroup manual Manual Kitsune
 * @{
 */
/**
 * If you choose to write transformation code entirely by hand, rather than use
 * the transformation generation (xfgen) tool, the following macros will be
 * useful.
 *
 * These macros allow you to name transformation functions so they will be
 * called when you use the MIGRATE_* macros.
 */
#define GLOBAL_XFORM(name) PASTE(XFORM_NAME_BASE,name)
/**
 * See GLOBAL_XFORM.
 */
#define LOCAL_XFORM(func, name) PASTE(PASTE(PASTE(XFORM_NAME_BASE,func),FUNC_PREFIX),name)
/**
 * See GLOBAL_XFORM.
 */
#define STATIC_XFORM(filename, name) PASTE(PASTE(PASTE(XFORM_NAME_BASE,filename),FILE_PREFIX),name)
/**
 * See GLOBAL_XFORM.
 */
#define LOCAL_STATIC_XFORM(filename, func, name)                        \
  PASTE(PASTE(PASTE(PASTE(PASTE(XFORM_NAME_BASE,filename),FILE_PREFIX),func),FUNC_PREFIX),name)
/**@}*/

/** \addtogroup manual 
 * @{
 */
/**
 * Retrieve the address of a global variable from the old version. This is
 * primarily useful in manually-written transformation code.
 */
#define GET_OLD_GLOBAL(name) \
  (kitsune_get_symbol_addr_old(#name, NULL, NULL, NULL))
/** See GET_OLD_GLOBAL. */
#define GET_OLD_LOCAL(funcname, name) \
  (kitsune_get_symbol_addr_old(#name, #funcname, NULL, NULL))
/** See GET_OLD_GLOBAL. */
#define GET_OLD_STATIC(filename, name) \
  (kitsune_get_symbol_addr_old(#name, NULL, #filename, NULL))
/** See GET_OLD_GLOBAL. */
#define GET_OLD_LOCAL_STATIC(filename, funcname, name)    \
  (kitsune_get_symbol_addr_old(#name, #funcname, #filename, NULL))

/**
 * Retrieve the address of a global variable from the new/current version. This
 * is primarily useful in manually-written transformation code.
 */
#define GET_NEW_GLOBAL(name) \
  (kitsune_get_symbol_addr_new(#name, NULL, NULL, NULL))
/** See GET_NEW_GLOBAL. */
#define GET_NEW_LOCAL(funcname, name) \
  (kitsune_get_symbol_addr_new(#name, #funcname, NULL, NULL))
/** See GET_NEW_GLOBAL. */
#define GET_NEW_STATIC(filename, name) \
  (kitsune_get_symbol_addr_new(#name, NULL, #filename, NULL))
/** See GET_NEW_GLOBAL. */
#define GET_NEW_LOCAL_STATIC(filename, funcname, name)    \
  (kitsune_get_symbol_addr_new(#name, #funcname, #filename, NULL))

/*
 * The following functions can be used to register variable names and addresses
 * with the runtime system.  It is only useful to call these functions in
 * programs that choose not to use the stackvars and globalreg phases of the
 * kitsune compiler (or choose not to use the compiler at all).
 */
/**
 * Call this macro at the start of a function to enable registering local
 * variables for migration. These calls are inserted if you use the Kitsune
 * compiler and the E_NOTELOCALS annotation, but must be inserted manually
 * otherwise.
 */ 
#define ENTER_FUNC() stackvars_note_entry(__func__)
/**
 * See ENTER_FUNC.
 */
#define EXIT_FUNC() stackvars_note_exit(__func__)

/**
 * Register a local variable for migration with the Kitsune runtime. This will
 * be automatically inserted for any function with the E_NOTELOCALS annotation
 * if you use the Kitsune compiler, but must be invoked manually otherwise.
 */ 
#define NOTE_LOCAL(local)                             \
  stackvars_note_local(#local, &local, sizeof(local))
/**
 * Similar to NOTE_LOCAL, but also calls MIGRATE_LOCAL on the variable.
 */ 
#define NOTE_AND_MIGRATE_LOCAL(local)           \
  (NOTE_LOCAL(local), MIGRATE_LOCAL(local))

/**
 * See NOTE_LOCAL.
 */
#define NOTE_STATIC(var)                              \
  (kitsune_note_var(#var, NULL, __FILE__, NULL, &var, sizeof(var)))
/**
 * See NOTE_AND_MIGRATE_LOCAL.
 */
#define NOTE_LOCAL_STATIC(var)                            \
  (kitsune_note_var(#var, __func__, __FILE__, NULL, &var, sizeof(var)))

/**
 * See NOTE_LOCAL.
 */
#define NOTE_AND_MIGRATE_STATIC(var)            \
  (NOTE_STATIC(var), MIGRATE_STATIC(var))
/**
 * See NOTE_AND_MIGRATE_LOCAL.
 */
#define NOTE_AND_MIGRATE_LOCAL_STATIC(func, var)       \
  (NOTE_LOCAL_STATIC(var), MIGRATE_LOCAL_STATIC(func, var))
/**@}*/

#endif

/**
 * This macro is used to register and manipulate heap addresses. The primary use
 * for this is heap allocated memory that is passed to an third-party library
 * that needs to be transformed. This is a rare occurence.
 */
#define NOTE_HEAP(heap, addr)                   \
  (kitsune_heaplist_add(heap, addr))

/**
 * Deregister an opaque heap address for transformation upon an update. The
 * primary use for this is heap allocated memory that is passed to a third-party
 * library that must be transformed in some way.
 */ 
#define UNNOTE_HEAP(heap, addr)                 \
  (kitsune_heaplist_del(heap, addr))
