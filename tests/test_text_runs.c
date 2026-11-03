#include "render/text_runs.h"
#include "core/utf8.h"
#include "utf8_text.h"
#include <windows.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

#define S(text) text, sizeof text - 1

static void expect_runs(const char *text, size_t length, const size_t *sizes, const int *scripts, size_t count)
{
    size_t at = 0, i;
    enum TextScript script;

    for (i = 0; i < count; i++) {
        size_t run = text_script_run(text + at, length - at, &script);
        assert(run == sizes[i] && (int)script == scripts[i]);
        at += run;
    }
    assert(at == length && text_script_run(text + at, 0, &script) == 0);
}

static void check_scripts(void)
{
    assert(text_script_of('a') == TEXT_SCRIPT_DEFAULT && text_script_of(' ') == TEXT_SCRIPT_COMMON);
    assert(text_script_of(0x416) == TEXT_SCRIPT_DEFAULT);          /* Cyrillic: Arial has it */
    assert(text_script_of(0x3b1) == TEXT_SCRIPT_DEFAULT);          /* Greek */
    assert(text_script_of(0xd55c) == TEXT_SCRIPT_HANGUL && text_script_of(0x1100) == TEXT_SCRIPT_HANGUL);
    assert(text_script_of(0x3131) == TEXT_SCRIPT_HANGUL && text_script_of(0xffa1) == TEXT_SCRIPT_HANGUL);
    assert(text_script_of(0xac00) == TEXT_SCRIPT_HANGUL && text_script_of(0xd7a3) == TEXT_SCRIPT_HANGUL);
    assert(text_script_of(0x4e2d) == TEXT_SCRIPT_HAN && text_script_of(0x300e) == TEXT_SCRIPT_HAN);
    assert(text_script_of(0xff21) == TEXT_SCRIPT_HAN && text_script_of(0x20000) == TEXT_SCRIPT_HAN);
    assert(text_script_of(0x3042) == TEXT_SCRIPT_KANA && text_script_of(0x30ab) == TEXT_SCRIPT_KANA);
    assert(text_script_of(0xff76) == TEXT_SCRIPT_KANA);
    assert(text_script_of(0x0e01) == TEXT_SCRIPT_THAI);
    assert(text_script_of(0x25b2) == TEXT_SCRIPT_SYMBOL && text_script_of(0x2661) == TEXT_SCRIPT_SYMBOL);
    assert(text_script_of(0x2192) == TEXT_SCRIPT_SYMBOL && text_script_of(0x2103) == TEXT_SCRIPT_SYMBOL);
    assert(text_script_of(0x2460) == TEXT_SCRIPT_SYMBOL && text_script_of(0x2bff) == TEXT_SCRIPT_SYMBOL);
    assert(text_script_of(0x203b) == TEXT_SCRIPT_SYMBOL && text_script_of(0x2025) == TEXT_SCRIPT_SYMBOL);
    /* Punctuation the selected face has stays in it. */
    assert(text_script_of(0x2019) == TEXT_SCRIPT_DEFAULT && text_script_of(0x2026) == TEXT_SCRIPT_DEFAULT);
    assert(text_script_of(0x20ac) == TEXT_SCRIPT_DEFAULT && text_script_of(0x2c00) == TEXT_SCRIPT_DEFAULT);
    assert(text_script_of(0x1f600) == TEXT_SCRIPT_EMOJI && text_script_of(0x1f000) == TEXT_SCRIPT_EMOJI);
    assert(text_script_of(0x0301) == TEXT_SCRIPT_COMMON && text_script_of(0x200d) == TEXT_SCRIPT_COMMON);
    assert(text_script_of(0xfe0f) == TEXT_SCRIPT_COMMON && text_script_of(0xe0100) == TEXT_SCRIPT_COMMON);
    assert(text_script_of(0x4dc0) == TEXT_SCRIPT_DEFAULT && text_script_of(0x10ffff) == TEXT_SCRIPT_DEFAULT);
}

static void check_runs(void)
{
    {
        /* A space joins the run before it. */
        static const size_t sizes[] = { 3, 8, 2 };
        static const int scripts[] = { TEXT_SCRIPT_DEFAULT, TEXT_SCRIPT_HANGUL, TEXT_SCRIPT_DEFAULT };
        expect_runs(S("ab " U8_HAN " " U8_GUK " cd"), sizes, scripts, 3);
    }
    {
        static const size_t sizes[] = { 4, 3, 3, 4, 2 };
        static const int scripts[] = { TEXT_SCRIPT_HANGUL, TEXT_SCRIPT_HAN, TEXT_SCRIPT_KANA,
                                       TEXT_SCRIPT_EMOJI, TEXT_SCRIPT_DEFAULT };
        expect_runs(S(" " U8_HAN "\xe4\xb8\xad\xe3\x81\x82" U8_GRIN U8_ZHE), sizes, scripts, 5);
    }
    {
        static const size_t sizes[] = { 3 };
        static const int scripts[] = { TEXT_SCRIPT_DEFAULT };
        expect_runs(S("   "), sizes, scripts, 1);
    }
    {
        /* Invalid bytes are drawn in the selected face and end a Hangul run. */
        static const size_t sizes[] = { 3, 2, 3 };
        static const int scripts[] = { TEXT_SCRIPT_HANGUL, TEXT_SCRIPT_DEFAULT, TEXT_SCRIPT_HANGUL };
        expect_runs(S(U8_HAN "\xe2\x82" U8_GUK), sizes, scripts, 3);
    }
    {
        /* A combining mark and a variation selector stay with their base character. */
        static const size_t sizes[] = { 3, 6 };
        static const int scripts[] = { TEXT_SCRIPT_DEFAULT, TEXT_SCRIPT_SYMBOL };
        expect_runs(S("e\xcc\x81" "\xe2\x98\x85\xef\xb8\x8f"), sizes, scripts, 2);
    }
}

static void check_guarded(void)
{
    static const char *const inputs[] = { U8_HAN U8_GUK, "\xf0\x9f\x98", "a \xe2\x82", U8_GRIN };
    SYSTEM_INFO info;
    char *page;
    DWORD old;
    size_t i, n;

    GetSystemInfo(&info);
    page = VirtualAlloc(NULL, info.dwPageSize * 2, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    assert(page && VirtualProtect(page + info.dwPageSize, info.dwPageSize, PAGE_NOACCESS, &old));
    for (i = 0; i < sizeof inputs / sizeof inputs[0]; i++)
        for (n = 0; n <= strlen(inputs[i]); n++) {
            char *text = page + info.dwPageSize - n;
            size_t at = 0;
            enum TextScript script;

            memcpy(text, inputs[i], n);
            while (at < n) {
                size_t run = text_script_run(text + at, n - at, &script);
                assert(run > 0 && at + run <= n);
                at += run;
            }
        }
    assert(VirtualFree(page, 0, MEM_RELEASE));
}

int main(void)
{
    check_scripts();
    check_runs();
    check_guarded();
    puts("Text runs: scripts by Unicode range, joining spaces and marks, invalid bytes and guarded endings passed.");
    return 0;
}
