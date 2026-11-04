#include "ui/controls/url_link.h"
#include "platform/desktop.h"
#include <string.h>

static int starts_with_scheme(struct TextView target, const char *scheme)
{
    size_t i, length = strlen(scheme);

    if (target.size <= length)
        return 0;
    for (i = 0; i < length; ++i) {
        char c = target.data[i];

        if (c >= 'A' && c <= 'Z')
            c = (char)(c - 'A' + 'a');
        if (c != scheme[i])
            return 0;
    }
    return 1;
}

int ui_open_url_link(struct TextView target)
{
    if (!target.data || memchr(target.data, '\0', target.size))
        return 0;
    if (!starts_with_scheme(target, "http://") && !starts_with_scheme(target, "https://"))
        return 0;
    platform_open_with_desktop(target.data);
    return 1;
}
