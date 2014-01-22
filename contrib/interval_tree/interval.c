
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "interval.h"
#include <string.h>


/* red-black tree node colors */
enum rb_color { RED, BLACK };

struct intnode {
  /* The interval data is defined by the user of the interval tree.  We hold
     onto it here to return as a result of a query. */
  void *interval;

  /* extracted interval endpoints */
  void *start;
  void *end;
  void *max;

  /* red-black tree fields */
  enum rb_color color;
  struct intnode *parent;
  struct intnode *left;
  struct intnode *right;
};

struct inttree {
  /* red-black tree node */
  struct intnode *root;

  /* callback functions for comparisons during tree maintenance */
  interval_start_fn *start_fn;
  interval_end_fn *end_fn;
  endpoint_compare_fn *compare_fn;
};

/* Following a rotation, we need to set the max field on the tree node to
   correspond to the maximum endpoint of a nodes descendants. */
static void fix_node_max(struct inttree *t, struct intnode *n) 
{
  assert(t);
  assert(t->compare_fn);

  n->max = n->end;
  if (n->left && t->compare_fn(n->left->max, n->max) > 0)
    n->max = n->left->max;
  if (n->right && t->compare_fn(n->right->max, n->max) > 0)
    n->max = n->right->max;
}

/* Standard red-black left-rotation, except as noted below */
static void left_rotate(struct inttree *t, struct intnode *x) 
{
  assert(t);
  assert(t->root);
  assert(x);
  assert(x->right);

  struct intnode *y = x->right;
  x->right = y->left;  
  if (y->left)
    y->left->parent = x;  
  y->parent = x->parent;
  if (!x->parent) {
    t->root = y;
  } else {
    if (x == x->parent->left)
      x->parent->left = y;
    else
      x->parent->right = y;
  }
  y->left = x;
  x->parent = y;

  /* Update the max fields on the two nodes affected by the rotation.  The order
     here is significant.  See related code in right_rotate(). */
  fix_node_max(t, x);
  fix_node_max(t, y);
}

/* Standard red-black tree right-rotation, except as noted below */
static void right_rotate(struct inttree *t, struct intnode *y) 
{
  assert(t);
  assert(t->root);
  assert(y);
  assert(y->left);

  struct intnode *x = y->left;
  y->left = x->right;
  if (x->right)
    x->right->parent = y;
  x->parent = y->parent;
  if (!y->parent) {
    t->root = x;
  } else {
    if (y == y->parent->left) 
      y->parent->left = x;
    else
      y->parent->right = x;
  }
  x->right = y;
  y->parent = x;

  /* Update the max fields on the two nodes affected by the rotation.  The order
     here is significant.  See related code in left_rotate(). */
  fix_node_max(t, y);
  fix_node_max(t, x);
}

/* Recursively free the nodes in the tree */
static void nodes_free(struct intnode *n) 
{
  if (!n) return;
  nodes_free(n->left);
  nodes_free(n->right);
  free(n);    
}


/*
 * Maintain Red-Black tree balance after deleting a black node.
 * http://doxygen.postgresql.org/rbtree_8c_source.html
 */
static void rb_delete_fixup(struct inttree *rb, struct intnode *x)
{
  assert(rb);
  if (!x)
	return;

  /*
   * x is always a black node.  Initially, it is the former child of the
   * deleted node.  Each iteration of this loop moves it higher up in the
   * tree.
   */
  struct intnode *w;
  while (x != rb->root && x->color == BLACK)
  {
    /*
     * Left and right cases are symmetric.  Any nodes that are children of
     * x have a black-height one less than the remainder of the nodes in
     * the tree.  We rotate and recolor nodes to move the problem up the
     * tree: at some stage we'll either fix the problem, or reach the root
     * (where the black-height is allowed to decrease).
     */
    if (x == x->parent->left)
    {
      w = x->parent->right;
      if (w->color == RED)
      {
        w->color = BLACK;
        x->parent->color = RED;
        left_rotate(rb, x->parent);
        w = x->parent->right;
      }
      if (w->left->color == BLACK && w->right->color == BLACK)
      {
        w->color = RED;
        x = x->parent;
      }
      else{
        if (w->right->color == BLACK){
          w->left->color = BLACK;
          w->color = RED;

          right_rotate(rb, w);
          w = x->parent->right;
        }
        w->color = x->parent->color;
        x->parent->color = BLACK;
        w->right->color = BLACK;
        left_rotate(rb, x->parent);
        x = rb->root;   /* Arrange for loop to terminate. */
      }
    } else {
      w = x->parent->left;
      if (w->color == RED){
        w->color = BLACK;
        x->parent->color = RED;
        right_rotate(rb, x->parent);
        w = x->parent->left;
      }
      if (w->right->color == BLACK && w->left->color == BLACK){
        w->color = RED;
        x = x->parent;
      } else{
        if (w->left->color == BLACK){
          w->right->color = BLACK;
          w->color = RED;
          left_rotate(rb, w);
          w = x->parent->left;
        }
        w->color = x->parent->color;
        x->parent->color = BLACK;
        w->left->color = BLACK;
        right_rotate(rb, x->parent);
        x = rb->root;   /* Arrange for loop to terminate. */
      }
    }
  }
  x->color = BLACK;
}

/*
 * Delete node z from tree.
 */
static void
rb_delete_node(struct inttree *rb, struct intnode *z) 
{
  assert(rb);
  struct intnode *y = NULL;
  struct intnode *x = NULL;

  if (!z || z == NULL)
      return;
  /*
   * y is the node that will actually be removed from the tree.  This will
   * be z if z has fewer than two children, or the tree successor of z
   * otherwise.
   */
  if (z->left == NULL || z->right == NULL){
      /* y has a NULL node as a child */
      y = z;
  } else {
      /* find tree successor */
      y = z->right;
      while (y->left != NULL)
          y = y->left;
  }
  /* x is y's only child */
  if (y->left != NULL)
      x = y->left;
  else 
      x = y->right;


  /* Remove y from the tree. */
   /*y had no children, being explicit for clarity */    
  if(!x){
    if (y->parent){
        if (y == y->parent->left)
            y->parent->left = NULL;
        else
            y->parent->right = NULL;
    } else{
        rb->root = NULL;
    }
  }
  else{
    x->parent = y->parent;
    if (y->parent)
    {
        if (y == y->parent->left)
            y->parent->left = x;
        else
            y->parent->right = x;
    } else {
        rb->root = x;
    }
  }

  /*
   * If we removed the tree successor of z rather than z itself, then move
   * the data for the removed node to the one we were supposed to remove.
   */
  if (y != z){
     z->interval = y->interval;
     z->start = y->start;
     z->end = y->end;
     z->max = y->max;
  }

  /*
   * Removing a black node might make some paths from root to leaf contain
   * fewer black nodes than others, or it might make two red nodes adjacent.
   */
  if (y->color == BLACK)
    rb_delete_fixup(rb, x);

  /* Now we can recycle the y node */
   // printf("freeing y @%p\n",y);
    free(y);
}


/* Standard red-black tree insertion fixup. */
static void rb_insert_fixup(struct inttree *t, struct intnode *z)
{
  assert(t);
  assert(t->root);  
  assert(z);

  struct intnode *y;
  while (z->parent && z->parent->parent && z->parent->color == RED) {    
    if (z->parent == z->parent->parent->left) {
      y = z->parent->parent->right;
      if (y && y->color == RED) {
        z->parent->color = BLACK;
        y->color = BLACK;
        z->parent->parent->color = RED;
        z = z->parent->parent;
      } else {
        if (z == z->parent->right) {
          z = z->parent;
          left_rotate(t, z);
        }
        z->parent->color = BLACK;
        z->parent->parent->color = RED;
        right_rotate(t, z->parent->parent);
      }
    } else {
      y = z->parent->parent->left;
      if (y && y->color == RED) {
        z->parent->color = BLACK;
        y->color = BLACK;
        z->parent->parent->color = RED;
        z = z->parent->parent;
      } else {
        if (z == z->parent->left) {
          z = z->parent;
          right_rotate(t, z);
        }
        z->parent->color = BLACK;
        z->parent->parent->color = RED;
        left_rotate(t, z->parent->parent);
      }
    }
  }
  t->root->color = BLACK;
}

/* Insert a new node into the red-black tree. */
static void rb_insert(struct inttree *t, struct intnode *z) 
{
  assert(t);
  struct intnode *y = NULL;
  struct intnode *x = t->root;
  while (x) {
    y = x;

    /* possibly update the max field of each node encountered as we recurse down
       the tree, since those nodes are the parents of the new node */
    if (t->compare_fn(x->max, z->end) < 0)
      x->max = z->end;

    if (t->compare_fn(z->start, x->start) < 0)
      x = x->left;
    else
      x = x->right;    
  }
  z->parent = y;
  if (y == NULL) {
    t->root = z;
  } else {
    if (t->compare_fn(z->start, y->start) < 0)
      y->left = z;
    else
      y->right = z;      
  }

  z->left = NULL;
  z->right = NULL;
  z->color = RED;

  rb_insert_fixup(t, z);
}

/** 
 * The following functions are part of the public interface for this library. 
 */

void interval_tree_init(interval_tree *tree,
                        interval_start_fn *start_fn, 
                        interval_end_fn *end_fn,
                        endpoint_compare_fn *compare_fn) 
{
  assert(tree);
  assert(start_fn != NULL);
  assert(end_fn != NULL);
  assert(compare_fn != NULL);

  struct inttree **tp = (struct inttree **)tree;
  *tp = malloc(sizeof(struct inttree));
  (*tp)->root = NULL;
  (*tp)->start_fn = start_fn;
  (*tp)->end_fn = end_fn;
  (*tp)->compare_fn = compare_fn;
}

void interval_tree_reinit_fptrs(interval_tree *tree,
                        interval_start_fn *start_fn, 
                        interval_end_fn *end_fn,
                        endpoint_compare_fn *compare_fn) 
{
  assert(tree);
  assert(start_fn != NULL);
  assert(end_fn != NULL);
  assert(compare_fn != NULL);

  struct inttree **t = (struct inttree **)tree;
  assert(t);
  (*t)->start_fn = start_fn;
  (*t)->end_fn = end_fn;
  (*t)->compare_fn = compare_fn;
}

void interval_tree_free(interval_tree *tree) 
{
  assert(tree);
  struct inttree **tp = (struct inttree **)tree;
  if (*tp) {
    nodes_free((*tp)->root);
    free(*tp);
    *tp = NULL;
  }
}

void interval_tree_insert(interval_tree *tree, void *interval) 
{
  assert(tree);
  struct inttree *t = *(struct inttree **)tree;
  assert(t);

  struct intnode *new = malloc(sizeof(struct intnode));
  new->interval = interval;
  new->start = t->start_fn(interval);
  new->end = t->end_fn(interval);
  new->max = new->end;
  new->left = new->right = new->parent = NULL;
  rb_insert(t, new);
}




void interval_tree_delete_node(interval_tree *tree, void *n) 
{
    assert(tree);
    rb_delete_node(*tree, n);
}


void * interval_tree_lookup_node(interval_tree *tree, void *query_interval) {
  assert(tree);
  struct inttree *t = *(struct inttree **)tree;
  assert(t);

  void *query_start = t->start_fn(query_interval);
  void *query_end = t->end_fn(query_interval);

#define OVERLAP(o_start, o_end, elem)                                                       \
  ( (t->compare_fn(o_start, elem->start) <= 0 && t->compare_fn(o_end, elem->start) >= 0) || \
    (t->compare_fn(o_start, elem->end)   <= 0 && t->compare_fn(o_end, elem->end) >= 0)   || \
    (t->compare_fn(o_start, elem->start) >= 0 && t->compare_fn(o_end, elem->end) <= 0) )

  struct intnode *cur = t->root;
  while (cur && !OVERLAP(query_start, query_end, cur)) {
    if (cur->left && t->compare_fn(cur->left->max, query_start) > 0) {
      cur = cur->left;
    } else {
      cur = cur->right;
    }
  }
  if (cur) 
    return cur;
  else 
    return NULL;
}


void *interval_tree_lookup(interval_tree *tree, void *query_interval) {
  assert(tree);
  struct inttree *t = *(struct inttree **)tree;
  assert(t);

  void *query_start = t->start_fn(query_interval);
  void *query_end = t->end_fn(query_interval);

#define OVERLAP(o_start, o_end, elem)                                                       \
  ( (t->compare_fn(o_start, elem->start) <= 0 && t->compare_fn(o_end, elem->start) >= 0) || \
    (t->compare_fn(o_start, elem->end)   <= 0 && t->compare_fn(o_end, elem->end) >= 0)   || \
    (t->compare_fn(o_start, elem->start) >= 0 && t->compare_fn(o_end, elem->end) <= 0) )

  struct intnode *cur = t->root;
  while (cur && !OVERLAP(query_start, query_end, cur)) {
    if (cur->left && t->compare_fn(cur->left->max, query_start) > 0) {
      cur = cur->left;
    } else {
      cur = cur->right;
    }
  }
  if (cur) 
    return cur->interval;
  else 
    return NULL;
}

static int node_count(struct intnode *n) {
  if (!n) return 0;
  else return 1 + node_count(n->left) + node_count(n->right);
}

int interval_tree_count(interval_tree *tree) {
  assert(tree);
  struct inttree *t = *(struct inttree **)tree;
  assert(t);  
  
  return node_count(t->root);
}
