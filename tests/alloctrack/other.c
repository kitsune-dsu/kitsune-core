
#include <unistd.h>
#include <kitsune.h>
#include <assert.h>

static int y = 250;


struct range {
  long start;
  long end;
  long color;
};


void foo(void)
{
  ENTER_FUNC();
  static int z = 350;
  int q = 450;

  if (!kitsune_is_updating()) {
    y = 202;
    z = 302;
    q = 402;
  }

  NOTE_AND_MIGRATE_STATIC(y);
  NOTE_AND_MIGRATE_LOCAL_STATIC(foo, z);
  NOTE_AND_MIGRATE_LOCAL(q);

  if (kitsune_is_updating()) {
    assert(y == 207);
    assert(z == 307);
    assert(q == 407);
    printf("Sucesss...\n");
  }

  kitsune_update("test");
  EXIT_FUNC();
  return;
}
