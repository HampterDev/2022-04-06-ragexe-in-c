#ifndef STD_TREE_H
#define STD_TREE_H

#include <stddef.h>
#include "std/container_types.h"

struct TreeNode {
    struct TreeNode *left;
    struct TreeNode *parent;
    struct TreeNode *right;
    char color;
    char is_head;
};

#define TREE_VALUE(node, type) ((type *)((struct TreeNode *)(node) + 1))
#define TREE_NODE_OF(value) ((struct TreeNode *)(value) - 1)

typedef int (*TreeLessFn)(const void *a, const void *b);
typedef void (*TreeDestroyFn)(void *value);
typedef void (*TreeCopyFn)(void *dst, const void *src);

void tree_init(struct Tree *tree);
struct TreeNode *tree_next(struct TreeNode *node);
struct TreeNode *tree_prev(struct TreeNode *node);
struct TreeNode *tree_lower_bound(const struct Tree *tree, const void *key, TreeLessFn less);
struct TreeNode *tree_upper_bound(const struct Tree *tree, const void *key, TreeLessFn less);
void *tree_find(const struct Tree *tree, const void *key, TreeLessFn less);
void *tree_add(struct Tree *tree, const void *value, size_t size, TreeLessFn less, int *added);
void *tree_add_multi(struct Tree *tree, const void *value, size_t size, TreeLessFn less);
void *tree_get(struct Tree *tree, const void *key, size_t key_size, size_t size, TreeLessFn less);
struct TreeNode *tree_erase(struct Tree *tree, struct TreeNode *node, TreeDestroyFn destroy);
size_t tree_erase_key(struct Tree *tree, const void *key, TreeLessFn less, TreeDestroyFn destroy);
void tree_clear(struct Tree *tree, TreeDestroyFn destroy);
void tree_free(struct Tree *tree, TreeDestroyFn destroy);
void tree_copy(struct Tree *dst, const struct Tree *src, size_t size, TreeCopyFn copy);

int tree_less_int(const void *a, const void *b);
int tree_less_uint(const void *a, const void *b);
int tree_less_int64(const void *a, const void *b);
int tree_less_uint64(const void *a, const void *b);

static __inline struct TreeNode *tree_first(const struct Tree *tree)
{
    return tree->head->left;
}

#endif
