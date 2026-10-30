#include "json/service_json.h"
#include <yyjson.h>
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Allocation { void *pointer; size_t size; };
static struct Allocation allocations[128];
static int attempts, fail_at, live;

static struct Allocation *find_allocation(void *pointer)
{
    size_t i;
    for (i = 0; i < sizeof allocations / sizeof allocations[0]; ++i)
        if (allocations[i].pointer == pointer) return &allocations[i];
    return NULL;
}

static void *guarded_malloc(size_t size)
{
    struct Allocation *slot;
    unsigned char *pointer;
    if (++attempts == fail_at) return NULL;
    assert(size <= SIZE_MAX - 32);
    slot = find_allocation(NULL); assert(slot);
    pointer = malloc(size + 32); assert(pointer);
    memset(pointer + size, 0xa7, 32);
    slot->pointer = pointer; slot->size = size;
    ++live;
    return pointer;
}

static void guarded_free(void *pointer)
{
    struct Allocation *slot;
    size_t i;
    if (!pointer) return;
    slot = find_allocation(pointer);
    if (slot) {
        for (i = 0; i < 32; ++i)
            assert(((unsigned char *)pointer)[slot->size + i] == 0xa7);
        slot->pointer = NULL; slot->size = 0; --live;
    }
    free(pointer);
}

static void *json_malloc(void *context, size_t size)
{
    (void)context;
    return guarded_malloc(size);
}

static void *json_realloc(void *context, void *pointer, size_t old_size, size_t size)
{
    void *replacement;
    (void)context;
    if (pointer) assert(find_allocation(pointer)->size == old_size);
    replacement = guarded_malloc(size);
    if (replacement && pointer) {
        memcpy(replacement, pointer, old_size < size ? old_size : size);
        guarded_free(pointer);
    }
    return replacement;
}

static void json_free(void *context, void *pointer)
{
    (void)context;
    guarded_free(pointer);
}

static const yyjson_alc test_allocator = {json_malloc, json_realloc, json_free, NULL};
static yyjson_mut_doc *test_doc_new(const yyjson_alc *allocator)
{
    assert(!allocator);
    return yyjson_mut_doc_new(&test_allocator);
}

static char *test_write(const yyjson_mut_doc *doc, yyjson_write_flag flags, size_t *size)
{
    return yyjson_mut_write_opts(doc, flags, &test_allocator, size, NULL);
}

#define malloc guarded_malloc
#define free guarded_free
#include "../src/core/owned_text.c"
#undef malloc
#define yyjson_mut_doc_new test_doc_new
#define yyjson_mut_write test_write
#include "../src/json/service_json.c"
#undef yyjson_mut_doc_new
#undef yyjson_mut_write
#undef free

static void make_document(struct JsonTree *root)
{
    struct JsonTree list, item, value;
    static const char counted[] = {'a', 0, 'b', '\x80', '\xff'};
    int i;
    json_tree_init(root); json_tree_init(&list);
    for (i = 0; i < 512; ++i) {
        json_tree_init(&item);
        json_value_from_int(&value, i);
        json_value_assign(json_value_member(&item, "index"), &value);
        json_tree_free(&value);
        json_value_from_text(&value, text_view(counted, sizeof counted));
        json_value_assign(json_value_member(&item, "text"), &value);
        json_tree_free(&value);
        json_value_from_bool(&value, i % 2);
        json_value_assign(json_value_member(&item, "flag"), &value);
        json_tree_free(&value);
        json_value_append(&list, &item);
        json_tree_free(&item);
    }
    json_value_assign(json_value_member(root, "items"), &list);
    json_tree_free(&list);
    json_value_from_cstr(&value, "한글");
    json_value_assign(json_value_member(root, "korean"), &value);
    json_tree_free(&value);
    json_value_member(root, "empty");
    json_tree_init(&value); value.type = JSON_TREE_UINT64; value.u.u64 = UINT64_MAX;
    json_value_assign(json_value_member(root, "unsigned"), &value);
    value.type = JSON_TREE_DOUBLE; value.u.dbl = 1.25;
    json_value_assign(json_value_member(root, "double"), &value);
}

int main(void)
{
    struct JsonTree root, decoded, invalid;
    struct OwnedText output = {0};
    const struct JsonTree *list, *value;
    char *previous;
    int count, i;
    make_document(&root);
    assert(owned_text_assign(&output, text_view_cstr("previous")));
    previous = output.data;
    attempts = 0;
    assert(json_write_request(&root, &output));
    count = attempts;
    assert(count > 4 && live == 1);
    assert(output.size && output.data[output.size - 1] == '\n' && !output.data[output.size]);
    assert(strstr(output.data, "\\u0000"));
    json_tree_init(&decoded);
    assert(json_parse_text(&decoded, output.data, output.size));
    list = json_tree_find(&decoded, "items", 5);
    assert(list && list->count == 512);
    value = json_tree_find(&list->u.items[511], "text", 4);
    assert(value && value->count == 5 && !memcmp(value->u.text, "a\0b\x80\xff", 5));
    value = json_tree_find(&decoded, "korean", 6);
    assert(value && !strcmp(value->u.text, "한글"));
    json_tree_free(&decoded);
    assert(owned_text_assign(&output, text_view_cstr("previous")));
    previous = output.data;
    for (i = 1; i <= count; ++i) {
        attempts = 0; fail_at = i;
        assert(!json_write_request(&root, &output));
        fail_at = 0;
        assert(output.data == previous && output.size == 8 && !strcmp(output.data, "previous"));
        assert(live == 1);
    }
    json_tree_init(&invalid); invalid.type = JSON_TREE_BYTES;
    assert(!json_write_request(&invalid, &output) && output.data == previous && live == 1);
    invalid.type = JSON_TREE_DOUBLE; invalid.u.dbl = NAN;
    assert(!json_write_request(&invalid, &output) && output.data == previous && live == 1);
    assert(!json_write_request(NULL, &output) && !json_write_request(&root, NULL));
    for (i = 0; i < 100; ++i) {
        assert(json_write_request(&root, &output));
        assert(live == 1);
    }
    owned_text_clear(&output); owned_text_clear(&output);
    json_tree_free(&root);
    assert(!live);
    printf("JSON text: C ownership, counted/Korean bytes, unchanged output at all %d allocation failures, invalid values and 100 replacements passed\n", count);
    return 0;
}
