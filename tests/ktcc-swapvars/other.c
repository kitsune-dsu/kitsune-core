
#include <unistd.h>
#include <kitsune.h>
#include <assert.h>

static int y = 250;

void foo(void) E_NOTELOCALS
{
  static int z = 350;
  int q = 450;

  if (!kitsune_is_updating()) {
    y = 202;
    z = 302;
    q = 402;
  }

  MIGRATE_STATIC(y);
  MIGRATE_LOCAL_STATIC(foo, z);
  MIGRATE_LOCAL(q);

  if (kitsune_is_updating()) {
    assert(y == 302);
    assert(z == 101);
    assert(q == 202);
    printf("Sucesss...\n");
  }

  kitsune_update("test");
  return;
}
