#include "session/state.h"
#include "core/text_util.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <wctype.h>
#include <oleauto.h>
#include "base.h"
#include "game/client.h"

static void text_to_wide(struct WString *out, UINT code_page, const struct String *src)
{
    BSTR wide;
    int count;

    wstring_init(out);
    count = MultiByteToWideChar(code_page, 0, string_cstr(src), src->size, NULL, 0);
    if (count == 0)
        return;
    wide = SysAllocStringLen(NULL, count);
    if (!wide)
        return;
    if (MultiByteToWideChar(code_page, 0, string_cstr(src), src->size, wide, count) != 0)
        wstring_assign(out, wide, wcslen(wide));
    SysFreeString(wide);
}

static void wide_to_text(struct String *out, UINT code_page, const struct WString *src)
{
    char *text;
    int count;

    string_init(out);
    count = WideCharToMultiByte(code_page, 0, wstring_cstr(src), -1, NULL, 0, NULL, NULL);
    if (count == 0)
        return;
    text = mem_alloc(count);
    if (WideCharToMultiByte(code_page, 0, wstring_cstr(src), -1, text, count, NULL, NULL) != 0)
        string_assign_cstr(out, text);
    mem_free(text);
}

struct String *text_utf8_to_codepage(struct String *out, const struct String *utf8, UINT code_page)
{
    struct WString wide;

    text_to_wide(&wide, CP_UTF8, utf8);
    if (wide.size == 0)
        string_init(out);
    else
        wide_to_text(out, code_page, &wide);
    wstring_free(&wide);
    return out;
}

static void money_commas(char *buf)
{
    struct String grouped;
    int len = (int)strlen(buf), first, i;

    if (len <= 3)
        return;
    string_init(&grouped);
    first = len % 3;
    for (i = 0; i < first; i++)
        string_append_char(&grouped, buf[i]);
    for (; i < len; i++) {
        if ((len - i) % 3 == 0 && i != 0)
            string_append_char(&grouped, ',');
        string_append_char(&grouped, buf[i]);
    }
    sprintf(buf, "%s", string_cstr(&grouped));
    string_free(&grouped);
}

char *format_money(int value, char *buf)
{
    sprintf(buf, "%d", value);
    money_commas(buf);
    return buf;
}

char *format_money64(long long value, char *buf, int size)
{
    _i64toa_s(value, buf, size, 10);
    money_commas(buf);
    return buf;
}

DWORD hex_after_prefix_value(const char *text)
{
    int i = (int)strlen(text) - 1;
    unsigned shift = 0;
    DWORD value = 0;
    char c;

    for (; i >= 2; i--, shift += 4) {
        c = text[i];
        if (c >= '0' && c <= '9')
            value += (DWORD)(c - '0') << (shift & 31);
        else if (c >= 'A' && c <= 'F')
            value += (DWORD)(c - 'A' + 10) << (shift & 31);
        else if (c >= 'a' && c <= 'f')
            value += (DWORD)(c - 'a' + 10) << (shift & 31);
    }
    return value;
}

void text_to_lower(struct String *s)
{
    unsigned i;

    for (i = 0; i < s->size; i++) {
        if ((signed char)string_cstr(s)[i] <= 0)
            i++;
        else if (isupper((signed char)string_cstr(s)[i]))
            s->data[i] += 0x20;
    }
}
