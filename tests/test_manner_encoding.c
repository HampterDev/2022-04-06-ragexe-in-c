#define ENCODING_FIXTURE_IMPLEMENTATION
#include "encoding_fixture.h"
#include "contents/manner_filter.h"
#include <assert.h>
#include <string.h>

UINT g_codePage = 949;

void test_manner_encoding(void)
{
    char *words[] = {"bad", "\xb0\xa1\xb3\xaa"};
    struct WordFilter filter = {words, 2, 2};
    char text[128];
    encoding_test_reset();
    strcpy(text, "ok BAD and bad");
    word_filter_censor(&filter, text);
    assert(!strcmp(text, "ok *** and ***") && !encoding_live);
    strcpy(text, "\xb0\xa1\xb3\xaa \xb4\xd9");
    word_filter_censor(&filter, text);
    assert(!strcmp(text, "** \xb4\xd9") && !encoding_live);
    strcpy(text, "clean");
    word_filter_censor(&filter, text);
    assert(!strcmp(text, "clean") && !encoding_live);
    encoding_test_reset();
    strcpy(text, "a bad word");
    word_filter_censor(&filter, text);
    assert(!strcmp(text, "a *** word"));
    assert(!encoding_allocations && !encoding_calls);
    {
        char long_text[4096];
        memset(long_text, 'a', sizeof long_text - 1);
        long_text[sizeof long_text - 1] = 0;
        encoding_test_reset(); encoding_fail_allocation = 1;
        assert(!word_filter_is_clean(&filter, long_text));
        assert(strlen(long_text) == sizeof long_text - 1 && !encoding_live);
        encoding_test_reset(); encoding_fail_allocation = 1;
        word_filter_censor(&filter, long_text);
        assert(!long_text[0] && !encoding_live);
        encoding_test_reset();
    }
}
