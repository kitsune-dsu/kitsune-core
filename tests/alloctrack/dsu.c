#include <kitsune.h>
#include <assert.h>

void GLOBAL_XFORM(x)(void *output) {
  assert(output);
  int *new = output;
  int *old = GET_OLD_GLOBAL(x);
  assert(old);
  *new = *old + 5;
}

void STATIC_XFORM(main_c, y)(void *output) {
  assert(output);
  int *new = output;
  int *old = GET_OLD_STATIC(main.c, y);
  assert(old);
  *new = *old + 5;
}

void LOCAL_XFORM(main, q)(void *output) {
  assert(output);
  int *new = output;
  int *old = GET_OLD_LOCAL(main, q);
  assert(old);
  *new = *old + 5;
}

void LOCAL_STATIC_XFORM(main_c, main, z)(void *output) {
  assert(output);
  int *new = output;
  int *old = GET_OLD_LOCAL_STATIC(main.c, main, z);
  assert(old);
  *new = *old + 5;
}

void LOCAL_STATIC_XFORM(other_c, foo, z)(void *output) {
  assert(output);
  int *new = output;
  int *old = GET_OLD_LOCAL_STATIC(other.c, foo, z);
  assert(old);
  *new = *old + 5;
}

void STATIC_XFORM(other_c, y)(void *output) {
  int *new = output;
  int *old = GET_OLD_STATIC(other.c, y);
  assert(old);
  *new = *old + 5;
}

void LOCAL_XFORM(foo, q)(void *output) {
  assert(output);
  int *new = output;
  int *old = GET_OLD_LOCAL(foo, q);
  assert(old);
  *new = *old + 5;
}

