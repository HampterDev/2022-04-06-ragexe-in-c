#ifndef RENDER_TEXTURE_CACHE_H
#define RENDER_TEXTURE_CACHE_H
#include "render/texture.h"

struct TextureCacheStorage;
struct TexCache { struct TextureCacheStorage *storage; };

extern struct TexCache g_textureCache;
struct Texture *texture_cache_get(struct TexCache *cache, const char *name, int keyed, int full_size);
void ui_tex_cache_release(struct TexCache *cache, struct Texture *texture);
void texcache_clear(struct TexCache *cache);
void texcache_reload_all(struct TexCache *cache);
void texcache_evict(struct TexCache *cache);
void tex_cache_destruct(struct TexCache *cache);

void texture_cache_init(void);

extern int g_textureDivisor;

#endif
