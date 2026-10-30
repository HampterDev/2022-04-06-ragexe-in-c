#ifndef UI_CONTROLS_CONTROLS_STD_H
#define UI_CONTROLS_CONTROLS_STD_H

#include "ui/ui_std.h"
#include <time.h>

struct StdBitIter;
struct StdBitVector;
struct List;

struct StdBitIter *std_vector_bool_erase_at(struct StdBitVector *self, struct StdBitIter *out, unsigned *where_word,
                                                       unsigned where_bit);

struct UIString;

#endif
