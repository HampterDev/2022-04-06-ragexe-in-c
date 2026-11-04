#include <assert.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/std/string.c"

static unsigned live_blocks, allocations;
static int fail_allocation, expect_fatal;
static jmp_buf fatal;

void *mem_alloc(size_t bytes)
{
    void *block;

    if (fail_allocation) {
        assert(expect_fatal);
        longjmp(fatal, 1);
    }
    block = malloc(bytes);
    assert(block);
    ++live_blocks;
    ++allocations;
    return block;
}

void mem_free(void *memory)
{
    assert(memory && live_blocks);
    --live_blocks;
    free(memory);
}

void fatal_error(const char *message)
{
    (void)message;
    assert(expect_fatal);
    longjmp(fatal, 1);
}

static void equal(const struct String *text, const char *bytes, size_t size)
{
    assert(text->size == size && text->capacity >= size);
    assert(!memcmp(string_cstr(text), bytes, size));
    assert(string_cstr(text)[size] == '\0');
}

static void check_bytes_and_capacity(void)
{
    struct String text;
    const char embedded[] = {'a', '\0', 'b', '\0', 'c'};
    char *storage;
    unsigned count;
    size_t i;

    string_init(&text);
    equal(&text, "", 0);
    assert(string_view(&text).data && !string_view(&text).size);
    string_clear(&text);
    string_free(&text);
    assert(!allocations && !live_blocks);
    string_reserve(&text, 255);
    storage = text.data;
    count = allocations;
    for (i = 0; i < 2000; ++i) {
        string_assign(&text, embedded, sizeof embedded);
        equal(&text, embedded, sizeof embedded);
        string_assign_cstr(&text, "앞이름뒤");
        equal(&text, "앞이름뒤", strlen("앞이름뒤"));
        string_clear(&text);
        assert(text.data == storage && allocations == count);
    }
    string_assign_cstr(&text, "abcdef");
    string_truncate(&text, 100);
    equal(&text, "abcdef", 6);
    string_truncate(&text, 3);
    equal(&text, "abc", 3);
    string_resize(&text, 5, 'x');
    equal(&text, "abcxx", 5);
    string_resize(&text, 2, 'x');
    equal(&text, "ab", 2);
    string_free(&text);
    string_free(&text);
    assert(!live_blocks && !text.data && !text.size && !text.capacity);
}

static void check_aliases(void)
{
    struct String text;
    const char *original = "0123456789abcdefghijklmnopqrstuv";
    char expected[65];
    char *old;

    string_init(&text);
    string_assign_cstr(&text, original);
    memcpy(expected, original, 32);
    memcpy(expected + 32, original, 32);
    expected[64] = 0;
    string_append(&text, text.data, text.size);
    equal(&text, expected, 64);
    string_assign(&text, text.data + 9, 40);
    equal(&text, expected + 9, 40);
    old = text.data;
    string_assign(&text, text.data, text.size);
    equal(&text, expected + 9, 40);
    assert(text.data == old);
    string_append(&text, text.data + 4, 20);
    assert(text.size == 60 && !memcmp(text.data + 40, expected + 13, 20));
    string_insert(&text, 0, text.data + 50, 5);
    assert(text.size == 65 && !memcmp(text.data, expected + 23, 5));
    string_substring(&text, &text, 5, 10);
    equal(&text, expected + 9, 10);
    string_format(&text, "%s-%s", string_cstr(&text), string_cstr(&text));
    assert(text.size == 21 && text.data[10] == '-' && !memcmp(text.data, text.data + 11, 10));
    string_assign(&text, text.data + text.size, 0);
    equal(&text, "", 0);
    string_free(&text);
    assert(!live_blocks);
}

static void check_operations(void)
{
    struct String text, other, copy;
    static const uint16_t wide_text[] = {'w', 'i', 'd', 'e', 0}, wide_middle[] = {'i', 'd', 0};
    static const uint16_t wide_high[] = {0xac00, 0};
    struct WString wide;

    string_init(&text);
    string_init(&other);
    string_format(&text, "%d-%s", 42, "x");
    equal(&text, "42-x", 4);
    string_append_format(&text, "/%02d", 7);
    equal(&text, "42-x/07", 7);
    string_append_char(&text, '!');
    string_insert(&text, 0, "<", 1);
    string_erase(&text, 1, 3);
    equal(&text, "<x/07!", 6);
    string_replace(&text, 1, 1, "abc", 3);
    equal(&text, "<abc/07!", 8);
    assert(string_find(&text, "/", 0) == 4 && string_find(&text, "/", 5) == STRING_NPOS);
    assert(string_find(&text, "", 3) == 3 && string_find(&text, "!!", 0) == STRING_NPOS);
    assert(string_rfind(&text, "c") == 3 && string_rfind(&text, "zz") == STRING_NPOS);
    assert(string_find_first_of(&text, "0123456789", 0) == 5);
    string_assign_cstr(&other, "x y");
    equal(&other, "x y", 3);
    string_copy(&copy, &other);
    assert(string_equal(&copy, &other) && string_equal_cstr(&copy, "x y") && !string_equal_cstr(&copy, "x"));
    assert(string_compare(&copy, &other) == 0 && string_compare(&text, &other) < 0);
    string_free(&copy);
    copy = string_from_cstr("from");
    equal(&copy, "from", 4);
    string_free(&copy);
    copy = string_from("a\0b", 3);
    equal(&copy, "a\0b", 3);
    string_free(&other);
    string_move(&other, &copy);
    equal(&other, "a\0b", 3);
    equal(&copy, "", 0);
    wstring_init(&wide);
    assert(!utf16_compare(wstring_cstr(&wide), wide_text + 4));
    wstring_assign(&wide, wide_text, 4);
    assert(wide.size == 4 && !utf16_compare(wstring_cstr(&wide), wide_text));
    wstring_assign(&wide, wide.data + 1, 2);
    assert(wide.size == 2 && !utf16_compare(wstring_cstr(&wide), wide_middle));
    assert(utf16_length(wide_text) == 4 && utf16_length(wide_text + 4) == 0);
    assert(utf16_length_limit(wide_text, 2) == 2 && utf16_length_limit(wide_text, 9) == 4);
    assert(utf16_compare(wide_middle, wide_text) < 0 && utf16_compare(wide_text, wide_middle) > 0);
    assert(utf16_compare(wide_text, wide_text + 4) > 0 && utf16_compare(wide_high, wide_text) > 0);
    wstring_free(&wide);
    string_free(&text);
    string_free(&other);
    assert(!live_blocks);
}

static void check_failures(void)
{
    static struct String text;
    static char *old;
    static size_t capacity;

    string_init(&text);
    string_assign_cstr(&text, "retained");
    old = text.data;
    capacity = text.capacity;
    expect_fatal = 1;
    fail_allocation = 1;
    if (!setjmp(fatal)) {
        string_reserve(&text, 1000);
        assert(!"expected allocation failure");
    }
    equal(&text, "retained", 8);
    assert(text.data == old && text.capacity == capacity);
    fail_allocation = 0;
    if (!setjmp(fatal)) {
        string_assign(&text, "", SIZE_MAX);
        assert(!"expected size overflow");
    }
    equal(&text, "retained", 8);
    if (!setjmp(fatal)) {
        string_append(&text, "", SIZE_MAX - 1);
        assert(!"expected append overflow");
    }
    equal(&text, "retained", 8);
    if (!setjmp(fatal)) {
        string_erase(&text, 9, 1);
        assert(!"expected position failure");
    }
    equal(&text, "retained", 8);
    expect_fatal = 0;
    string_free(&text);
    assert(!live_blocks);
}

int main(void)
{
    check_bytes_and_capacity();
    check_aliases();
    check_operations();
    check_failures();
    puts("String: counted Korean/embedded-NUL bytes, overlapping sources, capacity reuse, operations and fatal "
         "failures passed.");
    return 0;
}
