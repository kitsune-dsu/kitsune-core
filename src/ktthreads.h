#ifndef KT_THREADS
#define KT_THREADS

#ifndef ENABLE_THREADING
#define ENABLE_THREADING
#endif

#include <pthread.h>
#include <kitsune.h>

int kitsune_pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                          void *(*start_routine) (void *), void *arg);

typedef void *kt_pthread_fn(void *);
typedef struct {
  void *arg;
  kt_pthread_fn *start_fun;
  const char *update_pt;
  pthread_attr_t attr;
} ktthread_info;

void kitsune_threads_reset(void);
ktthread_info *kitsune_threads_next(void);
void kitsune_thread_remove(void);
void kitsune_thread_add(void *arg, kt_pthread_fn *start_fun, const char *update_pt, pthread_attr_t *attr);
int ktthreads_pthread_cond_timedwait(void *cond_a, void *mutex_a, const void *timeout_a);
int ktthreads_pthread_cond_wait(void *cond_a, void *mutex_a);
void ktthreads_ms_sleep(int timeInMs); 
void ktthreads_update_callback(void (*callback)(void*), void *cb_args);

#endif /* KT_THREADS */
