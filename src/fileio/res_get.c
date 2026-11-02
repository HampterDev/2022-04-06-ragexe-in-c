#include "fileio/res_mgr.h"
#include "fileio/resource_path.h"
#include "fileio/resource_errors.h"
#include "diag/perf.h"

#include <string.h>
#include "platform/clock.h"

#define KEY_SLOTS 4096
#define KEY_MAXLEN 119

struct res_key {
    DWORD hash;
    struct ResMgr *mgr;
    int typeIdx;
    DWORD tick;
    char name[KEY_MAXLEN + 1];
    char key[KEY_MAXLEN + 1];
};

static struct res_key keys[KEY_SLOTS];
static DWORD key_tick;

struct pending {
    const char *name;
    int captured;
    int typeIdx;
    int keyFits;
    char key[KEY_MAXLEN + 1];
};
static struct pending pend;
static int keys_enabled = -1;

static int keys_on(void)
{
    char env[8];
    if (keys_enabled < 0)
        keys_enabled = !(GetEnvironmentVariableA("RAGEXE_RES_KEYS", env, sizeof(env)) && env[0] == '0' && !env[1]);
    return keys_enabled;
}

static DWORD fnv1a(const char *s, size_t n)
{
    DWORD h = 2166136261u;
    while (n--)
        h = (h ^ (BYTE)*s++) * 16777619u;
    return h;
}

static struct res_key *key_find(struct ResMgr *mgr, const char *name, size_t len, DWORD hash, struct res_key **victim)
{
    struct res_key *a = &keys[hash & (KEY_SLOTS - 1)], *b = &keys[(hash & (KEY_SLOTS - 1)) ^ 1];
    if (a->hash == hash && a->mgr == mgr && memcmp(a->name, name, len + 1) == 0)
        return a;
    if (b->hash == hash && b->mgr == mgr && memcmp(b->name, name, len + 1) == 0)
        return b;
    *victim = a->tick <= b->tick ? a : b;
    return NULL;
}

static struct Res *res_find_capturing(struct ResMgr *self, int typeIdx, const char *key)
{
    if (pend.name && !pend.captured) {
        size_t n = strlen(key);
        pend.captured = 1;
        pend.typeIdx = typeIdx;
        pend.keyFits = n <= KEY_MAXLEN;
        if (pend.keyFits)
            memcpy(pend.key, key, n + 1);
    }
    return res_mgr_find(self, typeIdx, key);
}

static struct Res *get_body(struct ResMgr *self, const char *name)
{
    char lowered[260], path[sizeof ((struct Hash *)0)->name];
    const char *dot;
    int type;
    struct Res *res;

    if (!name)
        return NULL;
    platform_mutex_lock(self->lock);
    if (!resource_normalize_name(lowered, sizeof lowered, name)) {
        res = NULL;
        goto leave;
    }
    dot = strrchr(lowered, '.');
    type = res_mgr_get_type_index(dot ? dot + 1 : NULL);
    if (type < 0) {
        res = NULL;
        goto leave;
    }

    if (!resource_build_path(path, sizeof path, res_mgr_get_alias(self, lowered),
                             resource_type_directory(type))) {
        res = NULL;
        goto leave;
    }

    res = res_find_capturing(self, type, path);
    if (res) {
        res->lastUse = platform_ticks_ms();
        goto leave;
    }
    res = resource_create(type);
    if (!res)
        goto leave;
    if (resource_load(res, path) == 1) {
        int inserted;
        hash_set_string(&res->name, path);
        res->typeIndex = type;
        res->lastUse = platform_ticks_ms();
        cache_lock_write_begin();
        inserted = resource_cache_insert(&self->resources[type], res);
        cache_lock_write_end();
        if (!inserted) {
            struct Res *existing = resource_cache_find(&self->resources[type], &res->name);
            resource_destroy(res);
            res = existing;
        } else if (resource_is_resident(res) == 1) {
            resource_list_append(&self->resident, res);
        }
        goto leave;
    }

    resource_destroy(res);
    platform_mutex_unlock(self->lock);
    resource_error_report(name, path);
    return NULL;

leave:
    platform_mutex_unlock(self->lock);
    return res;
}

void *res_mgr_get(struct ResMgr *self, const char *name)
{
    struct pending save;
    struct res_key *e = NULL, *victim = NULL;
    size_t len;
    DWORD hash = 0;
    struct Res *res;

    if (!name)
        return NULL;
    platform_mutex_lock(self->lock);
    g_perf.resGets++;
    len = strlen(name);
    if (len <= KEY_MAXLEN && keys_on()) {
        hash = fnv1a(name, len);
        e = key_find(self, name, len, hash, &victim);
    }
    if (e) {
        res = res_mgr_find(self, e->typeIdx, e->key);
        if (res) {
            res->lastUse = platform_ticks_ms();
            e->tick = ++key_tick;
            g_perf.resHits++;
            platform_mutex_unlock(self->lock);
            return res;
        }
    }
    save = pend;
    pend.name = name;
    pend.captured = 0;
    res = get_body(self, name);
    if (pend.captured && pend.keyFits && len <= KEY_MAXLEN && keys_enabled) {
        if (!e)
            e = victim;
        e->hash = hash;
        e->mgr = self;
        e->typeIdx = pend.typeIdx;
        e->tick = ++key_tick;
        memcpy(e->name, name, len + 1);
        memcpy(e->key, pend.key, strlen(pend.key) + 1);
    }
    pend = save;
    platform_mutex_unlock(self->lock);
    return res;
}

void *res_mgr_get_string(struct ResMgr *self, struct String name)
{
    void *res = NULL;

    if (name.size)
        res = res_mgr_get(self, string_cstr(&name));
    string_free(&name);
    return res;
}
