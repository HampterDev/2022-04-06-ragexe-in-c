#ifndef NLIB_MINI_PARTY_AREA_H
#define NLIB_MINI_PARTY_AREA_H

#include "platform/win_types.h"

struct MiniPartySortArea;

struct MiniPartySortArea *mini_party_sort_area_construct(struct MiniPartySortArea *self,
                                                                 const RECT *in);

#endif
