#include <kitsune.h>
#include <assert.h>

void _kitsune_prestart_xform(void) {
  int *new_x = GET_NEW_GLOBAL(x);
  *new_x = 106;
}
