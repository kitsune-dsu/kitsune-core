
#include <unistd.h>
#include <kitsune.h>
#include <assert.h>
#include "header.h"


void foo(void){}

int * data = NULL;


PreprocPostConfigFuncNode * node;

int main(int argc, char **argv)
{

  printf("Inside mainv2.c\n");

  //these 2 calls migrate the data for mainv2.c
  kitsune_do_automigrate();
  kitsune_update("main"); 


    
  assert((int*)node->data == 7);
  printf("mainv2.c: this should be foo: %p, and foo is = %p\n", node->unionfptr.fptr, foo);
  assert(node->unionfptr.fptr == foo);
  printf("Sucesss...\n");

  kitsune_update("main"); // this update point is not used, already hit
  return 0;
}
