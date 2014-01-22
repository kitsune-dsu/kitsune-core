#ifndef ALLOCT_INTERNAL_H
#define ALLOCT_INTERNAL_H

#include <stdbool.h>

struct alarea;
typedef struct alarea alarea;

void alloctrack_free(void);
void alloctrack_init(void);

alarea *alloctrack_lookup(void *addr);
alarea *alloctrack_lookup_node(void *addr); 
void * alareas_get_start(alarea *a);

void kitsune_migrate_alloced_track(void * new_start_addr, void * new_end_addr, void * old_addr);
void * kitsune_malloc(int size);
void * kitsune_calloc(int numobj, int size);
void kitsune_free(void * head);
#endif
