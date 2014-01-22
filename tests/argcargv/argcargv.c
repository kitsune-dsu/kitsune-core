
#include <unistd.h>
#include <kitsune.h>
#include <assert.h>

int main(int argc, char **argv)
{   
  if (kitsune_is_updating()) {
    printf("Updating...\n");
    assert(argc == 4);
    assert(!strcmp(argv[2], "b"));
    assert(!strcmp(argv[3], "c"));
    printf("Success.\n");
  } else {
    printf("Non-updating...\n");
    assert(argc == 4);
    assert(!strcmp(argv[2], "b"));
    assert(!strcmp(argv[3], "c"));

    kitsune_signal_update();    
    kitsune_set_next_version(strdup(argv[1]));
  }

  kitsune_update("test");
  return 0;
}
