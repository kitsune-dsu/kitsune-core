#ifndef EKIDEN_STACKVARS_H_
#define EKIDEN_STACKVARS_H_

#include <stdlib.h>
#include <stdio.h>

void stackvars_note_entry(const char *fun_name);
void stackvars_note_exit(const char *fun_name);
void stackvars_note_local(const char *name, void *addr, size_t size);
void stackvars_note_formal(const char *name, void *addr, size_t size);

void *stackvars_get_local(const char *fun_name, const char *var_name);
void *stackvars_get_formal(const char *fun_name, const char *var_name);

void *stackvars_get_local_new(const char *fun_name, const char *var_name);
void *stackvars_get_formal_new(const char *fun_name, const char *var_name);

void stackvars_summary(void);

/**
\ingroup public
   This annotation can be applied to a function to cause the stackvars module of
   the compiler to automatically generate code to make the function's local
   variables available to the next version. 

   Example:
   void foo() E_NOTELOCALS 
   {
      int x, y, z;
   }

   Here, the x, y, and z locals from function foo() will be available to the
   next version's transformation code.
*/
#define E_NOTELOCALS __attribute__((e_notelocals))

#endif
