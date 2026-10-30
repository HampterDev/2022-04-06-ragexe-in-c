#include "std/vec.h"
#include "std/list.h"
#include "std/tree.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned live, allocations, destroyed;
static unsigned rng = 12345;

void *mem_alloc(size_t bytes)
{
    void *p = malloc(bytes ? bytes : 1);
    assert(p);
    ++live;
    ++allocations;
    return p;
}

void mem_free(void *p)
{
    if (!p)
        return;
    assert(live);
    --live;
    free(p);
}

__declspec(noreturn) void fatal_error(const char *message)
{
    (void)message;
    abort();
}

static unsigned next_random(void)
{
    rng = rng * 1103515245u + 12345u;
    return (rng >> 16) & 0x7fff;
}

static void count_destroy(void *value)
{
    (void)value;
    ++destroyed;
}

static int less_int(const void *a, const void *b)
{
    return *(const int *)a < *(const int *)b;
}

static void check_vec(void)
{
    struct Vec v;
    int i, *p;

    vec_init(&v);
    for (i = 0; i < 1000; i++)
        vec_push(&v, &i, sizeof i);
    assert(vec_count(&v, sizeof i) == 1000 && vec_capacity(&v, sizeof i) >= 1000);
    for (i = 0; i < 1000; i++)
        assert(VEC_AT(&v, int, i) == i);
    p = vec_insert(&v, (int *)v.first + 10, (int *)v.first + 500, sizeof i);
    assert(*p == 500 && VEC_AT(&v, int, 11) == 10 && vec_count(&v, sizeof i) == 1001);
    vec_erase(&v, (int *)v.first + 10, (int *)v.first + 11);
    for (i = 0; i < 1000; i++)
        assert(VEC_AT(&v, int, i) == i);
    p = vec_add(&v, 3, sizeof i);
    assert(!p[0] && !p[1] && !p[2] && vec_count(&v, sizeof i) == 1003);
    vec_insert_range(&v, v.first, (int *)v.first + 998, (int *)v.first + 1001, sizeof i);
    assert(VEC_AT(&v, int, 0) == 998 && VEC_AT(&v, int, 2) == 0 && VEC_AT(&v, int, 3) == 0);
    vec_resize(&v, 5, sizeof i);
    assert(vec_count(&v, sizeof i) == 5);
    {
        struct Vec copy;

        vec_copy(&copy, &v, sizeof i);
        assert(vec_count(&copy, sizeof i) == 5 && !memcmp(copy.first, v.first, 5 * sizeof i));
        vec_assign(&copy, v.first, (int *)v.first + 2, sizeof i);
        assert(vec_count(&copy, sizeof i) == 2);
        vec_free(&copy);
    }
    vec_free(&v);
    assert(!v.first && !v.last && !v.end && !live);
}

static void check_list(void)
{
    struct List list;
    struct ListNode *node;
    int i, previous, count;

    list_init(&list);
    list_sort(&list, less_int);
    assert(list.head->next == list.head && list.head->prev == list.head && !list.size);
    for (i = 0; i < 500; i++) {
        int value = (int)(next_random() % 97);

        if (i & 1)
            list_push_back(&list, &value, sizeof value);
        else
            list_push_front(&list, &value, sizeof value);
    }
    assert(list.size == 500);
    list_sort(&list, less_int);
    previous = -1;
    count = 0;
    for (node = list.head->next; node != list.head; node = node->next) {
        assert(node->next->prev == node && node->prev->next == node);
        assert(*LIST_VALUE(node, int) >= previous);
        previous = *LIST_VALUE(node, int);
        count++;
    }
    assert(count == 500);
    destroyed = 0;
    for (node = list.head->next; node != list.head;)
        node = *LIST_VALUE(node, int) % 2 ? list_erase(&list, node, count_destroy) : node->next;
    assert(destroyed + list.size == 500);
    for (node = list.head->next; node != list.head; node = node->next)
        assert(*LIST_VALUE(node, int) % 2 == 0);
    list_clear(&list, count_destroy);
    assert(destroyed == 500 && !list.size);
    list_free(&list, NULL);
    list_free(&list, NULL);
    assert(!list.head && !live);
}

struct Entry {
    int key;
    int value;
};

static int tree_depth(const struct TreeNode *node, int *black)
{
    int left, right, lb = 0, rb = 0;

    if (node->is_head) {
        *black = 1;
        return 0;
    }
    if (node->color == 0)
        assert(node->left->color == 1 && node->right->color == 1);
    left = tree_depth(node->left, &lb);
    right = tree_depth(node->right, &rb);
    assert(lb == rb);
    *black = lb + (node->color == 1);
    return 1 + (left > right ? left : right);
}

static void check_tree_shape(const struct Tree *tree, const int *present, int range)
{
    struct TreeNode *node;
    int black, count = 0, previous = -1, key;

    if (tree->size)
        tree_depth(tree->head->parent, &black);
    for (node = tree_first(tree); node != tree->head; node = tree_next(node)) {
        key = TREE_VALUE(node, struct Entry)->key;
        assert(key > previous && present[key]);
        previous = key;
        count++;
    }
    assert((unsigned)count == tree->size);
    for (key = 0, count = 0; key < range; key++)
        count += present[key] != 0;
    assert((unsigned)count == tree->size);
    if (tree->size) {
        assert(tree->head->left == tree_first(tree) && tree_prev(tree->head) == tree->head->right);
    }
}

static void check_tree(void)
{
    enum { RANGE = 300 };
    static int present[RANGE];
    struct Tree tree, copy;
    struct Entry entry, *found;
    int i, added, key;

    tree_init(&tree);
    assert(!tree_find(&tree, &i, tree_less_int) && tree_first(&tree) == tree.head);
    for (i = 0; i < 4000; i++) {
        key = (int)(next_random() % RANGE);
        if (next_random() % 3) {
            entry.key = key;
            entry.value = key * 7;
            found = tree_add(&tree, &entry, sizeof entry, tree_less_int, &added);
            assert(found->key == key && found->value == key * 7 && added == !present[key]);
            present[key] = 1;
        } else {
            destroyed = 0;
            assert(tree_erase_key(&tree, &key, tree_less_int, count_destroy) == (size_t)present[key]);
            assert(destroyed == (unsigned)present[key]);
            present[key] = 0;
        }
        if (i % 97 == 0)
            check_tree_shape(&tree, present, RANGE);
    }
    check_tree_shape(&tree, present, RANGE);
    for (key = 0; key < RANGE; key++) {
        found = tree_find(&tree, &key, tree_less_int);
        assert(present[key] ? found && found->key == key : !found);
        if (present[key]) {
            struct TreeNode *lower = tree_lower_bound(&tree, &key, tree_less_int);
            struct TreeNode *upper = tree_upper_bound(&tree, &key, tree_less_int);
            assert(TREE_VALUE(lower, struct Entry) == found && tree_next(lower) == upper);
        }
    }
    key = RANGE + 5;
    found = tree_get(&tree, &key, sizeof key, sizeof entry, tree_less_int);
    assert(found->key == key && !found->value);
    found->value = 9;
    assert(tree_get(&tree, &key, sizeof key, sizeof entry, tree_less_int) == found && found->value == 9);
    tree_copy(&copy, &tree, sizeof entry, NULL);
    assert(copy.size == tree.size);
    {
        struct TreeNode *a = tree_first(&tree), *b = tree_first(&copy);

        for (; a != tree.head; a = tree_next(a), b = tree_next(b))
            assert(!memcmp(TREE_VALUE(a, struct Entry), TREE_VALUE(b, struct Entry), sizeof entry));
        assert(b == copy.head);
    }
    tree_free(&copy, NULL);
    while (tree.size) {
        struct TreeNode *node = tree_first(&tree);

        for (i = (int)(next_random() % tree.size); i > 0; i--)
            node = tree_next(node);
        present[TREE_VALUE(node, struct Entry)->key % RANGE] = 0;
        tree_erase(&tree, node, NULL);
        if (tree.size % 13 == 0) {
            int black;

            if (tree.size)
                tree_depth(tree.head->parent, &black);
        }
    }
    for (i = 0; i < 50; i++) {
        entry.key = i % 5;
        entry.value = i;
        tree_add_multi(&tree, &entry, sizeof entry, tree_less_int);
    }
    key = 3;
    assert(tree.size == 50 && tree_erase_key(&tree, &key, tree_less_int, NULL) == 10 && tree.size == 40);
    {
        struct TreeNode *node = tree_lower_bound(&tree, &(int){2}, tree_less_int);
        int expected = 2;

        for (i = 0; i < 10; i++, node = tree_next(node)) {
            assert(TREE_VALUE(node, struct Entry)->key == 2 && TREE_VALUE(node, struct Entry)->value == expected);
            expected += 5;
        }
    }
    tree_clear(&tree, NULL);
    assert(!tree.size && tree_first(&tree) == tree.head);
    tree_free(&tree, NULL);
    tree_free(&tree, NULL);
    assert(!live);
}

int main(void)
{
    check_vec();
    check_list();
    check_tree();
    printf("Containers: vector growth/insert/erase/aliasing, list order/sort/erase/clear, red-black tree "
           "invariants under 4000 random adds and erases, find/bounds/get/copy and multimap order passed; "
           "%u allocations.\n", allocations);
    return 0;
}
