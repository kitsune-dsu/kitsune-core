#include "interval.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

struct range {
  long start;
  long end;
};

void *range_start(void *i) {
  struct range *r = i;
  return &r->start;
}
void *range_end(void *i) {
  struct range *r = i;
  return &r->end;
}
int range_endpoint_compare(void *i0, void *i1) {
  long *p0 = i0;
  long *p1 = i1;
  if (*p0 == *p1) 
    return 0;
  else if (*p0 < *p1)
    return -1;
  else // if (*p0 > *p1)
    return 1; 
}

void range_insert(interval_tree *tree, long start, long end) {
  struct range *new = malloc(sizeof(struct range));
  new->start = start;
  new->end = end;
  interval_tree_insert(tree, new);
}


struct range *range_lookup(interval_tree *tree, long start, long end) {
  struct range *query = malloc(sizeof(struct range));
  query->start = start;
  query->end = end;
  return (struct range *)interval_tree_lookup(tree, query);
}

struct intnode *node_lookup(interval_tree *tree, long start, long end) {
  struct range *query = malloc(sizeof(struct range));
  query->start = start;
  query->end = end;
  return (struct intnode *)interval_tree_lookup_node(tree, query);
}
 
void range_delete(interval_tree *tree, long start, long end) {

  /* lookup the entire range of the node that the range of the parameters is contained in*/
  struct range *r = range_lookup(tree, start, end);
  if(r)
	  printf("Range %ld-%ld found in %ld-%ld\n", start, end, r->start, r->end);

  struct intnode *to_del = node_lookup(tree, start, end);    
  if(to_del)
     interval_tree_delete_node(tree, to_del);


  if(r){
     if(start > r->start)
        range_insert(tree, r->start, start);
     if(end < r->end)
        range_insert(tree, end, r->end);
   }
}


/* The tests here are mediocre at best. */

int main() {
  int i,j;
  long start, end;
  interval_tree tree;

  interval_tree_init(&tree, range_start, range_end, range_endpoint_compare);
  
  /* insert some nodes for testing */
  range_insert(&tree, 10, 20);
  range_insert(&tree, 0, 2);
  range_insert(&tree, 3, 5);
  range_insert(&tree, 25, 28);
  range_insert(&tree, 29, 33);
  range_insert(&tree, 49, 51);
 
  /* test deletions (both red and black nodes for rotations)*/ 
  range_delete(&tree, 10, 20);
  assert(range_lookup(&tree, 10, 20) == NULL);
  range_delete(&tree, 0, 2);
  assert(range_lookup(&tree, 0, 2) == NULL);
  range_delete(&tree, 3, 5);
  assert(range_lookup(&tree, 3, 5) == NULL);
  range_delete(&tree, 25, 28);
  assert(range_lookup(&tree, 25, 28) == NULL);
  
/* test partial range deletion */ 
  range_delete(&tree, 30, 32);
  assert(range_lookup(&tree, 29, 30) != NULL);
  assert(range_lookup(&tree, 30, 31) == NULL);
  assert(range_lookup(&tree, 32, 33) != NULL);
  range_delete(&tree, 50, 51);
  assert(range_lookup(&tree, 49, 51) != NULL);



/* reset */
  range_delete(&tree, 29, 33);
  range_delete(&tree, 49, 51);
  range_insert(&tree, 10, 20);
  range_insert(&tree, 0, 2);
  range_insert(&tree, 3, 5);
  range_insert(&tree, 25, 28);
  range_insert(&tree, 29, 31);

/* Covering all cases of overlapping with the 10->20 interval. */
  assert(range_lookup(&tree, 10, 20)->start == 10);
  assert(range_lookup(&tree, 19, 20)->start == 10);
  assert(range_lookup(&tree, 10, 11)->start == 10);
  assert(range_lookup(&tree, 10, 21)->start == 10);
  assert(range_lookup(&tree, 9, 10)->start == 10);
  assert(range_lookup(&tree, 9, 11)->start == 10);
  assert(range_lookup(&tree, 19, 21)->start == 10);
  assert(range_lookup(&tree, 9, 21)->start == 10);
  assert(range_lookup(&tree, 11, 19)->start == 10);

  /* Overlapping with the first and last interval. */
  assert(range_lookup(&tree, -1, 1)->start == 0);
  assert(range_lookup(&tree, -1, 0)->start == 0);
  assert(range_lookup(&tree, 30, 30)->start == 29);
  assert(range_lookup(&tree, 31, 31)->start == 29);
  assert(range_lookup(&tree, 31, 32)->start == 29);

  /* Testing intervals that don't overlap */
  assert(range_lookup(&tree, -10, -9) == NULL);
  assert(range_lookup(&tree, 32, 33) == NULL);
  assert(range_lookup(&tree, 7, 9) == NULL);

  range_insert(&tree, 21, 30);
  range_delete(&tree, 10, 20);
  assert(range_lookup(&tree, 10, 20) == NULL);
  assert(range_lookup(&tree, 10, 21) != NULL);

#define NTESTS 10000
  for(i=0; i<NTESTS; i++) {
    interval_tree_free(&tree);
    interval_tree_init(&tree, range_start, range_end, range_endpoint_compare);

    srandom(i);
#define N 20
    for(j=0; j<N; j++) {     
      start = random();
      end = start + random();
      range_insert(&tree, start, end);            
    }

    assert(interval_tree_count(&tree) == N);

    srandom(i);
    for(j=0; j<N; j++) {     
      start = random();
      end = start + random();

      struct range *lookup = range_lookup(&tree, start, end);    
      
      /* Ensure that a range that is returned truly overlaps. */
      if (lookup) {
        int overlapping = 
          start <= lookup->start && end >= lookup->start || 
          start <= lookup->end   && end >= lookup->end   || 
          start >= lookup->start && end <= lookup->end;
        
        assert(overlapping);
      }
    }

    /* Note, this test doesn't currently ensure that a range is returned if
       there is an overlap. */
  }
  interval_tree_free(&tree);
  return 0;
}
