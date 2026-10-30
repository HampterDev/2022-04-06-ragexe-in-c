#include "render/cache_lock.h"
#include "std/string.h"
#ifndef FILEIO_RES_MGR_H
#define FILEIO_RES_MGR_H
#include "fileio/resource_api.h"
#include "resources/resource_registry.h"
#include "fileio/resource_cache_store.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "base.h"
#include "std/std.h"
#include "fileio/grf.h"
#include "fileio/resource_ops.h"

struct Res *res_construct(struct Res *self);
void res_destruct(struct Res *self);

struct ResMgr {
    struct ResourceList resources[RESOURCE_TYPE_COUNT];
    CRITICAL_SECTION lock;
    struct ResourceAliases aliases;
    struct ResourceList resident;
};

extern unsigned g_resPurgeAgeMs;

struct ResMgr *res_mgr_construct(struct ResMgr *self);
void res_mgr_destruct(struct ResMgr *self);
int res_mgr_get_type_index(const char *ext);
struct Res *res_mgr_find(struct ResMgr *self, int type, const char *key);
const char *res_mgr_get_alias(struct ResMgr *self, const char *name);

void cache_section_enter(CRITICAL_SECTION *cs, int calls, int contended);
void cache_section_leave(CRITICAL_SECTION *cs);
enum {
    CACHE_S_RELEASES = 0, CACHE_S_SKIPPED = 1, CACHE_S_CONTENDED = 2,
    CACHE_S_LOOKUP_CALLS = 6, CACHE_S_LOOKUP_CONTENDED = 7
};
extern unsigned cache_stat[16];

BYTE res_mgr_exists(struct ResMgr *self, struct String name);

#endif
