#include <assert.h>
#include <stdint.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct { void *pointer; size_t size; } allocations[128];
static size_t live;
static int fail_malloc, fail_realloc;

static size_t allocation_index(void *pointer)
{
    size_t i;
    for (i = 0; i < live; ++i)
        if (allocations[i].pointer == pointer) return i;
    assert(0); return 0;
}
static void *tracked_malloc(size_t size)
{
    void *pointer;
    if (fail_malloc) return NULL;
    assert(live < 128);
    pointer = malloc(size); assert(pointer);
    allocations[live].pointer = pointer; allocations[live++].size = size;
    return pointer;
}
static void tracked_free(void *pointer)
{
    size_t i;
    if (!pointer) return;
    i = allocation_index(pointer);
    free(pointer); allocations[i] = allocations[--live];
}
static void *moving_realloc(void *pointer, size_t size)
{
    void *next;
    size_t old_size;
    if (fail_realloc) return NULL;
    old_size = pointer ? allocations[allocation_index(pointer)].size : 0;
    next = tracked_malloc(size);
    if (!next) return NULL;
    if (pointer) memcpy(next, pointer, old_size < size ? old_size : size);
    tracked_free(pointer);
    return next;
}
static jmp_buf fatal;
static int expect_fatal;
static void expected_abort(void) { assert(expect_fatal); longjmp(fatal, 1); }
#define realloc moving_realloc
#define free tracked_free
#define abort expected_abort
#include "../src/core/byte_array.c"
#include "../src/ui/items/item_tag_buttons.c"
#undef abort
#undef free
#undef realloc

int main(void)
{
    struct ByteArray bytes = {0}, overflow = {0};
    struct ItemTagButtons buttons = {0}, too_many = {0};
    unsigned char *data;
    struct UIItemTagButton **items;
    size_t i, capacity;
    assert(!byte_array_at(&bytes, 0) && !byte_array_at(NULL, 0));
    assert(byte_array_resize(&bytes, 4));
    for (i = 0; i < 4; ++i) assert(!bytes.items[i]);
    bytes.items[0] = 3; bytes.items[3] = 7;
    data = bytes.items;
    assert(byte_array_append(&bytes, 9) && bytes.items != data);
    assert(bytes.items[0] == 3 && bytes.items[3] == 7 && bytes.items[4] == 9);
    capacity = bytes.capacity;
    assert(byte_array_resize(&bytes, 2) && byte_array_resize(&bytes, 5));
    assert(bytes.capacity == capacity && !bytes.items[3] && !bytes.items[4]);
    assert(!byte_array_at(&bytes, 99) && !byte_array_at(&bytes, (size_t)-1));
    assert(byte_array_resize(&bytes, bytes.capacity)); data = bytes.items;
    fail_realloc = 1;
    assert(!byte_array_append(&bytes, 1));
    assert(!byte_array_resize(&bytes, bytes.size + 1));
    assert(bytes.items == data && bytes.size == capacity && bytes.items[0] == 3);
    fail_realloc = 0;
    overflow.size = (size_t)-1;
    assert(!byte_array_append(&overflow, 0));
    byte_array_clear(&bytes); byte_array_clear(&bytes); assert(!live);

    for (i = 0; i < 40; ++i)
        item_tag_buttons_append(&buttons, (struct UIItemTagButton *)(uintptr_t)(i + 1));
    for (i = 0; i < 40; ++i)
        assert(buttons.items[i] == (struct UIItemTagButton *)(uintptr_t)(i + 1));
    while (buttons.size < buttons.capacity) item_tag_buttons_append(&buttons, buttons.items[0]);
    items = buttons.items; capacity = buttons.capacity;
    expect_fatal = fail_realloc = 1;
    if (!setjmp(fatal)) { item_tag_buttons_append(&buttons, NULL); assert(0); }
    fail_realloc = 0;
    assert(buttons.items == items && buttons.size == capacity && buttons.capacity == capacity);
    too_many.size = (size_t)-1 / sizeof *too_many.items;
    if (!setjmp(fatal)) { item_tag_buttons_append(&too_many, NULL); assert(0); }
    expect_fatal = 0;
    item_tag_buttons_clear(&buttons); item_tag_buttons_clear(&buttons);
    assert(!live && !buttons.items && !buttons.size && !buttons.capacity);
    puts("Multiline storage: byte initialization, relocation, flag bounds, overflow, allocation failures and borrowed-button cleanup passed.");
    return 0;
}
