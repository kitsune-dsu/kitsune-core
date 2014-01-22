
#include <unistd.h>
#include <kitsune.h>
#include <assert.h>

int x = 100;
static int y = 200;
const int ignore = 100;

int
foo(int a)
{
  return a + 1;
}
  
int main(int argc, char **argv) E_NOTELOCALS
{
  int b = foo(2);

  MIGRATE_LOCAL(b);
  kitsune_update("main");
  
  return 0;
}
