
/** 
 * driver.c manages loading the shared library for each version of the program
 * and calling into that library to initiate execution.
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <dlfcn.h>
#include <setjmp.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>

/** 
 * Update Requests
 * ===============
 */

/** 
 * signal handler to set update requested flag
 */
typedef void update_req_func_t(void);
update_req_func_t *req_func;
void handle_update_signal(int signum)
{
  req_func();
}

char **next_version_code = NULL;

typedef int init_func_t(jmp_buf *, void *, void*, char **,
                        const char *, int, char **);

/** 
 * get_upd_path returns the filename of the shared library containing the new
 * version of the program.
 */
char *get_upd_path() 
{
  if (*next_version_code != NULL) {
    char *result = *next_version_code;
    *next_version_code = NULL;
    return result;
  }
    
  FILE *fp;
  char filename[256];

  /** 
   * in this simple implementation, we load the filename from a file in the /tmp
   * folder with a name based on the pid of the current process.
   */
  snprintf(filename, 256, "/tmp/%d.upd", getpid());
  fp = fopen(filename, "r");
  if (fp) {
    char *result = malloc(sizeof(char) * 256);
    fgets(result, 255, fp);
    unlink(filename);
    return result;
  } else {
    /* note that it might eventually be preferable to check the existence of the
       patch file before performing the longjmp so the process doesn't have to
       fail here */
    printf("Could not read patch name from: %s\n", filename);
    exit(1); } 
}


/** 
 * Outer Updating "Loop"
 * =====================
 */

/**
 * Kitsune's main function contains the kernel of code that handles loading an
 * entering each subsequent version of the program.
 */
int main(int argc, char **argv) 
{
  jmp_buf env;
  char *upd_path, *init_path;;

  if (argc < 2) {
    printf("Please provide the path to initial version library.\n");
    return 1;
  }
  
  pid_t pid = getpid();
  if (pid < 0)
  {
    fprintf(stderr, "getpid() failed!\n");
    return(2);
  } 
  else
  {
    printf("The process id is (%d).\n", pid);
  }

  const char *bench_file = NULL;
  if (argc > 2) {
    if (strcmp(argv[1], "-b") == 0) {
      bench_file = argv[2];
      argc -= 2;
      argv += 2;
    }
  }

  if (argc < 2) {
    printf("Please provide the path to initial version library.\n");
    return 1;
  }

  /** 
   * we accept the filename of the library containing the initial version of the
   * program as an argument.
   */
  init_path = argv[1];
  argc--;
  argv++;
  
  /** 
   * we will later pass the rest of the arguments on to the program's main
   * function.
   */

  /** register the update signal handler */
  signal(SIGUSR2, handle_update_signal);

  /**
   * allocate lib_handle (the handle to the current version dynamic library) on
   * the heap so assignment will persist across the longjmp
   */
  void **lib_handle = malloc(sizeof(void *));
  *lib_handle = NULL;

  next_version_code = malloc(sizeof(char *));
  *next_version_code = NULL;

  /**
   * return to this point in main when an update is requested
   */
  setjmp(env);

  /**
   * if we're updating, call get_upd_path to get the filename for the next
   * version, otherwise use the filename passed in at the commandline.
   */
  if (*lib_handle) {    
    upd_path = get_upd_path();
  } else {
    upd_path = strdup(init_path);
  }
  
  /**
   * Find the absolute path if upd_path is a relative path.
   * Note that this must be relative to the path that driver was run from.
   */
  if(upd_path[0] != '/')
  {
    /* find the absolute path (requires _GNU_SOURCE)*/
    char* cwd = get_current_dir_name();
    int cwd_len = strlen(cwd);
    int upd_len = strlen(upd_path);
    char* path = malloc(cwd_len + upd_len + 2);
    
    memcpy(path, cwd, cwd_len);
    path[cwd_len] = '/';
    memcpy(path + cwd_len + 1, upd_path, upd_len);
    
    free(cwd);
    free(upd_path);
    upd_path = path;
  }

  /**
   * save the previous version library handle so it can be used to access state
   * from the old version from within the new version
   */
  void *prev_lib_handle = (*lib_handle);

  /**
   * load next version of the libary
   */
  (*lib_handle) = dlopen(upd_path, RTLD_NOW | RTLD_LOCAL);
  if ((*lib_handle) == NULL) {
    printf ("[%s] A dynamic linking error occurred: (%s)\n", upd_path, dlerror());
    exit(1);
  }
  free(upd_path);

  /**
   * retreive a pointer to the kitsune_init_inplace function from the new version
   * library
   */
  init_func_t *init_func = (init_func_t *)dlsym(*lib_handle, "kitsune_init_inplace");
  if (!init_func) {
    fprintf(stderr, 
            "Couldn't dynamically load Kitsune runtime initialization function."
            "Is the runtime linked in?\n");
    abort();
  }
  
  req_func = (update_req_func_t *)dlsym(*lib_handle, "kitsune_signal_update");
  if (!req_func) {
    fprintf(stderr, 
            "Couldn't dynamically load Kitsune's update signal function."
            "Is the runtime linked in?\n");
    abort();
  }  

  /**
   * call into the updated version of the library
   */
  return (*init_func)(&env, prev_lib_handle, (*lib_handle),
                      next_version_code, bench_file,
                      argc, argv);
}
