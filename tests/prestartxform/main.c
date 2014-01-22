
#include <unistd.h>
#include <kitsune.h>
#include <assert.h>

int x = 100;

int main(int argc, char **argv)
{
  if (!kitsune_is_updating()) {
    x = 101;
  }
   
  if (kitsune_is_updating()) {
    assert(x == 106);
    printf("Sucesss...\n");
  } else {
    kitsune_signal_update();    
    kitsune_set_next_version(strdup(argv[1]));
    kitsune_update("test");
  }
  return 0;
}
