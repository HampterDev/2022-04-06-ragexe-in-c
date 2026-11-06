#include "contents/manner_filter.h"
#include "world_state_memory.h"
#include <stdio.h>
#include <windows.h>

unsigned g_codePage = 949;

#define free mem_free
#include "../src/contents/manner_words.c"
#undef free

static void test_matching(void)
{
    char *words[] = {"bad", "  mixed \t", "\xb0\xa1\xb3\xaa"};
    struct WordFilter filter = {words, 3, 3};
    assert(word_filter_contains(&filter, "a BAD word"));
    assert(word_filter_contains(&filter, "\xb4\xd9\xb0\xa1\xb3\xaa"));
    assert(word_filter_contains(&filter, "x  MIXED \ty"));
    assert(!word_filter_contains(&filter, "<ITEML>bad</ITEML> good"));
    assert(word_filter_contains(&filter, "<ITEML>good</ITEML> BAD"));
    assert(word_filter_is_clean(&filter, "good"));
}

static void test_censor_bounds(void)
{
    char word[321], text[4097];
    char *long_word[] = {word};
    char *bad[] = {"bad"};
    char *bad_hangul[] = {"bad", "\xb0\xa1\xb3\xaa"};
    char *bad_lead[] = {"bad", "\xb0\xa1\xb3\xaa", "\x81"};
    struct WordFilter filter = {long_word, 1, 1};
    size_t i;
    memset(word, 'a', sizeof word - 1); word[sizeof word - 1] = 0;
    memset(text, 'a', sizeof text - 1); text[sizeof text - 1] = 0;
    assert(word_filter_contains(&filter, text));
    word_filter_censor(&filter, text);
    assert(strlen(text) == sizeof text - 1);
    for (i = 0; i < 3840; i++) assert(text[i] == '*');
    for (; i < sizeof text - 1; i++) assert(text[i] == 'a');
    filter.words = bad;
    strcpy(text, "<ITEML>bad</ITEML> BAD and bad");
    word_filter_censor(&filter, text);
    assert(!strcmp(text, "<iteml>bad</iteml> *** and ***"));
    strcpy(text, "<ITEML>\xb0\xa1\xb3\xaa</ITEML> BAD");
    assert(word_filter_contains(&filter, text));
    word_filter_censor(&filter, text);
    assert(!strcmp(text, "<iteml>\xb0\xa1\xb3\xaa</iteml> ***"));
    filter.words = bad_hangul; filter.count = filter.capacity = 2;
    strcpy(text, "\xb0\xa1\xb3\xaa BAD \xb0\xa1\xb3\xaa");
    word_filter_censor(&filter, text);
    assert(!strcmp(text, "** *** **"));
    strcpy(text, "BAD \x81 BAD \x81");
    word_filter_censor(&filter, text);
    assert(!strcmp(text, "*** \x81 *** \x81"));
    filter.words = bad_lead; filter.count = filter.capacity = 3;
    strcpy(text, "BAD \x81");
    word_filter_censor(&filter, text);
    assert(!strcmp(text, "*** *"));
    strcpy(text, "\x81\x41");
    assert(!word_filter_contains(&filter, text));
    word_filter_censor(&filter, text);
    assert(!strcmp(text, "\x81\x41"));
    assert(!live_blocks);
}

static void test_clear(void)
{
    char **words = mem_alloc(2 * sizeof *words);
    words[0] = strcpy(mem_alloc(4), "bad");
    words[1] = strcpy(mem_alloc(7), "global");
    g_nameFilter.words = words;
    g_nameFilter.count = g_nameFilter.capacity = 2;
    name_filter_init();
    assert(word_filter_contains(&g_nameFilter, "GLOBAL"));
    word_filter_clear(&g_nameFilter);
    word_filter_clear(&g_nameFilter);
    assert(!g_nameFilter.words && !g_nameFilter.count && !g_nameFilter.capacity && !live_blocks);
}

int main(void)
{
    LCID previous = GetThreadLocale();
    assert(SetThreadLocale(MAKELCID(MAKELANGID(LANG_KOREAN, SUBLANG_DEFAULT), SORT_DEFAULT)));
    test_matching();
    test_censor_bounds();
    test_clear();
    assert(SetThreadLocale(previous));
    puts("word filter: CP949 matching, item links, long-text bounds and clearing passed");
    return 0;
}
