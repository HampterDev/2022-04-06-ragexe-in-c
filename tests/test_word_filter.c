#include "contents/manner_filter.h"
#include "core/table_reader.h"
#include "core/client.h"
#include "world_state_memory.h"
#include <stdio.h>

struct Language *g_language;
unsigned g_codePage = 949;
static const char *resource_text;
static unsigned dictionary_allocations, fail_dictionary_allocation, localized_reads;

static void *dictionary_malloc(size_t size)
{
    if (++dictionary_allocations == fail_dictionary_allocation) return NULL;
    return mem_alloc(size);
}
static void dictionary_free(void *memory) { mem_free(memory); }
static void *dictionary_realloc(void *memory, size_t size)
{
    void *next;
    size_t old_size = 0;
    unsigned i;
    if (++dictionary_allocations == fail_dictionary_allocation) return NULL;
    if (memory) {
        for (i = 0; i < 2048 && blocks[i].memory != memory; i++) {}
        assert(i < 2048);
        old_size = blocks[i].size;
    }
    next = mem_alloc(size);
    if (memory) memcpy(next, memory, old_size < size ? old_size : size);
    dictionary_free(memory);
    return next;
}
#define malloc dictionary_malloc
#define realloc dictionary_realloc
#define free dictionary_free
#include "../src/contents/manner_words.c"
#undef malloc
#undef realloc
#undef free

const char *language_language_file(struct Language *language, const char *name)
{
    assert(language == g_language && language);
    assert(!strcmp(name, "manner.txt"));
    localized_reads++;
    return "localized/manner.txt";
}
struct File *file_construct(struct File *self)
{ memset(self, 0, sizeof *self); return self; }
unsigned char file_open(struct File *self, const char *name, int mode,
                        unsigned char a3, unsigned char a4)
{
    assert(mode == 0 && a3 == 1 && a4 == 0);
    assert(!strcmp(name, g_language ? "localized/manner.txt" : "manner.txt"));
    if (!resource_text) return 0;
    self->size = (int)strlen(resource_text);
    return 1;
}
unsigned char file_read(struct File *self, void *out, int size)
{
    assert(size == self->size && size == (int)strlen(resource_text));
    memcpy(out, resource_text, (size_t)size);
    return 1;
}
void file_close(struct File *self) { (void)self; }
void file_destruct(struct File *self) { (void)self; }

static void test_owned_lines(void)
{
    struct WordFilter filter = {0};
    char borrowed[] = "BAD";
    unsigned i, live;
    char **words;
    assert(word_filter_add(&filter, " \t"));
    assert(!filter.count && !live_blocks);
    assert(word_filter_add(&filter, borrowed));
    borrowed[0] = 'X';
    assert(!strcmp(filter.words[0], "bad"));
    assert(word_filter_add(&filter, "  MIXED \t"));
    assert(!strcmp(filter.words[1], "  mixed \t"));
    assert(word_filter_add(&filter, "\xb0\xa1\xb3\xaa"));
    assert(word_filter_contains(&filter, "a BAD word"));
    assert(word_filter_contains(&filter, "\xb4\xd9\xb0\xa1\xb3\xaa"));
    assert(!word_filter_contains(&filter, "<ITEML>bad</ITEML> good"));
    assert(word_filter_contains(&filter, "<ITEML>good</ITEML> BAD"));
    while (filter.count < filter.capacity) assert(word_filter_add(&filter, "word"));
    live = live_blocks; words = filter.words;
    for (i = 1; i <= 2; i++) {
        dictionary_allocations = 0; fail_dictionary_allocation = i;
        assert(!word_filter_add(&filter, "new"));
        assert(filter.count == 8 && filter.words == words && live_blocks == live);
        assert(!strcmp(filter.words[0], "bad"));
    }
    fail_dictionary_allocation = 0;
    assert(word_filter_add(&filter, "new"));
    assert(filter.count == 9 && filter.capacity == 16);
    word_filter_clear(&filter);
    word_filter_clear(&filter);
    assert(!filter.words && !filter.count && !filter.capacity && !live_blocks);
}

static void test_reload(void)
{
    struct WordFilter filter = {0}, independent = {0};
    char **old_words;
    unsigned allocations, fail, before, i;
    resource_text = "OLD\r\n";
    assert(word_filter_load(&filter, "manner.txt"));
    assert(filter.count == 1 && live_blocks == 2);
    assert(word_filter_load(&independent, "manner.txt"));
    assert(independent.words[0] != filter.words[0]);
    word_filter_clear(&independent);
    old_words = filter.words;
    before = live_blocks;
    resource_text = NULL;
    assert(!word_filter_load(&filter, "manner.txt"));
    assert(filter.words == old_words && live_blocks == before);
    assert(word_filter_contains(&filter, "OLD"));

    resource_text = "BAD\r\n  \t\r\n  WORD \r\n\xb0\xa1\xb3\xaa\r\n// comment\r\n";
    dictionary_allocations = 0;
    assert(word_filter_load(&independent, "manner.txt"));
    allocations = dictionary_allocations;
    assert(independent.count == 3);
    word_filter_clear(&independent);
    for (fail = 1; fail <= allocations; fail++) {
        dictionary_allocations = 0; fail_dictionary_allocation = fail;
        assert(!word_filter_load(&filter, "manner.txt"));
        assert(filter.words == old_words && filter.count == 1 && live_blocks == before);
        assert(word_filter_contains(&filter, "OLD"));
    }
    fail_dictionary_allocation = 0;
    g_language = (struct Language *)&localized_reads;
    for (i = 0; i < 20; i++) {
        assert(word_filter_load(&filter, "manner.txt"));
        assert(filter.count == 3 && live_blocks == 4);
        assert(!word_filter_contains(&filter, "old"));
        assert(word_filter_contains(&filter, "bad"));
        assert(!strcmp(filter.words[1], "  word "));
        assert(!strcmp(filter.words[2], "\xb0\xa1\xb3\xaa"));
    }
    assert(localized_reads == 20);
    resource_text = "";
    assert(word_filter_load(&filter, "manner.txt"));
    assert(!filter.words && !filter.count && !live_blocks);
    name_filter_init();
    assert(word_filter_add(&g_nameFilter, "global"));
    assert(word_filter_contains(&g_nameFilter, "GLOBAL"));
    word_filter_clear(&g_nameFilter);
    assert(!live_blocks);
}

static void test_censor_bounds(void)
{
    struct WordFilter filter = {0};
    char word[321], text[4097];
    size_t i;
    memset(word, 'A', sizeof word - 1); word[sizeof word - 1] = 0;
    assert(word_filter_add(&filter, word));
    memset(text, 'a', sizeof text - 1); text[sizeof text - 1] = 0;
    assert(word_filter_contains(&filter, text));
    word_filter_censor(&filter, text);
    assert(strlen(text) == sizeof text - 1);
    for (i = 0; i < 3840; i++) assert(text[i] == '*');
    for (; i < sizeof text - 1; i++) assert(text[i] == 'a');
    word_filter_clear(&filter);
    assert(word_filter_add(&filter, "bad"));
    strcpy(text, "<ITEML>bad</ITEML> BAD and bad");
    word_filter_censor(&filter, text);
    assert(!strcmp(text, "<iteml>bad</iteml> *** and ***"));
    strcpy(text, "<ITEML>\xb0\xa1\xb3\xaa</ITEML> BAD");
    assert(word_filter_contains(&filter, text));
    word_filter_censor(&filter, text);
    assert(!strcmp(text, "<iteml>\xb0\xa1\xb3\xaa</iteml> ***"));
    assert(word_filter_add(&filter, "\xb0\xa1\xb3\xaa"));
    strcpy(text, "\xb0\xa1\xb3\xaa BAD \xb0\xa1\xb3\xaa");
    word_filter_censor(&filter, text);
    assert(!strcmp(text, "** *** **"));
    strcpy(text, "BAD \x81 BAD \x81");
    word_filter_censor(&filter, text);
    assert(!strcmp(text, "*** \x81 *** \x81"));
    assert(word_filter_add(&filter, "\x81"));
    strcpy(text, "BAD \x81");
    word_filter_censor(&filter, text);
    assert(!strcmp(text, "*** *"));
    strcpy(text, "\x81\x41");
    assert(!word_filter_contains(&filter, text));
    word_filter_censor(&filter, text);
    assert(!strcmp(text, "\x81\x41"));
    word_filter_clear(&filter);
    assert(!live_blocks);
}

int main(void)
{
    LCID previous = GetThreadLocale();
    assert(SetThreadLocale(MAKELCID(MAKELANGID(LANG_KOREAN, SUBLANG_DEFAULT), SORT_DEFAULT)));
    test_owned_lines();
    test_reload();
    test_censor_bounds();
    assert(SetThreadLocale(previous));
    puts("word filter: owned copies, transactional reload, allocation failure, CP949 matching and long-text bounds passed");
    return 0;
}
