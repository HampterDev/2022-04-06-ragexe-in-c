#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "game/own_states.h"

static struct Block { unsigned char *base; size_t size; } blocks[32];
static int allocations, attempts, fail_at;
static int block_index(void *p)
{
    int i;
    for (i = 0; i < 32; ++i)
        if (blocks[i].base && blocks[i].base + 8 == p) return i;
    assert(0); return -1;
}
static void check_guards(void)
{
    int i, j;
    for (i = 0; i < 32; ++i) if (blocks[i].base)
        for (j = 0; j < 8; ++j) {
            assert(blocks[i].base[j] == 0xa7);
            assert(blocks[i].base[8 + blocks[i].size + j] == 0xa7);
        }
}
static void *allocate(size_t size)
{
    int i;
    if (++attempts == fail_at) return NULL;
    for (i = 0; i < 32 && blocks[i].base; ++i) {}
    assert(i < 32 && size <= SIZE_MAX - 16);
    blocks[i].base = malloc(size + 16); assert(blocks[i].base);
    blocks[i].size = size;
    memset(blocks[i].base, 0xa7, size + 16);
    ++allocations;
    return blocks[i].base + 8;
}
static void release(void *p)
{
    int i;
    if (!p) return;
    check_guards(); i = block_index(p);
    free(blocks[i].base); blocks[i].base = NULL; --allocations;
}
static void *resize(void *p, size_t size)
{
    void *result = allocate(size);
    if (result && p) {
        size_t old_size = blocks[block_index(p)].size;
        memcpy(result, p, size < old_size ? size : old_size);
        release(p);
    }
    return result;
}
#define malloc allocate
#define realloc resize
#define free release
#include "../src/game/own_states.c"
#undef malloc
#undef realloc
#undef free

static void same(const struct OwnStateEntry *a, const struct OwnStateEntry *b)
{
    assert(a && b && a->index == b->index && a->end_tick == b->end_tick);
    assert(a->value == b->value && a->total_ms == b->total_ms && a->remain_ms == b->remain_ms);
}

static void storage(void)
{
    struct OwnStateEntry entry = {17, 23, 45, 67, 89};
    const struct OwnStateEntry *first;
    size_t i;
    assert(!own_states_count() && !own_states_at(0) && !own_states_at(SIZE_MAX));
    assert(!own_states_find(17) && !own_states_push(NULL));
    own_states_erase_at(SIZE_MAX); own_states_remove(0, 9); own_states_set_value(0, 9);
    attempts = 0; fail_at = 1;
    assert(!own_states_set_present(17, 1) && !own_states_count());
    fail_at = 0;
    for (i = 0; i < 16; ++i) { entry.index = (int)i; assert(own_states_push(&entry)); }
    first = own_states_at(0); entry = *own_states_at(3);
    attempts = 0; fail_at = 1;
    assert(!own_states_push(own_states_at(3)) && own_states_count() == 16 && own_states_at(0) == first);
    fail_at = 0;
    assert(own_states_push(own_states_at(3)) && own_states_count() == 17 && own_states_at(0) != first);
    same(own_states_at(16), &entry);
    own_states_set_value(3, 900);
    assert(own_states_at(3)->value == 900 && own_states_at(16)->value == 45);
    assert(own_states_set_present(3, 1) && own_states_count() == 17);
    assert(own_states_set_present(3, 0) && own_states_count() == 16);
    same(own_states_find(3), &entry);
    own_states_remove(2, 4);
    assert(own_states_count() == 13 && !own_states_find(2) && !own_states_find(3) && !own_states_find(4));
    for (i = 2; i < 13; ++i) assert(own_states_at(i)->index == (int)i + 3);
    own_states_erase_at(0); own_states_erase_at(11); own_states_erase_at(11);
    assert(own_states_count() == 11 && own_states_at(0)->index == 1 && own_states_at(10)->index == 14);
    own_states_remove(INT_MIN, INT_MAX); assert(!own_states_count());
    attempts = 0; fail_at = 1;
    assert(own_states_set_present(0x2b7, 1));
    assert(attempts == 0);
    entry.index = 0x2b7; entry.end_tick = OWN_STATE_NO_END;
    entry.value = entry.total_ms = entry.remain_ms = 0;
    same(own_states_at(0), &entry);
    fail_at = 0;
    own_states_clear(); own_states_destroy(); own_states_destroy();
    assert(!allocations);
}

int main(void)
{
    storage(); check_guards();
    puts("player statuses: C ownership, stable ordering, duplicates, initialized entries and allocation rollback passed");
    return 0;
}
