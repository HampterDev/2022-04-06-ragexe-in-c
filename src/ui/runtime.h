#include "core/memory.h"
#include "core/fatal.h"
#ifndef UI_RUNTIME_H
#define UI_RUNTIME_H

#include "ui/ui_std.h"
#include <errno.h>
#include <stdlib.h>

static __inline int ui_stoi(const char *text)
{
    char *end;
    long value;

    errno = 0;
    value = strtol(text, &end, 10);
    if (text == end)
        fatal_error("invalid stoi argument");
    if (errno == ERANGE)
        fatal_error("stoi argument out of range");
    return (int)value;
}

#endif
