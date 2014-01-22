#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <assert.h>
#include <string.h>

#include "bench.h"

#define SHM_SIZE 360000

int
main(int argc, char *argv[])
{
  int size, length, i;
  int *data;
  int *target;

  int fds[2];
  int shm_size = SHM_SIZE;
  char *areas[2];

  double bs, as, br, ar;

  if (argc > 1) {
    size = atoi(argv[1]);
    length = size / sizeof(int);
    size = sizeof(int) * length;
  } else
    exit(1);

  if (argc > 2) {
    shm_size = atoi(argv[2]);    
  }

  if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == -1) {
    perror("socketpair");
    exit(1);
  }

  areas[0] = mmap(0, shm_size, PROT_READ|PROT_WRITE, MAP_ANON|MAP_SHARED, -1, 0);
  if (areas[0] == MAP_FAILED) {
    perror("mmap: ");
    abort();
  }
  areas[1] = mmap(0, shm_size, PROT_READ|PROT_WRITE, MAP_ANON|MAP_SHARED, -1, 0);
  if (areas[1] == MAP_FAILED) {
    perror("mmap: ");
    abort();
  }

  log_perf("before init, size=%d", size);
  data = malloc(size);
  for(i=0; i<length; i++)
    data[i] = i;
  log_perf("after init");

  int idx;
  int count = 0;

  if (!fork()) {  /* child */
    free(data);
    target = malloc(size);
    idx = 0;
    count = 0;
    syncup(fds[1]);

    br = log_perf("start read");
    syncup(fds[1]);
    while (count < size) {
      int read_size = (size - count) > shm_size ? shm_size : (size - count);
      memcpy(((void *)target)+count, areas[idx], read_size);
      count += read_size;
      idx = (idx + 1) % 2;
      syncup(fds[1]);
    }
    ar = log_perf("done read");

    internal_send(fds[1], &br, sizeof(double));
    internal_send(fds[1], &ar, sizeof(double));

    for(i=0; i<length; i++)
      assert(target[i] == i);
    
  } else { /* parent */
    idx = 0;
    count = 0;
    syncup(fds[0]);

    bs = log_perf("start write");
    while (count < size) {
      int write_size = (size - count) > shm_size ? shm_size : (size - count);
      memcpy(areas[idx], ((void *)data)+count, write_size);
      count += write_size;
      idx = (idx + 1) % 2;
      syncup(fds[0]);
    }
    syncup(fds[0]);
    as = log_perf("done write");

    internal_recv(fds[0], &br, sizeof(double));
    internal_recv(fds[0], &ar, sizeof(double));
    printf("TIME: %f\n", max(as,ar) - min(bs,br) );

    /* wait for child to die */
    wait(NULL);
  }  
  return 0;
}
