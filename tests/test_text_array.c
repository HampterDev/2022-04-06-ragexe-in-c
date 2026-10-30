#include <assert.h>
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
#define malloc tracked_malloc
#define realloc moving_realloc
#define free tracked_free
#include "../src/core/owned_text.c"
#include "../src/core/text_array.c"
#undef malloc
#undef realloc
#undef free

static void equal(const struct TextArray *array, size_t index, const char *text, size_t size)
{
    struct TextView value = text_array_at(array, index);
    assert(value.size == size && !memcmp(value.data, text, size));
    assert(!value.data[size]);
}
int main(void)
{
    struct TextArray array = {0}, overflow = {0};
    struct OwnedText *items;
    const char bytes[] = {'a', 0, 'b'};
    size_t capacity, count;

    equal(&array, 0, "", 0);
    assert(text_array_reserve(&array, 1) && array.capacity == 1 && !array.size);
    assert(text_array_append(&array, text_view(bytes, sizeof bytes)));
    items = array.items;
    assert(text_array_append(&array, text_array_at(&array, 0)));
    assert(array.items != items);
    equal(&array, 0, bytes, sizeof bytes); equal(&array, 1, bytes, sizeof bytes);
    assert(array.items[0].data != array.items[1].data);
    assert(text_array_append(&array, text_view_cstr("이름")));
    equal(&array, 2, "이름", strlen("이름"));
    capacity = array.capacity;
    text_array_erase(&array, 1);
    equal(&array, 0, bytes, sizeof bytes); equal(&array, 1, "이름", strlen("이름"));
    assert(array.capacity == capacity && array.size == 2);
    text_array_erase(&array, 999); assert(array.size == 2);
    text_array_erase(&array, 0); equal(&array, 0, "이름", strlen("이름"));
    text_array_erase(&array, 0); assert(!array.size && live == 1);
    assert(text_array_resize(&array, 3));
    assert(owned_text_assign(&array.items[1], text_view_cstr("removed")));
    assert(text_array_resize(&array, 1) && live == 1);
    assert(text_array_resize(&array, 6) && array.capacity == 6);
    equal(&array, 1, "", 0); equal(&array, 5, "", 0);
    assert(owned_text_assign(&array.items[0], text_view_cstr("preserved")));
    items = array.items; count = live;
    fail_malloc = 1;
    assert(!text_array_append(&array, text_view_cstr("copy fails")));
    fail_malloc = 0; fail_realloc = 1;
    assert(!text_array_append(&array, text_view_cstr("growth fails after copy")));
    assert(!text_array_resize(&array, 7));
    assert(!text_array_reserve(&array, 100));
    assert(text_array_reserve(&array, 6));
    fail_realloc = 0;
    assert(array.items == items && array.size == 6 && array.capacity == 6 && live == count);
    equal(&array, 0, "preserved", 9);
    assert(!text_array_resize(&array, (size_t)-1));
    assert(!text_array_append(&array, text_view("", (size_t)-1)));
    overflow.size = overflow.capacity = (size_t)-1 / sizeof *overflow.items;
    assert(!text_array_append(&overflow, text_view_cstr("overflow")));
    assert(live == count);
    text_array_clear(&array); text_array_clear(&array);
    assert(!live && !array.items && !array.size && !array.capacity);
    puts("Text array: aliasing, ordering, resize, allocation rollback and cleanup passed.");
    return 0;
}
