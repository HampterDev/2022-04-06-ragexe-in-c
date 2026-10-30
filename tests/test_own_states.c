#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "game/own_states.h"
#include "replay/snapshot.h"
#include "fixtures/own_states_wire.h"

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
#include "../src/replay/snapshot.c"
#include "../src/game/own_states.c"
#undef malloc
#undef realloc
#undef free

#include "../src/game/own_states_replay.c"
static struct ReplayRecorder recorder;
static struct SnapshotSpan input;
static int read_ok = 1, timer_calls;
unsigned char g_mapPropertyQuiet;
struct StateIconSheet *g_stateIconSheet = (struct StateIconSheet *)1;
int replay_read_chunk(struct ReplayRecorder *self, int id, struct ReplayBuffer *out)
{
    assert(self == &recorder && id == 0x11);
    if (!read_ok) return 0;
    if (input.size) {
        out->data = allocate(input.size);
        if (!out->data) return 0;
        memcpy(out->data, input.data, input.size); out->size = input.size;
    }
    return 1;
}
void icon_sheet_set_timer(struct StateIconSheet *sheet, unsigned index, unsigned total, unsigned start, unsigned offset)
{
    static const int ids[] = {0x37f, 0x37f, -1, 0x4dc, 0x37f};
    static const int totals[] = {8000, 0, INT_MIN, 1000, 7000};
    static const uint32_t starts[] = {0xfffffd78u, 0, 0x7ffff002u, 0xffffeed3u, 96};
    static const int offsets[] = {3000, 0, 1, 700, 3000};
    assert(sheet == g_stateIconSheet && timer_calls < 5 && own_states_count() == 6);
    assert(index == (unsigned)ids[timer_calls] && total == (unsigned)totals[timer_calls]);
    assert((uint32_t)start == starts[timer_calls] && offset == (unsigned)offsets[timer_calls]);
    ++timer_calls;
}

static const struct OwnStateEntry original[] = {
    {0xb8, 999999, 0, 9999, 9999},
    {0x37f, 0x2100, 7, 8000, 5000},
    {0x37f, 0, 8, 0, 0},
    {-1, 1, -3, INT_MIN, INT_MAX},
    {0x4dc, UINT32_MAX, 5, 1000, 300},
    {0x37f, 0x2000, 7, 7000, 4000}
};
static void seed(void)
{
    size_t i;
    own_states_clear();
    for (i = 0; i < sizeof original / sizeof *original; ++i)
        assert(own_states_push(&original[i]));
}
static void same(const struct OwnStateEntry *a, const struct OwnStateEntry *b)
{
    assert(a && b && a->index == b->index && a->end_tick == b->end_tick);
    assert(a->value == b->value && a->total_ms == b->total_ms && a->remain_ms == b->remain_ms);
}
static void invalid(const void *bytes, size_t size)
{
    struct OwnStateEntry before[6];
    const struct OwnStateEntry *first = own_states_at(0);
    size_t i;
    int live = allocations;
    assert(own_states_count() == 6);
    for (i = 0; i < 6; ++i) before[i] = *own_states_at(i);
    assert(own_states_load(bytes, size) == -1);
    assert(own_states_count() == 6 && own_states_at(0) == first && allocations == live);
    for (i = 0; i < 6; ++i) same(own_states_at(i), &before[i]);
    check_guards();
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
static void wire(void)
{
    struct ReplayBuffer out = {0}, custom = {0};
    struct SnapshotWriter writer = {0};
    unsigned char unaligned[sizeof expected_states + 2], payload[25] = {0};
    size_t i;
    seed();
    assert(own_states_save(0x1000, &out));
    assert(out.size == sizeof expected_states && !memcmp(out.data, expected_states, out.size));
    for (i = 0; i < 6; ++i) same(own_states_at(i), &original[i]);
    for (i = 0; i < sizeof expected_states; ++i) invalid(expected_states, i);
    invalid(NULL, 0); invalid(NULL, 10); invalid(expected_states, SIZE_MAX);
    for (i = 0; i <= sizeof payload; ++i) if (i != 20) {
        snapshot_write(&writer, 0x4a38, NULL, 0);
        snapshot_write(&writer, 0x4a43, payload, i);
        snapshot_write(&writer, 0x4a39, NULL, 0);
        assert(snapshot_finish(&writer, &custom)); invalid(custom.data, custom.size);
    }
    for (i = 0; i < 3; ++i) {
        snapshot_write(&writer, 0x4a38, payload, i == 0 ? 1 : 0);
        if (i == 1) snapshot_write(&writer, 0x4a38, NULL, 0);
        snapshot_write(&writer, 0x4a39, payload, i == 2 ? 1 : 0);
        assert(snapshot_finish(&writer, &custom)); invalid(custom.data, custom.size);
    }
    memcpy(unaligned + 1, expected_states, sizeof expected_states);
    unaligned[sizeof unaligned - 1] = 0xff;
    assert(own_states_load(unaligned + 1, sizeof unaligned - 1) == sizeof expected_states);
    assert(own_states_count() == 6 && own_states_at(1)->end_tick == 0x1100);
    assert(own_states_at(2)->end_tick == 0 && own_states_at(3)->end_tick == 0xfffff001u);
    assert(own_states_at(4)->end_tick == 0xffffefffu && own_states_at(5)->end_tick == 0x1000);
    assert(own_states_save(0, &out) && !memcmp(out.data, expected_states, out.size));
    snapshot_write(&writer, 0x4a38, NULL, 0);
    snapshot_write(&writer, 0x8888, payload, sizeof payload);
    snapshot_write(&writer, 0x4a43, expected_states + 12, 20);
    snapshot_write(&writer, 0x4a39, NULL, 0);
    assert(snapshot_finish(&writer, &custom));
    assert(own_states_load(custom.data, custom.size) == (int)custom.size && own_states_count() == 1);
    same(own_states_at(0), &original[0]);
    snapshot_write(&writer, 0x4a38, NULL, 0); snapshot_write(&writer, 0x4a39, NULL, 0);
    assert(snapshot_finish(&writer, &custom));
    assert(own_states_load(custom.data, custom.size) == 12 && !own_states_count());
    replay_buffer_destroy(&out); replay_buffer_destroy(&custom); own_states_destroy();
    assert(!allocations);
}
static void failures_and_hooks(void)
{
    struct ReplayBuffer out = {0};
    unsigned char *old;
    int i, needed, live;
    seed(); attempts = 0; fail_at = 1;
    invalid(expected_states, sizeof expected_states); fail_at = 0;
    for (i = 0; i < 100; ++i) assert(own_states_push(&original[i % 6]));
    assert(!own_states_save(0, NULL));
    attempts = 0; assert(own_states_save(0x1000, &out)); needed = attempts;
    assert(needed > 1); replay_buffer_destroy(&out);
    out.data = allocate(3); out.size = 3; memcpy(out.data, "old", 3); old = out.data;
    live = allocations;
    for (i = 1; i <= needed; ++i) {
        attempts = 0; fail_at = i;
        assert(!own_states_save(0x1000, &out));
        assert(out.data == old && out.size == 3 && !memcmp(out.data, "old", 3) && allocations == live);
        check_guards();
    }
    fail_at = 0; replay_buffer_destroy(&out); seed();
    input.data = expected_states;
    for (i = 1; i < sizeof expected_states; ++i) {
        input.size = (size_t)i;
        assert(!own_states_restore(&recorder) && own_states_count() == 6 && !timer_calls && !g_mapPropertyQuiet);
    }
    input.size = sizeof expected_states;
    for (i = 1; i <= 2; ++i) {
        attempts = 0; fail_at = i;
        assert(!own_states_restore(&recorder) && !timer_calls && !g_mapPropertyQuiet);
        same(own_states_at(1), &original[1]);
    }
    fail_at = 0; read_ok = 0;
    assert(!own_states_restore(&recorder) && !timer_calls); read_ok = 1;
    assert(own_states_restore(&recorder) && timer_calls == 5 && g_mapPropertyQuiet == 1);
    input.size = 0; timer_calls = 0; g_mapPropertyQuiet = 0;
    assert(own_states_restore(&recorder) && own_states_count() == 6 && !timer_calls && !g_mapPropertyQuiet);
    assert(!memcmp(input.data, expected_states, sizeof expected_states));
    own_states_destroy(); assert(!allocations);
}
int main(void)
{
    storage(); wire(); failures_and_hooks(); check_guards();
    puts("player statuses: C ownership, stable ordering, duplicates, initialized entries, frozen replay bytes, every truncation, allocation rollback and deferred timers passed");
    return 0;
}
