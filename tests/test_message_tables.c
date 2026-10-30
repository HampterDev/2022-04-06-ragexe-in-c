#include "core/msg_str.h"
#include "core/table_reader.h"
#include "core/client.h"
#include "world_state_memory.h"
#include <limits.h>
#include <stdio.h>

unsigned char table_token_is_number(const char *text);
int g_serviceType;
unsigned char g_useCharsetTag;
struct Language *g_language;
static const char *resource_text, *charset_tag = "|00";
static unsigned opens, closes, language_lookups, tag_lookups;
static int fail_tag_allocation, fail_read, negative_size;
static unsigned destructs, reads;

static void *message_malloc(size_t size)
{
    if (fail_tag_allocation) return NULL;
    return mem_alloc(size);
}
static void message_free(void *memory) { mem_free(memory); }
static void *reader_realloc(void *memory, size_t size)
{
    size_t old_size = 0;
    void *next;
    unsigned i;
    if (memory) {
        for (i = 0; i < 2048 && blocks[i].memory != memory; i++) {}
        assert(i < 2048);
        old_size = blocks[i].size;
    }
    next = mem_alloc(size);
    if (memory) memcpy(next, memory, old_size < size ? old_size : size);
    mem_free(memory);
    return next;
}
#define malloc mem_alloc
#define realloc reader_realloc
#define free mem_free
#include "../src/core/table_reader.c"
#undef malloc
#undef realloc
#undef free

#define malloc message_malloc
#define free message_free
#include "../src/core/msg_str.c"
#undef malloc
#undef free

const char *language_language_file(struct Language *language, const char *name)
{
    assert(language == g_language && language);
    assert(!strcmp(name, "msgStringTable.txt"));
    language_lookups++;
    return "localized/messages.txt";
}
const char *language_charset_tag(struct Language *language, unsigned char which)
{
    assert(language == g_language && which == 1);
    tag_lookups++;
    return charset_tag;
}
struct File *file_construct(struct File *self)
{
    memset(self, 0, sizeof *self);
    return self;
}
unsigned char file_open(struct File *self, const char *name, int mode,
                        unsigned char a3, unsigned char a4)
{
    assert(mode == 0 && a3 == 1 && a4 == 0);
    assert(!strcmp(name, g_language ? "localized/messages.txt" : "msgStringTable.txt"));
    opens++;
    if (!resource_text) return 0;
    self->size = negative_size ? -1 : (int)strlen(resource_text);
    return 1;
}
unsigned char file_read(struct File *self, void *out, int size)
{
    assert(size == self->size && size == (int)strlen(resource_text));
    reads++;
    if (fail_read) return 0;
    memcpy(out, resource_text, (size_t)size);
    return 1;
}
void file_close(struct File *self) { (void)self; closes++; }
void file_destruct(struct File *self) { (void)self; destructs++; }

static void test_ids(void)
{
    unsigned c;
    char token[2] = {0, 0};
    for (c = 0; c <= UCHAR_MAX; c++) {
        token[0] = (char)c;
        assert(!!table_token_is_number(token) == (!c || (c >= '0' && c <= '9')));
    }
    assert(table_token_is_number("0123456789"));
    assert(!table_token_is_number("123x"));
    assert(!table_token_is_number("-1"));
    assert(!table_token_is_number("1 2"));
}

static void test_bounds(void)
{
    assert(!strcmp(msg_str(-1), "NO MSG : -1"));
    assert(!strcmp(msg_str(INT_MIN), "NO MSG : -2147483648"));
    assert(!strcmp(msg_str(INT_MAX), "NO MSG : 2147483647"));
    assert(!strcmp(msg_str(MSG_STRING_COUNT), "NO MSG : 3985"));
    assert(!strcmp(msg_str_to_compare(INT_MIN), "NO MSG"));
    assert(!strcmp(msg_str_to_compare(MSG_STRING_COUNT), "NO MSG"));
    assert(!strcmp(msg_str_to_compare(INT_MAX), "NO MSG"));
    assert(!strcmp(msg_str_field(UINT_MAX), "NO MSG"));
}

static void test_reload(void)
{
    unsigned allocations, calls, i;
    char *cached;
    test_bounds();
    assert(!strcmp(msg_str(0), "NO MSG"));
    msg_strings_init();
    resource_text = "hello#// comment with # ignored\r\n/command#NO MSG#\xb0\xa1\xb3\xaa#";
    msg_str_load();
    assert(opens == 1 && closes == 1);
    assert(!strcmp(msg_str(0), "hello"));
    assert(!strcmp(msg_str(1), "/command"));
    assert(!strcmp(msg_str(2), "NO MSG"));
    assert(!strcmp(msg_str(3), "\xb0\xa1\xb3\xaa"));
    assert(!strcmp(msg_str(4), "NO MSG"));
    allocations = live_blocks;
    g_useCharsetTag = 1;
    cached = msg_str(0);
    assert(!strcmp(cached, "|00hello") && live_blocks == allocations + 1);
    calls = tag_lookups;
    assert(msg_str(0) == cached && tag_lookups == calls);
    assert(!strcmp(msg_str(1), "/command") && tag_lookups == calls);
    assert(!strcmp(msg_str(2), "NO MSG") && tag_lookups == calls);
    assert(!strcmp(msg_str_to_compare(0), "hello"));
    assert(!strcmp(msg_str(3), "|00\xb0\xa1\xb3\xaa"));

    g_serviceType = 0x13;
    charset_tag = "|19";
    assert(!strcmp(msg_str(0), "|19hello"));
    allocations = live_blocks;
    for (i = 0; i < 20; i++) assert(!strcmp(msg_str(0), "|19hello"));
    assert(live_blocks == allocations);
    fail_tag_allocation = 1;
    assert(!strcmp(msg_str(0), "hello") && live_blocks == allocations);
    fail_tag_allocation = 0;

    g_serviceType = 0;
    g_language = (struct Language *)&language_lookups;
    resource_text = "new#second#";
    msg_str_load();
    assert(language_lookups == 1);
    assert(!strcmp(msg_str(0), "|19new"));
    fail_tag_allocation = 1;
    allocations = live_blocks;
    assert(!strcmp(msg_str(1), "second") && live_blocks == allocations);
    fail_tag_allocation = 0;
    assert(!strcmp(msg_str(1), "|19second"));
    allocations = live_blocks;
    for (i = 0; i < 20; i++) {
        msg_str_load();
        assert(!strcmp(msg_str(0), "|19new"));
        assert(!strcmp(msg_str(1), "|19second"));
        assert(live_blocks == allocations);
    }
    msg_str_unload();
    msg_str_unload();
    assert(live_blocks == 0 && !strcmp(msg_str(0), "NO MSG"));
    resource_text = NULL;
    msg_str_load();
    assert(!strcmp(msg_str(0), "NO MSG"));
    msg_str_unload();
    assert(live_blocks == 0);
}

static void test_large_table(void)
{
    char input[(MSG_STRING_COUNT + 20) * 2 + 1];
    char long_text[402];
    size_t i;
    for (i = 0; i < sizeof input - 1; i += 2) {
        input[i] = 'x'; input[i + 1] = '#';
    }
    input[sizeof input - 1] = 0;
    resource_text = input;
    msg_str_load();
    assert(!strcmp(msg_str_to_compare(MSG_STRING_COUNT - 1), "x"));
    assert(!strcmp(msg_str_to_compare(MSG_STRING_COUNT), "NO MSG"));
    assert(!strcmp(msg_str_field(MSG_STRING_COUNT + 19), "x"));
    test_bounds();
    memset(long_text, 'a', 400);
    long_text[400] = '#'; long_text[401] = 0;
    resource_text = long_text;
    msg_str_load();
    assert(strlen(msg_str_to_compare(0)) == 400);
    assert(strlen(msg_str(0)) == 259);
    assert(!strncmp(msg_str(0), "|19", 3));
    msg_str_unload();
    assert(live_blocks == 0);
}

static void expect_fields(struct TableReader *reader, enum TableReadMode mode,
                          const char *const *expected, size_t count)
{
    size_t i;
    assert(table_reader_read(reader, "msgStringTable.txt", mode));
    assert(reader->count == count);
    for (i = 0; i < count; i++) {
        if (expected[i]) assert(reader->fields[i] && !strcmp(reader->fields[i], expected[i]));
        else assert(reader->fields[i] == NULL);
    }
}

static void test_reader_modes_and_lifetime(void)
{
    struct TableReader reader = {0};
    const char *fields[] = {"a", "", "b", "d"};
    const char *lines[] = {"a", "", "b", "tail", "line", "d"};
    const char *records[] = {"a", NULL, "", NULL, "b", NULL,
                             "tail", "line", "d", NULL};
    char *borrowed;
    char large[40001];
    unsigned live, calls, done;
    size_t i;
    g_language = NULL;
    resource_text = "a##b#tail\r\n//skip#ignored\r\nline\r\nlost\nd#unfinished";
    expect_fields(&reader, TABLE_FIELDS, fields, 4);
    borrowed = reader.fields[0];
    borrowed[0] = 'A';
    expect_fields(&reader, TABLE_FIELDS_AND_LINES, lines, 6);
    expect_fields(&reader, TABLE_RECORDS, records, 10);
    assert(!strcmp(borrowed, "A"));

    for (i = 0; i < sizeof large - 1; i += 2) {
        large[i] = 'x'; large[i + 1] = '#';
    }
    large[sizeof large - 1] = 0;
    resource_text = large;
    assert(table_reader_read(&reader, "msgStringTable.txt", TABLE_RECORDS));
    assert(reader.count == 40000);
    for (i = 0; i < reader.count; i += 2) {
        assert(!strcmp(reader.fields[i], "x"));
        assert(!reader.fields[i + 1]);
    }
    assert(!strcmp(borrowed, "A"));

    live = live_blocks;
    resource_text = NULL;
    done = destructs;
    assert(!table_reader_read(&reader, "msgStringTable.txt", TABLE_FIELDS));
    assert(!reader.count && live_blocks == live && destructs == done + 1);
    assert(!strcmp(borrowed, "A"));

    resource_text = "failed#";
    fail_read = 1;
    done = destructs;
    assert(!table_reader_read(&reader, "msgStringTable.txt", TABLE_FIELDS));
    assert(!reader.count && live_blocks == live && destructs == done + 1);
    fail_read = 0;
    negative_size = 1;
    calls = reads; done = destructs;
    assert(!table_reader_read(&reader, "msgStringTable.txt", TABLE_FIELDS));
    assert(!reader.count && reads == calls && live_blocks == live && destructs == done + 1);
    negative_size = 0;

    resource_text = "";
    expect_fields(&reader, TABLE_FIELDS, NULL, 0);
    assert(!strcmp(borrowed, "A"));
    table_reader_release(&reader);
    table_reader_release(&reader);
    assert(!reader.fields && !reader.count && !reader.capacity && !reader.blocks);
    assert(!live_blocks);
    resource_text = "again#";
    assert(table_reader_read(&reader, "msgStringTable.txt", TABLE_FIELDS));
    assert(reader.count == 1 && !strcmp(reader.fields[0], "again"));
    table_reader_release(&reader);
    assert(!live_blocks);
}

int main(void)
{
    test_ids();
    test_reload();
    test_large_table();
    test_reader_modes_and_lifetime();
    puts("message tables: ASCII IDs, bounds, resource parsing, tags, reload, allocation failure, reader modes/lifetime/growth and cleanup passed");
    return 0;
}
