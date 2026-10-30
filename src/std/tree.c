#include "std/tree.h"
#include <string.h>
#include "core/memory.h"

#define RED 0
#define BLACK 1

void tree_init(struct Tree *tree)
{
    struct TreeNode *head = mem_alloc(sizeof *head);

    head->left = head->parent = head->right = head;
    head->color = BLACK;
    head->is_head = 1;
    tree->head = head;
    tree->size = 0;
}

static struct TreeNode *min_node(struct TreeNode *node)
{
    while (!node->left->is_head)
        node = node->left;
    return node;
}

static struct TreeNode *max_node(struct TreeNode *node)
{
    while (!node->right->is_head)
        node = node->right;
    return node;
}

struct TreeNode *tree_next(struct TreeNode *node)
{
    struct TreeNode *parent;

    if (!node->right->is_head)
        return min_node(node->right);
    while (!(parent = node->parent)->is_head && node == parent->right)
        node = parent;
    return parent;
}

struct TreeNode *tree_prev(struct TreeNode *node)
{
    struct TreeNode *parent;

    if (node->is_head)
        return node->right;
    if (!node->left->is_head)
        return max_node(node->left);
    while (!(parent = node->parent)->is_head && node == parent->left)
        node = parent;
    return node->is_head ? node : parent;
}

static void rotate_left(struct Tree *tree, struct TreeNode *node)
{
    struct TreeNode *pivot = node->right;

    node->right = pivot->left;
    if (!pivot->left->is_head)
        pivot->left->parent = node;
    pivot->parent = node->parent;
    if (node == tree->head->parent)
        tree->head->parent = pivot;
    else if (node == node->parent->left)
        node->parent->left = pivot;
    else
        node->parent->right = pivot;
    pivot->left = node;
    node->parent = pivot;
}

static void rotate_right(struct Tree *tree, struct TreeNode *node)
{
    struct TreeNode *pivot = node->left;

    node->left = pivot->right;
    if (!pivot->right->is_head)
        pivot->right->parent = node;
    pivot->parent = node->parent;
    if (node == tree->head->parent)
        tree->head->parent = pivot;
    else if (node == node->parent->right)
        node->parent->right = pivot;
    else
        node->parent->left = pivot;
    pivot->right = node;
    node->parent = pivot;
}

static void attach(struct Tree *tree, struct TreeNode *parent, int add_left, struct TreeNode *node)
{
    struct TreeNode *head = tree->head, *uncle;

    node->left = node->right = head;
    node->color = RED;
    node->is_head = 0;
    node->parent = parent;
    tree->size++;
    if (parent == head) {
        head->parent = head->left = head->right = node;
    } else if (add_left) {
        parent->left = node;
        if (parent == head->left)
            head->left = node;
    } else {
        parent->right = node;
        if (parent == head->right)
            head->right = node;
    }
    while (node->parent->color == RED) {
        struct TreeNode *grand = node->parent->parent;

        if (node->parent == grand->left) {
            uncle = grand->right;
            if (uncle->color == RED) {
                node->parent->color = BLACK;
                uncle->color = BLACK;
                grand->color = RED;
                node = grand;
            } else {
                if (node == node->parent->right) {
                    node = node->parent;
                    rotate_left(tree, node);
                }
                node->parent->color = BLACK;
                node->parent->parent->color = RED;
                rotate_right(tree, node->parent->parent);
            }
        } else {
            uncle = grand->left;
            if (uncle->color == RED) {
                node->parent->color = BLACK;
                uncle->color = BLACK;
                grand->color = RED;
                node = grand;
            } else {
                if (node == node->parent->left) {
                    node = node->parent;
                    rotate_right(tree, node);
                }
                node->parent->color = BLACK;
                node->parent->parent->color = RED;
                rotate_left(tree, node->parent->parent);
            }
        }
    }
    head->parent->color = BLACK;
}

static void detach(struct Tree *tree, struct TreeNode *erased)
{
    struct TreeNode *head = tree->head;
    struct TreeNode *node = erased, *fix, *fix_parent, *sibling;
    char color;

    if (node->left->is_head) {
        fix = node->right;
    } else if (node->right->is_head) {
        fix = node->left;
    } else {
        node = tree_next(erased);
        fix = node->right;
    }
    if (node == erased) {
        fix_parent = erased->parent;
        if (!fix->is_head)
            fix->parent = fix_parent;
        if (head->parent == erased)
            head->parent = fix;
        else if (fix_parent->left == erased)
            fix_parent->left = fix;
        else
            fix_parent->right = fix;
        if (head->left == erased)
            head->left = fix->is_head ? fix_parent : min_node(fix);
        if (head->right == erased)
            head->right = fix->is_head ? fix_parent : max_node(fix);
    } else {
        erased->left->parent = node;
        node->left = erased->left;
        if (node == erased->right) {
            fix_parent = node;
        } else {
            fix_parent = node->parent;
            if (!fix->is_head)
                fix->parent = fix_parent;
            fix_parent->left = fix;
            node->right = erased->right;
            erased->right->parent = node;
        }
        if (head->parent == erased)
            head->parent = node;
        else if (erased->parent->left == erased)
            erased->parent->left = node;
        else
            erased->parent->right = node;
        node->parent = erased->parent;
        color = node->color;
        node->color = erased->color;
        erased->color = color;
    }
    if (erased->color == BLACK) {
        for (; fix != head->parent && fix->color == BLACK; fix_parent = fix->parent) {
            if (fix == fix_parent->left) {
                sibling = fix_parent->right;
                if (sibling->color == RED) {
                    sibling->color = BLACK;
                    fix_parent->color = RED;
                    rotate_left(tree, fix_parent);
                    sibling = fix_parent->right;
                }
                if (sibling->is_head) {
                    fix = fix_parent;
                } else if (sibling->left->color == BLACK && sibling->right->color == BLACK) {
                    sibling->color = RED;
                    fix = fix_parent;
                } else {
                    if (sibling->right->color == BLACK) {
                        sibling->left->color = BLACK;
                        sibling->color = RED;
                        rotate_right(tree, sibling);
                        sibling = fix_parent->right;
                    }
                    sibling->color = fix_parent->color;
                    fix_parent->color = BLACK;
                    sibling->right->color = BLACK;
                    rotate_left(tree, fix_parent);
                    break;
                }
            } else {
                sibling = fix_parent->left;
                if (sibling->color == RED) {
                    sibling->color = BLACK;
                    fix_parent->color = RED;
                    rotate_right(tree, fix_parent);
                    sibling = fix_parent->left;
                }
                if (sibling->is_head) {
                    fix = fix_parent;
                } else if (sibling->right->color == BLACK && sibling->left->color == BLACK) {
                    sibling->color = RED;
                    fix = fix_parent;
                } else {
                    if (sibling->left->color == BLACK) {
                        sibling->right->color = BLACK;
                        sibling->color = RED;
                        rotate_left(tree, sibling);
                        sibling = fix_parent->left;
                    }
                    sibling->color = fix_parent->color;
                    fix_parent->color = BLACK;
                    sibling->left->color = BLACK;
                    rotate_right(tree, fix_parent);
                    break;
                }
            }
        }
        fix->color = BLACK;
    }
    tree->size--;
}

struct TreeNode *tree_lower_bound(const struct Tree *tree, const void *key, TreeLessFn less)
{
    struct TreeNode *bound = tree->head, *node = tree->head->parent;

    while (!node->is_head) {
        if (less(node + 1, key)) {
            node = node->right;
        } else {
            bound = node;
            node = node->left;
        }
    }
    return bound;
}

struct TreeNode *tree_upper_bound(const struct Tree *tree, const void *key, TreeLessFn less)
{
    struct TreeNode *bound = tree->head, *node = tree->head->parent;

    while (!node->is_head) {
        if (less(key, node + 1)) {
            bound = node;
            node = node->left;
        } else {
            node = node->right;
        }
    }
    return bound;
}

void *tree_find(const struct Tree *tree, const void *key, TreeLessFn less)
{
    struct TreeNode *node = tree_lower_bound(tree, key, less);

    if (node->is_head || less(key, node + 1))
        return NULL;
    return node + 1;
}

static struct TreeNode *new_node(const void *value, size_t size)
{
    struct TreeNode *node = mem_alloc(sizeof *node + size);

    if (value)
        memcpy(node + 1, value, size);
    else
        memset(node + 1, 0, size);
    return node;
}

void *tree_add(struct Tree *tree, const void *value, size_t size, TreeLessFn less, int *added)
{
    struct TreeNode *parent = tree->head, *node = tree->head->parent, *made;
    int add_left = 1;

    while (!node->is_head) {
        parent = node;
        add_left = less(value, node + 1);
        node = add_left ? node->left : node->right;
    }
    node = parent;
    if (add_left && node != tree->head)
        node = node == tree->head->left ? NULL : tree_prev(node);
    if (node && node != tree->head && !less(node + 1, value)) {
        if (added)
            *added = 0;
        return node + 1;
    }
    made = new_node(value, size);
    attach(tree, parent, add_left, made);
    if (added)
        *added = 1;
    return made + 1;
}

void *tree_add_multi(struct Tree *tree, const void *value, size_t size, TreeLessFn less)
{
    struct TreeNode *parent = tree->head, *node = tree->head->parent, *made;
    int add_left = 1;

    while (!node->is_head) {
        parent = node;
        add_left = less(value, node + 1);
        node = add_left ? node->left : node->right;
    }
    made = new_node(value, size);
    attach(tree, parent, add_left, made);
    return made + 1;
}

void *tree_get(struct Tree *tree, const void *key, size_t key_size, size_t size, TreeLessFn less)
{
    struct TreeNode *parent = tree->head, *node = tree->head->parent, *made;
    int add_left = 1;
    void *value = tree_find(tree, key, less);

    if (value)
        return value;
    while (!node->is_head) {
        parent = node;
        add_left = less(key, node + 1);
        node = add_left ? node->left : node->right;
    }
    made = new_node(NULL, size);
    memcpy(made + 1, key, key_size);
    attach(tree, parent, add_left, made);
    return made + 1;
}

struct TreeNode *tree_erase(struct Tree *tree, struct TreeNode *node, TreeDestroyFn destroy)
{
    struct TreeNode *next = tree_next(node);

    detach(tree, node);
    if (destroy)
        destroy(node + 1);
    mem_free(node);
    return next;
}

size_t tree_erase_key(struct Tree *tree, const void *key, TreeLessFn less, TreeDestroyFn destroy)
{
    struct TreeNode *node = tree_lower_bound(tree, key, less);
    struct TreeNode *last = tree_upper_bound(tree, key, less);
    size_t count = 0;

    while (node != last) {
        node = tree_erase(tree, node, destroy);
        count++;
    }
    return count;
}

static void free_subtree(struct TreeNode *node, TreeDestroyFn destroy)
{
    while (!node->is_head) {
        struct TreeNode *left = node->left;

        free_subtree(node->right, destroy);
        if (destroy)
            destroy(node + 1);
        mem_free(node);
        node = left;
    }
}

void tree_clear(struct Tree *tree, TreeDestroyFn destroy)
{
    struct TreeNode *head = tree->head;

    if (!head)
        return;
    free_subtree(head->parent, destroy);
    head->left = head->parent = head->right = head;
    tree->size = 0;
}

void tree_free(struct Tree *tree, TreeDestroyFn destroy)
{
    if (!tree->head)
        return;
    tree_clear(tree, destroy);
    mem_free(tree->head);
    tree->head = NULL;
}

static struct TreeNode *copy_subtree(struct Tree *dst, const struct TreeNode *src, struct TreeNode *parent,
                                     size_t size, TreeCopyFn copy)
{
    struct TreeNode *node;

    if (src->is_head)
        return dst->head;
    node = mem_alloc(sizeof *node + size);
    if (copy)
        copy(node + 1, src + 1);
    else
        memcpy(node + 1, src + 1, size);
    node->color = src->color;
    node->is_head = 0;
    node->parent = parent;
    node->left = copy_subtree(dst, src->left, node, size, copy);
    node->right = copy_subtree(dst, src->right, node, size, copy);
    return node;
}

void tree_copy(struct Tree *dst, const struct Tree *src, size_t size, TreeCopyFn copy)
{
    struct TreeNode *head;

    tree_init(dst);
    head = dst->head;
    head->parent = copy_subtree(dst, src->head->parent, head, size, copy);
    if (!head->parent->is_head) {
        head->left = min_node(head->parent);
        head->right = max_node(head->parent);
    }
    dst->size = src->size;
}

int tree_less_int(const void *a, const void *b)
{
    return *(const int *)a < *(const int *)b;
}

int tree_less_uint(const void *a, const void *b)
{
    return *(const unsigned *)a < *(const unsigned *)b;
}

int tree_less_int64(const void *a, const void *b)
{
    return *(const long long *)a < *(const long long *)b;
}

int tree_less_uint64(const void *a, const void *b)
{
    return *(const unsigned long long *)a < *(const unsigned long long *)b;
}
