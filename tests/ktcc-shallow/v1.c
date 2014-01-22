
#include <unistd.h>
#include <kitsune.h>
#include <assert.h>

struct t0 {
  int f0;
  struct t0 *f1;
};

struct t0 g0;

struct t1a {
  int f0;  
};

struct t1b {
  struct t1a f2;
};

struct t1b g1a;
struct t1b *g1b = NULL;

struct t2a {
  int f0;  
};

struct t2b {
  struct t2a *f2;
};

struct t2b g2a;
struct t2b *g2b = NULL;

struct t3a {
  int f0;
  struct t3a *f1;
};

struct t3b {
  int f0;  
  struct t3a f2;
};

struct t3b g3a;
struct t3b *g3b = NULL;


int x = 100;
static int y = 200;

int *h = NULL;

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
    assert(x == 101);
    assert(y == 201);
    assert(h && *h == 42);
    assert(z == 301);
    assert(q == 401);
  } else {
    kitsune_signal_update();    
    kitsune_set_next_version(strdup(argv[1]));
    kitsune_update("main");
  }

  return 0;
}
