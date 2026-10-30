#include "core/fatal.h"
#include <stdio.h>
#include <stdlib.h>

void fatal_error(const char *message)
{
    fputs("fatal: ", stderr);
    fputs(message ? message : "unknown error", stderr);
    fputc('\n', stderr);
    fflush(stderr);
    abort();
}
