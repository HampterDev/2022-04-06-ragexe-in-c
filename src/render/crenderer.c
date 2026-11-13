#include "render/device.h"
#include "render/render_api.h"
#include "diag/perf.h"
#include "diag/diag.h"
#include "std/std_sort.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "render/render_options.h"
#include "platform/clock.h"

DWORD g_renderCheckpointLine = 0;
const char *g_renderCheckpointFile = "";

static const char renderer_source_file[] = "c:\\repositories\\client\\kr\\kr.ro1.live\\source\\ragnarokclient\\3dlib\\renderer.cpp";

static void renderer_draw_lists(struct Renderer *self);

#define TIMED(phase, call) do { perf_begin(phase); call; perf_end(phase); } while (0)

static void set_tex0(struct Renderer *self, gfx_surface *surface)
{
    gfx_set_texture(0, surface);
    self->boundTextures[0] = surface;
}

static void set_tex1(struct Renderer *self, gfx_surface *surface)
{
    gfx_set_texture(1, surface);
    self->boundTextures[1] = surface;
}

/* custom: an instance the client made, or the instances of its vertices (render/gfx.h) */
static void draw_quad(const struct RPQuad *n)
{
    if (n->instance)
        gfx_draw_quad(n->instance);
    else if (n->nverts)
        gfx_draw_vertices(n->verts, n->nverts, n->prim == RP_STRIP);
}

/* custom: a Granny model's mesh in its actor's pose, the pose put among the frame's quads when the
   first of the actor's meshes is drawn (render/gfx.h, gfx_draw_skin) */
static void draw_skin(const struct RPSkinDraw *d)
{
    struct RPSkinPose *p = d->pose;

    if (p->row < 0)
        p->row = gfx_skin_pose((const float (*)[4])p->rows, p->rowCount);
    gfx_draw_skin(d->model, d->first, d->count, p->row);
}

static unsigned char keyed_less(const void *a, const void *b, void *pred)
{
    const struct RenderEntry *x = (const struct RenderEntry *)a, *y = (const struct RenderEntry *)b;
    (void)pred;
    if (y->key > x->key)
        return 1;
    if (x->key > y->key)
        return 0;
    return x->order < y->order;
}

static unsigned char quad_less(const void *a, const void *b, void *pred)
{
    const struct RenderEntry *x = a, *y = b;
    (void)pred;
    return x->textures[0].sort_key < y->textures[0].sort_key;
}

static unsigned char multitexture_less(const void *a, const void *b, void *pred)
{
    const struct RenderEntry *x = a, *y = b;
    (void)pred;
    if (x->textures[0].sort_key != y->textures[0].sort_key)
        return x->textures[0].sort_key < y->textures[0].sort_key;
    return x->textures[1].sort_key < y->textures[1].sort_key;
}

static void queue_sort(struct RenderQueue *queue, StdSortLessFn less)
{
    if (queue->count > 1)
        std_sort(queue->items, queue->items + queue->count, sizeof *queue->items, less, NULL);
}

/* custom: a draw of the static world among a queue's entries (render/gfx.h, struct RPWorldDraw): a
   record, or the cells of water from entry i on that follow one another with one texture, drawn as
   one run. Answers the entry after those drawn. */
static size_t draw_world_entries(struct Renderer *self, const struct RenderQueue *l, size_t i)
{
    const struct RPWorldDraw *w = l->items[i].primitive;
    size_t j, n = 0;

    if (w->kind == RP_WORLD_RECORD) {
        gfx_world_draw_record(&w->record, &w->draw);
        return i + 1;
    }
    for (j = i; j < l->count; j++) {
        const struct RPWorldDraw *x = l->items[j].primitive;

        if (l->items[j].kind != RP_ENTRY_WORLD || x->kind != RP_WORLD_WATER || l->items[j].textures[0].surface != l->items[i].textures[0].surface
            || x->colour != w->colour)
            break;
        if (!renderer_water_cells(self, n + 1))
            break;
        self->waterCells[n++] = x->cell;
    }
    if (!n)
        return i + 1;
    set_tex0(self, l->items[i].textures[0].surface);
    gfx_world_draw_water(self->waterCells, n, w->colour);
    return i + n;
}

static void flush_keyed(struct Renderer *self, int list, int sort, int skipNull)
{
    struct RenderQueue *l = &self->queues[list];
    struct RenderEntry *e;
    if (sort)
        queue_sort(l, keyed_less);
    for (size_t i = 0; i < l->count; ++i) {
        e = l->items + i;
        struct RPQuad *n = e->primitive;
        if (e->kind == RP_ENTRY_WORLD) {
            i = draw_world_entries(self, l, i) - 1;
            continue;
        }
        if (skipNull && !n)
            continue;
        set_tex0(self, e->textures[0].surface);
        draw_quad(n);
    }
}

static void flush_plain(struct Renderer *self, int list, int sort, int skipNull)
{
    struct RenderQueue *l = &self->queues[list];
    if (sort)
        queue_sort(l, quad_less);
    for (size_t i = 0; i < l->count; ++i) {
        struct RPQuad *n = l->items[i].primitive;
        if (skipNull && !n)
            continue;
        set_tex0(self, l->items[i].textures[0].surface);
        if (l->items[i].kind == RP_ENTRY_SKIN)
            draw_skin(l->items[i].primitive);
        else
            draw_quad(n);
    }
}

/* An effect list (render/draw_modes.h, DRAW_EFFECT_*): each quad with its own blending and
   texture-stage preset. The list starts with the blending and preset 0 the list before it left, as
   the Direct3D 9 client's states did; a preset that is none keeps the one before it. */
static void flush_effects(struct Renderer *self, int list, int mode, int src, int dst, int overlay)
{
    struct RenderQueue *l = &self->queues[list];
    int combine = DRAW_COMBINE_MODULATE;

    if (overlay)
        queue_sort(l, keyed_less);
    gfx_set_effect(src, dst, combine);
    gfx_set_mode(mode);
    for (size_t i = 0; i < l->count; ++i) {
        const struct RenderEntry *e = l->items + i;
        const struct RPQuad *n = e->primitive;
        if (!n)
            continue;
        set_tex0(self, e->textures[0].surface);
        combine = draw_effect_combine(n->stageMode, combine);
        if (overlay && e->key >= 0.99f)
            gfx_set_effect(DRAW_BLEND_SRCALPHA, DRAW_BLEND_INVSRCALPHA, combine);
        else
            gfx_set_effect((int)n->srcBlend, (int)n->destBlend, combine);
        draw_quad(n);
    }
}

/* The alpha lists of models and sprites, clamped then wrapped, in a pair of modes. */
static void flush_alpha_keyed_lists(struct Renderer *self, int clampMode, int wrapMode)
{
    gfx_set_mode(clampMode);
    flush_keyed(self, RENDER_ALPHA_CLAMP, 1, 0);
    gfx_set_mode(wrapMode);
    flush_keyed(self, RENDER_ALPHA_WRAP, 1, 0);
}

/* custom: draws a queue's casters of the static world into the shadow map cascade chosen
   (render/gfx.h, gfx_world_cast) */
static void draw_shadow_casters(struct Renderer *self, int list)
{
    struct RenderQueue *l = &self->queues[list];

    for (size_t i = 0; i < l->count; ++i) {
        const struct RPWorldDraw *w = l->items[i].primitive;

        gfx_world_cast(w->firstIndex, w->count, w->baseVertex, w->textureArray, w->ids);
    }
}

/* custom: the shadow maps of this frame's casters, models and ground, which every mesh draw of
   the frame then receives (self->shadowDrawn) */
static void draw_shadow_maps(struct Renderer *self)
{
    int cascade;

    self->shadowDrawn = 0;
    if (!self->shadowPending)
        return;
    self->shadowPending = 0;
    if (!self->queues[RENDER_SHADOW_CASTERS].count)
        return;
    if (!gfx_shadow_begin(&self->shadow))
        return;
    for (cascade = 0; cascade < self->shadow.cascades; ++cascade) {
        if (!gfx_shadow_cascade(cascade))
            continue;
        draw_shadow_casters(self, RENDER_SHADOW_CASTERS);
        self->shadowDrawn = 1;
    }
    gfx_shadow_end();
}

/* custom: the opaque models of the static world (render/gfx.h, gfx_world_draw) come first in their
   lists, in the order of their textures, then the lists' quads in theirs. The renderer sorted both
   together by the textures' creation order: a quad whose texture was made before a model's (a Granny
   actor's cached from an earlier map) drew before it, and now draws after it, which shows only
   where the two lie at the same depth. */
static void flush_opaque_lists(struct Renderer *self)
{
    draw_shadow_maps(self);
    gfx_shadow_receive(self->shadowDrawn);
    gfx_set_mode(DRAW_OPAQUE_CLAMP);
    if (self->worldSet)
        gfx_world_draw(SW_PASS_OPAQUE_CLAMP, &self->worldModels);
    flush_plain(self, RENDER_OPAQUE_CLAMP, 1, 1);
    gfx_set_mode(DRAW_OPAQUE_WRAP);
    if (self->worldSet)
        gfx_world_draw(SW_PASS_OPAQUE_WRAP, &self->worldModels);
    flush_plain(self, RENDER_OPAQUE_WRAP, 1, 1);
}

static void flush_multitexture(struct Renderer *self)
{
    struct RenderQueue *l = &self->queues[RENDER_MULTITEXTURE];
    if (!l->count)
        return;
    queue_sort(l, multitexture_less);
    gfx_set_mode(DRAW_MULTITEXTURE);
    for (size_t i = 0; i < l->count; ++i) {
        set_tex0(self, l->items[i].textures[0].surface);
        set_tex1(self, l->items[i].textures[1].surface);
        draw_quad(l->items[i].primitive);
    }
    gfx_set_texture(1, NULL);
    self->boundTextures[1] = NULL;
}

struct Renderer *g_renderer;
DWORD g_fogColor;
float g_fogNear;
float g_fogFar;

void renderer_enable_fog(struct Renderer *self, int enable)
{
    self->fogEnable = enable;
}

void renderer_set_world(struct Renderer *self, const struct GfxWorldDraw *models, const struct GfxWorldDraw *ground)
{
    self->worldModels = *models;
    self->worldGround = *ground;
    self->worldSet = 1;
}

void renderer_set_light(struct Renderer *self, const float *dir, const float *ambient, float shadowOpacity)
{
    float darkness = 1.0f - (shadowOpacity == shadowOpacity ? shadowOpacity : 0.5f);

    memcpy(self->lightDir, dir, sizeof self->lightDir);
    memcpy(self->lightAmbient, ambient, sizeof self->lightAmbient);
    self->shadowDarkness = darkness < 0.0f ? 0.0f : darkness > 1.0f ? 1.0f : darkness;
    self->lightSet = 1;
}

void rp_quad_draw(struct RPQuad *self)
{
    draw_quad(self);
}

/* The lists in the order and the modes (render/draw_modes.h) the Direct3D 9 client drew them in,
   with the frame's fog: g_fogNear and g_fogFar are shares of the view depths from 10 to 1,500. */
static void renderer_draw_lists(struct Renderer *self)
{
    struct GfxFog fog;
    if (self->cameraSet) {
        struct GfxWorldView camera;
        renderer_world_view(self, self->cameraView, &camera);
        gfx_set_world_camera(&camera);
        self->cameraSet = 0;
    }
    fog.on = self->fogEnable;
    fog.colour = g_fogColor;
    fog.start = g_fogNear;
    fog.end = g_fogFar;
    fog.origin = 10.0f;
    fog.span = 1490.0f;
    gfx_set_fog(&fog);
    TIMED(PERF_L_GROUND, flush_opaque_lists(self));
    perf_begin(PERF_L_WATER);
    /* custom: the ground of the static world, without its lightmaps the texture times the colour,
       as the original's ground quads without them */
    gfx_set_mode(g_lightmap ? DRAW_GROUND_LIGHTMAP : DRAW_GROUND);
    if (self->worldSet)
        gfx_world_draw(SW_PASS_GROUND, &self->worldGround);
    perf_end(PERF_L_WATER);
    TIMED(PERF_L_MULTITEXTURE, flush_multitexture(self));
    TIMED(PERF_L_BLENDED_NO_DEPTH,
          flush_effects(self, RENDER_BLENDED_NO_DEPTH, DRAW_EFFECT_DEPTH, DRAW_BLEND_ONE, DRAW_BLEND_ONE, 0));
    gfx_set_mode(DRAW_ALPHA_NO_DEPTH);
    TIMED(PERF_L_ALPHA_NO_DEPTH, flush_keyed(self, RENDER_ALPHA_NO_DEPTH, 1, 0));
    gfx_set_mode(DRAW_ALPHA_DEPTH);
    TIMED(PERF_L_ALPHA_DEPTH, flush_keyed(self, RENDER_ALPHA_DEPTH, 1, 0));
    gfx_set_mode(DRAW_ADDITIVE_FOG);
    TIMED(PERF_L_ADDITIVE_FOG, flush_keyed(self, RENDER_ADDITIVE_FOG, 1, 0));
    perf_begin(PERF_L_ALPHATEST);
    if (self->twoPassAlpha == 1) {
        flush_alpha_keyed_lists(self, DRAW_SOLID_CLAMP, DRAW_SOLID_WRAP);
        flush_alpha_keyed_lists(self, DRAW_FRINGE_CLAMP, DRAW_FRINGE_WRAP);
        gfx_set_mode(DRAW_ALPHA_CLAMP);
        flush_keyed(self, RENDER_ALPHA_REMAINDER, 1, 0);
    } else {
        flush_alpha_keyed_lists(self, DRAW_ALPHA_CLAMP, DRAW_ALPHA_WRAP);
    }
    perf_end(PERF_L_ALPHATEST);
    gfx_set_mode(DRAW_ALPHA_DEPTH);
    TIMED(PERF_L_ALPHA_WRITE_DEPTH, flush_keyed(self, RENDER_ALPHA_WRITE_DEPTH, 1, 0));
    gfx_set_mode(DRAW_ALPHA_CUTOUT);
    TIMED(PERF_L_ALPHA_CUTOUT, flush_keyed(self, RENDER_ALPHA_CUTOUT, 1, 0));
    gfx_set_mode(DRAW_ADDITIVE);
    TIMED(PERF_L_ADDITIVE, flush_keyed(self, RENDER_ADDITIVE, 1, 0));
    TIMED(PERF_L_BLENDED_WRAP,
          flush_effects(self, RENDER_BLENDED_WRAP, DRAW_EFFECT_WRAP, DRAW_BLEND_ONE, DRAW_BLEND_ONE, 0));
    gfx_set_mode(DRAW_ADDITIVE_OVERLAY);
    TIMED(PERF_L_ADDITIVE_OVERLAY, flush_keyed(self, RENDER_ADDITIVE_OVERLAY, 1, 0));
    gfx_set_mode(DRAW_OVERLAY);
    TIMED(PERF_L_OVERLAY_ORDERED, flush_keyed(self, RENDER_OVERLAY_ORDERED, 0, 0));
    TIMED(PERF_L_BLENDED_OVERLAY, flush_effects(self, RENDER_BLENDED_OVERLAY, DRAW_EFFECT_OVERLAY,
                                                DRAW_BLEND_SRCALPHA, DRAW_BLEND_INVSRCALPHA, 1));
    gfx_shadow_receive(0);
    self->worldSet = 0;
}

int renderer_draw_scene(struct Renderer *self)
{
    /* custom: timed for the counters (diag/perf.h) */
    int64_t start = platform_stopwatch_ns();
    if (gfx_begin_scene() < 0) {
        renderer_destroy_all_rp_list(self);
        g_perf.drawSceneNs += platform_stopwatch_ns() - start;
        return 0;
    }
    g_renderCheckpointLine = 0x341;
    g_renderCheckpointFile = renderer_source_file;
    TIMED(PERF_SCENE, renderer_draw_lists(self));
    g_renderCheckpointLine = 0x343;
    g_renderCheckpointFile = renderer_source_file;
    TIMED(PERF_DESTROY, renderer_destroy_all_rp_list(self));
    perf_overlay_draw(self);
    g_renderCheckpointLine = 0x344;
    g_renderCheckpointFile = renderer_source_file;
    TIMED(PERF_ENDSCENE, gfx_end_scene());
    g_renderCheckpointLine = 0x346;
    g_renderCheckpointFile = renderer_source_file;
    g_perf.drawSceneNs += platform_stopwatch_ns() - start;
    return 1;
}

void renderer_flip(struct Renderer *self, char vsync)
{
    self->frame++;
    if ((float)(unsigned)(platform_coarse_ticks_ms() - self->fpsTick) >= 1000.0f) {
        self->fpsTick = platform_coarse_ticks_ms();
        self->fps = self->fpsCount;
        self->fpsCount = 0;
    } else {
        self->fpsCount++;
    }
    TIMED(PERF_PRESENT, c3d_device_show_frame(&g_3dDevice, vsync));
    perf_frame_end(self);
}
