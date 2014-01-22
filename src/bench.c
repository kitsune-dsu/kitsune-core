
#include <sys/time.h> 
#include <sys/resource.h> 
#include <unistd.h>
#include "ktlog.h"
#include "kitsune.h"
#include "ktthreads_internal.h"
#include <assert.h>

const char *bench_log_filename = NULL;
struct timeval bench_start_time;
float bench_quiesce_time;
struct timeval bench_restart_time;
int bench_started = 0;

void bench_init(const char *bench_filename) {
  bench_log_filename = bench_filename;
  if (bench_log_filename && kitsune_is_updating()) {
    bench_start_time = *(struct timeval *)kitsune_get_val("bench_start_time");
    bench_quiesce_time = *(float *)kitsune_get_val("bench_quiesce_time");
    bench_started = *(int *)kitsune_get_val("bench_started");
  }
}


void bench_log_resource_usage(void) {

  if (!bench_log_filename)
    return; /* benchmarking not enabled */

  struct rusage usage;
  
  if (getrusage(RUSAGE_SELF, &usage)) {
    kitsune_log("getrusage: failed");
    return;
  }

  kitsune_log("RESOURCE USAGE:\n"
             "maxrss: %ul\n" /* maximum resident set size */
             "ixrss: %ul\n" /* integral shared memory size */
             "idrss: %ul\n" /* integral unshared data size */
             "isrss: %ul\n" /* integral unshared stack size */
             "minfl: %ul\n" /* page reclaims */
             "majfl: %ul\n" /* page faults */
             "nswap: %ul\n" /* swaps */
             "inblo: %ul\n" /* block input operations */
             "oublo: %ul\n" /* block output operations */
             "msgsn: %ul\n" /* messages sent */
             "msgrc: %ul\n" /* messages received */
             "nsign: %ul\n" /* signals received */
             "nvcsw: %ul\n" /* voluntary context switches */
             "nivcs: %ul\n" /* involuntary context switches */,             
             usage.ru_maxrss, usage.ru_ixrss, usage.ru_idrss, usage.ru_isrss, usage.ru_minflt, 
             usage.ru_majflt, usage.ru_nswap, usage.ru_inblock, usage.ru_oublock, usage.ru_msgsnd, 
             usage.ru_msgrcv, usage.ru_nsignals, usage.ru_nvcsw, usage.ru_nivcsw);

}

static int total_alloc_size = 0;
static int total_alloc_count = 0;

void bench_xform_alloc(int sz) {
  total_alloc_count++;
  total_alloc_size += sz;
}

void bench_start(void) {
  if (!bench_log_filename)
    return; /* benchmarking not enabled */

#ifdef ENABLE_THREADING
  ktthread_lock();
#endif
  if (!bench_started) {
    gettimeofday(&bench_start_time, NULL);
    bench_started = 1;
  }
#ifdef ENABLE_THREADING
  ktthread_unlock();
#endif
}

/* timeval_subtract taken from: 
   http://www.gnu.org/s/libc/manual/html_node/Elapsed-Time.html */
/* Subtract the `struct timeval' values X and Y,
   storing the result in RESULT.
   Return 1 if the difference is negative, otherwise 0.  */
static int timeval_subtract(struct timeval *result, 
                     struct timeval *x, 
                     struct timeval *y)
{
  /* Perform the carry for the later subtraction by updating y. */
  if (x->tv_usec < y->tv_usec) {
    int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
    y->tv_usec -= 1000000 * nsec;
    y->tv_sec += nsec;
  }
  if (x->tv_usec - y->tv_usec > 1000000) {
    int nsec = (y->tv_usec - x->tv_usec) / 1000000;
    y->tv_usec += 1000000 * nsec;
    y->tv_sec -= nsec;
  }

  /* Compute the time remaining to wait.
     tv_usec is certainly positive. */
  result->tv_sec = x->tv_sec - y->tv_sec;
  result->tv_usec = x->tv_usec - y->tv_usec;
  
  /* Return 1 if result is negative. */
  return x->tv_sec < y->tv_sec;
}

void bench_finish(void) {
  if (!bench_log_filename)
    return; /* benchmarking not enabled */

  assert(bench_started);
#ifdef ENABLE_THREADING
  assert(ktthread_is_main());
#endif
  bench_started = 0;

  struct timeval end_time, diff_time;
  gettimeofday(&end_time, NULL);
  if (timeval_subtract(&diff_time, &end_time, &bench_start_time)) {
    assert(0);
  }
  long total = diff_time.tv_sec * 1000000 + diff_time.tv_usec;

  if (timeval_subtract(&diff_time, &end_time, &bench_restart_time)) {
    assert(0);
  }
  long restart = diff_time.tv_sec * 1000000 + diff_time.tv_usec;

  FILE *results = fopen(bench_log_filename, "a");
  if (results) {
    fprintf(results, "%.4f %.4f %.4f\n", bench_quiesce_time, restart/1000.0, total/1000.0);
    fclose(results);
  } 
}

void bench_quiesce_finish(void) {
  if (!bench_log_filename)
    return; /* benchmarking not enabled */

  assert(bench_started);
#ifdef ENABLE_THREADING
  assert(ktthread_is_main());
#endif
  
  struct timeval end_time, diff_time;
  gettimeofday(&end_time, NULL);
  if (timeval_subtract(&diff_time, &end_time, &bench_start_time)) {
    assert(0);
  }

  bench_quiesce_time = (diff_time.tv_sec * 1000000 + diff_time.tv_usec) / 1000.0;
}

void bench_restart_start(void) {
  if (!bench_log_filename)
    return; /* benchmarking not enabled */

#ifdef ENABLE_THREADING
  assert(ktthread_is_main());
#endif

  gettimeofday(&bench_restart_time, NULL);
}
