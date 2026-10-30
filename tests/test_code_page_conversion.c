#define ENCODING_FIXTURE_IMPLEMENTATION
#include "encoding_fixture.h"
#include "core/text_encoding.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

static void compare(unsigned cp, const char *text)
{
    int count = MultiByteToWideChar(cp, 0, text, -1, NULL, 0);
    wchar_t *expected = malloc((size_t)count * sizeof *expected);
    wchar_t *actual;
    char *roundtrip, *narrow;
    int bytes;
    assert(count && expected);
    assert(MultiByteToWideChar(cp, 0, text, -1, expected, count) == count);
    actual = encoding_to_wide(cp, text);
    assert(actual && !wcscmp(actual, expected));
    bytes = WideCharToMultiByte(cp, 0, expected, -1, NULL, 0, NULL, NULL);
    narrow = malloc((size_t)bytes);
    assert(narrow);
    assert(WideCharToMultiByte(cp, 0, expected, -1, narrow, bytes, NULL, NULL) == bytes);
    roundtrip = encoding_from_wide(cp, actual);
    assert(roundtrip && !strcmp(roundtrip, narrow));
    encoding_test_free(roundtrip);
    encoding_test_free(actual);
    free(expected);
    free(narrow);
}

int main(void)
{
    const unsigned pages[] = {949, 1252, CP_UTF8, CP_THREAD_ACP};
    SYSTEM_INFO info;
    unsigned char *guard;
    DWORD old;
    char long_text[8193];
    unsigned i, n;
    wchar_t *wide;
    char *narrow;
    encoding_test_reset();
    for (i = 0; i < sizeof pages / sizeof pages[0]; ++i) {
        compare(pages[i], "");
        compare(pages[i], "plain text");
        compare(pages[i], "\xb0\xa1\xb3\xaa");
        compare(pages[i], "\xea\xb0\x80\xeb\x82\x98");
        compare(pages[i], "\xff\xc0\x80");
    }
    memset(long_text, 'x', sizeof long_text - 1);
    long_text[sizeof long_text - 1] = 0;
    compare(CP_UTF8, long_text);
    wide = encoding_to_wide(949, "\xb0\xa1\xb3\xaa");
    assert(wide && wide[0] == 0xac00 && wide[1] == 0xb098 && !wide[2]);
    encoding_test_free(wide);
    for (n = 1; n <= 2; ++n) {
        encoding_test_reset(); encoding_fail_call = n;
        assert(!encoding_to_wide(949, "abc") && GetLastError() == ERROR_NO_UNICODE_TRANSLATION && !encoding_live);
        encoding_test_reset(); encoding_fail_call = n;
        assert(!encoding_from_wide(949, L"abc") && GetLastError() == ERROR_NO_UNICODE_TRANSLATION && !encoding_live);
    }
    encoding_test_reset(); encoding_fail_allocation = 1;
    assert(!encoding_to_wide(949, "abc") && GetLastError() == ERROR_NOT_ENOUGH_MEMORY);
    encoding_test_reset(); encoding_fail_allocation = 1;
    assert(!encoding_from_wide(949, L"abc") && GetLastError() == ERROR_NOT_ENOUGH_MEMORY);
    encoding_test_reset();
    assert(!encoding_to_wide(949, NULL) && GetLastError() == ERROR_INVALID_PARAMETER);
    assert(!encoding_from_wide(949, NULL) && GetLastError() == ERROR_INVALID_PARAMETER);
    assert(!encoding_to_wide(0xffffffffu, "abc"));
    assert(!encoding_from_wide(0xffffffffu, L"abc"));
    GetSystemInfo(&info);
    guard = VirtualAlloc(NULL, info.dwPageSize * 2, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    assert(guard && VirtualProtect(guard + info.dwPageSize, info.dwPageSize, PAGE_NOACCESS, &old));
    for (n = 0; n <= 127; ++n) {
        char *text = (char *)guard + info.dwPageSize - n - 1;
        memset(text, 'x', n); text[n] = 0;
        wide = encoding_to_wide(949, text);
        assert(wide && wcslen(wide) == n);
        encoding_test_free(wide);
    }
    for (n = 0; n <= 127; ++n) {
        wchar_t *text = (wchar_t *)(guard + info.dwPageSize) - n - 1;
        wmemset(text, 0xac00, n); text[n] = 0;
        narrow = encoding_from_wide(949, text);
        assert(narrow && strlen(narrow) == 2 * n);
        encoding_test_free(narrow);
    }
    assert(VirtualFree(guard, 0, MEM_RELEASE));
    assert(!encoding_live);
    puts("PASS: code-page conversions, Korean bytes, long/empty/invalid text, guarded terminators and every allocation/API failure");
    return 0;
}
