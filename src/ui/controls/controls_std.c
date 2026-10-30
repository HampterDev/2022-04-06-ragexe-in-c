#include <errno.h>
#include "core/fatal.h"
#include <malloc.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ui/ui_std.h"
#include "ui/items/styling_shop_std.h"
#include "ui/controls/controls_std.h"

struct BitIter *bitvec_copy(struct BitIter *out, DWORD *first, unsigned first_bit, DWORD *last,
                                    unsigned last_bit, DWORD *dest, unsigned dest_bit)
{
    while (first != last || first_bit != last_bit) {
        if (*first & (1u << first_bit))
            *dest |= 1u << dest_bit;
        else
            *dest &= ~(1u << dest_bit);
        if (dest_bit < 31) {
            dest_bit++;
        } else {
            dest_bit = 0;
            dest++;
        }
        if (first_bit < 31) {
            first_bit++;
        } else {
            first_bit = 0;
            first++;
        }
    }
    out->word = dest;
    out->bit = dest_bit;
    return out;
}

struct StdBitIter *std_vector_bool_erase_at(struct StdBitVector *self, struct StdBitIter *out, unsigned *where_word,
                                                       unsigned where_bit)
{
    unsigned *first = self->words.first;
    unsigned at = 0, size, words;
    struct BitIter end;

    if (self->size)
        at = (unsigned)(where_word - first) * 32 + where_bit;
    bitvec_copy(&end, (DWORD *)first + (at + 1) / 32, (at + 1) % 32, (DWORD *)first + self->size / 32,
                self->size % 32, (DWORD *)first + at / 32, at % 32);
    size = self->size - 1;
    if (size > 0x7fffffff)
        fatal_error("vector<bool> too long");
    words = (size + 31) >> 5;
    if (words < VEC_COUNT(&self->words, unsigned) && first + words != (unsigned *)self->words.last)
        self->words.last = first + words;
    self->size = size;
    if (size & 31)
        first[words - 1] &= (1u << (size & 31)) - 1;
    out->word = first + at / 32;
    out->bit = at % 32;
    return out;
}

void ui_tag_link_copy(struct UiTagLink *where, const struct UiTagLink *value)
{
    where->pos = value->pos;
    where->x = value->x;
    where->y = value->y;
    string_copy(&where->label, &value->label);
    string_copy(&where->args, &value->args);
}
