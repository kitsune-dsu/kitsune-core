#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "bench.h"

int
main(int argc, char *argv[])
{
  int size, length, i;
  int *data;
  int *target;
  double bt, at;

  if (argc > 1) {
    size = atoi(argv[1]);
    length = size / sizeof(int);
    size = sizeof(int) * length;
  } else
    exit(1);

  log_perf("before init, size=%d", size);
  data = malloc(size);
  for(i=0; i<length; i++)
    data[i] = i;
  log_perf("after init");

  target = malloc(size);
  bt = log_perf("after target malloc");
  memcpy(target, data, size);
  at = log_perf("after memcpy");

  for(i=0; i<length; i++)
    assert(target[i] == i);

  printf("TIME: %f\n", (at-bt));
  return 0;
}
