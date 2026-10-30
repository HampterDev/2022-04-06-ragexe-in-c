#ifndef RESOURCE_API_H
#define RESOURCE_API_H

struct Res;
struct ResMgr;
struct String;

struct ResMgr *g_resMgr(void);
void *res_mgr_get(struct ResMgr *self, const char *name);
void *res_mgr_get_string(struct ResMgr *self, struct String name);
unsigned char res_mgr_is_available(struct ResMgr *self, const char *name);
void res_mgr_release(struct ResMgr *self, struct Res *res);
void res_lock(struct Res *self);
void res_unlock(struct Res *self);

void res_mgr_delete_all(struct ResMgr *self);
void res_mgr_call_residents(struct ResMgr *self);
void res_mgr_purge_by_extension(struct ResMgr *self, const char *ext);
void res_mgr_purge_unlocked(struct ResMgr *self);
void res_mgr_read_res_name_table(struct ResMgr *self);
void res_mgr_purge_stale(struct ResMgr *self);

char *res_mgr_resolve_map_name(struct ResMgr *self, const char *name);

#endif
