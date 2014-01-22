
/* Internal kitsune library code should ignore ktcc annotations */
#define E_NOANNOT

#include <kitsune.h>
#include <log_internal.h>

/**
 * Assert expr. If expr is false, print the error varargs.
 */
#define kitsune_assert(expr, ...) do {                        \
    if (__builtin_expect(!(expr), 0)) {                                 \
      kitsune_log(__VA_ARGS__);                               \
      kitsune_log("%s:%d: %s: Assertion %s failed; aborting.",          \
                  __FILE__, __LINE__, __func__, #expr);                 \
      fprintf(stderr, __VA_ARGS__);                           \
      fprintf(stderr,"%s:%d %s: Assertion %s failed; aborting.\n",      \
              __FILE__, __LINE__, __func__, #expr);                     \
      abort();                                                          \
    }} while(0)

#define INIT_OR_MIGRATE_VAL(v, t, initcode, updating) \
  if (!updating) {                                    \
    initcode;                                         \
  } else {                                            \
    v = *(t *)kitsune_get_val(#v);                    \
  }                                             

#define INIT_OR_MIGRATE_HEAP(v, t, initcode, updating)  \
  if (!updating) {                                      \
    v = malloc(sizeof(t));                              \
    initcode;                                           \
  } else {                                              \
    v = *(t **)kitsune_get_val(#v);                     \
  }                                             

void kitsune_automigrate_key(const char *key, void* var_addr, size_t var_size, xform_fn_t xf);

int kitsune_is_loading(void);
