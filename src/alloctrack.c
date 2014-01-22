#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>

#include <interval.h>

#include "kitsune_internal.h"
#include "alloctrack_internal.h"

typedef struct _alloc_area {
  void *start;
  void *end;
} alloc_area;

interval_tree alloced_areas;

static void *al_area_start(void *r) {
  return ((alloc_area *)r)->start;
}
static void *al_area_end(void *r) {
  return ((alloc_area *)r)->end;
}
void * alareas_get_start(alarea *a) {
  return ((alloc_area *)a)->start;
}

static int addr_compare(void *p0, void *p1)
{
  /* Strictly speaking, we shouldn't be comparing pointers that might point to
     arbitary addresses (e.g., not in the same array). :) */
  ptrdiff_t diff = p0 - p1;
  if (diff == 0)
    return 0;
  else if (diff > 0)
    return 1;
  else
    return -1;
}

void * kitsune_calloc(int numobj, int size)
{
  alloc_area *new_area = malloc(sizeof(alloc_area));
  void * start_addr = calloc(numobj, size);

  new_area->start = start_addr;
  new_area->end = start_addr + (numobj*size);
  interval_tree_insert(&alloced_areas, new_area);

  return start_addr;
}

void * kitsune_malloc(int size)
{
  alloc_area *new_area = malloc(sizeof(alloc_area));
  void * start_addr = malloc(size);

  new_area->start = start_addr;
  new_area->end = start_addr + size;
  interval_tree_insert(&alloced_areas, new_area);

  return start_addr;
}

void kitsune_migrate_alloced_track(void * new_start_addr, void * new_end_addr, void * old_addr)
{
  alloc_area *new_area = malloc(sizeof(alloc_area));
  struct alarea *to_del = alloctrack_lookup_node(old_addr);    

  new_area->start = new_start_addr;
  new_area->end = new_end_addr;
  interval_tree_insert(&alloced_areas, new_area);

  if(to_del){
     interval_tree_delete_node(&alloced_areas, to_del);
  } else {
     kitsune_log("WARNING: Attempted to remove memory from tree at %p but no mapping was found", old_addr);
  }
}

//do we need to add thread-safety here, or can we rely on the underlying program (since we are basically just calling free as normal
void kitsune_free(void * head)
{
  struct alarea *to_del = alloctrack_lookup_node(head);    
  if(to_del){
     interval_tree_delete_node(&alloced_areas, to_del);
     free(head);
  } else {
     kitsune_assert(0, 
                    "Attempted to free memory at %p but no mapping was found", 
                    head);
  }
}

alarea *alloctrack_lookup_node(void *addr) 
{  
  alloc_area query;
  query.start = query.end = addr;
  return (alarea *)interval_tree_lookup_node(&alloced_areas, &query);
}

alarea *alloctrack_lookup(void *addr) 
{  
  alloc_area query;
  query.start = query.end = addr;
  return (alarea *)interval_tree_lookup(&alloced_areas, &query);
}


/* clear should be called once we reach the target update point */
void alloctrack_free(void)
{
  //Do we need to check for unmigrated areas?
  interval_tree_free(&alloced_areas);
}

/* init should be called during startup */
void alloctrack_init(void)
{
  if (kitsune_is_updating()) {
    alloced_areas = *(char **)kitsune_get_val("alloced_areas");
    interval_tree_reinit_fptrs(&alloced_areas, al_area_start, al_area_end, addr_compare);
    kitsune_log("Updating alloctree:  %p\n", alloced_areas);
  }

  else{
    interval_tree_init(&alloced_areas, al_area_start, al_area_end, addr_compare);
  }

}
