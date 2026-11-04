#include "platform/error_message.h"
#include "fileio/res_mgr.h"
#include "fileio/resource_path.h"
#include <stdint.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "platform/clock.h"

unsigned g_resPurgeAgeMs = 15000;

int res_mgr_get_type_index(const char *ext)
{
    return resource_type_index(ext);
}

struct Res *res_mgr_find(struct ResMgr *self, int type, const char *key)
{
    struct Hash name;
    if (type < 0 || type >= RESOURCE_TYPE_COUNT || !key || strlen(key) >= sizeof name.name)
        return NULL;
    hash_construct(&name, key);
    return resource_cache_find(&self->resources[type], &name);
}

const char *res_mgr_get_alias(struct ResMgr *self, const char *name)
{
    return resource_alias_resolve(&self->aliases, name);
}

char *res_mgr_resolve_map_name(struct ResMgr *self, const char *name)
{
    size_t length = name ? strlen(name) : 0;
    char *key, *result;
    const char *alias, *end;
    if (length > SIZE_MAX - 5)
        abort();
    key = malloc(length + 5);
    if (!key)
        abort();
    if (length)
        memcpy(key, name, length);
    memcpy(key + length, ".gat", 5);
    alias = res_mgr_get_alias(self, key);
    end = strstr(alias, ".gat");
    length = end ? (size_t)(end - alias) : strlen(alias);
    result = malloc(length + 1);
    if (!result)
        abort();
    memcpy(result, alias, length);
    result[length] = 0;
    free(key);
    return result;
}

static BYTE is_available(struct ResMgr *self, const char *name)
{
    char lowered[260], path[sizeof ((struct Hash *)0)->name];
    const char *dot;
    int type;
    struct Res *res;
    if (!resource_normalize_name(lowered, sizeof lowered, name))
        return 0;
    dot = strrchr(lowered, '.');
    type = res_mgr_get_type_index(dot ? dot + 1 : NULL);
    if (type < 0 || !resource_build_path(path, sizeof path, lowered, resource_type_directory(type)))
        return 0;
    res = res_mgr_find(self, type, path);
    if (res) {
        res->lastUse = platform_ticks_ms();
        return 1;
    }
    return file_make_file_name(path, 0);
}

BYTE res_mgr_is_available(struct ResMgr *self, const char *name)
{
    BYTE found;

    cache_section_enter(self->lock, CACHE_S_LOOKUP_CALLS, CACHE_S_LOOKUP_CONTENDED);
    found = name ? is_available(self, name) : 0;
    cache_section_leave(self->lock);
    return found;
}

BYTE res_mgr_exists(struct ResMgr *self, struct String name)
{
    const char *text = string_cstr(&name);
    BYTE found;

    cache_section_enter(self->lock, CACHE_S_LOOKUP_CALLS, CACHE_S_LOOKUP_CONTENDED);
    found = text ? is_available(self, text) : 0;
    string_free(&name);
    cache_section_leave(self->lock);
    return found;
}

void res_mgr_read_res_name_table(struct ResMgr *self)
{
    struct File file;
    resource_alias_release(&self->aliases);
    file_construct(&file);
    if (file_open(&file, "ResNameTable.txt", 0, 1, 0)) {
        if (file.size >= 0) {
            size_t size = (size_t)file.size;
            char *text = malloc(size + 1);
            if (!text)
                abort();
            if (file_read(&file, text, (int)size))
                resource_alias_parse(&self->aliases, text, size);
            free(text);
        }
        file_close(&file);
    }
    file_destruct(&file);
}

void res_mgr_call_residents(struct ResMgr *self)
{
    size_t i = 0;
    while (i < self->resident.count) {
        struct Res *res = self->resident.items[i];
        resource_update(res);
        if (i < self->resident.count && self->resident.items[i] == res)
            ++i;
    }
}

static void delete_resource(struct Res *res)
{
    resource_destroy(res);
}

static struct Res *cache_take(struct ResourceList *cache, size_t index)
{
    struct Res *res;
    cache_lock_write_begin();
    res = resource_list_take(cache, index);
    cache_lock_write_end();
    return res;
}

static size_t purge_resource(struct ResMgr *self, struct ResourceList *cache, size_t index)
{
    struct Res *res = cache_take(cache, index);
    struct Hash key = res->name;
    resource_list_remove(&self->resident, res);
    delete_resource(res);
    return resource_cache_after(cache, &key);
}

void res_mgr_delete_all(struct ResMgr *self)
{
    int type;
    self->resident.count = 0;
    for (type = 0; type < RESOURCE_TYPE_COUNT; ++type) {
        struct ResourceList *cache = &self->resources[type];
        while (cache->count)
            delete_resource(cache_take(cache, 0));
    }
}

void res_mgr_release(struct ResMgr *self, struct Res *res)
{
    struct ResourceList *cache = NULL;
    struct Res *found = NULL;
    size_t index;
    if (!res)
        return;
    cache_section_enter(self->lock, CACHE_S_RELEASES, CACHE_S_CONTENDED);
    resource_list_remove(&self->resident, res);
    if (res->typeIndex >= 0 && res->typeIndex < RESOURCE_TYPE_COUNT) {
        cache = &self->resources[res->typeIndex];
        found = resource_cache_find(cache, &res->name);
    }
    if (found) {
        index = resource_cache_after(cache, &found->name) - 1;
        resource_list_remove(&self->resident, found);
        delete_resource(cache_take(cache, index));
    }
    else {
        platform_atomic_increment((volatile int32_t *)&cache_stat[CACHE_S_SKIPPED]);
    }
    cache_section_leave(self->lock);
}

void res_mgr_purge_stale(struct ResMgr *self)
{
    unsigned cutoff = platform_ticks_ms() - g_resPurgeAgeMs;
    int type;
    for (type = 0; type < RESOURCE_TYPE_COUNT; ++type) {
        struct ResourceList *cache = &self->resources[type];
        size_t i = 0;
        while (i < cache->count) {
            struct Res *res = cache->items[i];
            if (res->lockCount == 0 && res->lastUse < cutoff)
                i = purge_resource(self, cache, i);
            else
                ++i;
        }
    }
}

void res_mgr_purge_by_extension(struct ResMgr *self, const char *ext)
{
    int type;
    cache_section_enter(self->lock, CACHE_S_LOOKUP_CALLS, CACHE_S_LOOKUP_CONTENDED);
    type = res_mgr_get_type_index(ext);
    if (type >= 0) {
        struct ResourceList *cache = &self->resources[type];
        size_t i = 0;
        while (i < cache->count)
            i = cache->items[i]->lockCount == 0 ? purge_resource(self, cache, i) : i + 1;
    }
    cache_section_leave(self->lock);
}

void res_mgr_purge_unlocked(struct ResMgr *self)
{
    int type;
    cache_section_enter(self->lock, CACHE_S_LOOKUP_CALLS, CACHE_S_LOOKUP_CONTENDED);
    for (type = 0; type < RESOURCE_TYPE_COUNT; ++type) {
        struct ResourceList *cache = &self->resources[type];
        size_t i = 0;
        while (i < cache->count)
            i = cache->items[i]->lockCount == 0 ? purge_resource(self, cache, i) : i + 1;
    }
    cache_section_leave(self->lock);
}

struct ResMgr *res_mgr_construct(struct ResMgr *self)
{
    memset(self->resources, 0, sizeof self->resources);
    memset(&self->aliases, 0, sizeof self->aliases);
    memset(&self->resident, 0, sizeof self->resident);
    self->lock = platform_mutex_create();
    return self;
}

void res_mgr_destruct(struct ResMgr *self)
{
    int type;
    res_mgr_delete_all(self);
    platform_mutex_destroy(self->lock);
    resource_list_release(&self->resident);
    resource_alias_release(&self->aliases);
    for (type = 0; type < RESOURCE_TYPE_COUNT; ++type)
        resource_list_release(&self->resources[type]);
}

struct Res *res_construct(struct Res *self)
{
    self->ops = NULL;
    self->lockCount = 0;
    self->lastUse = 0;
    self->typeIndex = -1;
    hash_construct(&self->name, "");
    return self;
}

void res_destruct(struct Res *self)
{
    self->ops = NULL;
}

void res_lock(struct Res *self)
{
    self->lockCount++;
}

void res_unlock(struct Res *self)
{
    self->lastUse = platform_ticks_ms();
    self->lockCount--;
}

const char *res_get_name(struct Res *self)
{
    return self->name.name;
}
