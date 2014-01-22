#include <stdlib.h>
#include <stdio.h>

extern void kitsune_register_var(const char *var_name, const char *funcname,
                         const char *filename, const char *namespace,
                         void* var_addr, size_t size, int auto_migrate);


static void __kitsune_register_symbols(void)  __attribute__((__constructor__)) ;
static void __kitsune_register_symbols(void)
{
  printf("pluginv2: inside kit_reg_symbols, calling kit_reg_var\n");
  kitsune_register_var("buf_num", "ObfuscateIpToText", "pluginv2.c", 0, "pluginv2", 10, 1);
}

