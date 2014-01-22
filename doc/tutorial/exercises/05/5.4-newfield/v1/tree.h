#ifndef ___TREE_H___
#define ___TREE_H___

struct tree {
  void *data;
  struct tree *left;
  struct tree *right;
};

void * tree_delete(void *, struct tree **, int (*)(void *, void *));
void * tree_search(void *, struct tree **, int (*)(void *, void *));

#endif
