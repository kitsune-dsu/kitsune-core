
#include <unistd.h>
#include <kitsune.h>
#include <assert.h>

int x = 100;
int xx E_MANUAL_MIGRATE = 100;
static int y = 200;

extern void foo(void);

int main(int argc, char **argv) E_NOTELOCALS
{
  static int z = 300;
  int q = 400;

  kitsune_do_automigrate();

  if (!kitsune_is_updating()) {
    x = 101;
    xx = 101;
    y = 201;
    z = 301;
    q = 401;
  }

  MIGRATE_LOCAL(q);
    
  if (kitsune_is_updating()) {
    assert(x == 101);
    assert(xx == 100); /* NOT AUTOMIGRATED */
    assert(y == 201);
    assert(z == 301);
    assert(q == 401);
  } else {
    kitsune_signal_update();    
    kitsune_set_next_version(strdup(argv[1]));
  }

  foo();
  return 0;
}
