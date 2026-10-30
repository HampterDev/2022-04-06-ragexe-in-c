#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ui/quests/npc_menu.h"
#include "ui/quests/say_dialog.h"
#include "ui/quests/tag_text_viewer.h"
#include "ui/controls/listbox.h"
#include "owned_storage_test_allocator.h"

static void *moving_realloc(void *old, size_t size)
{
    void *next = checked_malloc(size);
    if (next && old) {
        size_t before = ((struct Allocation *)old - 1)->size;
        memcpy(next, old, before < size ? before : size);
        checked_free(old);
    }
    return next;
}

#define realloc moving_realloc
#define free checked_free
#include "../src/replay/snapshot.c"
#undef realloc
#undef free
#include "../src/ui/record_stream.c"

static unsigned char expected[8192];
static size_t expected_size;
static void expect_record(unsigned tag, const void *payload, size_t size)
{
    size_t i;
    assert(expected_size + 6 + size <= sizeof expected);
    expected[expected_size++] = (unsigned char)tag;
    expected[expected_size++] = (unsigned char)(tag >> 8);
    for (i = 0; i < 4; ++i)
        expected[expected_size++] = (unsigned char)(size >> (i * 8));
    if (size) memcpy(expected + expected_size, payload, size);
    expected_size += size;
}

static void check_writer(struct UIWindow *window, size_t owner_size, int valid)
{
    struct GuardedOutput {
        uint32_t before;
        struct ReplayBuffer records;
        uint32_t after;
    } output = {0x12345678, {0}, 0xabcdef01};
    unsigned char owner[512];
    unsigned calls = 0, failure;
    assert(owner_size <= sizeof owner);
    memcpy(owner, window, owner_size);
    for (failure = 0; failure <= calls; ++failure) {
        unsigned char *old;
        output.records.data = checked_malloc(3);
        output.records.size = 3;
        memcpy(output.records.data, "old", 3);
        old = output.records.data;
        allocation_calls = 0;
        fail_at = failure;
        if (!failure) {
            assert(ui_window_get_records(UI_WINDOW(window), &output.records));
            calls = allocation_calls;
            assert(calls);
            assert(output.records.size == expected_size);
            assert(!memcmp(output.records.data, expected, expected_size));
            assert((ui_window_records_validate(output.records.data, output.records.size) >= 0) == valid);
        } else {
            assert(!ui_window_get_records(UI_WINDOW(window), &output.records));
            assert(output.records.data == old && output.records.size == 3);
            assert(!memcmp(output.records.data, "old", 3));
        }
        fail_at = 0;
        assert(output.before == 0x12345678 && output.after == 0xabcdef01);
        assert(!memcmp(window, owner, owner_size));
        assert(outstanding == 1);
        replay_buffer_destroy(&output.records);
        assert(!outstanding && !output.records.data && !output.records.size);
    }
}

static void string_fixture(struct String *out, const char *text)
{
    memset(out, 0, sizeof *out);
    out->size = out->capacity = strlen(text);
    out->data = (char *)text;
}

static void menu_case(const char *const *lines, size_t count)
{
    static struct UIWindowOps menu_ops = {.get_records = ui_choose_wnd_save_state};
    static struct UIWindowOps warp_ops = {.get_records = ui_choose_warp_wnd_save_state};
    struct UIChooseWnd menu = {0};
    struct UIChooseWarpWnd warp = {0};
    struct UIListBox list = {0}, saved_list;
    struct String strings[8], saved_strings[8];
    size_t i;
    int kind, valid = 1;
    assert(count <= 8);
    memset(strings, 0, sizeof strings);
    for (i = 0; i < count; ++i) {
        string_fixture(&strings[i], lines[i]);
        if (strlen(lines[i]) > 259) valid = 0;
    }
    memcpy(saved_strings, strings, sizeof strings);
    list.items.first = count ? strings : NULL;
    list.items.last = count ? strings + count : NULL;
    list.items.end = list.items.last;
    saved_list = list;
    menu.frame.base.ops = &menu_ops;
    menu.list = &list;
    menu.line_index = 99;
    warp.frame.base.ops = &warp_ops;
    warp.destinations.first = list.items.first;
    warp.destinations.last = list.items.last;
    warp.destinations.end = list.items.end;
    for (kind = 0; kind < 2; ++kind) {
        expected_size = 0;
        expect_record(kind ? 0x55be : 0x558c, NULL, 0);
        for (i = 0; i < count; ++i)
            expect_record(kind ? 0x55c1 : 0x558f, lines[i], strlen(lines[i]));
        expect_record(kind ? 0x55bf : 0x558d, NULL, 0);
        if (kind) check_writer(&warp.frame.base, sizeof warp, valid);
        else check_writer(&menu.frame.base, sizeof menu, valid);
        assert(!memcmp(&list, &saved_list, sizeof list));
        assert(!memcmp(strings, saved_strings, sizeof strings));
    }
}

static void say_case(const char *text, int has_text, int clear)
{
    static struct UIWindowOps ops = {.get_records = ui_say_dialog_wnd_save_state};
    struct UISayDialogWnd dialog = {0};
    struct UITagTextViewer viewer = {0}, saved_viewer;
    unsigned char clear_bytes[4];
    unsigned i;
    dialog.frame.base.ops = &ops;
    dialog.text = &viewer;
    dialog.clear_on_text = clear;
    viewer.has_text = has_text;
    viewer.text.data = (char *)text;
    viewer.text.size = viewer.text.capacity = text ? strlen(text) : 0;
    saved_viewer = viewer;
    for (i = 0; i < 4; ++i) clear_bytes[i] = (unsigned char)((uint32_t)clear >> (i * 8));
    expected_size = 0;
    expect_record(0x57e4, NULL, 0);
    if (has_text) expect_record(0x57e7, text, viewer.text.size);
    expect_record(0x57e8, clear_bytes, sizeof clear_bytes);
    expect_record(0x57e5, NULL, 0);
    check_writer(&dialog.frame.base, sizeof dialog, !has_text || viewer.text.size <= 259);
    assert(!memcmp(&viewer, &saved_viewer, sizeof viewer));
}

int main(void)
{
    const char *lines[] = {"", "한글", "a longer destination name", "last"};
    char boundary[260], long_text[1025];
    const char *long_lines[] = {boundary, "한글", long_text};
    memset(boundary, 'b', sizeof boundary - 1); boundary[sizeof boundary - 1] = 0;
    memset(long_text, 'x', sizeof long_text - 1); long_text[sizeof long_text - 1] = 0;
    menu_case(NULL, 0);
    menu_case(lines, 1);
    menu_case(lines, 4);
    menu_case(long_lines, 1);
    menu_case(long_lines, 3);
    say_case(NULL, 0, 0);
    say_case("ignored", 0, 1);
    say_case(NULL, 1, 0);
    say_case("", 1, 1);
    say_case("한글\nsecond line", 1, 0x12345678);
    say_case(boundary, 1, -1);
    say_case(long_text, 1, 1);
    puts("NPC replay: three typed callbacks, exact record bytes, Korean and long text, output guards, "
         "replacement ownership and every allocation failure passed.");
    return 0;
}
