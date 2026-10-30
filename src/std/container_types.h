#ifndef STD_CONTAINER_TYPES_H
#define STD_CONTAINER_TYPES_H

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
