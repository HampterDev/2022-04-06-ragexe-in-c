#include "session/csession.h"
#include "session/session_state.h"

#include <string.h>

struct String *session_scramble_number(struct Session *self,
                                                     struct String *out, unsigned value)
{
    unsigned divisor;

    string_init(out);
    for (divisor = 1000000000; divisor != 0; divisor /= 10) {
        unsigned digit;

        if (out->size != 0 && divisor == 100)
            string_append_char(out, '-');
        digit = value / divisor;
        value %= divisor;
        if (digit > 9)
            continue;
        if (digit == 0 && out->size == 0)
            continue;
        string_append_char(out, "3867012495"[digit]);
    }
    return out;
}

void session_class_from_code(struct Session *self, int *out, int code)
{
    if (*out != 0)
        return;
    if (code >= 1 && code <= 6)
        *out = code + 1;
    else
        *out = 1;
}

BYTE session_is_number_text(struct Session *self, char *text)
{
    int len = (int)strlen(text);
    int i;

    for (i = 0; i < len; i++) {
        signed char c = (signed char)text[i];

        if (c == ',' && i < len) {
            int j;
            for (j = i; j < len; j++)
                text[j] = j == len - 1 ? ' ' : text[j + 1];
            c = (signed char)text[i];
        }
        if ((c < '0' || c > '9') && c != ' ')
            return 0;
    }
    return 1;
}

BYTE session_has_control_char(struct Session *self, struct String text)
{
    const char *p = string_cstr(&text);
    const char *end = p + text.size;
    BYTE found = 0;

    for (; p != end; p++) {
        signed char c = (signed char)*p;
        if (c < 0)
            continue;
        if ((BYTE)c <= 0x1f || c == 0x7f) {
            found = 1;
            break;
        }
    }
    string_free(&text);
    return found;
}
