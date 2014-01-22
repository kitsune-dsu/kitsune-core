#ifndef _EKIDEN_TRANSFORM
#define _EKIDEN_TRANSFORM

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include <kitsunemisc.h>
#include <stdlib.h>
/**
 * \ingroup public
 * Expands to the name-mangled type corresponding to type t in the previous
 * version.
 */ 
#define EKIDEN_OLD_TYPE(t) _kitsune_old_rename_##t
/**
 * \ingroup public
 * Expands to the name-mangled type corresponding to type t in the new version.
 */ 
#define EKIDEN_NEW_TYPE(t) _kitsune_new_rename_##t

/**
 * \ingroup public
 * 
 * Simply assign the old value to the new-version variable.
 */ 
#define XF_ASSIGN(target, src) memcpy(&(target), &(src), sizeof(target))
/**
 * \ingroup public
 * 
 * Assert that the source is null, and assign NULL to the target variable.
 */
#define XF_ASSERT_NULL(target, src) {assert(NULL == src); target = NULL;}

typedef void (*xf)(void *in, void *out, int num_gen_args, void **gen_args);

struct closure;
typedef struct closure closure;

typedef enum { XF_SHALLOW, XF_DEEP, XF_TARGET } copy_opt;

void transform_register_renaming(const char *old_key, const char *new_key);
char *transform_mapped_name(const char *old_key);

closure *transform_make_closure(xf f, copy_opt copy_opt, 
                                size_t size_old, size_t size_new, 
                                int nargs, ...);
void transform_invoke_closure(closure *c, void *in, void *out);
void transform_raw(void *in, void *out, int num_args, void **args);
void transform_ptr(void *in, void *out, int num_args, void **args);
void transform_array(void *in, void *out, int num_args, void **args);
void transform_ptrarray(void *in, void *out, int num_args, void **args);
void transform_ntarray(void *in, void *out, int num_args, void **args);
void transform_fptr(void *in, void *out, int num_args, void **args);


/**
 * \ingroup public
 * 
 * Invoke a closure created with another XF macro. 
 */ 
#define XF_INVOKE(c, in, out) \
  transform_invoke_closure(c, in, out)

/**
 * \ingroup internal
 * 
 * Create a closure to be invoked with XF_INVOKE.
 */ 
#define XF_CLOSURE(fun, strategy, size_old, size_new, args...) \
  transform_make_closure((fun), (strategy), (size_old), (size_new), N_ARGS(args), args)
#define XF_LIFT(fun, strategy, size_old, size_new) \
  transform_make_closure((fun), (strategy), (size_old), (size_new), 0)

/**
 * \ingroup public
 * 
 * Default transformer: Shallow-copy a raw value like a pointer or primitive
 * value.
 */ 
#define XF_RAW(sz) \
  XF_CLOSURE(transform_raw, XF_SHALLOW, sz, sz, sz)

/**
 * \ingroup public
 * 
 * Treat the target as a poiner to another type. 
 * \param target_xf a transformer function for the type pointed to.
 */ 
#define XF_PTR(target_xf) \
  XF_CLOSURE(transform_ptr, XF_TARGET, (sizeof(void*)), (sizeof(void*)), target_xf)

/**
 * \ingroup public
 * 
 * Treat the target as an array of a certain type.
 * \param target_xf a transformer function for the type in the array
 * \param count the number of elements in the array
 * \param from_elem_size the size of the array elements in the old version.
 * \param to_elem_size the size of the array elements in the new version.
 */ 
#define XF_ARRAY(count, from_elem_size, to_elem_size, target_xf)      \
  XF_CLOSURE(transform_array, XF_TARGET,                              \
             (count) * (from_elem_size),                              \
             (count) * (to_elem_size),                                \
             (count), (from_elem_size), (to_elem_size), (target_xf))

/**
 * \ingroup public
 * 
 * Treat the target as an array of pointers to a certain type. Think of this as
 * the composition of XF_ARRAY and XF_PTR.
 * 
 * \param target_xf a transformer function for the type in the array
 * \param count the number of elements in the array
 * \param from_elem_size the size of the array elements in the old version.
 * \param to_elem_size the size of the array elements in the new version.
 */ 
#define XF_PTRARRAY(count, from_elem_size, to_elem_size, target_xf) \
  XF_PTR(XF_ARRAY(count, from_elem_size, to_elem_size, target_xf))

/**
 * \ingroup public
 * 
 * Treat the target as an array of elements terminated by a NULL value.
 * 
 * \param target_xf a transformer function for the type in the array
 * \param from_elem_size the size of the array elements in the old version.
 * \param to_elem_size the size of the array elements in the new version.
 */ 
#define XF_NTARRAY(from_elem_size, to_elem_size, target_xf) \
  XF_CLOSURE(transform_ntarray, XF_TARGET, (sizeof(void*)), (sizeof(void*)), \
             from_elem_size, to_elem_size, target_xf)

/**
 * \ingroup public
 * 
 * Treat the target as a NULL-terminated string.
 * 
 */ 
#define XF_NTSTR() \
  XF_NTARRAY(sizeof(char), sizeof(char), XF_RAW(sizeof(char)))
#define XF_FPTR() \
  XF_LIFT(transform_fptr, XF_DEEP, (sizeof(void*)), (sizeof(void*)))

#ifdef E_NOANNOT
#define E_PTR
#define E_OPAQUE
#define E_PTRARRAY(S)
#define E_ARRAY(S)
#define E_CUSTOM(MF,FF)
#define E_IGNORE
#define E_AUTO
#define E_SZ
#define E_GENERIC(...)
#define E_G(...)
#define E_T(...)
#else
#define E_PTR __attribute__((e_ptr))
#define E_OPAQUE __attribute__((e_ptropaque))
#define E_PTRARRAY(S) __attribute__((e_ptrarray(S)))
#define E_ARRAY(S) __attribute__((e_array(S)))
#define E_CUSTOM(MF,FF) __attribute__((e_custom_malloc(#MF,#FF)))
#define E_IGNORE __attribute__((e_ignore))
#define E_AUTO __attribute__((e_auto))
#define E_SZ __attribute__((e_sz))
#define E_GENERIC(...) __attribute__((e_generic(#__VA_ARGS__)))
#define E_G(...) __attribute__((e_genuse(#__VA_ARGS__)))
#define E_T(...) __attribute__((e_type(#__VA_ARGS__)))
#endif

#endif // _EKIDEN_TRANSFORM
