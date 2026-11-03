#ifdef _WIN32
#include "core/once.h"
#include <stdlib.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

typedef char once_matches_init_once[sizeof(struct Once) == sizeof(INIT_ONCE) ? 1 : -1];

int once_begin(struct Once *once)
{
    BOOL pending;
    if (!InitOnceBeginInitialize((INIT_ONCE *)once, 0, &pending, NULL))
        abort();
    return pending != FALSE;
}

void once_complete(struct Once *once)
{
    if (!InitOnceComplete((INIT_ONCE *)once, 0, NULL))
        abort();
}
#endif
