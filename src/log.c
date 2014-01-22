#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <libgen.h>

#include "kitsune_internal.h"
#include "ktthreads_internal.h"
#include "log_internal.h"

/**
 * \addtogroup internal
 * @{
 */

/**
 * The name of the application as passed in from kitsune.c.
 */ 
char * kitsune_app_name = NULL;
/**
 * The file handle to write the log to.
 */ 
FILE * kitsune_log_file = NULL;

/**
 * The path for the log file. This should probably be configurable in the
 * future.
 */ 
static char * kitsune_log_path = "/tmp/kitsune";

/**
 * Open the log file, or if we're updating, copy the file handle from the
 * old-version address space.
 *
 */ 
int kitsune_logging_init(const char *appname_raw) {
  char *local_appname = strdup(appname_raw);

  if (kitsune_is_updating()) {
    kitsune_app_name = *(char **)kitsune_get_val("kitsune_app_name");
    kitsune_log_file = *(FILE **)kitsune_get_val("kitsune_log_file");
    kitsune_log("Updating logging: kitsune_log_file %p and kitsune_app_name %p\n",
               kitsune_log_file, kitsune_app_name);
    return 1;
  }

  char *substr_ptr = strstr(local_appname, ".so");
  if (substr_ptr == NULL) {
    /* the application name didn't use the shared object prefix; use the raw
       name */
    kitsune_app_name = strdup(basename(local_appname));
  } else {
    local_appname[substr_ptr - local_appname] = '\0';
    kitsune_app_name = strdup(basename(local_appname));
  }
  

  /* Ensure logging directory exists */
  errno = 0;
  DIR *kitsune_log_dir = opendir(kitsune_log_path);
  if (kitsune_log_dir == NULL && errno == ENOENT) {
    /* the directory doesn't exist; create it */
    if (mkdir(kitsune_log_path, S_IRWXU) == -1) 
      return 0;                 /* couldn't create directory */
  } 
  else if (kitsune_log_dir == NULL){
    /* something else failed */
    return 0;
  }
  /* by now, the directory exists. Open the log file. */
  char *kitsune_log_fname = malloc(sizeof(char) * 128);
  snprintf(kitsune_log_fname, sizeof(char) * 128,
           "%s/%s.%d", kitsune_log_path, kitsune_app_name, getpid());
  kitsune_log_file = fopen(kitsune_log_fname, "a");
  if (kitsune_log_file == NULL)
    return 0;

  /* it isn't important to free this if we return 0 because we just abort right
     afterwards. */
  free(kitsune_log_fname);
  free(local_appname);
  kitsune_log("Opening new kitsune log.");
  return 1;
}

/** 
 * Write a message fmt to the log file.
 */
void kitsune_log(const char *fmt, ...) {
  va_list args;
  if (kitsune_app_name == NULL || kitsune_log_file == NULL) {
    /* We probably got here from a test. Ignore logging. */
    return;
  }
#ifdef ENABLE_THREADING
  ktthread_lock();
#endif
  fprintf(kitsune_log_file, "Kitsune %s:%d: ", kitsune_app_name, getpid());
  
  va_start(args, fmt);
  vfprintf(kitsune_log_file, fmt, args);
  va_end(args);
  fprintf(kitsune_log_file, "\n");
  fflush(kitsune_log_file);
#ifdef ENABLE_THREADING
  ktthread_unlock();
#endif
}

/** @} */
