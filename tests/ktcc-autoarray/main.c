#include <kitsune.h>
#include <assert.h>

int x[2] = {0,1};


int main(int argc, char **argv) 
{

  kitsune_do_automigrate();

  assert(x[0] == 0);
  assert(x[1] == 1);
    
  if (!kitsune_is_updating()) {
    kitsune_signal_update();    
    kitsune_set_next_version(strdup(argv[1]));
  }
  kitsune_update("test");
  return 0;
}
