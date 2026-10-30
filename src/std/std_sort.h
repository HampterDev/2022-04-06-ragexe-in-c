#ifndef STD_SORT_H
#define STD_SORT_H

#include "base.h"

#ifndef STD_SORT_DECLARED
#define STD_SORT_DECLARED
typedef unsigned char (*StdSortLessFn)(const void *a, const void *b, void *pred);
void std_sort(void *first, void *last, size_t elem_size, StdSortLessFn less, void *pred);
#endif

void std_stable_sort(void *first, void *last, size_t elem_size, StdSortLessFn less, void *pred);

#endif
