#include "render/device.h"
#include "render/render_api.h"
#include "render/sprite_atlas.h"
#include "render/texture.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "render/rp_pool.h"
#include <mmsystem.h>

void (*g_renderBeforeClearLists)(void);

struct RPQuad *rp_quad_construct(struct RPQuad *self)
{
    self->verts = NULL;
    self->nverts = 0;
    self->texture = NULL;
    self->indices = NULL;
    self->nindices = 0;
    self->alpha_mode = 1;
    self->srcBlend = 0;
    self->destBlend = 0;
    self->stageMode = 0;
    self->prim = 4;
    return self;
}

static void two_tex_vertex_construct(struct RPTwoTexVertex *v)
{
    v->x = 0.0f;
    v->y = 0.0f;
    v->z = 0.0f;
    v->rhw = 0.0f;
    v->diffuse = 0;
    v->specular = 0xff000000u;
    v->u0 = 0.0f;
    v->v0 = 0.0f;
}

static void two_tex_quad_header(struct RPTwoTexQuad *self)
{
    int i;

    for (i = 0; i < 4; i++)
        two_tex_vertex_construct(&self->quad[i]);
    self->prim.verts = self->quad;
    self->prim.nverts = 4;
    self->prim.indices = NULL;
    self->prim.nindices = 0;
    self->prim.prim = 5;
}

struct RPTwoTexQuad *rp_two_tex_quad_construct(struct RPTwoTexQuad *self)
{
    two_tex_quad_header(self);
    return self;
}

static void tex_vertex_default(struct TexDrawVertex *v)
{
    v->x = 0.0f;
    v->y = 0.0f;
    v->z = 0.0f;
    v->rhw = 0.0f;
    v->diffuse = 0;
    v->specular = 0xff000000u;
    v->u = 0.0f;
    v->v = 0.0f;
}

struct TexDrawVertex *tex_draw_vertex_construct(struct TexDrawVertex *self)
{
    tex_vertex_default(self);
    return self;
}

static void tex_draw_prim_header(struct TexDrawPrim *self)
{
    struct RPQuad *h = &self->base;
    int i;

    h->verts = self->quad;
    h->nverts = 4;
    h->texture = NULL;
    h->indices = NULL;
    h->nindices = 0;
    h->alpha_mode = 1;
    h->srcBlend = 0;
    h->destBlend = 0;
    h->stageMode = 0;
    for (i = 0; i < 4; i++)
        tex_vertex_default(&self->quad[i]);
    h->prim = 5;
}

struct TexDrawPrim *rp_quad_batch_construct(struct TexDrawPrim *self)
{
    tex_draw_prim_header(self);
    return self;
}

struct TexDrawPrim *rp_quad_batch_copy_verts(struct TexDrawPrim *self, const struct TexDrawPrim *src)
{
    memcpy(self->quad, src->quad, sizeof(self->quad));
    return self;
}

struct RenderBlock {
    struct RenderBlock *next;
    void *items;
    size_t capacity, used;
};

struct RenderPool {
    struct RenderBlock *first, *current;
    size_t element_size, block_capacity;
    void (*initialize)(void *item);
};

struct RenderStorage {
    struct RenderPool quads, primitives, two_texture_quads, vertices;
};

static void pool_free(struct RenderPool *pool)
{
    struct RenderBlock *block = pool->first;
    while (block) {
        struct RenderBlock *next = block->next;
        free(block->items);
        free(block);
        block = next;
    }
    pool->first = pool->current = NULL;
}

static struct RenderBlock *pool_block(struct RenderPool *pool, size_t count)
{
    struct RenderBlock *block;
    size_t i, capacity = count > pool->block_capacity ? count : pool->block_capacity;
    if (!capacity || capacity > SIZE_MAX / pool->element_size) return NULL;
    block = calloc(1, sizeof *block);
    if (!block) return NULL;
    block->items = calloc(capacity, pool->element_size);
    if (!block->items) { free(block); return NULL; }
    block->capacity = capacity;
    if (pool->initialize)
        for (i = 0; i < capacity; ++i)
            pool->initialize((char *)block->items + i * pool->element_size);
    return block;
}

static int pool_init(struct RenderPool *pool, size_t element_size, size_t capacity,
                     void (*initialize)(void *))
{
    pool->element_size = element_size;
    pool->block_capacity = capacity;
    pool->initialize = initialize;
    pool->first = pool->current = pool_block(pool, capacity);
    return pool->first != NULL;
}

static void *pool_take(struct RenderPool *pool, size_t count)
{
    struct RenderBlock *block = pool->current;
    void *items;
    if (!count) return NULL;
    while (count > block->capacity - block->used) {
        if (!block->next) {
            block->next = pool_block(pool, count);
            if (!block->next) abort();
        }
        block = block->next;
    }
    pool->current = block;
    items = (char *)block->items + block->used * pool->element_size;
    block->used += count;
    return items;
}

static void pool_rewind(struct RenderPool *pool)
{
    struct RenderBlock *block;
    for (block = pool->first; block; block = block->next) block->used = 0;
    pool->current = pool->first;
}

static void initialize_quad(void *item) { rp_quad_construct(item); }
static void initialize_primitive(void *item) { rp_quad_batch_construct(item); }
static void initialize_two_texture_quad(void *item) { rp_two_tex_quad_construct(item); }

static void queue_clear(struct RenderQueue *queue)
{
    size_t i;
    for (i = 0; i < queue->count; ++i) {
        unsigned stage;
        for (stage = 0; stage < 2; ++stage) {
            gfx_surface *surface = queue->items[i].textures[stage].surface;
            if (surface) gfx_release(surface);
        }
    }
    queue->count = 0;
}

static void renderer_storage_free(struct Renderer *renderer)
{
    struct RenderStorage *storage = renderer->storage;
    size_t i;
    for (i = 0; i < RENDER_PASS_COUNT; ++i) queue_clear(&renderer->queues[i]);
    renderer->boundTextures[0] = renderer->boundTextures[1] = NULL;
    if (storage) {
        pool_free(&storage->quads);
        pool_free(&storage->primitives);
        pool_free(&storage->two_texture_quads);
        pool_free(&storage->vertices);
        free(storage);
        renderer->storage = NULL;
    }
    for (i = 0; i < RENDER_PASS_COUNT; ++i) {
        free(renderer->queues[i].items);
        memset(&renderer->queues[i], 0, sizeof renderer->queues[i]);
    }
}

struct Renderer *renderer_construct(struct Renderer *self)
{
    struct RenderStorage *storage;
    memset(self->queues, 0, sizeof self->queues);
    self->sprite_atlas = NULL;
    self->storage = storage = calloc(1, sizeof *storage);
    if (!storage) return NULL;
    if (!pool_init(&storage->quads, sizeof(struct RPQuad), 1024, initialize_quad)
        || !pool_init(&storage->primitives, sizeof(struct TexDrawPrim), 512, initialize_primitive)
        || !pool_init(&storage->two_texture_quads, sizeof(struct RPTwoTexQuad), 8192, initialize_two_texture_quad)
        || !pool_init(&storage->vertices, sizeof(struct TexDrawVertex), 10240, NULL)) {
        renderer_storage_free(self);
        return NULL;
    }
    self->clearColor = 0;
    self->frame = self->fpsCount = self->fps = 0;
    self->fpsTick = timeGetTime();
    self->boundTextures[0] = self->boundTextures[1] = NULL;
    self->fogEnable = self->fogVertexCaps = 0;
    self->clipLeft = self->clipRight = self->clipTop = self->clipBottom = 0.0f;
    self->twoPassAlpha = 0;
    sprite_atlas_initialize(self);
    return self;
}

void renderer_destruct(struct Renderer *self)
{
    if (!self) return;
    if (g_renderer == self) g_renderer = NULL;
    renderer_storage_free(self);
    sprite_atlas_destroy(self);
}

void renderer_save_frame(struct Renderer *self)
{
    (void)self;
    c3d_device_save_frame(&g_3dDevice);
}

struct TexDrawPrim *renderer_new_prim(struct Renderer *renderer)
{
    return pool_take(&renderer->storage->primitives, 1);
}

struct RPTwoTexQuad *renderer_new_two_tex_quad(struct Renderer *renderer)
{
    return pool_take(&renderer->storage->two_texture_quads, 1);
}

struct RPQuad *renderer_new_quad(struct Renderer *renderer)
{
    struct RPQuad *quad = pool_take(&renderer->storage->quads, 1);
    quad->nindices = 0;
    return quad;
}

struct TexDrawVertex *renderer_new_tex_draw_vertices(struct Renderer *renderer, int count)
{
    struct TexDrawVertex *vertices;
    int i;
    if (count <= 0) return NULL;
    vertices = pool_take(&renderer->storage->vertices, (size_t)count);
    for (i = 0; i < count; ++i) tex_vertex_default(vertices + i);
    return vertices;
}

static struct RenderTexture retain_texture(const struct Texture *texture)
{
    struct RenderTexture retained;
    retained.surface = texture ? texture->base.surface : NULL;
    retained.sort_key = (uintptr_t)texture;
    if (retained.surface) gfx_addref(retained.surface);
    return retained;
}

static void queue_append(struct RenderQueue *queue, void *primitive, float key,
    const struct Texture *texture0, const struct Texture *texture1)
{
    struct RenderEntry *items;
    if (queue->count == queue->capacity) {
        size_t capacity = queue->capacity ? queue->capacity * 2 : 64;
        if (capacity < queue->capacity || capacity > SIZE_MAX / sizeof *items) abort();
        items = realloc(queue->items, capacity * sizeof *items);
        if (!items) abort();
        queue->items = items;
        queue->capacity = capacity;
    }
    queue->items[queue->count].primitive = primitive;
    queue->items[queue->count].key = key;
    queue->items[queue->count].textures[0] = retain_texture(texture0);
    queue->items[queue->count].textures[1] = retain_texture(texture1);
    ++queue->count;
}

int rp_vertex_fog_alpha(float rhw)
{
    float range = g_fogFar - g_fogNear;
    float t = (1.0f / rhw - 10.0f) / 1490.0f;

    if (range != 0.0f)
        t = (t - g_fogNear) / range;
    t = 0.0f > t ? 0.0f : t;
    t = 1.0f < t ? 1.0f : t;
    t = (1.0f - t) * 255.0f;
    return (int)t;
}

void renderer_add_two_tex_prim(struct Renderer *self, struct RPTwoTex *prim, int which)
{
    int i;

    if (self->fogVertexCaps) {
        for (i = 0; i < prim->nverts; i++) {
            int alpha = rp_vertex_fog_alpha(((struct RPTwoTexVertex *)prim->verts)[i].rhw);

            ((struct RPTwoTexVertex *)prim->verts)[i].specular = (DWORD)alpha << 24;
        }
    }
    if (which == 0)
        queue_append(&self->queues[RENDER_WATER_BUMP], prim, 0.0f, prim->tex0, prim->tex1);
    else if (which == 1)
        queue_append(&self->queues[RENDER_TWO_TEXTURE_ALT], prim, 0.0f, prim->tex0, prim->tex1);
}

void renderer_add_multi_tex_batch(struct Renderer *self, struct RPTwoTex *prim)
{
    int i;

    for (i = 0; i < prim->nverts; i++) {
        int alpha = rp_vertex_fog_alpha(((struct RPTwoTexVertex *)prim->verts)[i].rhw);

        ((struct RPTwoTexVertex *)prim->verts)[i].specular = (DWORD)alpha << 24;
    }
    queue_append(&self->queues[RENDER_MULTITEXTURE], prim, 0.0f, prim->tex0, prim->tex1);
}

static float edge_key(const struct TexDrawVertex *v)
{
    return (v[0].rhw - v[1].rhw) * 0.5f + v[1].rhw;
}

void renderer_add_prim(struct Renderer *r, struct RPQuad *p, int flags)
{
    struct RPQuad *q = p;
    const struct TexDrawVertex *v;
    int i;

    if (r->fogVertexCaps && q->nverts > 0) {
        for (i = 0; i < q->nverts; i++) {
            int alpha = rp_vertex_fog_alpha(((struct TexDrawVertex *)q->verts)[i].rhw);

            ((struct TexDrawVertex *)q->verts)[i].specular = (DWORD)alpha << 24;
        }
    }
    v = (const struct TexDrawVertex *)q->verts;
    if (!(flags & 1)) {
        if (flags & 0x800)
            queue_append(&r->queues[RENDER_WATER_BASE], q, 0.0f, q->texture, NULL);
        else if (flags & 0x1000)
            queue_append(&r->queues[RENDER_WATER_BLEND], q, 0.0f, q->texture, NULL);
        else if (flags & 0x20)
            queue_append(&r->queues[RENDER_OPAQUE_WRAP], q, 0.0f, q->texture, NULL);
        else
            queue_append(&r->queues[RENDER_OPAQUE_CLAMP], q, 0.0f, q->texture, NULL);
        return;
    }
    if (flags & 4) {
        if (flags & 8)
            queue_append(&r->queues[RENDER_BLENDED_NO_DEPTH], q, 0.0f, q->texture, NULL);
        else if (flags & 0x200)
            queue_append(&r->queues[RENDER_BLENDED_OVERLAY], q, edge_key(v), q->texture, NULL);
        else
            queue_append(&r->queues[RENDER_BLENDED_WRAP], q, 0.0f, q->texture, NULL);
        return;
    }
    if (flags & 2) {
        if (flags & 0x100)
            queue_append(&r->queues[RENDER_ADDITIVE_FOG], q, edge_key(v), q->texture, NULL);
        else if (flags & 0x200)
            queue_append(&r->queues[RENDER_ADDITIVE_OVERLAY], q, edge_key(v), q->texture, NULL);
        else
            queue_append(&r->queues[RENDER_ADDITIVE], q, edge_key(v), q->texture, NULL);
        return;
    }
    if (flags & 0x10) {
        queue_append(&r->queues[RENDER_ALPHA_CUTOUT], q, edge_key(v), q->texture, NULL);
        return;
    }
    if (flags & 0x100) {
        queue_append(&r->queues[RENDER_ALPHA_DEPTH], q, edge_key(v), q->texture, NULL);
        return;
    }
    if (flags & 0x200) {
        if (flags & 8)
            queue_append(&r->queues[RENDER_ALPHA_NO_DEPTH], q, edge_key(v), q->texture, NULL);
        else
            queue_append(&r->queues[RENDER_OVERLAY_ORDERED], q, edge_key(v), q->texture, NULL);
        return;
    }
    if (flags & 0x20) {
        queue_append(&r->queues[RENDER_ALPHA_WRAP], q, v[0].rhw, q->texture, NULL);
        return;
    }
    if (q->alpha_mode == 1) {
        if (flags & 0x8000)
            queue_append(&r->queues[RENDER_ALPHA_WRITE_DEPTH], q, v[0].rhw, q->texture, NULL);
        else
            queue_append(&r->queues[RENDER_ALPHA_CLAMP], q, v[0].rhw, q->texture, NULL);
        return;
    }
    queue_append(&r->queues[RENDER_ALPHA_REMAINDER], q, v[0].rhw, q->texture, NULL);
}

void renderer_add_triangle(struct Renderer *r, struct PrimTriangle *tri, int list_flags)
{
    struct RPQuad *q = renderer_new_quad(r);

    q->verts = tri->v;
    q->nverts = 3;
    q->texture = tri->texture;
    q->stageMode = 0;
    q->srcBlend = tri->src_blend;
    q->destBlend = tri->dest_blend;
    renderer_add_prim(r, q, list_flags);
}

void rp_quad_set_vertex_pos(struct RPQuad *self, int i, const struct TexDrawVertex *vertex)
{
    struct TexDrawVertex *v = (struct TexDrawVertex *)self->verts;

    v[i].x = vertex->x;
    v[i].y = vertex->y;
    v[i].z = vertex->z;
    v[i].rhw = vertex->rhw;
}

void renderer_destroy_all_rp_list(struct Renderer *self)
{
    struct RenderStorage *storage = self->storage;
    size_t i;
    if (g_renderBeforeClearLists)
        g_renderBeforeClearLists();
    for (i = 0; i < RENDER_PASS_COUNT; ++i) queue_clear(&self->queues[i]);
    self->boundTextures[0] = self->boundTextures[1] = NULL;
    if (storage) {
        pool_rewind(&storage->quads);
        pool_rewind(&storage->primitives);
        pool_rewind(&storage->two_texture_quads);
        pool_rewind(&storage->vertices);
    }
    sprite_atlas_finish_frame(self);
}
