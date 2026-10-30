#include <assert.h>
#include <limits.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ui/character/select_char_storage.h"

static struct Block { unsigned char *data; size_t bytes; } blocks[4096];
static unsigned live, allocations;
static int expect_failure;
static jmp_buf failed;

void *mem_alloc(size_t bytes)
{
    unsigned i;
    unsigned char *raw;
    assert(bytes && bytes < 1000000);
    for (i = 0; i < 4096 && blocks[i].data; i++) {}
    assert(i < 4096);
    raw = malloc(bytes + 32);
    assert(raw);
    memset(raw, 0xa7, 16);
    memset(raw + 16, 0x5b, bytes);
    memset(raw + 16 + bytes, 0x7a, 16);
    blocks[i].data = raw + 16;
    blocks[i].bytes = bytes;
    live++;
    allocations++;
    return blocks[i].data;
}

void mem_free(void *memory)
{
    unsigned i, j;
    unsigned char *data = memory;
    if (!memory) return;
    for (i = 0; i < 4096 && blocks[i].data != memory; i++) {}
    assert(i < 4096);
    for (j = 0; j < 16; j++) {
        assert(data[(int)j - 16] == 0xa7);
        assert(data[blocks[i].bytes + j] == 0x7a);
    }
    free(data - 16);
    blocks[i].data = NULL;
    live--;
}

__declspec(noreturn) void fatal_error(const char *message)
{
    assert(expect_failure);
    assert(!strcmp(message, "invalid character slot count") ||
           !strcmp(message, "too many character slots"));
    longjmp(failed, 1);
}

static struct String *preview_string(struct CharPreview *preview, unsigned index)
{
    if (index < 5) return &preview->act[index];
    if (index < 10) return &preview->spr[index - 5];
    if (index == 10) return &preview->imf;
    if (index == 11) return &preview->head_palette_name;
    assert(index == 12);
    return &preview->body_palette_name;
}

static void fill_test_preview(struct CharPreview *preview)
{
    static const char long_path[] = "data\\sprite\\character-preview\\head-and-body-palette.pal";
    unsigned i;
    for (i = 0; i < 13; i++) {
        struct String *text = preview_string(preview, i);
        assert(text->size == 0 && string_cstr(text)[0] == 0);
        string_assign(text, long_path, sizeof long_path - 1);
        assert(!strcmp(string_cstr(text), long_path));
    }
}

static void test_preview(void)
{
    struct CharPreview preview;
    unsigned i;
    memset(&preview, 0x5b, sizeof preview);
    char_preview_init(&preview);
    assert(preview.x == 0x5b5b5b5b && preview.y == 0x5b5b5b5b);
    assert(preview.head_palette == 0x5b5b5b5b && preview.body_palette == 0x5b5b5b5b);
    assert(preview.action_base == 0x5b5b5b5b && preview.action == 0x5b5b5b5b);
    assert(preview.frame == 0x5b5b5b5b && preview.job == 0x5b5b5b5b && preview.robe == 0x5b5b5b5b);
    fill_test_preview(&preview);
    assert(live == 13);
    char_preview_destroy(&preview);
    assert(live == 0);
    char_preview_init(&preview);
    for (i = 0; i < 13; i++) {
        struct String *text = preview_string(&preview, i);
        string_assign(text, "봄", 2);
        assert(text->size == 2 && !memcmp(string_cstr(text), "봄", 3));
    }
    char_preview_destroy(&preview);
    assert(live == 0);
}

static void test_slots(int usable, int max_slot, int expected)
{
    struct SelectCharStorage window;
    int i;
    unsigned before = allocations;
    memset(&window, 0x5b, sizeof window);
    select_char_init_storage(&window, usable, max_slot);
    assert(window.slot_count == expected && window.page_count == expected / 15);
    assert(window.per_page == 15 && window.usable_slots == usable && window.listed_slots == usable);
    for (i = 0; i < SELECT_CHAR_INFO_LINES; i++) assert(window.info_lines[i] == NULL);
    for (i = 0; i < SELECT_CHAR_PER_PAGE; i++) assert(window.names[i] == NULL && window.slots[i] == NULL);
    assert(allocations - before == (expected ? 3u : 0u));
    for (i = 0; i < expected; i++) {
        assert(window.empty[i] == 1);
        window.headgear[i] = (short)(i + 7);
        fill_test_preview(&window.previews[i]);
    }
    assert(live == (expected ? 3u + 13u * expected : 0u));
    window.slots[0] = (struct UIWindow *)&window;
    window.names[0] = (struct UIWindow *)&window;
    window.info_lines[0] = (struct UIWindow *)&window;
    select_char_destroy_storage(&window);
    assert(!window.slot_count && !window.previews && !window.empty && !window.headgear);
    assert(live == 0);
    select_char_destroy_storage(&window);
    assert(live == 0);
}

static void test_invalid(int64_t usable, int max_slot)
{
    static struct SelectCharStorage window, before;
    unsigned count = allocations;
    memset(&window, 0x5b, sizeof window);
    before = window;
    expect_failure = 1;
    if (!setjmp(failed)) {
        select_char_init_storage(&window, usable, max_slot);
        assert(!"invalid count accepted");
    }
    expect_failure = 0;
    assert(allocations == count && live == 0);
    assert(!memcmp(&window, &before, sizeof window));
}

int main(void)
{
    test_preview();
    test_slots(0, -1, 0);
    test_slots(0, 0, 15);
    test_slots(1, -1, 15);
    test_slots(12, 14, 15);
    test_slots(15, 15, 30);
    test_slots(16, -1, 30);
    test_slots(30, 14, 30);
    test_slots(31, 30, 45);
    test_slots(200, 254, 255);
    test_invalid(-1, -1);
    test_invalid(15, -2);
    test_invalid(INT_MAX, -1);
    test_invalid((int64_t)INT_MAX * 3, -1);
    test_invalid(INT64_MAX, -1);
    test_invalid(0, INT_MAX);
    test_invalid((int64_t)SIZE_MAX / sizeof(struct CharPreview), -1);
    puts("Character previews: shared strings, fixed controls, page boundaries and guarded C arrays passed.");
    return 0;
}
