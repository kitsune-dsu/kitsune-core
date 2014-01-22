#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "kitsune_internal.h"
#include "ktthreads.h"
#include "stackvars_internal.h"

typedef struct threadinfo {
  ktthread_info info;
  pthread_t thread;
  int removed;
  /* rapid qui. stuff */
  pthread_mutex_t thread_mutex;
  pthread_cond_t *cond_waiting;
  void (*update_callback)(void*); 
  void * update_callback_args;
  int reached_update;

  /* Thread-local Kitsune data */
  void *stackvars_top;
  void *stackvars_top_old;

  /* Thread list links */
  struct threadinfo *prev;
  struct threadinfo *next;
} threadinfo;
void ktthreads_execute_update_callback(threadinfo * t);

/* Not transferred during an update */
pthread_key_t threadinfo_key;
static int ktthreads_initialized = 0;
static int mainwake_init = 0;
pthread_cond_t mainwake_cond = PTHREAD_COND_INITIALIZER; 
pthread_cond_t * main_cond_waiting = NULL; //used in place of threadinfo->cond_waiting.

/* Transferred during an update */
pthread_t main_tid;
pthread_mutexattr_t *ktthreads_mutex_attr; 
pthread_mutex_t *ktthreads_mutex;
threadinfo *thread_list;
pthread_cond_t *threads_ready_cond;
pthread_cond_t *threads_proceed_cond;
int *main_is_waiting;
int *updated_count;
int *threads_count;

/* Data for new threads added during transformation */
static threadinfo *added_thread_list = NULL;

static void threadinfo_dtor(void *);

void ktthread_init(void)
{
  ktthreads_initialized = 1;
  int upd = kitsune_is_updating();

  INIT_OR_MIGRATE_VAL(main_tid, pthread_t, main_tid=pthread_self(), upd);
  INIT_OR_MIGRATE_HEAP(ktthreads_mutex_attr, pthread_mutexattr_t,       
                       { pthread_mutexattr_init(ktthreads_mutex_attr);
                         pthread_mutexattr_settype(ktthreads_mutex_attr, PTHREAD_MUTEX_RECURSIVE); },
                       upd);
  INIT_OR_MIGRATE_HEAP(ktthreads_mutex, pthread_mutex_t, pthread_mutex_init(ktthreads_mutex, ktthreads_mutex_attr), upd);
  INIT_OR_MIGRATE_VAL(thread_list, threadinfo*, thread_list=NULL, upd);
  INIT_OR_MIGRATE_HEAP(threads_ready_cond, pthread_cond_t, pthread_cond_init(threads_ready_cond, NULL), upd);
  INIT_OR_MIGRATE_HEAP(threads_proceed_cond, pthread_cond_t, pthread_cond_init(threads_proceed_cond, NULL), upd);
  INIT_OR_MIGRATE_HEAP(main_is_waiting, int, *main_is_waiting=0, upd);
  INIT_OR_MIGRATE_HEAP(updated_count, int, *updated_count=0, upd);
  INIT_OR_MIGRATE_HEAP(threads_count, int, *threads_count=0, upd);
  
  /* we initialize a new key rather than inheriting the key from the previous
     version because the destructor callback is invalid after the old version
     library is unloaded. */
  pthread_key_create(&threadinfo_key, threadinfo_dtor);


}

static void add_threadinfo(threadinfo *ti, threadinfo **list)
{
  pthread_mutex_lock(ktthreads_mutex);
  ti->prev = NULL;
  ti->next = *list;
  if (ti->next)
    ti->next->prev = ti;
  *list = ti;
  pthread_mutex_unlock(ktthreads_mutex);
}
static void free_threadinfo(threadinfo *ti, threadinfo **list)
{
  pthread_mutex_lock(ktthreads_mutex);
  if (ti->prev) ti->prev->next = ti->next;
  if (ti->next) ti->next->prev = ti->prev;
  if (*list == ti) *list = ti->next;
  free(ti);
  pthread_mutex_unlock(ktthreads_mutex);
}

static void threadinfo_dtor(void *data) 
{
  struct threadinfo *tinfo = data;

  pthread_mutex_lock(ktthreads_mutex);
  if (!tinfo->info.update_pt) {
    /* If the thread died normally (i.e., not as a result of reaching an update
       point after an update has been requested, we remove it. */
    kitsune_log("Thread died.");
    free_threadinfo(tinfo, &thread_list);
    (*threads_count)--;
  } else {
    kitsune_log("Thread reached update point: %s (%d/%d), main: %s", tinfo->info.update_pt, *updated_count, *threads_count, *main_is_waiting ? "waiting" : "not yet waiting");
    (*updated_count)++;
  }
  if (*main_is_waiting && *threads_count == *updated_count)
    pthread_cond_signal(threads_ready_cond);
  pthread_mutex_unlock(ktthreads_mutex);
}

static void *thread_wrap(void *data)
{
  threadinfo *tinfo = data;
  assert(tinfo);
  int ret = pthread_setspecific(threadinfo_key, data);
  assert(!ret); /* Success. */
  if (kitsune_is_updating()) {
    stackvars_flip();
  }
  return tinfo->info.start_fun(tinfo->info.arg);
}

static threadinfo *cur_threadinfo(void) 
{
  threadinfo *tinfo = pthread_getspecific(threadinfo_key);
  assert(tinfo);
  return tinfo;
}

void ktthread_lock(void) {
  if (ktthreads_initialized) 
    pthread_mutex_lock(ktthreads_mutex);
}
void ktthread_unlock(void) {
  if (ktthreads_initialized) 
    pthread_mutex_unlock(ktthreads_mutex);
}
static void ktthread_singlethread_lock(threadinfo *t) {
  pthread_mutex_lock(&t->thread_mutex);
}
static void ktthread_singlethread_unlock(threadinfo *t) {
  pthread_mutex_unlock(&t->thread_mutex);
}

int kitsune_pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                          void *(*start_fun) (void *), void *arg) 
{
  threadinfo *tinfo = calloc(1, sizeof(threadinfo));
  tinfo->info.start_fun = start_fun;
  if (attr) {
    tinfo->info.attr = *attr; /* TODO: is it safe to copy pthread_attr_t's? */
  }
  tinfo->info.arg = arg;
  tinfo->info.update_pt = NULL;

  tinfo->removed = 0;
  tinfo->reached_update = 0;
  tinfo->cond_waiting = NULL;
  tinfo->update_callback = NULL;
  tinfo->update_callback_args = NULL;
  pthread_mutex_init(&tinfo->thread_mutex, ktthreads_mutex_attr);
  tinfo->stackvars_top = NULL;
  tinfo->stackvars_top_old = NULL;


  /* add the thread metadata to the list of active threads */
  add_threadinfo(tinfo, &thread_list);

  int result = pthread_create(&tinfo->thread, &tinfo->info.attr, thread_wrap, tinfo);

  if(result == 0){ //don't increment unless result is success!!!
    pthread_mutex_lock(ktthreads_mutex);
    (*threads_count)++; 
    pthread_mutex_unlock(ktthreads_mutex);
  }

  /* return the correct pthread_t to the caller.  TODO: unfortunately, we may
     need to do something clever to transform these during an update.  Perhaps
     the answer is to have a special translation lookup that happens when
     instances of this type are encountered during the traversal.  Perhaps this
     is a good argument for later switching to setjmp/longjmp for threads (so
     the thread metadata remains valid). */
  *thread = tinfo->thread; 
  
  return result;
}

void **ktthread_get_top(void) 
{ return &cur_threadinfo()->stackvars_top; }

void **ktthread_get_top_old(void) 
{ return &cur_threadinfo()->stackvars_top_old; }

int ktthread_is_main(void) 
{ return main_tid == pthread_self(); }

void ktthread_main_wait(void)
{
  assert(ktthread_is_main());
  pthread_mutex_lock(ktthreads_mutex);
  if (*threads_count != *updated_count)  {
    *main_is_waiting = 1;
    kitsune_log("thread[main]: waiting for threads to reach kitsune_update.");
    pthread_cond_wait(threads_ready_cond, ktthreads_mutex);
    kitsune_log("thread[main]: done waiting for threads.");
  }
  (*updated_count) = 0;
  pthread_mutex_unlock(ktthreads_mutex);
}

void ktthread_launch_wait(void)
{
  assert(ktthread_is_main());
  pthread_mutex_lock(ktthreads_mutex);
  kitsune_log("thread[main]: reached update point, launching threads.");
  (*updated_count) = 0;
  (*threads_count) = 0;
  threadinfo *cur = thread_list;
  /* launch existing threads */
  while (cur) {
    if (cur->removed) {
      threadinfo *tmp = cur->next;
      free_threadinfo(cur, &thread_list);
      cur = tmp;
      continue;
    }
    /* FIXME: Should not fail if the address was updated during a user
       transformation */
    void *tmp = cur->info.start_fun;
    transform_fptr(&tmp, &cur->info.start_fun, 0, NULL);
    (*threads_count)++;
    pthread_create(&cur->thread, &cur->info.attr, thread_wrap, cur);
    cur = cur->next;
  }

  /* launch added threads */
  cur = added_thread_list;
  while (cur) {
    threadinfo *tmp = cur->next;
    cur->prev = NULL;
    cur->next = thread_list;
    thread_list = cur;
    (*threads_count)++;
    pthread_create(&cur->thread, &cur->info.attr, thread_wrap, cur);
    cur = tmp;
  }

  pthread_mutex_unlock(ktthreads_mutex);
  ktthread_main_wait();
}


/* this is where the threads kick the other threads*/
void ktthread_rapidq(void){

   if(!ktthread_is_main()){
     ktthread_singlethread_lock(cur_threadinfo());
     cur_threadinfo()->reached_update = 1;
     ktthread_singlethread_unlock(cur_threadinfo());
   }
 
   kitsune_log("waiting for other threads (%d/%d)", *updated_count, *threads_count);
   /* Give the other threads a kick in the pants! */
   if (mainwake_init)
     pthread_cond_signal(&mainwake_cond);
   if(*threads_count != *updated_count){
     /* this list only contains the helper threads, not the main thread */
     threadinfo *cur = thread_list;
     while (cur) {
       ktthread_singlethread_lock(cur);
       ktthreads_execute_update_callback(cur);
       int thread_reached_update = cur->reached_update;
       ktthread_singlethread_unlock(cur);

       if (!thread_reached_update) {
         if (cur->cond_waiting) 
           pthread_cond_signal(cur->cond_waiting);
         else
           pthread_kill(cur->thread, SIGUSR2);
       }
       cur = cur->next;
     }
   }

   if(!ktthread_is_main()){
     ktthread_singlethread_lock(cur_threadinfo());
     cur_threadinfo()->reached_update = 0;
     ktthread_singlethread_unlock(cur_threadinfo());
   }
}

void ktthread_do_update(const char *pt_name)
{
  assert(!ktthread_is_main());
  assert(!kitsune_is_updating());
  cur_threadinfo()->info.update_pt = pt_name;
  pthread_exit(0);
}

void ktthread_main_finish_update(void) {
  assert(ktthread_is_main());
  pthread_mutex_lock(ktthreads_mutex);
  kitsune_clear_request(); //clear this, may get set from spurious signals from rapid_q.
  pthread_cond_broadcast(threads_proceed_cond);  
  pthread_mutex_unlock(ktthreads_mutex);
}

void ktthread_finish_update(void) {
  assert(!ktthread_is_main());
  assert(kitsune_is_updating());
  pthread_mutex_lock(ktthreads_mutex);
  kitsune_log("thread[%s]: reached update point.", cur_threadinfo()->info.update_pt);
  cur_threadinfo()->info.update_pt = NULL;
  (*updated_count)++;
  if (*main_is_waiting && *threads_count == *updated_count)
    pthread_cond_signal(threads_ready_cond);

  /* Wait for the main thread to finish some cleanup */
  pthread_cond_wait(threads_proceed_cond, ktthreads_mutex);
  pthread_mutex_unlock(ktthreads_mutex);
}

int ktthread_has_reached_update(const char *pt_name)
{ return strcmp(pt_name, cur_threadinfo()->info.update_pt) == 0; }


/* Public functions for manipulating the set of threads during transformation */
static int cur_reached_end = 0;
static threadinfo *cur_thread_iter = NULL;

void kitsune_threads_reset(void)
{
  cur_reached_end = thread_list ? 0 : 1;
  cur_thread_iter = NULL;
}

ktthread_info *kitsune_threads_next(void)
{
  if (cur_thread_iter) {
    cur_thread_iter = cur_thread_iter->next;
    if (!cur_thread_iter) 
      cur_reached_end = 1;
  } else if (!cur_reached_end) {
    cur_thread_iter = thread_list;
  }

  if (!cur_thread_iter)
    return NULL;
  else
    return &cur_thread_iter->info;
}

void kitsune_thread_remove(void)
{
  assert(cur_thread_iter);
  cur_thread_iter->removed = 1;
}

void kitsune_thread_add(void *arg, kt_pthread_fn *start_fun, const char *update_pt, pthread_attr_t *attr)
{
  threadinfo *tinfo = malloc(sizeof(threadinfo));
  tinfo->info.start_fun = start_fun;
  if (attr) {
    tinfo->info.attr = *attr;
  }
  tinfo->info.arg = arg;
  tinfo->info.update_pt = strdup(update_pt);

  tinfo->removed = 0;
  tinfo->stackvars_top = NULL;
  tinfo->stackvars_top_old = NULL;

  /* add the thread metadata to the list of active threads */
  add_threadinfo(tinfo, &added_thread_list);
}

static void *mainwake_thread(void *_ignored) {
  ktthread_lock();
  //while (1) {
    //static struct timespec time_to_wait = {0, 0};
    //time_to_wait.tv_sec = time(NULL) + 5;
    //pthread_cond_timedwait(&mainwake_cond, ktthreads_mutex, &time_to_wait);
    if (kitsune_is_updating()) {
      if (main_cond_waiting) {
        kitsune_log("mainwake: attempting to wake main");
        pthread_cond_signal(main_cond_waiting);
      }
      mainwake_init = 0;
    //  break;
    }
  //}
  ktthread_unlock();
  return NULL;
}

static void init_mainwake_thread(void) {
  mainwake_init = 1;
  pthread_t tid;
  pthread_create(&tid, NULL, mainwake_thread, NULL);
}

int ktthreads_pthread_cond_wait(void *cond_a, void *mutex_a) {
  if (!mainwake_init && ktthread_is_main()) {
    init_mainwake_thread();
  }
  pthread_cond_t *cond = cond_a;
  pthread_mutex_t *mutex = mutex_a;
  ktthread_lock();
  if(!ktthread_is_main())
    cur_threadinfo()->cond_waiting = cond;
  else
    main_cond_waiting = cond;
  ktthread_unlock();
  int result = pthread_cond_wait(cond, mutex);
  ktthread_lock();
  if(!ktthread_is_main())
    cur_threadinfo()->cond_waiting = NULL;
  else
    main_cond_waiting = NULL;
  ktthread_unlock();
  return result;
}

int ktthreads_pthread_cond_timedwait(void *cond_a, void *mutex_a, const void *timeout_a) {
  if (!mainwake_init && ktthread_is_main()) {
    init_mainwake_thread();
  }                       
  pthread_cond_t *cond = cond_a;
  pthread_mutex_t *mutex = mutex_a;
  const struct timespec *timeout = timeout_a;
  ktthread_lock();
  if(!ktthread_is_main())
    cur_threadinfo()->cond_waiting = cond;
  else
    main_cond_waiting = cond;
  ktthread_unlock();              
  int result = pthread_cond_timedwait(cond, mutex, timeout);
  ktthread_lock();
  if(!ktthread_is_main())
    cur_threadinfo()->cond_waiting = NULL;
  else
    main_cond_waiting = NULL;
  ktthread_unlock();
  return result;                                
}

void ktthreads_update_callback(void (*callback)(void*), void *cb_args) {
  cur_threadinfo()->update_callback = callback;
  cur_threadinfo()->update_callback_args = cb_args;
}

void ktthreads_execute_update_callback(threadinfo * t) {
  if (t->update_callback)
    t->update_callback(t->update_callback_args);
}

void ktthreads_ms_sleep(int timeInMs) { 
  struct timespec timeToWait;   
  pthread_mutex_t fakeMutex = PTHREAD_MUTEX_INITIALIZER;
  pthread_cond_t fakeCond = PTHREAD_COND_INITIALIZER;
   
  clock_gettime(CLOCK_REALTIME, &timeToWait);
  timeToWait.tv_nsec += 1000000 * timeInMs;
 
  pthread_mutex_lock(&fakeMutex); 
  ktthreads_pthread_cond_timedwait(&fakeCond, &fakeMutex, &timeToWait);
  pthread_mutex_unlock(&fakeMutex);                                           
} 

