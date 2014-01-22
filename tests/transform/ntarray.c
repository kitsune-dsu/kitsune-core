#include <kitsune.h>
#include <transform.h>
#include <assert.h>

#include "test_util.h"

int main(int argc, char *argv[]) {

#define LENGTH 10
  int *x = malloc(LENGTH * sizeof(int));
  long *y;

  int i;
  for(i=0; i<LENGTH-1; i++) x[i] = 50;
  
  x[LENGTH-1] = 0; /* mark end of array with NULL */
  
  XF_INVOKE(XF_NTARRAY(sizeof(int), sizeof(long), XF_LIFT(int_to_long, sizeof(long))), &x, &y);

  assert((void *)x != (void *)y);
  assert(y != NULL);
  for(i=0; i<LENGTH-1; i++) assert(y[i] == 50);
  assert(y[LENGTH-1] == 0);

  printf("Success!\n");
  return 0;
}
