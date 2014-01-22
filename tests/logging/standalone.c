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
#include "../../src/log_internal.h"

int main(int argc, char *argv[]) {
  assert(kitsune_logging_init(argv[0]) == 1);

  kitsune_log("Test log entry");

  char *logname = malloc(sizeof(char) * 128);
  snprintf(logname, sizeof(char) * 128, 
           "/tmp/kitsune/%s.%d", basename(argv[0]), getpid());
  FILE *logfile = fopen(logname, "r");
  assert(logfile);
  fclose(logfile);
  
  assert(unlink(logname) == 0);
  printf("Success!\n");
  return 0;
}
