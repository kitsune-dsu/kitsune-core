#include <dlfcn.h>
#include <stdlib.h>
#include <stdio.h>
#include <setjmp.h>
#include <assert.h>


typedef int init_func_t(jmp_buf *, int, char **); 



void * load(char * library_name){
  void * libhandle;

  printf("driver: Calling dlopen on %s from driver\n", library_name);

  //DEEPBIND not necessary w/linking scripts but GLOBAL is still necessary to see kit_reg_var. 
  //however, in actual kistsune's driver, LOCAL is fine....
  libhandle = dlopen(library_name, RTLD_NOW | RTLD_GLOBAL ); 

  if (libhandle == NULL)
	printf("driver: Failed to load %s: %s\n", library_name, dlerror());
  else
	printf("driver: loaded %s.  Jumping into %s's main\n", library_name, library_name);

  return libhandle;
}

int main(int argc, char **argv) 
{
  jmp_buf env;
  void * handle;


  /* load mainv1.so */
  handle = load("./mainv1.so");

  /*return to this spot after loading mainv1.c*/
  setjmp(env);

   /* jump into mainv1.so*/
  init_func_t *init_func = (init_func_t *)dlsym(handle, "kitsune_init_inplace");
  assert(init_func);
  (*init_func)(&env, argc, argv);

  /*UPDATE*/
  printf("\n\n<-----fake updating---->\n\n");

  /* load mainv2.so */
  handle = load("./mainv2.so");

  /*return to this spot after loading mainv2.c*/
  setjmp(env);

   /* jump into mainv2.so*/
  init_func = (init_func_t *)dlsym(handle, "kitsune_init_inplace");
  assert(init_func);
  (*init_func)(&env, argc, argv);


  printf("exiting all\n");

  return 0;
}
