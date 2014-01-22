
#include <unistd.h>
#include <kitsune.h>
#include <assert.h>

int x = 100;
static int y = 200;

extern void foo(void);

int main(int argc, char **argv) E_NOTELOCALS
{
  static int z = 300;
  int q = 400;

  if (!kitsune_is_updating()) {
    x = 101;
    y = 201;
    z = 301;
    q = 401;
  }

  MIGRATE_GLOBAL(x);
  MIGRATE_LOCAL_STATIC(main, z);
  MIGRATE_STATIC(y);
  MIGRATE_LOCAL(q);
    
  if (kitsune_is_updating()) {
    assert(x == 106);
    assert(y == 206);
    assert(z == 306);
    assert(q == 406);
  } else {
    kitsune_signal_update();    
    kitsune_set_next_version(strdup(argv[1]));
  }

  foo();
  return 0;
}
