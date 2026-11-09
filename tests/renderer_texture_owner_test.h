#define malloc checked_malloc
#define calloc checked_calloc
#define realloc checked_realloc
#define free checked_free
#include "../src/render/csurface.c"
#include "../src/resources/spr.c"
#include "../src/render/tex_cache.c"
#include "../src/fileio/file.c"
#undef free
#undef realloc
#undef calloc
#undef malloc
#include "../src/render/device_palette.c"

#define SPR_RGBA_IMAGES 1

struct perf_counters g_perf;
struct GameFileMgr g_fileMgr;
int g_sprHalfWidth, g_sprHalfHeight;
unsigned g_resPurgeAgeMs = 0;
static struct ResMgr resource_manager;
static struct BitmapRes input_bitmap;
static DWORD bitmap_pixels[4];
static unsigned surface_id;
static int upload_failure;
static BYTE sprite_file[16];
static unsigned file_borrows;

gfx_surface *gfx_create_texture(int width, int height, int format)
{
    gfx_surface *surface;
    assert(width > 0 && height > 0 && width <= 16 && height <= 16);
    surface = checked_calloc(1, sizeof *surface);
    if (!surface) return NULL;
    surface->id = ++surface_id; surface->refs = 1; surface->allocated = 1;
    surface->width = width; surface->height = height; surface->format = format;
    ++live_surfaces;
    return surface;
}
int gfx_texture_format_ok(int format) { (void)format; return 1; }
int gfx_get_size(gfx_surface *surface, int *width, int *height)
{ assert(surface && surface->refs); *width = surface->width; *height = surface->height; return 0; }
int gfx_lock(gfx_surface *surface, const RECT *rect, int flags, struct gfx_lock *out)
{
    unsigned stride;
    (void)flags;
    assert(surface && surface->refs);
    if (upload_failure) return -1;
    stride = surface->format == GFX_FMT_8888 ? 4 : 2;
    out->bits = (BYTE *)surface->pixels + (rect ? rect->top * surface->width + rect->left : 0) * stride;
    out->pitch = surface->width * stride; out->bpp = stride * 8;
    out->width = rect ? rect->right - rect->left : surface->width;
    out->height = rect ? rect->bottom - rect->top : surface->height;
    return 0;
}
int gfx_unlock(gfx_surface *surface) { assert(surface && surface->refs); return 0; }
int gfx_refresh_mips(gfx_surface *surface, const RECT *rect)
{ assert(surface && surface->refs && rect); return 0; }
unsigned gfx_content_version(gfx_surface *surface) { assert(surface); return 0; }
void gfx_evict_managed_textures(struct C3dDevice *device) { assert(device == &g_3dDevice); }
void c3d_device_clamp_texture_size(struct C3dDevice *device, unsigned *width, unsigned *height)
{ assert(device == &g_3dDevice); assert(*width <= 16 && *height <= 16); }

struct Res *res_construct(struct Res *resource)
{ memset(resource, 0, sizeof *resource); resource->typeIndex = -1; return resource; }
struct ResMgr *g_resMgr(void) { return &resource_manager; }
void *res_mgr_get(struct ResMgr *self, const char *name)
{ assert(self == &resource_manager && name); return &input_bitmap; }
void res_mgr_release(struct ResMgr *self, struct Res *resource)
{ assert(self == &resource_manager && resource == &input_bitmap.res); }
BYTE *game_file_mgr_get_data(struct GameFileMgr *manager, const char *name, int *size, BYTE disk)
{
    assert(manager == &g_fileMgr && name && !disk && !file_borrows);
    ++file_borrows; *size = sizeof sprite_file; return sprite_file;
}
void game_file_mgr_release_data(struct GameFileMgr *manager, void *data)
{ assert(manager == &g_fileMgr && data == sprite_file && file_borrows == 1); --file_borrows; }
BYTE game_file_mgr_is_file_exist(struct GameFileMgr *manager, const char *name, BYTE disk)
{ (void)manager; (void)name; (void)disk; return 1; }
void sprite_atlas_invalidate_images(struct Renderer *renderer, const struct SprImg *images, size_t count)
{ (void)renderer; (void)images; assert(!count); }
void sprite_atlas_invalidate_palette(struct Renderer *renderer, const DWORD *palette)
{ (void)renderer; assert(palette); }
static int texture_lock_depth, cache_write_depth;
void *tex_lock_enter(int operation) { (void)operation; ++texture_lock_depth; return &texture_lock_depth; }
void tex_lock_leave(void *section)
{ assert(section == &texture_lock_depth && texture_lock_depth > 0); --texture_lock_depth; }
void cache_lock_write_begin(void) { ++cache_write_depth; }
void cache_lock_write_end(void) { assert(cache_write_depth > 0); --cache_write_depth; }

void texture_draw_surface_stretch(struct Texture *texture, int x, int y, int w, int h)
{ (void)texture; (void)x; (void)y; (void)w; (void)h; abort(); }

static void queue_texture(struct Renderer *renderer, struct Texture *texture, int flags)
{
    struct TexDrawPrim *quad = renderer_new_prim(renderer);
    quad->base.texture = texture;
    quad->base.alpha_mode = 1;
    quad->quad[0].rhw = quad->quad[1].rhw = 0.00125f;
    renderer_add_prim(renderer, &quad->base, flags);
}
static void capture_frame(struct Renderer *renderer)
{
    sampled_count = 0; capture_draws = 1;
    renderer_draw_lists(renderer);
    capture_draws = 0;
}
static void sprite_fixture(BYTE red)
{
    static const BYTE header[12] = {'S','P',0,2,0,0,1,0,1,0,1,0};
    memcpy(sprite_file, header, sizeof header);
    sprite_file[12] = 255; sprite_file[13] = 0x30;
    sprite_file[14] = 0x20; sprite_file[15] = red;
}
static void queued_sprite_replacement(struct Renderer *renderer)
{
    struct Res *resource = sprite_resource_create();
    struct SprRes *sprite = (struct SprRes *)resource;
    gfx_surface *original, *replacement;
    unsigned original_id, replacement_id;
    DWORD original_pixel, replacement_pixel;
    assert(resource);
    sprite_fixture(0x10); assert(resource_load(resource, "queued.spr"));
    original = sprite->images[SPR_RGBA_IMAGES].items[0].texture->base.surface;
    original_id = original->id; original_pixel = original->pixels[0];
    queue_texture(renderer, sprite->images[SPR_RGBA_IMAGES].items[0].texture, 0x201);
    assert(original->refs == 2);
    sprite_fixture(0x90); upload_failure = 1;
    assert(!resource_load(resource, "queued.spr")); upload_failure = 0;
    assert(sprite->images[SPR_RGBA_IMAGES].items[0].texture->base.surface == original && original->refs == 2);
    assert(resource_load(resource, "queued.spr"));
    replacement = sprite->images[SPR_RGBA_IMAGES].items[0].texture->base.surface;
    replacement_id = replacement->id; replacement_pixel = replacement->pixels[0];
    assert(original != replacement && original->refs == 1 && original_pixel != replacement_pixel);
    queue_texture(renderer, sprite->images[SPR_RGBA_IMAGES].items[0].texture, 0x201);
    resource_destroy(resource);
    assert(original->refs == 1 && replacement->refs == 1 && live_surfaces == 2 && !file_borrows);
    capture_frame(renderer);
    assert(sampled_count == 2 && sampled[0].id[0] == original_id && sampled[1].id[0] == replacement_id);
    assert(sampled[0].pixel[0] == original_pixel && sampled[1].pixel[0] == replacement_pixel);
    renderer_destroy_all_rp_list(renderer); assert(!live_surfaces);
}

static void queued_cache_replacement(struct Renderer *renderer)
{
    struct TexCache cache = {0};
    struct Texture *texture;
    gfx_surface *original, *replacement;
    unsigned original_id, replacement_id;
    DWORD original_pixel, replacement_pixel;
    input_bitmap.width = input_bitmap.height = 1; input_bitmap.pixels = bitmap_pixels; input_bitmap.hasAlpha = 1;
    bitmap_pixels[0] = 0xff102030;
    texture = texture_cache_get(&cache, "queued.bmp", 0, 1);
    assert(texture != &g_whiteTexture);
    original = texture->base.surface; original_id = original->id; original_pixel = original->pixels[0];
    queue_texture(renderer, texture, 0x201);
    bitmap_pixels[0] = 0xff902030; upload_failure = 1;
    texcache_reload_all(&cache); upload_failure = 0;
    assert(texture->base.surface == original && original->refs == 2 && original->pixels[0] == original_pixel);
    texcache_reload_all(&cache);
    replacement = texture->base.surface; replacement_id = replacement->id; replacement_pixel = replacement->pixels[0];
    assert(replacement != original && original->refs == 1 && original_pixel != replacement_pixel);
    queue_texture(renderer, texture, 0x201);
    texcache_clear(&cache); assert(!cache.storage && live_surfaces == 2);
    capture_frame(renderer);
    assert(sampled_count == 2 && sampled[0].id[0] == original_id && sampled[1].id[0] == replacement_id);
    assert(sampled[0].pixel[0] == original_pixel && sampled[1].pixel[0] == replacement_pixel);
    renderer_destroy_all_rp_list(renderer); assert(!live_surfaces);
    texture = texture_cache_get(&cache, "release.bmp", 0, 1);
    queue_texture(renderer, texture, 0); ui_tex_cache_release(&cache, texture);
    assert(live_surfaces == 1); renderer_destroy_all_rp_list(renderer); assert(!live_surfaces);
    texture = texture_cache_get(&cache, "evict.bmp", 0, 1);
    queue_texture(renderer, texture, 0); texture->lastUse -= 10; texcache_evict(&cache);
    assert(live_surfaces == 1); tex_cache_destruct(&cache);
    renderer_destroy_all_rp_list(renderer); assert(!live_surfaces);
}

static void queued_all_routes(struct Renderer *renderer)
{
    struct Texture *texture = texture_create(1, 1, GFX_FMT_8888);
    gfx_surface *surface = texture->base.surface;
    struct TexDrawPrim *quad = renderer_new_prim(renderer);
    struct RPTwoTexQuad *pair = renderer_new_two_tex_quad(renderer);
    struct PrimTriangle triangle = {0};
    struct RPMeshDraw draw;
    unsigned flags, mode, i, submissions = 0;
    quad->base.texture = texture;
    for (i = 0; i < 4; ++i) quad->quad[i].rhw = 0.00125f;
    for (mode = 0; mode < 2; ++mode) {
        quad->base.alpha_mode = (BYTE)mode;
        for (flags = 0; flags < 65536; ++flags) { renderer_add_prim(renderer, &quad->base, flags); ++submissions; }
    }
    pair->prim.tex0 = pair->prim.tex1 = texture;
    for (i = 0; i < 4; ++i) pair->quad[i].rhw = 0.00125f;
    renderer_add_multi_tex_batch(renderer, &pair->prim);
    triangle.texture = texture; renderer_add_triangle(renderer, &triangle, 0);
    memset(&draw, 0, sizeof draw);
    draw.mesh = (struct gfx_mesh *)&mesh_token;
    memcpy(draw.view, mesh_view, sizeof draw.view);
    draw.draw.count = 3;
    renderer_add_mesh_draw(renderer, RENDER_GROUND_MESH, &draw, texture, texture, 0.0f);
    renderer_add_mesh_draw(renderer, RENDER_SHADOW_CASTERS, &draw, texture, NULL, 0.0f);
    draw.cull = GFX_CULL_CW;
    renderer_add_mesh_draw(renderer, renderer_mesh_pass(0x21), &draw, texture, NULL, 0.00125f);
    assert(surface->refs == 1 + submissions + 2 + 1 + 4);
    for (i = 0; i < RENDER_PASS_COUNT; ++i) {
        size_t n;
        assert(renderer->queues[i].count);
        for (n = 0; n < renderer->queues[i].count; ++n) {
            const struct RenderTexture *retained = renderer->queues[i].items[n].textures;
            assert(retained[0].surface == surface && retained[0].sort_key == texture->serial);
        }
    }
    texture_destroy(texture); assert(live_surfaces == 1);
    for (i = 0; i < 4; ++i) {
        renderer->twoPassAlpha = (BYTE)(i & 1);
        g_lightmap = (int)(i >> 1);
        renderer_draw_lists(renderer);
        assert(live_surfaces == 1 && surface->refs == submissions + 2 + 1 + 4);
    }
    g_lightmap = 0;
    renderer_destroy_all_rp_list(renderer); renderer_destroy_all_rp_list(renderer); assert(!live_surfaces);
    texture = texture_create(1, 1, GFX_FMT_8888);
    queue_texture(renderer, texture, 0); texture_destroy(texture);
    renderer_destruct(renderer); renderer_destruct(renderer); assert(!live_surfaces && !outstanding);
}

static void queued_texture_ownership(void)
{
    struct Renderer renderer = {0};
    assert(renderer_construct(&renderer)); g_renderer = &renderer;
    queued_sprite_replacement(&renderer);
    queued_cache_replacement(&renderer);
    queued_all_routes(&renderer);
    assert(!live_surfaces && !outstanding && !file_borrows && !g_renderer);
    puts("Queued textures: real SPR/cache reload and release, preserved pixels, all primitive queues, actual draw passes after release, duplicate handles, skipped frames and repeated teardown passed.");
}
