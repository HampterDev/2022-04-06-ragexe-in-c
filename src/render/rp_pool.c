#include "render/device.h"
#include "render/render_api.h"
#include "render/sprite_atlas.h"
#include "render/texture.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "render/rp_pool.h"
#include "platform/clock.h"

void (*g_renderBeforeClearLists)(void);

struct RPQuad *rp_quad_construct(struct RPQuad *self)
{
    self->verts = NULL;
    self->nverts = 0;
    self->texture = NULL;
    self->alpha_mode = 1;
    self->world = 0;
    self->srcBlend = 0;
    self->destBlend = 0;
    self->stageMode = 0;
    self->prim = RP_LIST;
    self->instance = NULL;
    self->rhw[0] = self->rhw[1] = 0.0f;
    return self;
}

static void tex_vertex_default(struct TexDrawVertex *v)
{
    v->x = 0.0f;
    v->y = 0.0f;
    v->z = 0.0f;
    v->rhw = 0.0f;
    v->diffuse = 0;
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
    h->instance = NULL;
    h->rhw[0] = h->rhw[1] = 0.0f;
    h->alpha_mode = 1;
    h->world = 0;
    h->srcBlend = 0;
    h->destBlend = 0;
    h->stageMode = 0;
    for (i = 0; i < 4; i++)
        tex_vertex_default(&self->quad[i]);
    h->prim = RP_STRIP;
}

struct TexDrawPrim *rp_quad_batch_construct(struct TexDrawPrim *self)
{
    tex_draw_prim_header(self);
    return self;
}

struct TexDrawPrim *rp_instance_construct(struct TexDrawPrim *self)
{
    tex_draw_prim_header(self);
    memset(&self->instance, 0, sizeof self->instance);
    self->base.instance = &self->instance;
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
    struct RenderPool quads, primitives, multitexture_quads, vertices, mesh_draws;
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
static void initialize_multitexture_quad(void *item) { rp_instance_construct(item); }

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
        pool_free(&storage->multitexture_quads);
        pool_free(&storage->vertices);
        pool_free(&storage->mesh_draws);
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
        || !pool_init(&storage->multitexture_quads, sizeof(struct TexDrawPrim), 1024, initialize_multitexture_quad)
        || !pool_init(&storage->vertices, sizeof(struct TexDrawVertex), 10240, NULL)
        || !pool_init(&storage->mesh_draws, sizeof(struct RPMeshDraw), 256, NULL)) {
        renderer_storage_free(self);
        return NULL;
    }
    self->clearColor = 0;
    self->frame = self->fpsCount = self->fps = 0;
    self->fpsTick = platform_ticks_ms();
    self->boundTextures[0] = self->boundTextures[1] = NULL;
    self->fogEnable = 0;
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
    struct TexDrawPrim *prim = pool_take(&renderer->storage->primitives, 1);
    prim->base.world = 0;
    prim->base.instance = NULL;
    return prim;
}

struct TexDrawPrim *renderer_new_prim_instance(struct Renderer *renderer)
{
    struct TexDrawPrim *prim = renderer_new_prim(renderer);
    memset(&prim->instance, 0, sizeof prim->instance);
    prim->base.instance = &prim->instance;
    return prim;
}

struct TexDrawPrim *renderer_new_multi_tex_quad(struct Renderer *renderer)
{
    struct TexDrawPrim *quad = pool_take(&renderer->storage->multitexture_quads, 1);
    memset(&quad->instance, 0, sizeof quad->instance);
    quad->base.instance = &quad->instance;
    return quad;
}

struct RPQuad *renderer_new_quad(struct Renderer *renderer)
{
    struct RPQuad *quad = pool_take(&renderer->storage->quads, 1);
    quad->world = 0;
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
    retained.sort_key = texture ? texture->serial : 0;
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
    queue->items[queue->count].order = (unsigned)queue->count;
    queue->items[queue->count].textures[0] = retain_texture(texture0);
    queue->items[queue->count].textures[1] = retain_texture(texture1);
    queue->items[queue->count].mesh = 0;
    ++queue->count;
}

void renderer_add_multi_tex_batch(struct Renderer *self, struct RPQuad *prim, struct Texture *texture1)
{
    queue_append(&self->queues[RENDER_MULTITEXTURE], prim, 0.0f, prim->texture, texture1);
}

static float edge_key(const float *rhw)
{
    return (rhw[0] - rhw[1]) * 0.5f + rhw[1];
}

/* The lists of a depth order sort a primitive by the rhw of its first two vertices, an instance by
   that of its first two corners. */
void renderer_add_prim(struct Renderer *r, struct RPQuad *p, int flags)
{
    struct RPQuad *q = p;
    float v[2];

    v[0] = q->instance ? q->rhw[0] : q->verts[0].rhw;
    v[1] = q->instance ? q->rhw[1] : q->verts[1].rhw;

    if (!(flags & 1)) {
        if (flags & 0x20)
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
        queue_append(&r->queues[RENDER_ALPHA_WRAP], q, v[0], q->texture, NULL);
        return;
    }
    if (q->alpha_mode == 1) {
        if (flags & 0x8000)
            queue_append(&r->queues[RENDER_ALPHA_WRITE_DEPTH], q, v[0], q->texture, NULL);
        else
            queue_append(&r->queues[RENDER_ALPHA_CLAMP], q, v[0], q->texture, NULL);
        return;
    }
    queue_append(&r->queues[RENDER_ALPHA_REMAINDER], q, v[0], q->texture, NULL);
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

/* the queue renderer_add_prim puts a quad with these flags in, of those a model's quads use */
int renderer_mesh_pass(int flags)
{
    if (!(flags & 1))
        return flags & 0x20 ? RENDER_OPAQUE_WRAP : RENDER_OPAQUE_CLAMP;
    if (flags & 0x20)
        return RENDER_ALPHA_WRAP;
    return flags & 0x8000 ? RENDER_ALPHA_WRITE_DEPTH : RENDER_ALPHA_CLAMP;
}

void renderer_add_mesh_draw(struct Renderer *self, int pass, const struct RPMeshDraw *draw, struct Texture *texture0,
                            struct Texture *texture1, float key)
{
    struct RPMeshDraw *copy;
    struct RenderQueue *queue;

    if (!draw || !draw->mesh || draw->draw.count <= 0)
        return;
    switch (pass) {
    case RENDER_OPAQUE_CLAMP:
    case RENDER_OPAQUE_WRAP:
    case RENDER_ALPHA_CLAMP:
    case RENDER_ALPHA_WRAP:
    case RENDER_ALPHA_WRITE_DEPTH:
    case RENDER_GROUND_MESH:
    case RENDER_SHADOW_CASTERS:
        break;
    default:
        return;
    }
    copy = pool_take(&self->storage->mesh_draws, 1);
    *copy = *draw;
    queue = &self->queues[pass];
    queue_append(queue, copy, key, texture0, texture1);
    queue->items[queue->count - 1].mesh = 1;
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
        pool_rewind(&storage->multitexture_quads);
        pool_rewind(&storage->vertices);
        pool_rewind(&storage->mesh_draws);
    }
    sprite_atlas_finish_frame(self);
}
