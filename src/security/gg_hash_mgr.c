#include "core/thread_text.h"
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "session/state.h"
#include "security/gameguard.h"
#include "security/gg_hash_mgr.h"
#include "security/themida.h"
#include "initializers/gg_hash.h"
#include "core/msg_str.h"

struct GGHashManager {
    struct GGHash hash;
    CRITICAL_SECTION cs;
};

struct GGHashManager *g_ggHashMgr;

static void report_failure(struct GGHashManager *self)
{
    const char *text = thread_text_format("%s ( %d )", msg_str(0xef0),
                                          gg_hash_last_error(&self->hash));
    gameguard_set_error(g_gameGuard, text);
}

int gg_hash_manager_is_intact(struct GGHashManager *self)
{
    return self && gg_hash_is_intact(&self->hash) && gg_hash_is_intact(&self->hash);
}

int gg_hash_manager_check_file(struct GGHashManager *self, const char *name)
{
    int ok;
    if (!self) return 0;
    EnterCriticalSection(&self->cs);
    ok = gg_hash_lookup_file_ansi(&self->hash, name) != 0;
    if (!ok) report_failure(self);
    LeaveCriticalSection(&self->cs);
    return ok;
}

int gg_hash_manager_check_data(struct GGHashManager *self, const char *name,
                             const void *data, unsigned size)
{
    int ok;
    if (!self) return 0;
    EnterCriticalSection(&self->cs);
    ok = gg_hash_lookup_data_ansi(&self->hash, name, data, size) != 0;
    if (!ok) report_failure(self);
    LeaveCriticalSection(&self->cs);
    return ok;
}

static SRWLOCK lua_file_queue_lock = SRWLOCK_INIT;

void client_lua_loadfile_hook(const char *name)
{
    security_queue_lua_file(name);
}

int security_queue_lua_file(const char *name)
{
    struct OwnedText lower = {0};
    struct TextArray *queue = &g_session1.luaFileNames;
    size_t i;
    int accepted = 0;
    if (!name || !owned_text_assign(&lower, text_view_cstr(name))) return 0;
    for (i = 0; i < lower.size; ++i)
        lower.data[i] = (char)(tolower)((unsigned char)lower.data[i]);
    AcquireSRWLockExclusive(&lua_file_queue_lock);
    for (i = 0; i < queue->size; ++i) {
        if (queue->items[i].size == lower.size &&
            !memcmp(owned_text_cstr(&queue->items[i]), owned_text_cstr(&lower), lower.size)) {
            accepted = 1;
            break;
        }
    }
    if (!accepted && queue->size != (size_t)-1 && text_array_resize(queue, queue->size + 1)) {
        queue->items[queue->size - 1] = lower;
        owned_text_init(&lower);
        accepted = 1;
    }
    ReleaseSRWLockExclusive(&lua_file_queue_lock);
    owned_text_clear(&lower);
    return accepted;
}

void security_poll(struct GGHashManager *self)
{
    struct TextArray *queue = &g_session1.luaFileNames;
    size_t i;
    if (!self) return;
    EnterCriticalSection(&self->cs);
    AcquireSRWLockExclusive(&lua_file_queue_lock);
    for (i = 0; i < queue->size; ++i) {
        if (!gg_hash_lookup_file_ansi(&self->hash, owned_text_cstr(&queue->items[i])))
            report_failure(self);
    }
    text_array_resize(queue, 0);
    ReleaseSRWLockExclusive(&lua_file_queue_lock);
    LeaveCriticalSection(&self->cs);
}

int gg_hash_manager_startup(void)
{
    struct GGHashManager *manager;
    if (g_ggHashMgr) return 0;
    manager = malloc(sizeof *manager);
    if (!manager) return 0;
    gg_hash_init(&manager->hash);
    if (!InitializeCriticalSectionEx(&manager->cs, 0, 0)) {
        free(manager);
        return 0;
    }
    g_ggHashMgr = manager;
    return 1;
}

void gg_hash_manager_shutdown(void)
{
    struct GGHashManager *manager = g_ggHashMgr;
    if (!manager) return;
    g_ggHashMgr = NULL;
    gg_hash_clear(&manager->hash);
    DeleteCriticalSection(&manager->cs);
    free(manager);
}
