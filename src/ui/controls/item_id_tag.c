#include "ui/controls/item_id_tag.h"
#include <limits.h>
#include <string.h>

int parse_item_id_tag(const char *text, const char *end, int *len, int *item_id)
{
    static const char tag[] = "^nItemID^";
    const size_t prefix = sizeof tag - 1;
    const char *at;
    unsigned value = 0;

    if (text >= end || (size_t)(end - text) <= prefix || memcmp(text, tag, prefix))
        return 0;
    at = text + prefix;
    if (*at < '0' || *at > '9')
        return 0;
    do {
        unsigned digit = (unsigned)(*at - '0');
        if (value > ((unsigned)INT_MAX - digit) / 10)
            return 0;
        value = value * 10 + digit;
        ++at;
    } while (at < end && *at >= '0' && *at <= '9');
    if (at - text > INT_MAX)
        return 0;
    *len = (int)(at - text);
    *item_id = (int)value;
    return 1;
}
