#include <stdlib.h>
#include <string.h>
#include "json/json_tree.h"

void json_tree_init(struct JsonTree *v)
{
    memset(v, 0, sizeof(*v));
}

void json_tree_free(struct JsonTree *v)
{
    size_t i;

    switch (v->type) {
    case JSON_TREE_STRING:
        free(v->u.text);
        break;
    case JSON_TREE_BYTES:
        free(v->u.bytes);
        break;
    case JSON_TREE_ARRAY:
        for (i = 0; i < v->count; i++)
            json_tree_free(&v->u.items[i]);
        free(v->u.items);
        break;
    case JSON_TREE_OBJECT:
        for (i = 0; i < v->count; i++) {
            free(v->u.members[i].key);
            json_tree_free(&v->u.members[i].value);
        }
        free(v->u.members);
        break;
    }
    json_tree_init(v);
}

static int key_compare(const char *a, size_t a_length, const char *b, size_t b_length)
{
    int c = memcmp(a, b, a_length < b_length ? a_length : b_length);

    if (c)
        return c;
    return a_length < b_length ? -1 : a_length > b_length ? 1 : 0;
}

const struct JsonTree *json_tree_find(const struct JsonTree *v, const char *key, size_t key_length)
{
    size_t lo = 0, hi;

    if (v->type != JSON_TREE_OBJECT)
        return NULL;
    hi = v->count;
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        const struct JsonTreeMember *m = &v->u.members[mid];

        if (key_compare(m->key, m->key_length, key, key_length) < 0)
            lo = mid + 1;
        else
            hi = mid;
    }
    if (lo < v->count && key_compare(v->u.members[lo].key, v->u.members[lo].key_length, key, key_length) == 0)
        return &v->u.members[lo].value;
    return NULL;
}

static int member_compare(const struct JsonTreeMember *a, const struct JsonTreeMember *b)
{
    return key_compare(a->key, a->key_length, b->key, b->key_length);
}

static void merge_sort(struct JsonTreeMember *m, struct JsonTreeMember *scratch, size_t n)
{
    size_t half = n / 2, i = 0, j = half, k = 0;

    if (n < 2)
        return;
    merge_sort(m, scratch, half);
    merge_sort(m + half, scratch, n - half);
    while (i < half && j < n) {
        if (member_compare(&m[j], &m[i]) < 0)
            scratch[k++] = m[j++];
        else
            scratch[k++] = m[i++];
    }
    while (i < half)
        scratch[k++] = m[i++];
    while (j < n)
        scratch[k++] = m[j++];
    memcpy(m, scratch, n * sizeof(*m));
}

int json_tree_sort_members(struct JsonTree *object)
{
    struct JsonTreeMember *m = object->u.members, *scratch;
    size_t n = object->count, i, kept;

    if (n < 2)
        return 1;
    scratch = (struct JsonTreeMember *)malloc(n * sizeof(*scratch));
    if (!scratch)
        return 0;
    merge_sort(m, scratch, n);
    free(scratch);
    kept = 1;
    for (i = 1; i < n; i++) {
        if (member_compare(&m[kept - 1], &m[i]) == 0) {
            free(m[i].key);
            json_tree_free(&m[i].value);
        } else {
            m[kept++] = m[i];
        }
    }
    object->count = kept;
    return 1;
}
