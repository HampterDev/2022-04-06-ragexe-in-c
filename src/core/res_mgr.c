#include <stdlib.h>
#include "fileio/res_mgr.h"
#include "core/res_mgr.h"

struct ResMgr g_resMgrObject;
struct Once g_resMgrGuard;

void res_mgr_atexit(void);

struct ResMgr *g_resMgr(void)
{
    if (once_begin(&g_resMgrGuard)) {
        res_mgr_construct(&g_resMgrObject);
        atexit(res_mgr_atexit);
        once_complete(&g_resMgrGuard);
    }
    return &g_resMgrObject;
}

void resource_cache_init(void)
{
    g_resMgr();
}

void res_mgr_atexit(void)
{
    res_mgr_destruct(&g_resMgrObject);
}
