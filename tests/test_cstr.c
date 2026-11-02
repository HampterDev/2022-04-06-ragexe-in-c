#include <assert.h>
#include <string.h>
#include "core/cstr.h"

static void check_copy(void)
{
    char buf[6];

    memset(buf, 'x', sizeof buf);
    cstr_copy(buf, sizeof buf, "abc");
    assert(!memcmp(buf, "abc\0xx", sizeof buf));
    cstr_copy(buf, sizeof buf, "abcdefgh");
    assert(!strcmp(buf, "abcde"));
    cstr_copy(buf, 1, "abc");
    assert(buf[0] == 0);
    buf[0] = 'q';
    cstr_copy(buf, 0, "abc");
    assert(buf[0] == 'q');
}

static void check_copy_n(void)
{
    char buf[7];
    const char unterminated[3] = {'a', 'b', 'c'};

    cstr_copy_n(buf, sizeof buf, "ff00aa^", 6);
    assert(!strcmp(buf, "ff00aa"));
    cstr_copy_n(buf, sizeof buf, unterminated, sizeof unterminated);
    assert(!strcmp(buf, "abc"));
    cstr_copy_n(buf, sizeof buf, "ab", 6);
    assert(!strcmp(buf, "ab"));
    cstr_copy_n(buf, 3, "abcdef", 6);
    assert(!strcmp(buf, "ab"));
    cstr_copy_n(buf, sizeof buf, "abc", 0);
    assert(buf[0] == 0);
}

static void check_case(void)
{
    char text[] = "Data\\Texture.BMP_[Z] \xc0\xaf\xc0\xfa";

    assert(cstr_compare_nocase(".BmP", ".bmp") == 0);
    assert(cstr_compare_nocase(".bmp", ".BMQ") < 0);
    assert(cstr_compare_nocase("ab", "A") > 0);
    assert(cstr_compare_nocase("", "") == 0);
    assert(cstr_compare_nocase("[", "A") < 0);
    assert(cstr_compare_nocase("\xb0", "z") > 0);
    cstr_lower(text);
    assert(!strcmp(text, "data\\texture.bmp_[z] \xc0\xaf\xc0\xfa"));
}

int main(void)
{
    check_copy();
    check_copy_n();
    check_case();
    return 0;
}
