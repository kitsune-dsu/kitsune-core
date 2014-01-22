#include <kitsune.h>
#include <transform.h>
#include <assert.h>

#include "test_util.h"

int main(int argc, char *argv[]) {

#define LENGTH 10
  int x[LENGTH];
  long y[LENGTH];

  int i;
  for(i=0; i<10; i++) x[i] = i;
  
  XF_INVOKE(XF_ARRAY(LENGTH, sizeof(int), sizeof(long),
                     XF_LIFT(int_to_long, sizeof(long))), 
            &x, &y);
 
  for(i=0; i<10; i++) assert(y[i] == i); 

  printf("Success!\n");
  return 0;
}
