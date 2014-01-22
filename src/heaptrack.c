#include <assert.h>

#include "kitsune_internal.h"
#include <ktheaptrack.h>
#include "uthash.h"

#ifdef ENABLE_THREADING
#include "ktthreads_internal.h"
#endif

static void nodes_free(struct kitsune_heaplist_node* list_node) {
	if(list_node) {
		nodes_free(list_node->next);
		free(list_node);
	}
}


void kitsune_heaplist_init(kitsune_heaplist *list)
{
  assert(list);
	list->begin = NULL;
	list->last = NULL;
}

void kitsune_heaplist_free(kitsune_heaplist* list)
{
	nodes_free(list->begin);
	free(list);
}

void kitsune_heaplist_add(kitsune_heaplist* list, void* addr)
{
	if (!list->begin) {
		list->begin = list->last = malloc(sizeof(struct kitsune_heaplist_node));
		list->last->next = NULL;
	}	else 	{
		list->last->next = malloc(sizeof(struct kitsune_heaplist_node));
		list->last = list->last->next;
		list->last->next = NULL;
	}
	list->last->addr = addr;
}

int kitsune_heaplist_del(kitsune_heaplist* list, void* addr)
{
	kitsune_heaplist_node* cur = list->begin;
  kitsune_heaplist_node* prev = NULL;
	
  while (cur) {
    if (cur->addr == addr) {
      if (prev == NULL) 
        list->begin = cur->next;
      else 
        prev->next = cur->next;

      if (list->last == cur)
        list->last = prev;

      free(cur);
      return 1;
    }
    prev = cur;
    cur = cur->next;
  }
  return 0;
}

kitsune_heaplist_iterator* kitsune_heaplist_begin(kitsune_heaplist* list)
{
	assert(list);
	return list->begin;
}

kitsune_heaplist_iterator* kitsune_heaplist_end(kitsune_heaplist* list)
{
	assert(list);
	return NULL;
}

kitsune_heaplist_iterator* kitsune_heaplist_last(kitsune_heaplist* list)
{
	assert(list);
	return list->last;
}

kitsune_heaplist_iterator* kitsune_heaplist_next(kitsune_heaplist_iterator* iter)
{
	if(iter == NULL)
		return NULL;
	else
		return iter->next;
}

int kitsune_heaplist_isempty(kitsune_heaplist* list)
{
	return list->begin == NULL;
}
