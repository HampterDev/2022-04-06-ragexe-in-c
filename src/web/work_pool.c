#include "web/work_pool_internal.h"
#include <windows.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct WorkAllocation {
    struct AsyncWork *work;
    struct WorkAllocation *next, *next_free;
    int leased;
};

struct WorkPool {
    struct WorkPoolType type;
    CRITICAL_SECTION lock;
    struct WorkAllocation *first, *last, *first_free, *last_free;
    size_t free_count;
};

struct WorkPoolMgr {
    size_t count;
    struct WorkPool pools[];
};

struct WorkPoolMgr *work_pool_create_with_types(const struct WorkPoolType *types, size_t count)
{
    struct WorkPoolMgr *manager;
    size_t i, j;
    if (!types || !count || count > (SIZE_MAX - sizeof *manager) / sizeof manager->pools[0])
        return NULL;
    for (i = 0; i < count; ++i) {
        if (!types[i].name || !types[i].name[0] || types[i].size < sizeof(struct AsyncWork) ||
            !types[i].initialize || !types[i].dispose ||
            !types[i].operations.run || !types[i].operations.done)
            return NULL;
        for (j = 0; j < i; ++j)
            if (!strcmp(types[j].name, types[i].name))
                return NULL;
    }
    manager = calloc(1, sizeof *manager + count * sizeof manager->pools[0]);
    if (!manager)
        return NULL;
    for (i = 0; i < count; ++i) {
        struct WorkPool *pool = &manager->pools[i];
        pool->type = types[i];
        if (!InitializeCriticalSectionAndSpinCount(&pool->lock, 0)) {
            work_pool_mgr_destroy(manager);
            return NULL;
        }
        ++manager->count;
    }
    return manager;
}

void work_pool_mgr_destroy(struct WorkPoolMgr *manager)
{
    size_t i;
    if (!manager)
        return;
    for (i = 0; i < manager->count; ++i) {
        struct WorkPool *pool = &manager->pools[i];
        struct WorkAllocation *entry = pool->first;
        while (entry) {
            struct WorkAllocation *next = entry->next;
            pool->type.dispose(entry->work);
            free(entry->work);
            free(entry);
            entry = next;
        }
        DeleteCriticalSection(&pool->lock);
    }
    free(manager);
}

static struct AsyncWork *pool_acquire(struct WorkPool *pool)
{
    struct WorkAllocation *entry;
    EnterCriticalSection(&pool->lock);
    if (pool->free_count >= 2) {
        entry = pool->first_free;
        pool->first_free = entry->next_free;
        if (!pool->first_free)
            pool->last_free = NULL;
        --pool->free_count;
        entry->next_free = NULL;
    } else {
        entry = calloc(1, sizeof *entry);
        if (!entry) {
            LeaveCriticalSection(&pool->lock);
            return NULL;
        }
        entry->work = calloc(1, pool->type.size);
        if (!entry->work) {
            free(entry);
            LeaveCriticalSection(&pool->lock);
            return NULL;
        }
        pool->type.initialize(entry->work);
        entry->work->ops = &pool->type.operations;
        if (pool->last)
            pool->last->next = entry;
        else
            pool->first = entry;
        pool->last = entry;
    }
    entry->leased = 1;
    LeaveCriticalSection(&pool->lock);
    return entry->work;
}

struct AsyncWork *work_pool_mgr_acquire(struct WorkPoolMgr *manager, const char *name)
{
    size_t i;
    if (!manager || !name)
        return NULL;
    for (i = 0; i < manager->count; ++i)
        if (!strcmp(manager->pools[i].type.name, name))
            return pool_acquire(&manager->pools[i]);
    return NULL;
}

void work_pool_mgr_release(struct WorkPoolMgr *manager, struct AsyncWork *work)
{
    size_t i;
    if (!manager || !work)
        return;
    for (i = 0; i < manager->count; ++i) {
        struct WorkPool *pool = &manager->pools[i];
        struct WorkAllocation *entry;
        EnterCriticalSection(&pool->lock);
        for (entry = pool->first; entry; entry = entry->next) {
            if (entry->work != work)
                continue;
            if (entry->leased) {
                entry->leased = 0;
                entry->next_free = NULL;
                if (pool->last_free)
                    pool->last_free->next_free = entry;
                else
                    pool->first_free = entry;
                pool->last_free = entry;
                ++pool->free_count;
            }
            LeaveCriticalSection(&pool->lock);
            return;
        }
        LeaveCriticalSection(&pool->lock);
    }
}
