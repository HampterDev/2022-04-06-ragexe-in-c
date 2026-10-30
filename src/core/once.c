#include "core/once.h"
#include <stdlib.h>

int once_begin(INIT_ONCE *once)
{
    BOOL pending;
    if (!InitOnceBeginInitialize(once, 0, &pending, NULL))
        abort();
    return pending != FALSE;
}

void once_complete(INIT_ONCE *once)
{
    if (!InitOnceComplete(once, 0, NULL))
        abort();
}
