#ifndef CACHE_LOCK_H
#define CACHE_LOCK_H
enum TextureCacheLockOperation {
    TEXTURE_CACHE_LOAD,
    TEXTURE_CACHE_RELEASE,
    TEXTURE_CACHE_PURGE,
    TEXTURE_CACHE_MAINTENANCE
};
void *tex_lock_enter(int operation);
void tex_lock_leave(void *section);
void cache_lock_write_begin(void);
void cache_lock_write_end(void);

struct ResMgr;
void cache_purge(struct ResMgr *self);

#endif
