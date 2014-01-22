
#include <assert.h>
#include <string.h>
#include "kitsune_internal.h"
#include "stackvars.h"
#include "stackvars_internal.h"

#ifdef ENABLE_THREADING
#include "ktthreads_internal.h"
#endif /* ENABLE_THREADING */

typedef struct var_node {
  const char *name;
  void *addr;
  size_t size;
  struct var_node *next;
} var_node;

typedef struct stack_node {
  const char *fun_name;
  var_node *locals;
  var_node *formals;
  struct stack_node *next;
} stack_node;

stack_node *stackvars_top = NULL;
stack_node *stackvars_top_old = NULL;

void *stackvars_stack_init(void)
{
  return NULL;
}

static stack_node **get_top(void)
{
#ifdef ENABLE_THREADING
  if (ktthread_is_main())
#endif
    return &stackvars_top;
#ifdef ENABLE_THREADING
  else
    return (stack_node **)ktthread_get_top();
#endif
}

static stack_node **get_top_old(void)
{
#ifdef ENABLE_THREADING
  if (ktthread_is_main())
#endif 
    return &stackvars_top_old;
#ifdef ENABLE_THREADING
  else
    return (stack_node **)ktthread_get_top_old();
#endif
}

static void copy_vars_to_heap(var_node *vars)
{
  var_node *cur = vars;
  while (cur) {
    void *newmem = malloc(cur->size);
    memcpy(newmem, cur->addr, cur->size);
    cur->addr = newmem;
    cur = cur->next;
  }
}

static void free_vars_from_heap(var_node *vars)
{
  var_node *cur = vars;
  while (cur) {
    free(cur->addr);
    cur->addr = NULL;
    cur = cur->next;
  }  
}

static void free_vars(var_node **var_top) 
{
  var_node *current = *var_top;  
  while (current) {
    var_node *tmp = current;    
    current = current->next;
    free(tmp);
  }
  *var_top = NULL;
}

static void free_stack_node(stack_node *node)
{
  free_vars(&node->locals);
  free_vars(&node->formals);
  free(node);
}

void stackvars_move_to_heap(void)
{
  stack_node **top = get_top();
  stack_node *cur = *top;

  while (cur) {
    copy_vars_to_heap(cur->formals);
    copy_vars_to_heap(cur->locals);
    cur = cur->next;
  }
}

void stackvars_flip(void)
{  
  stack_node **old_top = get_top_old();

#ifdef ENABLE_THREADING
  stack_node **top = get_top();
  if (ktthread_is_main()) {
#endif
    stack_node **top_old_ver = kitsune_get_val("stackvars_top");
    assert(top_old_ver);
    *old_top = *top_old_ver;
#ifdef ENABLE_THREADING
  } else {
    *old_top = *top;
    *top = NULL;
  }
#endif
}

void stackvars_free(void) {
  stack_node **old_top = get_top_old();
  stack_node *last, *cur = *old_top;
  while (cur) {
    free_vars_from_heap(cur->formals);
    free_vars_from_heap(cur->locals);
    last = cur;
    cur = cur->next;
    free_stack_node(last);
  }
  stackvars_top_old = NULL;
}

static void add_var(const char *name, void *addr, size_t size, var_node **var_top)
{
  var_node *new_var = malloc(sizeof(var_node));
  new_var->name = name;
  new_var->addr = addr;
  new_var->size = size;
  new_var->next = *var_top;
  *var_top = new_var;
}

void stackvars_note_entry(const char *fun_name) 
{
  stack_node **top = get_top();

  stack_node *new_node = malloc(sizeof(stack_node));
  new_node->fun_name = fun_name;
  new_node->formals = NULL;
  new_node->locals = NULL;
  new_node->next = *top;
  *top = new_node;  
}

void stackvars_note_exit(const char *fun_name)
{
  stack_node **top = get_top();
  assert(*top != NULL && strcmp(fun_name, (*top)->fun_name) == 0);
  stack_node *old_node = *top;
  *top = (*top)->next;
  free_stack_node(old_node);
}

void stackvars_note_local(const char *name, void *addr, size_t size)
{
  stack_node **top = get_top();
  assert(*top);
  add_var(name, addr, size, &((*top)->locals));
}

void stackvars_note_formal(const char *name, void *addr, size_t size)
{
  stack_node **top = get_top();
  assert(*top);
  add_var(name, addr, size, &((*top)->formals));
}

static void var_summary(const char *category, var_node *vars) 
{
  kitsune_log("%s\n", category);
  var_node *cur = vars;
  while (cur) {
    kitsune_log("  %s\n", cur->name);
    cur = cur->next;
  }
}

void stackvars_summary(void)
{
  stack_node **top = get_top();
  stack_node *cur = *top;
  kitsune_log("Dumping the stack...\n");
  while (cur) {
    kitsune_log("%s\n", cur->fun_name);
    var_summary("formals", cur->formals);
    var_summary("locals", cur->locals);
    cur = cur->next;
  }
}

static stack_node *get_matching_frame(stack_node *stack, const char *fun_name) 
{
  stack_node *cur = stack;
  while (cur && strcmp(cur->fun_name, fun_name) != 0) {
    cur = cur->next;
  }
  return cur;
}

static void *get_matching_var_addr(const char *var_name, var_node *vars)
{
  while (vars && strcmp(vars->name, var_name) != 0) {
    vars = vars->next;
  }
  return vars;
}

void *stackvars_get_local(const char *fun_name, const char *var_name)
{
  stack_node **top_old = get_top_old();
  assert(*top_old);

  stack_node *match_frame = get_matching_frame(*top_old, fun_name);
  if (!match_frame)
    return NULL;

  var_node *match_var = get_matching_var_addr(var_name, match_frame->locals);
  if (!match_var)
    return NULL;

  return match_var->addr;
}

void *stackvars_get_formal(const char *fun_name, const char *var_name)
{
  stack_node **top_old = get_top_old();
  assert(*top_old);

  stack_node *match_frame = get_matching_frame(*top_old, fun_name);
  if (!match_frame)
    return NULL;

  var_node *match_var = get_matching_var_addr(var_name, match_frame->formals);
  if (!match_var)
    return NULL;

  return match_var->addr;
}

void *stackvars_get_local_new(const char *fun_name, const char *var_name)
{
  stack_node **top = get_top();
  assert(*top);

  stack_node *match_frame = get_matching_frame(*top, fun_name);
  if (!match_frame)
    return NULL;

  var_node *match_var = get_matching_var_addr(var_name, match_frame->locals);
  if (!match_var)
    return NULL;

  return match_var->addr;
}

void *stackvars_get_formal_new(const char *fun_name, const char *var_name)
{
  stack_node **top = get_top();
  assert(*top);

  stack_node *match_frame = get_matching_frame(*top, fun_name);
  if (!match_frame)
    return NULL;

  var_node *match_var = get_matching_var_addr(var_name, match_frame->formals);
  if (!match_var)
    return NULL;

  return match_var->addr;
}
