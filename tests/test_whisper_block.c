#include "contents/whisper_block.h"
#include "contents/user_hotkeys.h"
#include "contents/connection_api.h"
#include "web/account_data.h"
#include "web/service_managers.h"
#include "web/service_manager_state.h"
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
static int hotkey_calls;

int session_read_chat_macros(struct Session *session, const struct JsonTree *list)
{
    (void)list; assert(session == &g_session1); return 1;
}
void user_hotkey_mgr_clear_saved(struct UserHotkeyMgr *manager) { (void)manager; ++hotkey_calls; }
void user_hotkey_mgr_save(struct UserHotkeyMgr *manager) { (void)manager; ++hotkey_calls; }
void account_user_data_read_hotkey_tab(struct AccountUserDataMgr *manager, const struct JsonTree *tab, int group)
{
    (void)manager; (void)tab; (void)group; ++hotkey_calls;
}

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
    assert(!manager.pending);
    manager.loaded = 1;
    alias = whisper_block_name(&manager, 2);
    attempts = 0; fail_at = 1;
    assert(whisper_block_add(&manager, alias));
    assert(!attempts && manager.pending == 1);
    fail_at = 0;
    whisper_block_remove(&manager, alias);
    assert(!whisper_block_contains(&manager, "Alice"));
    whisper_block_remove(&manager, "missing");
    assert(manager.pending == 3);
    whisper_block_clear(&manager); whisper_block_clear(&manager);
    assert(manager.loaded && manager.pending == 5 && !live);
    whisper_block_reset(&manager); whisper_block_reset(&manager);
    manager.loaded = 1;
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
        assert(whisper_block_count(&manager) == index && manager.pending == step + 1);
    }
    whisper_block_reset(&manager);
    assert(!manager.loaded && !manager.pending && !live);
}

static void test_failure_and_lifetime(void)
{
    struct WhisperBlockMgr manager = {0};
    int failure, baseline;
    const char *original;
    assert(whisper_block_add(&manager, "existing"));
    original = whisper_block_name(&manager, 0);
    manager.loaded = 1; manager.pending = 9;
    baseline = live;
    for (failure = 1; failure <= 2; ++failure) {
        attempts = 0; fail_at = failure;
        assert(!whisper_block_add(&manager, "new name needing its own allocation"));
        assert(whisper_block_count(&manager) == 1 && live == baseline);
        assert(whisper_block_name(&manager, 0) == original && manager.pending == 9);
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

static void test_loading(void)
{
    static const char payload[] = "{\"WhisperBlockList\":[\"z\",\"A\",\"A\\u0000ignored\",true,false,null,\"existing\"]}";
    const char *expected[] = {"", "A", "existing", "false", "true", "z"};
    struct WhisperBlockMgr manager = {0};
    struct JsonTree data;
    int failure, baseline, result = 0;
    const char *original;
    g_whisperBlockMgr = &manager;
    assert(whisper_block_add(&manager, "existing"));
    original = whisper_block_name(&manager, 0);
    manager.loaded = 1; manager.pending = 9;
    json_tree_init(&data); assert(json_parse_text(&data, payload, strlen(payload)));
    baseline = live;
    for (failure = 1; failure < 30; ++failure) {
        attempts = 0; fail_at = failure;
        result = account_user_data_read_block_list(&data);
        if (result) break;
        assert(live == baseline && manager.loaded && manager.pending == 9);
        assert(whisper_block_count(&manager) == 1 && whisper_block_name(&manager, 0) == original);
    }
    fail_at = 0; assert(result && failure > 5);
    expect_names(&manager, expected, 6);
    assert(manager.pending == 16);
    assert(account_user_data_read_block_list(NULL));
    assert(manager.pending == 16);
    json_tree_free(&data);
    whisper_block_reset(&manager);
    assert(json_parse_text(&data, "{\"WhisperBlockList\":{\"a\":1,\"b\":2}}", strlen("{\"WhisperBlockList\":{\"a\":1,\"b\":2}}")));
    manager.loaded = 1;
    assert(account_user_data_read_block_list(&data));
    assert(whisper_block_count(&manager) == 1 && whisper_block_contains(&manager, ""));
    assert(manager.pending == 2);
    json_tree_free(&data); whisper_block_reset(&manager);
    g_whisperBlockMgr = NULL; assert(!live);
}

static void test_load_publication(void)
{
    struct WhisperBlockMgr manager = {0};
    struct UserHotkeyMgr hotkeys = {0};
    struct AccountUserDataMgr account = {0};
    const char *response = "{\"Type\":1,\"data\":{\"WhisperBlockList\":[\"Alice\",\"Bob\"]}}";
    int failure, completed = 0;
    g_whisperBlockMgr = &manager; g_userHotkeyMgr = &hotkeys;
    for (failure = 1; failure < 20; ++failure) {
        attempts = 0; fail_at = failure;
        account_user_data_on_loaded(&account, 1, 42, "world", response);
        if (account.loaded) { completed = 1; break; }
        assert(!account.aid && !manager.loaded && !hotkeys.settings_read);
        assert(!whisper_block_count(&manager) && !manager.pending);
        owned_text_clear(&account.world); assert(!live);
    }
    fail_at = 0; assert(completed && failure > 3);
    assert(account.aid == 42 && manager.loaded && hotkeys.settings_read);
    assert(whisper_block_count(&manager) == 2 && !manager.pending && !hotkey_calls);
    owned_text_clear(&account.world); whisper_block_reset(&manager);
    g_whisperBlockMgr = NULL; g_userHotkeyMgr = NULL; assert(!live);
}

void test_whisper_commands(void);

int main(void)
{
    test_names(); test_failure_and_lifetime(); test_loading();
    test_load_publication(); test_whisper_commands();
    puts("Whisper blocks: sorted names, Korean text, 2000 edit sequences, allocation rollback, atomic load, publication, reblocking and packets passed");
    return 0;
}
