

/*
 * Each version of a program that is prepared for updating using in-place
 * kitsune will link in this source file. It contains the "entry point" to the
 * new program version as well the API functions used to initiate an update and
 * also access values from the previous program version.
 */

#include <setjmp.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <dlfcn.h>
#include <string.h>
#include <search.h>

#include "kitsune_internal.h"
#include "stackvars_internal.h"
#include "registervars_internal.h"
#include "log_internal.h"
#include "addresscheck_internal.h"
#include "transform_internal.h"
#include "bench_internal.h"
#include "alloctrack_internal.h"

#ifdef ENABLE_THREADING
#include "ktthreads_internal.h"
#endif

/* 
 * Kitsune requires that the shared library containing the new program version
 * to include a "main" function that is its entry point into non-kitsune code.
 */
extern int main(int, char**);

/*
 * Global Variables
 * ================
 */
/**
 * \defgroup internal Kitsune Internal API
 * @{
 */
/**
 * jmp_env is a pointer to the jmp_buf initialized using setjmp in the driver.c
 * code.  We will use it to longjmp back to driver.c when an update is
 * requested.
 */
jmp_buf *jmp_env = NULL;

/**
 * prev_ver_handle is the handle returned by dlopen when the previous version of
 * the program was opened. cur_ver_handle is the handle returned by dlopen when
 * the current version of the program was opened. We make it available here in
 * the current version so that we can access the global variables and function
 * addresses from the previous version.
 */
void *prev_ver_handle =  NULL;
void *cur_ver_handle =  NULL;

/**
 * Flag set while the version is loaded, but unset as soon as execution 
 * enters the code in the version's library.
 */
static int is_loading = 1;

/**
 * update_pt is assigned the string name of an update point when an update
 * begins.  The next version of the program can access the value of this
 * variable to learn the name of the update point point that was taken.
 */
const char *update_pt;

/**
 * flag set when an update has been requested
 */
volatile sig_atomic_t update_requested = 0;

/**
 * The filename to contain the updated code.  Driver will ordinarily load this
 * from a file on the filesystem, but this field can be set to override that.
 */
char **next_version_code = NULL;

/**
 * True if kitsune has updated.
 */
int kitsune_has_updated_p = 0;

/*
 * Entry Point
 * ===========
 */

/**
 * After driver.c loads a new version of a program as a shared library, it
 * accesses the symbol "kitsune_init_inplace" from the library and calls it,
 * passing in:
 *
 * 1. env - the jmp_buf to use with longjmp when updating
 * 2. prev_handle - the handle to the previous version shared library
 * 4. argc/argv - the arguments to pass on to main
 */
int kitsune_init_inplace(jmp_buf *env, void *prev_handle, void *cur_handle, 
                        char **next_code, const char *bench_filename,
                        int argc, char **argv)
{
  /* We've beguin executing code in this version. */
  is_loading = 0;

  jmp_env = env;
  prev_ver_handle = prev_handle;
  cur_ver_handle = cur_handle;
  next_version_code = next_code;

  bench_init(bench_filename);

#ifdef ENABLE_THREADING
  ktthread_init();
#endif

  /*
   * If the handle to the previous version was NULL, we infer that we are the
   * first version starting up.
   */
  if (kitsune_is_updating()) {

    kitsune_has_updated_p = 1;

#ifdef ENABLE_THREADING
    /*
     * Wait for all child threads to reach update points (or terminate)
     */
    ktthread_main_wait();
    bench_quiesce_finish();
#endif
    
    /* 
     * Setup handle to the old version's stack variables that were captured from
     * the stackvars API/compiler.
     */
    stackvars_flip();

    addresscheck_init();

    transform_init();
    
    /*
     * Get the pointer to the saved static variables.
     */
    registervars_migrate();
  }
  
  /*
   * Initialize the log.
   */
  if(!kitsune_logging_init(argv[0])) {
    printf("Couldn't initialize logging!\n");
    abort();
  }

  /* initialize the memory allocation tracker tree*/
  alloctrack_init();

  /*
   * We may wish to perform some initialization (e.g., altering the set of
   * threads or the argc/argv arguments to the program) before entering main().
   * Here, we call such a transformer if it exists.
   */
  if (kitsune_is_updating()) {
    state_xform_fn_t ps_fn = kitsune_get_cur_val("_kitsune_prestart_xform");
    if (ps_fn) {
      kitsune_log("Calling prestart transformation function.");
      ps_fn();
    }
  }

  /*
   * After saving the information passed from the driver, we invoke the main
   * function of current version shared library.
   */
  bench_restart_start();
  kitsune_log("Entering target program: %s\n", argv[0]);
  return main(argc, argv);
}
/** @} 
 * ends doxygen group internal
 */

/*
 * Updating
 * ========
 */

/**
 * \defgroup public Kitsune API
 * 
 * Calls to kitsune_update indicate that a possible update point has been
 * reached.  The argument "pt_name" provides an identifier indicating which
 * point was reached.
 * @{
 */
void kitsune_update(const char *pt_name)
{
  /*
   * If we were starting up following an update from a previous version (and so
   * the updating flag is set) the fact that we have reached an update point
   * indicates that we are done starting up and now resume regular execution.
   */
  if (kitsune_is_updating()) {

    /*
     * If we're starting up following an update, but we're not at the update
     * point corresponding to the long-running loop that we need to reach, then
     * we just return.  This allows reaching intermediate upate points on the
     * way to the final point.
     */
    if (!kitsune_is_updating_from(pt_name))
      return;
    
    /*
     * We may wish to perform some initialization (e.g., altering the set of
     * threads) after we reach the main thread's update point, but before any
     * child threads are created.
     */
#ifdef ENABLE_THREADING
    if (ktthread_is_main()) {
#endif
      state_xform_fn_t mu_fn = kitsune_get_cur_val("_kitsune_mainupdate_xform");
      if (mu_fn) {
        kitsune_log("Calling main-update transformation function.");
        mu_fn();
      }
#ifdef ENABLE_THREADING
    }
#endif

#ifdef ENABLE_THREADING
    if (ktthread_is_main()) {
      
      /* Now that we've reached the main loop of the main thread, we can kick
         off execution of the child threads and then wait until they reach their
         main loops (or die) */
      ktthread_launch_wait();
#endif
      
      /*
       * Free any memory used to store the old versions stack variables (those
       * managed through the stackvars API).
       */
      kitsune_log("before freeing....");
      bench_log_resource_usage();
      stackvars_free();
      registervars_free();
      addresscheck_free();
      
      /*
       * Since we're done accessing values from the previous version, we now close
       * our handle to its shared library which makes its state inaccessible and
       * unloads its code.
       */
      if (dlclose(prev_ver_handle)) {
        kitsune_log("dlclose: error occurred: (%s)\n", dlerror());
        exit(1);
      }

      prev_ver_handle = NULL;
      transform_free();

      bench_finish();
      kitsune_log("teardown complete....");
      bench_log_resource_usage();

#ifdef ENABLE_THREADING
      /* Signal threads to continue running */
      ktthread_main_finish_update();
    } else {
      stackvars_free();
      ktthread_finish_update();
    }
#endif
  }
  
  /*
   * Check whether an update is available.
   */
  if (update_requested) {
    kitsune_log("Updating(%s)...\n", pt_name);

    bench_log_resource_usage();
    bench_start();

    /*
     * Move current stack variables (managed through the stackvars API) from the
     * stack to the heap.
     */
    stackvars_move_to_heap();
    
#ifdef ENABLE_THREADING
    ktthread_rapidq();
    if (ktthread_is_main()) {
#endif
      /*
       * To update, we store the current update point taken to make it available
       * to the next version 
       */
      update_pt = pt_name;
      
      /* 
       * And then longjmp back to the driver code.
       */
      assert(jmp_env != NULL);
      longjmp(*jmp_env, 1);
#ifdef ENABLE_THREADING
    } else {
      ktthread_do_update(pt_name);
    }
#endif
  }
}

/*
 * Starting-up Following an Update
 * ===============================
 */

/**
 * kitsune_is_updating is used by user code to check whether we are starting up
 * following an update (vs. starting up from scratch).
 */
int kitsune_is_updating(void)
{
  return prev_ver_handle != NULL;
}
/** @} 
 * Closes doxygen group public.
 */

/**
 * \ingroup internal
 * Kitsune-internal function. See documentation for is_loading.
 */
int kitsune_is_loading(void)
{
  return is_loading;
}

/**
 * \ingroup public
 * kitsune_has_updated returns true if an update has occurred at some point in
 * the past. This was useful at least once for debugging.
 */ 
int kitsune_has_updated(void)
{
  return kitsune_has_updated_p;
}


/** 
 * \ingroup manual
 * 
 * kitsune_get_val is used to access the values of global variables from the
 * previous version of the program by name (var_name).  This will typically be
 * called from within a tranformation function.
 */
void *kitsune_get_val(const char *var_name) 
{
  assert(prev_ver_handle != NULL);
  void *var_ptr = dlsym(prev_ver_handle, var_name);
  return var_ptr;
}


/**
 * \ingroup manual
 * kitsune_get_cur_val is just like kitsune_get_val, except is uses an
 * up-to-date version of the symbol table instead of the previous version's
 * symbol table.
 */
void *kitsune_get_cur_val(const char *var_name)
{
  kitsune_log("GETTING %s\n", var_name);
  assert(cur_ver_handle != NULL);
  void *var_ptr = dlsym(cur_ver_handle, var_name);
  return var_ptr;
}

static int is_identifier_char(char c) 
{
  return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9');
}

/**
 * \ingroup internal
 * kitsune_get_xform returns a transformer function for a variable named "name",
 * with optional prefix "prefix". If you don't need a prefix, pass NULL as the
 * prefix. If the transormer function doesn't exist, it returns NULL. So far,
 * this is only used in the migrate_global/local/static functions. See
 * kitsune_get_global_xform and kitsune_get_local_xform for usage examples.
 */
xform_fn_t kitsune_get_xform(const char *name, const char *func, const char *file, const char *namespace)
{
#define XFORM_NAME_BASE_S STRINGIFY(XFORM_NAME_BASE)
#define NS_PREFIX_S STRINGIFY(NS_PREFIX)
#define FILE_PREFIX_S STRINGIFY(FILE_PREFIX)
#define FUNC_PREFIX_S STRINGIFY(FUNC_PREFIX)
  
  void *result = NULL;
  if (kitsune_is_updating()) {
    
    int len = strlen(XFORM_NAME_BASE_S);
    if (namespace)
      len += strlen(NS_PREFIX_S) + strlen(namespace);
    if (file)
      len += strlen(FILE_PREFIX_S) + strlen(file);
    if (func)
      len += strlen(FUNC_PREFIX_S) + strlen(func);
    assert(name);
    len += strlen(name);
    
    char *buffer = malloc(sizeof(char) * (len + 1));
    strcpy(buffer, XFORM_NAME_BASE_S);
    if (namespace) {
      char *tmp_namespace = strdup(namespace);
      int i;
      for(i=0;i<strlen(namespace);i++) {
        if (!is_identifier_char(tmp_namespace[i]))
          tmp_namespace[i] = '_';
      }
      strcat(buffer, tmp_namespace);
      free(tmp_namespace);
      strcat(buffer, NS_PREFIX_S);
    }    
    if (file) {
      char *tmp_file = strdup(file);
      int i;
      for(i=0;i<strlen(file);i++) {
        if (!is_identifier_char(tmp_file[i]))
          tmp_file[i] = '_';
      }
      strcat(buffer, tmp_file);
      free(tmp_file);
      strcat(buffer, FILE_PREFIX_S);
    }
    if (func) {
      strcat(buffer, func);
      strcat(buffer, FUNC_PREFIX_S);
    }
    strcat(buffer, name);
    result = kitsune_get_cur_val(buffer);
    free(buffer);
  }
  return result;
}

/**
 * \ingroup public
 * The following function defines the "key" used to lookup the address of a
 * program symbol. The particular patterns used are:
 *
 *    "symb_name" (for non-statics in the default namespace)
 *    "namespace@symb_name" (for non-statics in a non-default namespace)
 *    "filename/symb_name" (for statics in the default namespace)
 *    "namespace@filename/symb_name" (for statics in a non-default namespace)
 *    "filename/functionname#symb_name" (for function scoped statics in the default namespace)
 *    "namespace@filename/functionname#symb_name" (for function scoped statics in a non-default namespace)
 *
 * NOTE: These patterns and the logic to compute them are duplicated in the
 * kitsune compiler code.
 */
char *kitsune_get_symbol_key(const char *name, const char *funcname, 
                            const char *filename, const char *namespace)
{
  int len = strlen(name);
  if (namespace)
    len += strlen(namespace) + 1;
  if (filename)
    len += strlen(filename) + 1;
  if (funcname) 
    len += strlen(funcname) + 1;

  char *result = malloc((len + 1) * sizeof(char));

  strcpy(result, "");
  if (namespace) {
    strcat(result, namespace);
    strcat(result, "@");
  }
  if (filename) {
    strcat(result, filename);
    strcat(result, "/");
  }
  if (funcname) {
    strcat(result, funcname);
    strcat(result, "#");
  }
  strcat(result, name);
  return result;
}

/**
 * \ingroup public
 * Given the components of the lookup key for a symbol, retrieve the address of
 * the old-version of the symbol using either Kitsune's internal symbol table or
 * kitsune_get_val.
 * 
 * \param name the name of the symbol 
 * \param funcname the name of the function the symbol appears in, if the
 * symbol is local
 * \param filename the name of the file the symbol appears in, if the symbol is
 * static
 * \param namespace a user-provided namespace for the symbol
 * \return The address of the symbol in the previous version.
 */
void *kitsune_get_symbol_addr_old(const char *name, const char *funcname, 
                                 const char *filename, const char *namespace)
{
  if (funcname && !filename) { 
    /* If we're retreiving a non-static local, use the stackvars module */
    return stackvars_get_local(funcname, name);
  } else {
    /* Otherwise, return the symbol from the registration table */
    char *key = kitsune_get_symbol_key(name, funcname, filename, namespace);
    void *result = kitsune_lookup_key_old(key);
    free(key);

    /* If we didn't find a global variable in the symbol table, that
       might be due to a program not using the kitsune compiler.  In that case,
       we should looking the variable in the dynamic library symbol table */
    if (!result && !funcname && !filename && !namespace) {
      result = kitsune_get_val(name);
    }
    return result;
  }
}

/**
 * \ingroup public
 * Given the components of the lookup key for a symbol, retrieve the address of
 * the new-version symbol using either the Kitsune symbol table or
 * kitsune_get_cur_val.
 * 
 * \param name the name of the symbol 
 * \param funcname the name of the function the symbol appears in, if the
 * symbol is local
 * \param filename the name of the file the symbol appears in, if the symbol is
 * static
 * \param namespace a user-provided namespace for the symbol
 * \return The address of the symbol in the current or new version.
 */
void *kitsune_get_symbol_addr_new(const char *name, const char *funcname, 
                                 const char *filename, const char *namespace)
{
  if (funcname && !filename) { 
    /* If we're retreiving a non-static local, use the stackvars module */
    return stackvars_get_local_new(funcname, name);
  } else {
    /* Otherwise, return the symbol from the registration table */
    char *key = kitsune_get_symbol_key(name, funcname, filename, namespace);
    void *result = kitsune_lookup_key_new(key);
    free(key);

    /* If we didn't find a global variable in the symbol table, that
       might be due to a program not using the kitsune compiler.  In that case,
       we should looking the variable in the dynamic library symbol table */
    if (!result && !funcname && !filename && !namespace) {
      result = kitsune_get_cur_val(name);
    }
    return result;
  }
}

/**
 * \ingroup internal
 * Register a variable with the Kitsune runtime. 
 * 
 * \param var_name the name of the variable
 * \param funcname the name of the function in which the variable is declared,
 * if the variable is local
 * \param filename the name of the file in which the variable is declared, if
 * the variable is static
 * \param namespace a user-defined namespace
 * \param addr the address of the variable
 * \param size the size of the variable
 */
void kitsune_note_var(const char *var_name, const char *funcname, 
                     const char *filename, const char *namespace,
                     void *addr, size_t size) 
{
  kitsune_register_var(var_name, funcname, filename, namespace, addr, size, 0);
}

/**
 * \ingroup internal
 * Migrate the previous value of a variable forward into a new variable,
 * var_addr, using kitsune_get_symbol_addr_old to retrieve the previous state.
 * If no transformer is provided, just memcpy var_size bytes from the old
 * variable's address into var_addr.
 * 
 * \param var_name the name of the variable
 * \param funcname the name of the function in which the variable is declared,
 * if the variable is local
 * \param filename the name of the file in which the variable is declared, if
 * the variable is static
 * \param namespace a user-defined namespace
 * \param var_addr the address of the variable in the new versoin
 * \param var_size the size of the variable in the new version
 * \param xform_fun a transformer function that provides a mapping between the
 * old and new version's state representation
 */
int kitsune_migrate_var(const char *var_name, const char *funcname,
                       const char *filename, const char *namespace,
                       void *var_addr, size_t var_size, xform_fn_t xform_fun)
{
  if (kitsune_is_updating()) {
    if (xform_fun) {
      return xform_fun(var_addr);
    } else {
      /* TODO: possible bug: this should be looking up the mappings between
         versions!  weirdly, tests for this are working - I suspect because they
         create transformation functions. */
      void *old_var = kitsune_get_symbol_addr_old(var_name, funcname, filename, namespace);
      if (!old_var) {
        return 0;
      }
      memcpy(var_addr, old_var, var_size);
    }
    return 1;
  }
  return 0;
}

/**
 * \ingroup internal
 * Automatically migrate a variable.
 * 
 * This functin is called from kitsune_do_automigrate, for each symbol
 * registered as automigrating with the Kitsune runtime. If no transformation
 * functin exists, it will use memcpy instead.
 */
void kitsune_automigrate_key(const char *key, void* var_addr, size_t var_size, xform_fn_t xf)
{
  assert(kitsune_is_updating());
  if (xf)
    xf(var_addr);
  else {
    const char *mapped_key = transform_mapped_name(key);     
    if (!mapped_key) mapped_key = key;
    void *old_var = kitsune_lookup_key_old(mapped_key);
    if (!old_var) {
      kitsune_log("kitsune_automigrate_key: could not find old-version address for mapped key (%s -> %s)\n",
                 mapped_key, key);
      return;
    }
    memcpy(var_addr, old_var, var_size);
  }
}

/** 
 * \ingroup public
 * kitsune_old_address_matches is used to test whether a pointer contained in
 * the data accessed from the old version points to a particular global variable
 * or function (name).
 */
int kitsune_old_address_matches(const char *name, void *ptr)
{
  return ptr == kitsune_get_val(name);
}

/** 
 * \ingroup public
 * kitsune_is_updating_from is used to query whether the identifier of the
 * update point taken in the previous version matches a particular string (pt).
 */
int kitsune_is_updating_from(const char *pt)
{
  if (!kitsune_is_updating())
    return 0;

#ifdef ENABLE_THREADING
  if (ktthread_is_main()) {
#endif
    char *prev_pt = *(char **)kitsune_get_val("update_pt");
    assert(prev_pt);
    return strcmp(prev_pt, pt) == 0;
#ifdef ENABLE_THREADING
  } else {
    return ktthread_has_reached_update(pt);
  }
#endif
}

/**
 * \ingroup public
 * kitsune_signal_update is used to tell kitsune an update is ready. If you are
 * manually managing updates rather than using the doupd program, you should
 * probably call this somewhere.
 */
void kitsune_signal_update(void)
{
  update_requested = 1;
}
void kitsune_clear_request(void)
{
  update_requested = 0;
}

/** 
 * \ingroup public
 * kitsune_set_next_version allows the executing version to set which files
 * should be loaded when the next update point is reached.
 */
void kitsune_set_next_version(char *code)
{
  *next_version_code = code;
}
