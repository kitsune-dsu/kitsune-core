#ifndef EKIDEN_HEAPTRACK_H_
#define EKIDEN_HEAPTRACK_H_

#include "transform.h"

/**
 * Types for address list access
 */
struct kitsune_heaplist_node
{
	void E_T(@t) * addr;
	struct kitsune_heaplist_node E_G(@t) * next;
} E_GENERIC(@t);

typedef struct kitsune_heaplist_node E_G(@t) kitsune_heaplist_node E_GENERIC(@t);

struct kitsune_heaplist
{
	kitsune_heaplist_node E_G(@t) * begin;
	kitsune_heaplist_node E_G(@t) * last;
} E_GENERIC(@t);

typedef struct kitsune_heaplist E_G(@t) kitsune_heaplist E_GENERIC(@t);

typedef kitsune_heaplist_node kitsune_heaplist_iterator;

void kitsune_heaplist_init(kitsune_heaplist *);
void kitsune_heaplist_add(kitsune_heaplist *, void *);
int kitsune_heaplist_del(kitsune_heaplist *, void *);

/**
 * Functions for walking the list of addresses
 */
kitsune_heaplist_iterator* kitsune_heaplist_begin(kitsune_heaplist *);
kitsune_heaplist_iterator* kitsune_heaplist_end(kitsune_heaplist *);
kitsune_heaplist_iterator* kitsune_heaplist_last(kitsune_heaplist *);
kitsune_heaplist_iterator* kitsune_heaplist_next(kitsune_heaplist_iterator *);
int kitsune_heaplist_isempty(kitsune_heaplist *);

/* for each loop for address lists.  Define an temp iterator to be used as the iteration variable. */
#define HEAPLIST_FOR_EACH(list, cur, type, tmp)                         \
  for((tmp) = kitsune_heaplist_begin(list), (cur) = (tmp) ? (type)((tmp)->addr) : (void*)0; \
      (tmp) != kitsune_heaplist_end(list);                               \
      (tmp) = kitsune_heaplist_next(tmp), (cur) = (tmp) ? (type)((tmp)->addr) : NULL)

#endif
