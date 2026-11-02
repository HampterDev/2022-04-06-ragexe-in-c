#ifndef JSON_TREE_H
#define JSON_TREE_H

#include <stddef.h>
#include <stdint.h>

enum JsonTreeType {
    JSON_TREE_NULL = 0,
    JSON_TREE_BOOL = 1,
    JSON_TREE_INT64 = 2,
    JSON_TREE_UINT64 = 3,
    JSON_TREE_HALF = 4,
    JSON_TREE_DOUBLE = 5,
    JSON_TREE_STRING = 6,
    JSON_TREE_BYTES = 7,
    JSON_TREE_ARRAY = 8,
    JSON_TREE_OBJECT = 9,
};

enum JsonTreeTag {
    JSON_TREE_TAG_NONE = 0,
    JSON_TREE_TAG_EPOCH_MILLI = 1,
};

struct JsonTreeMember;

struct JsonTree {
    unsigned char type;
    unsigned char boolean;
    unsigned char tag;
    unsigned char subtype;
    size_t count;
    union {
        int64_t i64;
        uint64_t u64;
        double dbl;
        char *text;
        unsigned char *bytes;
        struct JsonTree *items;
        struct JsonTreeMember *members;
    } u;
};

struct JsonTreeMember {
    char *key;
    size_t key_length;
    struct JsonTree value;
};

void json_tree_init(struct JsonTree *v);
void json_tree_free(struct JsonTree *v);
int json_tree_sort_members(struct JsonTree *object);
const struct JsonTree *json_tree_find(const struct JsonTree *v, const char *key, size_t key_length);

int json_tree_from_bson(struct JsonTree *out, const unsigned char *data, size_t size);

#endif
