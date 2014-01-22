
#include <unistd.h>
#include <kitsune.h>
#include <assert.h>

extern void bar(void);
extern int x;

int y;
void foo(void) {}

void (*g0)(void) = NULL;
void (*g1)(void) = NULL;

int *g2 = NULL;
int *g3 = NULL;

int main(int argc, char **argv)
{
  printf("Here...1\n");
  if (!MIGRATE_GLOBAL(g0))
    g0 = foo;

  printf("Here...2\n");
  if (!MIGRATE_GLOBAL(g1))
    g1 = bar;

  printf("Here...3\n");
  if (!MIGRATE_GLOBAL(g2))
    g2 = &x;
  
  printf("Here...4\n");
  if (!MIGRATE_GLOBAL(g3))
    g3 = &y;
      
  if (kitsune_is_updating()) {
    printf("Here...\n");
    assert(g0 == foo);
    assert(g1 == bar);
    assert(g2 == &x);
    assert(g3 == &y);
    printf("Success...\n");
  } else {
    kitsune_signal_update();    
    kitsune_set_next_version(strdup(argv[1]));
  }

  kitsune_update("test");
  return 0;
}
