#include "session/snapshot_values.h"
#include "session/state.h"
#include "std/std.h"
#include "session/session_windows.h"
#include "ui/window_access.h"
#include "game/context.h"
#include "actor/player_state.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fixtures/session_values_wire.h"

static unsigned allocations, attempts, fail_at, notices, windows, player_updates;
static struct Session *notified_session;
static int player_token, last_madogear;

void *mem_alloc(size_t size)
{
    void *p = malloc(size ? size : 1);
    assert(p); ++allocations; return p;
}
void mem_free(void *p)
{
    if (p) { assert(allocations); --allocations; free(p); }
}
void fatal_error(const char *message) { (void)message; abort(); }

static void *resize(void *old, size_t size)
{
    void *p;
    if (++attempts == fail_at) return NULL;
    p = realloc(old, size); assert(p);
    if (!old) ++allocations;
    return p;
}
static void release(void *p) { mem_free(p); }
#define realloc resize
#define free release
#include "../src/replay/snapshot.c"
#undef realloc
#undef free

void session_invalidate_windows(struct Session *session)
{ ++notices; notified_session = session; }
struct UIWindow *ui_window_open(int id)
{ assert(id == 0x89); ++windows; return NULL; }
struct Player *game_current_player(void) { return (struct Player *)&player_token; }
void player_restore_madogear(struct Player *player, int state)
{
    assert(player == (struct Player *)&player_token);
    ++player_updates; last_madogear = state;
}

static void fixture_pattern(void *field, size_t size, unsigned first_byte, int pattern)
{
    size_t i;
    for (i = 0; i < size; ++i)
        ((unsigned char *)field)[i] = pattern ? (unsigned char)(first_byte + i * 17) : 0;
}

static void initialize(struct Session *session, int pattern)
{
    size_t i;
    for (i = 0; i < sizeof *session; ++i)
        ((unsigned char *)session)[i] = pattern ? (unsigned char)(i * 17 + 9) : 0;
    fixture_pattern(&session->hotkeys0, sizeof session->hotkeys0, 0x99, pattern);
    fixture_pattern(&session->quickSlots, sizeof session->quickSlots, 0x55, pattern);
    fixture_pattern(&session->prohibitTarget, sizeof session->prohibitTarget, 0xf9, pattern);
    fixture_pattern(&session->char_info_hair_color, sizeof session->char_info_hair_color, 0xcd, pattern);
    fixture_pattern(&session->hairStyle, sizeof session->hairStyle, 0x55, pattern);
    session->inventoryExtension = pattern ? (int)UINT32_C(0xd8c7b6a5) : 0;
    string_init(&session->shopTitle);
    string_init(&session->worldName);
    string_init(&session->mdName);
    if (pattern) {
        string_assign_cstr(&session->shopTitle, "shop");
        string_assign_cstr(&session->worldName, "world");
        string_assign_cstr(&session->mdName, "dungeon");
        session->mdWndShown = 1;
        session->madogear = 1;
    }
}
static void destroy(struct Session *session)
{
    string_free(&session->shopTitle);
    string_free(&session->worldName);
    string_free(&session->mdName);
}

static const struct SnapshotSpan fixtures[] = {
    {expected_identity, sizeof expected_identity},
    {expected_character, sizeof expected_character},
    {expected_companions, sizeof expected_companions},
    {expected_dungeon, sizeof expected_dungeon},
    {expected_hotkeys, sizeof expected_hotkeys}
};

static void legacy_bytes_and_load(void)
{
    struct Session source, target;
    struct ReplayBuffer out = {0};
    unsigned kind;
    initialize(&source, 0);
    for (kind = 0; kind < sizeof fixtures / sizeof fixtures[0]; ++kind) {
        const struct SnapshotSpan *f = &fixtures[kind];
        initialize(&target, 0);
        target.nextJobExp = UINT64_C(0x1234567800000000);
        target.jobExp = UINT64_C(0xabcdef1200000000);
        assert(session_values_load(&source, kind, f->data, f->size) == (int)f->size);
        notices = windows = player_updates = 0;
        assert(session_values_save(&source, kind, &out));
        if (out.size != f->size || memcmp(out.data, f->data, f->size)) {
            size_t byte;
            fprintf(stderr, "Snapshot %u: actual size %u, expected %u\n", kind, (unsigned)out.size, (unsigned)f->size);
            for (byte = 0; byte < out.size && byte < f->size; ++byte)
                if (out.data[byte] != f->data[byte]) {
                    fprintf(stderr, "First difference at %u: %02x != %02x\n", (unsigned)byte, out.data[byte], f->data[byte]);
                    break;
                }
        }
        assert(out.size == f->size && !memcmp(out.data, f->data, f->size));
        assert(session_values_load(&target, kind, f->data, f->size) == (int)f->size);
        assert(session_values_save(&target, kind, &out));
        assert(out.size == f->size && !memcmp(out.data, f->data, f->size));
        if (kind == SESSION_VALUES_IDENTITY) {
            assert((uint64_t)target.nextJobExp >> 32 == UINT64_C(0x12345678));
            assert((uint64_t)target.jobExp >> 32 == UINT64_C(0xabcdef12));
        }
        assert(notices == (kind == SESSION_VALUES_CHARACTER));
        assert(player_updates == (kind == SESSION_VALUES_CHARACTER));
        if (notices) assert(notified_session == &target && last_madogear == 1);
        assert(windows == (kind == SESSION_VALUES_DUNGEON));
        destroy(&target);
    }
    replay_buffer_destroy(&out); replay_buffer_destroy(&out); destroy(&source);
    assert(!allocations);
}

static void malformed_is_atomic(void)
{
    struct Session target, before;
    unsigned kind;
    size_t i;
    initialize(&target, 1);
    before = target;
    for (kind = 0; kind < sizeof fixtures / sizeof fixtures[0]; ++kind) {
        const struct SnapshotSpan *f = &fixtures[kind];
        struct SnapshotReader reader = {{f->data, f->size}, 0};
        struct SnapshotRecord record;
        notices = windows = player_updates = 0;
        for (i = 0; i < f->size; ++i) {
            assert(session_values_load(&target, kind, f->data, i) == -1);
            assert(!memcmp(&target, &before, sizeof target));
        }
        while (snapshot_next(&reader, &record) == 1) {
            unsigned char bad[4096];
            size_t start = reader.offset - record.value.size - 6;
            assert(f->size + 1 <= sizeof bad);
            if (record.tag == 0x449 || record.tag == 0x7f6 || record.tag == 0x158b)
                continue;
            memcpy(bad, f->data, reader.offset);
            bad[start + 2] = (unsigned char)(record.value.size + 1);
            bad[start + 3] = (unsigned char)((record.value.size + 1) >> 8);
            bad[reader.offset] = 0;
            memcpy(bad + reader.offset + 1, f->data + reader.offset, f->size - reader.offset);
            assert(session_values_load(&target, kind, bad, f->size + 1) == -1);
            assert(!memcmp(&target, &before, sizeof target));
        }
        assert(!notices && !windows && !player_updates);
    }
    assert(session_values_load(&target, (enum SessionValueSnapshot)-1, NULL, 0) == -1);
    assert(session_values_load(NULL, SESSION_VALUES_IDENTITY, expected_identity, sizeof expected_identity) == -1);
    destroy(&target); assert(!allocations);
}

static void sparse_records_and_strings(void)
{
    struct Session target;
    struct SnapshotWriter writer = {0};
    struct ReplayBuffer out = {0};
    char large[700];
    int a = 456, b = 789;
    initialize(&target, 0);
    memset(large, 'x', sizeof large); large[5] = 0;
    snapshot_write(&writer, 0x1388, NULL, 0);
    snapshot_write(&writer, 0x146e, &a, 4);
    snapshot_write(&writer, 0x146f, &b, 4);
    snapshot_write(&writer, 0x7777, "unknown", 7);
    snapshot_write(&writer, 0x1470, NULL, 0);
    assert(snapshot_finish(&writer, &out));
    assert(session_values_load(&target, SESSION_VALUES_COMPANIONS, out.data, out.size) == (int)out.size);
    assert(target.homunSp == a && target.homun_ai_flags[1] == b);
    snapshot_write(&writer, 0x3e8, NULL, 0);
    snapshot_write(&writer, 0x449, large, sizeof large);
    snapshot_write(&writer, 0x3e9, NULL, 0);
    snapshot_write(&writer, 0x7777, NULL, 0);
    assert(snapshot_finish(&writer, &out));
    assert(session_values_load(&target, SESSION_VALUES_IDENTITY, out.data, out.size) == (int)out.size - 6);
    assert(target.shopTitle.size == sizeof large && !memcmp(string_cstr(&target.shopTitle), large, sizeof large));
    snapshot_write(&writer, 0x157c, NULL, 0);
    snapshot_write(&writer, 0x158b, large, sizeof large);
    snapshot_write(&writer, 0x157d, NULL, 0);
    assert(snapshot_finish(&writer, &out));
    assert(session_values_load(&target, SESSION_VALUES_DUNGEON, out.data, out.size) == (int)out.size);
    assert(target.mdName.size == 5 && !memcmp(string_cstr(&target.mdName), large, 5));
    replay_buffer_destroy(&out); destroy(&target); assert(!allocations);
}

static void allocation_failures(void)
{
    struct Session source;
    unsigned kind, failure, owned;
    initialize(&source, 0);
    for (kind = 0; kind < sizeof fixtures / sizeof fixtures[0]; ++kind)
        assert(session_values_load(&source, kind, fixtures[kind].data, fixtures[kind].size) == (int)fixtures[kind].size);
    owned = allocations;
    for (kind = 0; kind < sizeof fixtures / sizeof fixtures[0]; ++kind) {
        for (failure = 1; failure <= 6; ++failure) {
            struct ReplayBuffer out = {0};
            unsigned char *old;
            fail_at = 0;
            out.data = resize(NULL, 3); out.size = 3; memcpy(out.data, "old", 3); old = out.data;
            attempts = 0; fail_at = failure;
            if (session_values_save(&source, kind, &out)) {
                assert(failure > attempts);
                assert(out.size == fixtures[kind].size && !memcmp(out.data, fixtures[kind].data, out.size));
            } else {
                assert(failure == attempts);
                assert(out.data == old && out.size == 3 && !memcmp(out.data, "old", 3));
            }
            assert(allocations == owned + 1);
            replay_buffer_destroy(&out); assert(allocations == owned);
        }
    }
    fail_at = 0;
    destroy(&source); assert(!allocations);
}

int main(void)
{
    legacy_bytes_and_load(); malformed_is_atomic(); sparse_records_and_strings(); allocation_failures();
    puts("Session values: five legacy byte fixtures, low experience words, truncation, widths, hooks, strings, aliases and allocation failures passed");
    return 0;
}
