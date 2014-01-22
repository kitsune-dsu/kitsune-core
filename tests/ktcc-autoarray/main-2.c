#include <kitsune.h>
#include <assert.h>

int x[4] = {0,1,0,0};

int main(int argc, char **argv) 
{

  kitsune_do_automigrate();

  assert(x[0] == 0);
  assert(x[1] == 1);
  
  return 0;
}
