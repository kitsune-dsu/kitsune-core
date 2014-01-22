#ifndef BENCH_INTERNAL_H
#define BENCH_INTERNAL_H

#include <stdlib.h>

void bench_init(const char *bench_filename);
void bench_log_resource_usage(void);
void bench_start(void);
void bench_finish(void);
void bench_xform_alloc(size_t sz);
void bench_quiesce_finish(void);
void bench_restart_start(void);

#endif
