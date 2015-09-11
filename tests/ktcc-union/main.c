
#include <unistd.h>
#include <kitsune.h>
#include <assert.h>
#include "header.h"



void foo(void){}

int * data = NULL;


PreprocPostConfigFuncNode * node;

int main(int argc, char **argv)
{

  printf("Inside main.c\n");

  //these 2 calls are no-ops in main.c
  kitsune_do_automigrate();
  kitsune_update("main"); 

  node = (PreprocPostConfigFuncNode *)malloc(sizeof(PreprocPostConfigFuncNode));
  node->data = (int*)7; 
  node->unionfptr.fptr = foo;
  printf("main.c: this should be foo: %p, and foo is = %p\n", node->unionfptr.fptr, foo);

  kitsune_signal_update();    
  kitsune_set_next_version(strdup(argv[1]));
  kitsune_update("main"); // this update point will jump to other version
  return 0;
}
