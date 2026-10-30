#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ui/window.h"
#include "ui/controls/text_viewer.h"
#include "replay/snapshot.h"
#define WINDOW_WRITER_CASE_TABLE
#include "window_writer_cases.h"
const char *window_test_dialog_text = "(Alice)Question?";
struct TreeNode *window_test_tree[3];
static int text_length_override = INT_MIN, text_copy_fails;
char *msg_str(int id)
{
    (void)id;
    return "Question?";
}
BOOL ui_text_viewer_get_text(struct UIWindow *viewer, char *out, int count)
{
    const char *text = (const char *)viewer;
    size_t length = strlen(text);
    assert(count >= 0 && (!count || (size_t)count <= length));
    if (text_copy_fails)
        return FALSE;
    memcpy(out, text, count ? (size_t)count : length);
    return TRUE;
}
int ui_text_viewer_text_length(struct UIWindow *viewer)
{
    return text_length_override != INT_MIN ? text_length_override : (int)strlen((const char *)viewer);
}
struct TreeNode *tree_next(struct TreeNode *node)
{
    if (node == window_test_tree[1])
        return window_test_tree[2];
    assert(node == window_test_tree[2]);
    return window_test_tree[0];
}
void *mem_alloc(size_t size)
{
    void *p = malloc(size);
    assert(p);
    return p;
}
void mem_free(void *p)
{
    free(p);
}
void fatal_error(const char *text)
{
    (void)text;
    abort();
}
static struct Block {
    unsigned char *base;
    size_t size;
} blocks[256];
static int allocations, attempts, fail_at;
static int block_index(void *p)
{
    int i;
    for (i = 0; i < 256; ++i)
        if (blocks[i].base && blocks[i].base + 8 == p)
            return i;
    assert(0);
    return -1;
}
static void check_guards(void)
{
    int i, j;
    for (i = 0; i < 256; ++i)
        if (blocks[i].base)
            for (j = 0; j < 8; ++j) {
                assert(blocks[i].base[j] == 0xa7);
                assert(blocks[i].base[8 + blocks[i].size + j] == 0xa7);
            }
}
void *window_allocate(size_t size)
{
    int i;
    if (++attempts == fail_at)
        return NULL;
    for (i = 0; i < 256 && blocks[i].base; ++i) {
    }
    assert(i < 256 && size <= SIZE_MAX - 16);
    blocks[i].base = malloc(size + 16);
    assert(blocks[i].base);
    blocks[i].size = size;
    memset(blocks[i].base, 0xa7, size + 16);
    ++allocations;
    return blocks[i].base + 8;
}
void window_release(void *p)
{
    int i;
    if (!p)
        return;
    check_guards();
    i = block_index(p);
    free(blocks[i].base);
    blocks[i].base = NULL;
    --allocations;
}
void *window_resize(void *p, size_t size)
{
    void *result = window_allocate(size);
    if (result && p) {
        size_t old_size = blocks[block_index(p)].size;
        memcpy(result, p, size < old_size ? size : old_size);
        window_release(p);
    }
    return result;
}

#ifndef WINDOW_WRITER_ORACLE
#define malloc window_allocate
#define realloc window_resize
#define free window_release
#include "../src/replay/snapshot.c"
#include "../src/ui/record_stream.c"
#include "../src/ui/record_text.c"
#undef malloc
#undef realloc
#undef free
#include "fixtures/window_writers_wire.h"
#include "window_record_readers.h"
#endif

int main(void)
{
    size_t i, fixture = 0;
    for (i = 0; i < sizeof window_writer_cases / sizeof window_writer_cases[0]; ++i) {
        const struct WindowWriterCase *test = &window_writer_cases[i];
        int variant;
        for (variant = 0; variant < test->variants; ++variant) {
            struct ReplayBuffer out = {0};
            size_t j;
            attempts = 0;
            assert(test->write(variant, &out));
#ifdef WINDOW_WRITER_ORACLE
            printf("WIRE %s %d ", test->name, variant);
            for (j = 0; j < out.size; ++j)
                printf("%02x", out.data[j]);
            putchar('\n');
            free(out.data);
#else
            int count = attempts, fail;
            (void)j;
            assert(fixture < sizeof expected_window_writers / sizeof expected_window_writers[0]);
            assert(!strcmp(test->name, expected_window_writers[fixture].name));
            assert(variant == expected_window_writers[fixture].variant);
            assert(out.size == expected_window_writers[fixture].size);
            assert(!out.size || !memcmp(out.data, expected_window_writers[fixture].data, out.size));
            assert(ui_window_records_validate(out.data, out.size) == (int)out.size);
            replay_buffer_destroy(&out);
            assert(!allocations);
            for (fail = 1; fail <= count; ++fail) {
                unsigned char *old;
                out.data = window_allocate(3);
                out.size = 3;
                memcpy(out.data, "old", 3);
                old = out.data;
                attempts = 0;
                fail_at = fail;
                assert(!test->write(variant, &out));
                fail_at = 0;
                assert(out.data == old && out.size == 3 && !memcmp(out.data, "old", 3));
                replay_buffer_destroy(&out);
                assert(!allocations);
            }
            ++fixture;
#endif
        }
    }
#ifndef WINDOW_WRITER_ORACLE
    {
        struct ReplayBuffer out = {0};
        char long_text[1024];
        memset(long_text, 'x', sizeof long_text - 1);
        long_text[sizeof long_text - 1] = 0;
        out.data = window_allocate(3);
        out.size = 3;
        memcpy(out.data, "old", 3);
        assert(!ui_dialog_records_save(NULL, "Question?", JOIN_FRIEND_REC_BEGIN, JOIN_FRIEND_REC_NAME,
                                       JOIN_FRIEND_REC_END, &out));
        assert(!ui_dialog_records_save((struct UIWindow *)"x", "Question?", JOIN_FRIEND_REC_BEGIN,
                                       JOIN_FRIEND_REC_NAME, JOIN_FRIEND_REC_END, &out));
        assert(!ui_dialog_records_save((struct UIWindow *)long_text, "Question?", JOIN_FRIEND_REC_BEGIN,
                                       JOIN_FRIEND_REC_NAME, JOIN_FRIEND_REC_END, &out));
        text_length_override = -1;
        assert(!test_write_UIJoinFriendAcceptWnd_GetRecords(0, &out));
        text_length_override = INT_MIN;
        text_copy_fails = 1;
        assert(!test_write_UIExchangeAcceptWnd_GetRecords(0, &out));
        text_copy_fails = 0;
        assert(out.size == 3 && !memcmp(out.data, "old", 3));
        replay_buffer_destroy(&out);
        assert(!allocations);
    }
    assert(fixture == 35);
    test_window_record_readers();
    puts("window writers: all 31 owners, 35 archived-wire cases, shared signatures, bounded dialog names, "
         "packet lists, every C allocation failure and retained output passed");
#else
    (void)fixture;
#endif
    return 0;
}
