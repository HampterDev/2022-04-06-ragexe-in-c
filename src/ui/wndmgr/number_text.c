#include <stdlib.h>
#include "ui/wndmgr/wndmgr.h"
#include <stdio.h>
#include <string.h>

static void group_thousands(char *out, const char *digits)
{
    size_t len = strlen(digits), i;

    for (i = 0; i < len; i++) {
        if ((len - i) % 3 == 0 && i != 0)
            *out++ = ',';
        *out++ = digits[i];
    }
    *out = '\0';
}

void number_text(struct String *out, int value)
{
    char digits[16], text[40];
    unsigned magnitude = value < 0 ? 0u - (unsigned)value : (unsigned)value;

    snprintf(digits, sizeof digits, "%d", (int)magnitude);
    text[0] = value < 0 ? '-' : '\0';
    group_thousands(text + (text[0] != '\0'), digits);
    string_init(out);
    string_assign_cstr(out, text);
}
