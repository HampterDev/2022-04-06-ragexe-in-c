#ifndef _WIN32
#include "platform/embedded_fonts.h"
#include <stddef.h>

int embedded_fonts_load(const char **missing_file)
{
    if (missing_file)
        *missing_file = NULL;
    return 0;
}

void embedded_fonts_clear(void)
{
}
#endif
