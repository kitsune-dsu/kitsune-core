#include <stdlib.h>
#include <assert.h>

struct tree {
  void *data;
  struct tree *left;
  struct tree *right;
};

static struct tree *
_mknode(void *n)
{
  struct tree *ret =  calloc(1, sizeof(struct tree));
  ret->data = n;
  return ret;
}

void *
tree_delete(void *n, struct tree **root, int (*compar)(void *, void *))
{
  struct tree *iter = *root, *parent = NULL;;
  int cmp = 0;

  if (*root == NULL)
    return NULL;
  
  while(iter != NULL) {
    cmp = compar(n, iter->data);
    parent = iter;
    
    if (cmp == 0) {
      break;
    }
    else {
      parent = iter;
      if (cmp < 0)
        iter = iter->left;
      else if (cmp > 0)
        iter = iter->right;
    }
  }
  if (iter == parent->left)
    parent->left = NULL;
  else if (iter == parent->right)
    parent->right = NULL;
  else if (iter == *root)
    *root = NULL;
  else
    abort();

  if (iter == NULL)
    return NULL;
  else
    return parent->data;
}

void *
tree_search(void *n, struct tree **root, int (*compar)(void *, void *))
{
  struct tree *iter = *root, *parent = NULL;
  int cmp = 0;

  if (*root == NULL) {
    *root = _mknode(n);
    return n;
  }
  while (iter != NULL) {
    cmp = compar(n, iter->data);

    if (cmp == 0)
      return iter->data;
    else {
      parent = iter;
      if (cmp < 0)
        iter = iter->left;
      else if (cmp > 0)
        iter = iter->right;
    }
  }
  assert(iter == NULL && parent != NULL);
  if (cmp < 0)
    parent->left = _mknode(n);
  else if (cmp > 0)
    parent->right = _mknode(n);
  else
    abort();
  return n;
}
