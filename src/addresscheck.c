
#include "kitsune_internal.h"

#include <interval.h>
#include <stdlib.h>
#include <stddef.h>
#include <ktlog.h>

#include "addresscheck_internal.h"


struct mem_range {
  char *descriptor;
  void *start;
  void *end;
};

void *get_start(void *r) { 
  return ((struct mem_range *)r)->start; 
}
void *get_end(void *r) { 
  return ((struct mem_range *)r)->end; 
}
int addr_compare(void *p0, void *p1) 
{
  /* Strictly speaking, we shouldn't be comparing pointers that might point to
     arbitary addresses (e.g., not in the same array). */
  ptrdiff_t diff = p0 - p1;
  if (diff == 0) 
    return 0;
  else if (diff > 0)
    return 1;
  else
    return -1;
}

interval_tree memory_ranges;

/* clear should be called once we reach the target update point */
void addresscheck_free(void)
{
  interval_tree_free(&memory_ranges);
}

/* init should be called during startup */
void addresscheck_init(void)
{
  interval_tree_init(&memory_ranges, get_start, get_end, addr_compare);
}

void addresscheck(char *descriptor, void *addr, size_t size)
{
  struct mem_range *new_range = malloc(sizeof(struct mem_range));
  new_range->descriptor = descriptor;
  new_range->start = addr;
  new_range->end = addr + (size - 1);

  struct mem_range *lookup = interval_tree_lookup(&memory_ranges, new_range);
  if (lookup && lookup->start != new_range->start && lookup->end != new_range->end) {
    if (descriptor)
      kitsune_log("Memory overlap: insertion: %s", descriptor);
    if (lookup->descriptor)
      kitsune_log("Memory overlap: existing: %s", lookup->descriptor);

    if (lookup->start == new_range->start && lookup->end != new_range->end) {
      kitsune_log("Memory overlap: matching pointers with differnent sizes!");
    } else if ((lookup->start >= new_range->start && lookup->end <= new_range->end) ||
               (lookup->start <= new_range->start && lookup->end >= new_range->end)) {
      kitsune_log("Memory overlap: pointer internal to memory region!");
    } else {
      kitsune_log("Memory overlap: weird overlapping pattern!");
    }

    ptrdiff_t d0 = lookup->start - new_range->start;
    ptrdiff_t d1 = lookup->end - lookup->start;
    ptrdiff_t d2 = new_range->end - new_range->start;

    kitsune_log("Memory overlap: start diff [%ld] size1 [%ld] size2 [%ld]", d0, d1, d2);

  }

  interval_tree_insert(&memory_ranges, new_range);
}
