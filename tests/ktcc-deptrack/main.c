
#include <unistd.h>
#include <kitsune.h>
#include <assert.h>

int x = 100;
static int y = 200;

struct deptrack_test_complete {
  int i;
};

typedef struct deptrack_test_complete deptrack_test_only_typedef;
typedef deptrack_test_only_typedef deptrack_test_incomplete;

struct deptrack_test_top_level {
  int x;
  int y;
  deptrack_test_incomplete b_in_top_level;
};
  
int main(int argc, char **argv) E_NOTELOCALS
{
  struct deptrack_test_top_level *f = NULL;

  MIGRATE_LOCAL(f);
  kitsune_update("main");
  
  return 0;
}
