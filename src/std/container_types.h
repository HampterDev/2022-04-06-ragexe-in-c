#ifndef STD_CONTAINER_TYPES_H
#define STD_CONTAINER_TYPES_H

#include <stddef.h>

struct Vec {
    void *first, *last, *end;
};

#define VEC_COUNT(v, type) ((size_t)((type *)(v)->last - (type *)(v)->first))
#define VEC_AT(v, type, i) (((type *)(v)->first)[i])

struct ListNode;
struct TreeNode;

struct List {
    struct ListNode *head;
    unsigned size;
};

struct Tree {
    struct TreeNode *head;
    unsigned size;
};

#endif
