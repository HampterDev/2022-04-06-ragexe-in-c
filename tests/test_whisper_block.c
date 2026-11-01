#include "session/state.h"
#include "contents/whisper_block.h"
#include "contents/user_hotkeys.h"
#include "contents/connection_api.h"
#include "json/service_json.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct { void *pointer; size_t size; } allocations[512];
int attempts, fail_at, live;

static int allocation_slot(void *pointer)
{
    int i;
    for (i = 0; i < 512; ++i)
        if (allocations[i].pointer == pointer) return i;
    assert(0); return 0;
}

static void *checked_malloc(size_t size)
{
    int slot;
    unsigned char *pointer;
    if (++attempts == fail_at) return NULL;
    assert(size <= SIZE_MAX - 32);
    slot = allocation_slot(NULL);
    pointer = malloc(size + 32); assert(pointer);
    memset(pointer + size, 0xa7, 32);
    allocations[slot].pointer = pointer; allocations[slot].size = size;
    ++live;
    return pointer;
}

static void checked_free(void *pointer)
{
    size_t i;
    int slot;
    if (!pointer) return;
    slot = allocation_slot(pointer);
    for (i = 0; i < 32; ++i)
        assert(((unsigned char *)pointer)[allocations[slot].size + i] == 0xa7);
    allocations[slot].pointer = NULL; --live;
    free(pointer);
}

static void *checked_realloc(void *pointer, size_t size)
{
    void *copy = checked_malloc(size);
    if (copy && pointer) {
        size_t old = allocations[allocation_slot(pointer)].size;
        memcpy(copy, pointer, old < size ? old : size);
        checked_free(pointer);
    }
    return copy;
}

#define malloc checked_malloc
#define realloc checked_realloc
#define free checked_free
#include "../src/core/owned_text.c"
#include "../src/core/text_array.c"
#include "../src/contents/whisper_block.c"
#undef malloc
#undef realloc
#undef free

struct Session g_session1;
struct UserHotkeyMgr *g_userHotkeyMgr;

static void expect_names(const struct WhisperBlockMgr *manager, const char *const *names, size_t count)
{
    size_t i;
    assert(whisper_block_count(manager) == count);
    for (i = 0; i < count; ++i) {
        assert(!strcmp(whisper_block_name(manager, i), names[i]));
        assert(whisper_block_contains(manager, names[i]));
    }
    assert(!whisper_block_name(manager, count));
}

static void test_names(void)
{
    struct WhisperBlockMgr manager = {0};
    const char *expected[] = {"", "A", "Alice", "alice", "이름"};
    const char *alias;
    unsigned char present[128] = {0};
    unsigned seed = 12345, step, i;
    char name[32];
    size_t index;
    assert(!whisper_block_contains(&manager, "absent"));
    assert(whisper_block_add(&manager, "이름"));
    assert(whisper_block_add(&manager, "alice"));
    assert(whisper_block_add(&manager, "Alice\0ignored"));
    assert(whisper_block_add(&manager, ""));
    assert(whisper_block_add(&manager, "A"));
    expect_names(&manager, expected, 5);
    alias = whisper_block_name(&manager, 2);
    attempts = 0; fail_at = 1;
    assert(whisper_block_add(&manager, alias));
    assert(!attempts);
    fail_at = 0;
    whisper_block_remove(&manager, alias);
    assert(!whisper_block_contains(&manager, "Alice"));
    whisper_block_remove(&manager, "missing");
    whisper_block_clear(&manager); whisper_block_clear(&manager);
    assert(!live);
    whisper_block_reset(&manager); whisper_block_reset(&manager);
    for (step = 0; step < 2000; ++step) {
        seed = seed * 1664525u + 1013904223u;
        i = (seed >> 16) % 128;
        sprintf(name, "name-%03u", i);
        if (seed & 1) { assert(whisper_block_add(&manager, name)); present[i] = 1; }
        else { whisper_block_remove(&manager, name); present[i] = 0; }
        index = 0;
        for (i = 0; i < 128; ++i) {
            sprintf(name, "name-%03u", i);
            assert(whisper_block_contains(&manager, name) == present[i]);
            if (present[i]) assert(!strcmp(whisper_block_name(&manager, index++), name));
        }
        assert(whisper_block_count(&manager) == index);
    }
    whisper_block_reset(&manager);
    assert(!live);
}

static void test_failure_and_lifetime(void)
{
    struct WhisperBlockMgr manager = {0};
    int failure, baseline;
    const char *original;
    assert(whisper_block_add(&manager, "existing"));
    original = whisper_block_name(&manager, 0);
    baseline = live;
    for (failure = 1; failure <= 2; ++failure) {
        attempts = 0; fail_at = failure;
        assert(!whisper_block_add(&manager, "new name needing its own allocation"));
        assert(whisper_block_count(&manager) == 1 && live == baseline);
        assert(whisper_block_name(&manager, 0) == original);
    }
    fail_at = 0;
    assert(whisper_block_add(&manager, "new name needing its own allocation"));
    whisper_block_reset(&manager);
    attempts = 0; fail_at = 1;
    assert(!whisper_block_create() && !g_whisperBlockMgr && !live);
    fail_at = 0;
    assert(whisper_block_create());
    assert(!whisper_block_create());
    assert(whisper_block_add(g_whisperBlockMgr, "owned by singleton"));
    whisper_block_destroy(); whisper_block_destroy();
    assert(!g_whisperBlockMgr && !live);
}

void test_whisper_commands(void);

int main(void)
{
    test_names(); test_failure_and_lifetime();
    test_whisper_commands();
    puts("Whisper blocks: sorted names, Korean text, 2000 edit sequences, allocation rollback, reblocking and packets passed");
    return 0;
}
