/*******************

 The goal is to test if we can do this:
//
// T*{1} g;  // pointer to single T object
// T*{10} x; // pointer to 10 T objects
// foo() {
//   x = malloc(sizeof(T)*10);
//   g = &x[3];
//   ...
//   update
//   ...
// }
//
********************/

#include <unistd.h>
#include <kitsune.h>
#include <assert.h>
#include "../../contrib/interval_tree/interval.h"

int x = 100;
static int y = 200;

struct range {
  long start;
  long end;
};


struct range *my_struct_array_ten;
struct range *my_struct_array_one;





extern void foo(void);

int main(int argc, char **argv)
{
  ENTER_FUNC();
  static int z = 300;
  int q = 400;
  
  my_struct_array_ten = malloc(sizeof(struct range)*10);
  //my_struct_array_ten = kitsune_malloc(sizeof(struct range)*10);
  my_struct_array_one = &my_struct_array_ten[3];

  if (!kitsune_is_updating()) {
    x = 101;
    y = 201;
    z = 301;
    q = 401;
  }

  MIGRATE_GLOBAL(x);
  NOTE_AND_MIGRATE_LOCAL_STATIC(main, z);
  NOTE_AND_MIGRATE_STATIC(y);
  NOTE_AND_MIGRATE_LOCAL(q);
    
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
  EXIT_FUNC();
  return 0;
}
