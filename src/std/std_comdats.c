#include "core/memory.h"
#include "core/fatal.h"
#include <stdlib.h>
#include <string.h>
#include "std/std.h"
#include "std/bit_vector.h"

#define BYTES_MAX 0x7fffffffu

#define BIT_WORD(bv, i) (((unsigned *)(bv)->words.first)[(i) >> 5])
#define BIT_GET(bv, i) ((BIT_WORD(bv, i) >> ((i) & 31)) & 1)
#define BIT_SET(bv, i, on)                                                  \
    do {                                                                    \
        if (on)                                                             \
            BIT_WORD(bv, i) |= 1u << ((i) & 31);                            \
        else                                                                \
            BIT_WORD(bv, i) &= ~(1u << ((i) & 31));                         \
    } while (0)

static struct StdBitIter *bit_iter(const struct StdBitVector *self, struct StdBitIter *out, size_t index)
{
    out->word = (unsigned *)self->words.first + (index >> 5);
    out->bit = (unsigned)(index & 31);
    return out;
}

static size_t bit_index(const struct StdBitVector *self, const unsigned *word, unsigned bit)
{
    return (size_t)(word - (const unsigned *)self->words.first) * 32 + bit;
}

static void bits_trim(struct StdBitVector *self, size_t size)
{
    size_t words;

    if (size > BYTES_MAX)
        fatal_error("vector<bool> too long");
    words = (size + 31) >> 5;
    if (words < VEC_COUNT(&self->words, unsigned))
        self->words.last = (unsigned *)self->words.first + words;
    self->size = (unsigned)size;
    if (size & 31)
        ((unsigned *)self->words.first)[words - 1] &= (1u << (size & 31)) - 1;
}

struct StdBitIter *std_vector_bool_insert(struct StdBitVector *self,
                                                     struct StdBitIter *out, unsigned *where_word,
                                                     unsigned where_bit, size_t count, const unsigned char *value)
{
    size_t offset = bit_index(self, where_word, where_bit), i;

    if (count) {
        size_t old_size = self->size;

        if (BYTES_MAX - old_size < count)
            fatal_error("vector<bool> too long");
        vec_resize(&self->words, (old_size + count + 31) >> 5, sizeof(unsigned));
        if (old_size == 0) {
            self->size = (unsigned)count;
        } else {
            self->size = (unsigned)(old_size + count);
            for (i = old_size; i-- > offset;)
                BIT_SET(self, i + count, BIT_GET(self, i));
        }
    }
    bit_iter(self, out, offset);
    for (i = 0; i < count; i++)
        BIT_SET(self, offset + i, *value != 0);
    return out;
}

struct StdBitIter *std_vector_bool_erase(struct StdBitVector *self,
                                                    struct StdBitIter *out, unsigned *first_word,
                                                    unsigned first_bit, unsigned *last_word, unsigned last_bit)
{
    size_t first = 0, last = 0, size = self->size;

    if (size) {
        first = bit_index(self, first_word, first_bit);
        last = bit_index(self, last_word, last_bit);
    }
    if (first != last) {
        size_t to = first, from;

        for (from = last; from < size; from++, to++)
            BIT_SET(self, to, BIT_GET(self, from));
        bits_trim(self, to);
    }
    return bit_iter(self, out, first);
}
