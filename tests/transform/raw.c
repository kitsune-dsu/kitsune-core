#include <kitsune.h>
#include <transform.h>
#include <assert.h>

#include "test_util.h"

int main(int argc, char *argv[]) {

  int x;
  int y;

  x = 15;
  
  XF_INVOKE(XF_RAW(sizeof(int)), &x, &y);
 
  assert(y == 15);

  printf("Success!\n");
  return 0;
}
