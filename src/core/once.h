#ifndef CORE_ONCE_H
#define CORE_ONCE_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

int once_begin(INIT_ONCE *once);
void once_complete(INIT_ONCE *once);

#endif
