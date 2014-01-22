
#include <unistd.h>
#include <kitsune.h>
#include <assert.h>

struct list {
  int dummy;
  void E_T(@t0) *data0;
  void E_T(@t1) *data1;
  void * E_T(@t2) data2;
  struct list E_G(@t0,@t1,@t2) *next;
} E_GENERIC(@t0,@t1,@t2);

struct list E_G(int, struct list<int,int,int>, [opaque]) *l;

typedef struct list E_G(@t0,@t1,@t2) list E_GENERIC(@t0,@t1,@t2);

list E_G(int,int,int) *l2 = NULL;

int main(int argc, char **argv) 
{
  if (!kitsune_is_updating()) {
    l = malloc(sizeof(struct list));
    l->dummy = 5;
    l->data0 = malloc(sizeof(int));
    (*(int *)l->data0) = 16;
    l->data1 = NULL;
    l->data2 = (void *)123; /* testing migration of opaque pointer */
    l->next = l;
  }

  MIGRATE_GLOBAL(l);
  MIGRATE_GLOBAL(l2);
    
  if (kitsune_is_updating()) {
    assert(l->dummy == 5);
    assert(l->data0 != NULL && *(int *)l->data0 == 16);
    assert(l->data1 == NULL);
    assert(l->data2 == (void *)123);
    assert(l->next == l);    
    assert(l2 == NULL);    
  } else {
    kitsune_signal_update();    
    kitsune_set_next_version(strdup(argv[1]));
  }

  kitsune_update("main");
  return 0;
}
