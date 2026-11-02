#include "iteminfo/iteminfo.h"
#include "platform/language_api.h"
#include "core/memory.h"
#include "core/fatal.h"
#include <assert.h>
#include <limits.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

void exercise_item_restrictions(void);
struct Language *g_language;
static struct { void *pointer; size_t size; } allocations[32];
static struct { int id; int flags[8]; } rows[16];
static const char *input;
static int row_count, failed_read, negative_size, warning_count, failed, aborted;
static int opened, closed, destroyed, read_count, localized;
static jmp_buf stop;

void *mem_alloc(size_t size)
{
    size_t i;
    void *p = malloc(size ? size : 1);
    assert(p);
    memset(p, 0xa5, size);
    for (i = 0; i < 32 && allocations[i].pointer; ++i) {}
    assert(i < 32);
    allocations[i].pointer = p;
    allocations[i].size = size;
    return p;
}

void mem_free(void *p)
{
    size_t i;
    if (!p) return;
    for (i = 0; i < 32 && allocations[i].pointer != p; ++i) {}
    assert(i < 32);
    free(p);
    allocations[i].pointer = NULL;
}

__declspec(noreturn) void fatal_error(const char *message)
{
    fprintf(stderr, "Unexpected fatal error: %s\n", message);
    abort();
}

const char *language_language_file(struct Language *language, const char *name)
{
    assert(language == g_language && !strcmp(name, "ItemMoveInfoV5.txt"));
    ++localized;
    return "localized/ItemMoveInfoV5.txt";
}

struct File *file_construct(struct File *file)
{
    memset(file, 0, sizeof *file);
    return file;
}

unsigned char file_open(struct File *file, const char *name, int mode, unsigned char resolve, unsigned char disk_only)
{
    assert(!mode && resolve && !disk_only);
    assert(!strcmp(name, g_language ? "localized/ItemMoveInfoV5.txt" : "ItemMoveInfoV5.txt"));
    ++opened;
    if (!input) return 0;
    file->size = negative_size ? -1 : (int)strlen(input);
    return 1;
}

unsigned char file_read(struct File *file, void *out, int size)
{
    assert(!negative_size && size == file->size && size == (int)strlen(input));
    ++read_count;
    if (failed_read) return 0;
    memcpy(out, input, (size_t)size);
    return 1;
}

void file_close(struct File *file) { (void)file; ++closed; }
void file_destruct(struct File *file) { (void)file; ++destroyed; }

int WINAPI restriction_error_box(HWND window, LPCSTR text, LPCSTR caption, UINT type)
{
    assert(!window && !strcmp(text, "Can't open file") && !strcmp(caption, "ErrorMessage") && type == MB_OK);
    ++warning_count;
    return 0;
}

void item_move_info_add(int id, const int flags[8])
{
    size_t i;
    uintptr_t start = (uintptr_t)flags, end = start + 8 * sizeof *flags;
    for (i = 0; i < 32; ++i) {
        uintptr_t base = (uintptr_t)allocations[i].pointer;
        if (base && start >= base && end <= base + allocations[i].size)
            break;
    }
    if (i == 32) {
        fprintf(stderr, "FAIL: startup tried to read eight restriction flags beyond the parsed buffer (item %d).\n", id);
        ++failed;
        longjmp(stop, 1);
    }
    assert(row_count < 16);
    rows[row_count].id = id;
    memcpy(rows[row_count].flags, flags, sizeof rows[row_count].flags);
    ++row_count;
}

static void run(const char *text, int expected_rows)
{
    size_t i;
    input = text;
    row_count = opened = closed = destroyed = read_count = 0;
    aborted = 0;
    if (setjmp(stop)) aborted = 1;
    else exercise_item_restrictions();
    if (row_count != expected_rows) {
        fprintf(stderr, "FAIL: expected %d rows, got %d.\n", expected_rows, row_count);
        ++failed;
    }
    if (!aborted) {
        assert(opened == 1 && destroyed == 1);
        assert(read_count == (text && !negative_size ? 1 : 0));
        for (i = 0; i < 32; ++i) assert(!allocations[i].pointer);
    } else {
        for (i = 0; i < 32; ++i) if (allocations[i].pointer) mem_free(allocations[i].pointer);
    }
}

static void expect_first(void)
{
    static const int flags[8] = {1, 0, 1, 0, 1, 0, 1, 0};
    assert(row_count && rows[0].id == 501 && !memcmp(rows[0].flags, flags, sizeof flags));
}

int main(void)
{
    run("501 1 0 1 0 1 0 1 0 \r\n", 1); expect_first();
    run("501\t1\t0\t1\t0\t1\t0\t1\t0\t// Red_Potion\r\n502\t0\t1\t0\t1\t0\t1\t0\t1\t//\r\n", 2); expect_first();
    assert(rows[1].id == 502 && rows[1].flags[7] == 1);
    run("501 1 0 1 0 1 0 1 0\n", 0);
    run("501 1 0 1 0 1 0 1 0", 0);
    run("501 1 0 1 0 1 0 1 0\t\n502 0 1 0 1 0 1 0 1", 1); expect_first();
    run("501\t1  0\t1 0 1 0 1 0\t", 1);
    assert(rows[0].id == 501 && rows[0].flags[0] == 1 && rows[0].flags[1] == 0 && rows[0].flags[2] == 0 && rows[0].flags[3] == 1);
    run(" 501 1 0 1 0 1 0 1 ", 1);
    assert(rows[0].id == 0 && rows[0].flags[0] == 501 && rows[0].flags[7] == 1);
    run("// 10 20\r\n501 1 0 1 0 1 0 1 0 // 99 88\n//end", 1); expect_first();
    run("\xef\xbb\xbf// ItemMoveInfoV5\r\n501\t1\t0\t1\t0\t1\t0\t1\t0\t//\r\n", 1); expect_first();
    run("501 1 0 broken 0 1 0 1 0 ", 1);
    assert(rows[0].flags[0] == 1 && rows[0].flags[2] == 0 && rows[0].flags[4] == 1);
    run("501 1 0 1x 0 1 0 1 0 # note\r\n", 1);
    assert(rows[0].flags[2] == 1 && rows[0].flags[7] == 0);
    run("501 1 ", 0);
    run("501 1 0 1 0 1 0 1 0 \n502 1 ", 1); expect_first();
    run("", 0);
    run(" \r\n\t// no records", 0);
    run("501 -2147483648 2147483647 +1 0 1 0 1 0 ", 1);
    assert(rows[0].flags[0] == INT_MIN && rows[0].flags[1] == INT_MAX && rows[0].flags[2] == 1);
    failed_read = 1; run("501 1 0 1 0 1 0 1 0 ", 0); failed_read = 0;
    negative_size = 1; run("501 ", 0); negative_size = 0;
    run(NULL, 0); assert(warning_count == 1);
    g_language = (struct Language *)&localized;
    run("501 1 0 1 0 1 0 1 0 ", 1); expect_first();
    assert(localized == 1);
    if (failed) return 1;
    puts("Item restrictions: the original's tab and space tokenizer with atoi, // comments, line ends, stray words and a BOM, partial records, read failures, localization and cleanup passed.");
    return 0;
}
