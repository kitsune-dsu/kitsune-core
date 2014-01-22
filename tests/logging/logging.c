#include <assert.h>

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

#include <kitsune.h>

int main(int argc, char *argv[]) {
  char *substr_ptr, *kitsune_app_name;
  
  if (kitsune_is_updating()) {
    printf("Starting up following update.\n");
    kitsune_log("LOG ENTRY 2");

    /* try to open the logfile */
    substr_ptr = strstr(argv[0], ".so");
    if (substr_ptr == NULL) {
      /* the application name didn't use the shared object prefix; use the raw
         name */
      kitsune_app_name = strdup(basename(argv[0]));
    } else {
      argv[0][substr_ptr - argv[0]] = '\0';
      kitsune_app_name = strdup(basename(argv[0]));
    }

    char *logname = malloc(sizeof(char) * 128);
    snprintf(logname, sizeof(char) * 128, 
             "/tmp/kitsune/%s.%d", kitsune_app_name, getpid());
    FILE *logfile = fopen(logname, "r");
    assert(logfile);
    fclose(logfile);    
    assert(unlink(logname) == 0);
    printf("Success!\n");

  } else {
    printf("Starting up normally.\n");
    kitsune_log("LOG ENTRY 1");
    kitsune_signal_update();    
    kitsune_set_next_version(strdup(argv[1]));
  }
  kitsune_update("test");
  return 0;
}
