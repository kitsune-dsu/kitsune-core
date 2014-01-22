#ifndef INTERVAL_H_
#define INTERVAL_H_

typedef void *interval_tree;

/* To instantiate an interval tree, the developer should create a type to
   represent intervals and an implementation of each of these callbacks allowing
   the library to manipulate those intervals. */

/* typedef for a function to extract the starting endpoint of an interval. */
typedef void *interval_start_fn(void *interval);

/* typedef for a function to extract the ending endpoint of an interval. */
typedef void *interval_end_fn(void *interval);

/* typedef for a function to compare two interval endpoints.  The function's
   return value should work as follows:

     <0 -> if endpoint i0 is before i1
      0 -> if endpoint i0 and i1 are equal
     >0 -> if endpoint i0 is after i1
*/
typedef int endpoint_compare_fn(void *i0, void *i1);

/* Initialize an interval tree. */
void interval_tree_init(interval_tree *tree, 
                        interval_start_fn *start_fn, 
                        interval_end_fn *end_fn,
                        endpoint_compare_fn *compare_fn);

/* Reinit function pointers after update */
void interval_tree_reinit_fptrs(interval_tree *tree,
                        interval_start_fn *start_fn, 
                        interval_end_fn *end_fn,
                        endpoint_compare_fn *compare_fn);

/* Free the intervals in the tree. */
void interval_tree_free(interval_tree *tree);

/* Insert a new interval in the tree. */
void interval_tree_insert(interval_tree *tree, void *interval);

/* Determine whether the interval provided as an argument overlaps with any
   intervals that have been inserted. */
void *interval_tree_lookup(interval_tree *tree, void *interval);

/* return the corresponding node pointer for the given interval */
void * interval_tree_lookup_node(interval_tree *tree, void *query_interval);

/* delete a single node */
void interval_tree_delete_node(interval_tree *tree, void *interval); 

/* Count the nodes in an interval tree.  (Used for testing the library.) */
int interval_tree_count(interval_tree *tree);

#endif
