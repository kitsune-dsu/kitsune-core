
#include <unistd.h>
#include <kitsune.h>
#include <assert.h>

int x = 100;
static int y = 200;

int *h = NULL;

extern void foo(void);

int main(int argc, char **argv) E_NOTELOCALS
{
  static int z = 300;
  int q = 400;

  kitsune_do_automigrate();

  if (!kitsune_is_updating()) {
    x = 101;
    y = 201;
    h = malloc(sizeof(int));
    (*h) = 42;
    z = 301;
    q = 401;
  }

  MIGRATE_LOCAL(q);
    
  if (kitsune_is_updating()) {
    printf("%d\n", x);
    assert(x == 106);
    assert(y == 206);
    assert(*h == 42);
    assert(z == 306);
    assert(q == 406);
  } else {
    kitsune_signal_update();    
    kitsune_set_next_version(strdup(argv[1]));
  }

  foo();
  return 0;
}
