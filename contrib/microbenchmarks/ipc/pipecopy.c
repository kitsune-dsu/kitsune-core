#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <sys/wait.h>

#include "bench.h"

int
main(int argc, char *argv[])
{
  int size, length, i;
  int *data;
  int *target;
  char sync;
  int fds1[2], fds2[2];
  
  double bs, as, br, ar;

  if (argc > 1) {
    size = atoi(argv[1]);
    length = size / sizeof(int);
    size = sizeof(int) * length;
  } else
    exit(1);

  // pipes on uni-directional so we need two
  if (pipe(fds1) < 0 || pipe(fds2) < 0) {
    perror("pipe");
    exit(1);
  }
  
  log_perf("before init, size=%d", size);
  data = malloc(size);
  for(i=0; i<length; i++)
    data[i] = i;
  log_perf("after init");

  if (!fork()) {  /* child */
    free(data);
    target = malloc(size);
    internal_write(fds1[1], &sync, sizeof(char));
    br = log_perf("start read");
    internal_read(fds2[0], target, size);
    ar = log_perf("done read");

    internal_write(fds1[1], &br, sizeof(double));
    internal_write(fds1[1], &ar, sizeof(double));

    for(i=0; i<length; i++)
      assert(target[i] == i);
    
  } else { /* parent */
    internal_read(fds1[0], &sync, sizeof(char));
    bs = log_perf("start write");
    internal_write(fds2[1], data, size);
    as = log_perf("done write");

    internal_read(fds1[0], &br, sizeof(double));
    internal_read(fds1[0], &ar, sizeof(double));
    printf("TIME: %f\n", max(as,ar) - min(bs,br) );
    
    /* wait for child to die */
    wait(NULL);
  }

  return 0;
}
