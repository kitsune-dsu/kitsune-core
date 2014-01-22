
#include <unistd.h>
#include <kitsune.h>
#include <assert.h>

int x = 100;
static int y = 200;

int main(int argc, char **argv)
{
  static int z = 300;

  kitsune_register_var("NS@x", &x);
  kitsune_register_var("NS@y", &y);
  kitsune_register_var("NS@z", &z);
    
  if (kitsune_is_updating()) {
    printf("Updating...\n");

    int *old_x = kitsune_lookup_key_old(kitsune_lookup_addr_new(&x));
    int *old_y = kitsune_lookup_key_old(kitsune_lookup_addr_new(&y));
    int *old_z = kitsune_lookup_key_old(kitsune_lookup_addr_new(&z));
    
    assert(old_x != &x);
    assert(old_y != &y);
    assert(old_z != &z);

    assert(*old_x == x);
    assert(*old_y == y);
    assert(*old_z == z);

    printf("Sucesss...\n");
  } else {
    printf("Not updating...\n");
    kitsune_signal_update();    
    kitsune_set_next_version(strdup(argv[1]));
  }

  kitsune_update("test");
  return 0;
}
