#include <unistd.h>
#include <assert.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>


/* copied Kit functions */
int kitsune_init_inplace(jmp_buf *env,  
                        int argc, char **argv){

  return main(argc, argv);

}

void kitsune_register_var(const char *var_name, const char *funcname,
                         const char *filename, const char *namespace,
                         void* var_addr, size_t size, int auto_migrate){
  			
  printf("******NEW:  inside kit_reg_var:v2, called from %s\n", filename);

                          
}


static void __kitsune_register_symbols(void)  __attribute__((__constructor__)) ;
static void __kitsune_register_symbols(void)
{

  printf("mainv2: inside kit_reg_symbols, calling kit_reg_var\n");
  kitsune_register_var("buf_num", "ObfuscateIpToText", "mainv2.c", 0, "mainv2", 10, 1);
}



int main(int argc, char **argv) 
{

  void * libhandle;
  char * library_name = "./pluginv2.so";

  printf("mainv2: inside main, loading pluginv2.so from mainv2.so\n"); 
  libhandle = dlopen(library_name, RTLD_NOW | RTLD_GLOBAL);
  if (libhandle == NULL)
	printf("Failed to load %s: %s\n", library_name, dlerror());

  return 0;
}
