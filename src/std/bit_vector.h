#ifndef STD_BIT_VECTOR_H
#define STD_BIT_VECTOR_H

#include "base.h"

struct StdBitVector {
    struct Vec words;
    unsigned size;
};

struct StdBitIter {
    unsigned *word;
    unsigned bit;
};

#endif
