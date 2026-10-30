#include "json/service_json.h"
#include "std/std.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

static unsigned allocations;

void *mem_alloc(size_t size)
{
    void *p = malloc(size ? size : 1);
    assert(p);
    ++allocations;
    return p;
}

void mem_free(void *p)
{
    if (p) {
        assert(allocations);
        --allocations;
        free(p);
    }
}

void fatal_error(const char *message)
{
    (void)message;
    abort();
}

char *native_json_roundtrip(const char *input, size_t length)
{
    struct JsonTree root;
    struct OwnedText result = {0};
    char *out = NULL;
    json_tree_init(&root);
    if (json_parse_text(&root, input, length) && json_write_request(&root, &result)) {
        out = malloc(result.size + 1);
        assert(out);
        memcpy(out, owned_text_cstr(&result), result.size + 1);
    }
    owned_text_clear(&result);
    json_tree_free(&root);
    assert(!allocations);
    return out;
}
