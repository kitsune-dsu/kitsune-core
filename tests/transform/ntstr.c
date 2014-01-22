#include <kitsune.h>
#include <transform.h>
#include <assert.h>

#include "test_util.h"

int main(int argc, char *argv[]) {
  char *str = "test djlkadj asjdlka ashdkjasdh hsadjhakdjh";
  char *x = strdup(str);
  char *y;
  
  XF_INVOKE(XF_NTSTR(), &x, &y);
 
  assert(x != y);
  assert(y != NULL);
  assert(!strcmp(str, y));
 
  printf("Success!\n");
  return 0;
}
