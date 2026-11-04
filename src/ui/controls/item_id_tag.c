#include "ui/controls/item_id_tag.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

int parse_item_id_tag(const char *text, const char *end, int *len, int *item_id)
{
    static const char tag[] = "^nItemID^";
    const size_t prefix = sizeof tag - 1;
    char digits[10];
    size_t count = 0;

    if (text >= end || (size_t)(end - text) <= prefix || memcmp(text, tag, prefix))
        return 0;
    while (prefix + count < (size_t)(end - text) && isxdigit((unsigned char)text[prefix + count]))
        if (++count == sizeof digits)
            return 0;
    if (!count)
        return 0;
    memcpy(digits, text + prefix, count);
    digits[count] = 0;
    *len = (int)(prefix + count);
    *item_id = atoi(digits);
    return 1;
}
