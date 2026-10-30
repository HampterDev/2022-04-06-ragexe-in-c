#ifndef SECURITY_GG_HASH_MGR_H
#define SECURITY_GG_HASH_MGR_H

struct GGHashManager;
extern struct GGHashManager *g_ggHashMgr;
int gg_hash_manager_startup(void);
void gg_hash_manager_shutdown(void);
int gg_hash_manager_is_intact(struct GGHashManager *self);
int gg_hash_manager_check_file(struct GGHashManager *self, const char *name);
int gg_hash_manager_check_data(struct GGHashManager *self, const char *name,
                             const void *data, unsigned size);
int security_queue_lua_file(const char *name);
void client_lua_loadfile_hook(const char *name);
void security_poll(struct GGHashManager *self);

#endif
