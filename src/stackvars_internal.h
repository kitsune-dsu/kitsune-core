#ifndef EKIDEN_STACKVARS_INTERNAL_H_
#define EKIDEN_STACKVARS_INTERNAL_H_

void *stackvars_stack_init(void);
void stackvars_move_to_heap(void);
void stackvars_free(void);
void stackvars_flip(void);

#endif // EKIDEN_STACKVARS_INTERNAL_H_
