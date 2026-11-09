#include "render/device.h"
#include "render/render_api.h"
#include "diag/perf.h"
#include "diag/diag.h"
#include "std/std_sort.h"
#include <stdint.h>
#include <string.h>
#include "render/render_options.h"
#include "platform/clock.h"

DWORD g_renderCheckpointLine = 0;
const char *g_renderCheckpointFile = "";

static const char renderer_source_file[] = "c:\\repositories\\client\\kr\\kr.ro1.live\\source\\ragnarokclient\\3dlib\\renderer.cpp";

static void renderer_do_fog_(struct Renderer *self);
static void flush_alpha_keyed_lists(struct Renderer *self);
static void flush_blended_wrap(struct Renderer *self);
static void flush_opaque_lists(struct Renderer *self);
static void flush_blended_overlay(struct Renderer *self);
static void flush_multitexture(struct Renderer *self);
static void flush_blended_no_depth(struct Renderer *self);
static void renderer_draw_lists(struct Renderer *self);

#define HRS(self, state, value) gfx_set_render_state((state), (value))
#define TIMED(phase, call) do { perf_begin(phase); call; perf_end(phase); } while (0)
#define HTSS(self, stage, state, value) gfx_set_stage_state((stage), (state), (value))

static DWORD get_rs(struct Renderer *self, int state)
{
    DWORD v = 0;
    (void)self;
    gfx_get_render_state(state, &v);
    return v;
}

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

static void draw_quad(const struct RPQuad *n)
{
    if (!n->nverts)
        return;
    gfx_draw(n->prim, n->world ? GFX_VTX_WORLD : GFX_VTX_TEX1, n->verts, n->nverts,
             n->nindices ? (const WORD *)n->indices : NULL, n->nindices);
}

static void draw_twotex(const struct RPTwoTex *n)
{
    gfx_draw(n->prim, GFX_VTX_TEX2, n->verts, n->nverts, (const WORD *)n->indices, n->nindices);
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

static unsigned char two_tex_less(const void *a, const void *b, void *pred)
{
    const struct RenderEntry *x = a, *y = b;
    (void)pred;
    if (x->textures[0].sort_key != y->textures[0].sort_key)
        return x->textures[0].sort_key < y->textures[0].sort_key;
    return x->textures[1].sort_key < y->textures[1].sort_key;
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

/* custom: a mesh draw among a queue's entries (render/gfx.h): the world view of its view where that
   differs from the last one set this frame, then for its colour its cull mode, put back after, and
   its textures, or as a shadow caster (the shadow pass culls nothing) its first texture */
static void draw_mesh_entry(struct Renderer *self, const struct RenderEntry *e, int caster)
{
    const struct RPMeshDraw *n = e->primitive;
    DWORD cull = 0;

    if (!self->meshViewSet || memcmp(self->meshView, n->view, sizeof n->view)) {
        struct GfxWorldView world;
        renderer_world_view(self, n->view, &world);
        gfx_set_world_view(&world);
        memcpy(self->meshView, n->view, sizeof self->meshView);
        self->meshViewSet = 1;
    }
    if (n->cull && !caster) {
        cull = get_rs(self, GFX_RS_CULLMODE);
        HRS(self, GFX_RS_CULLMODE, (DWORD)n->cull);
    }
    set_tex0(self, e->textures[0].surface);
    if (!caster)
        set_tex1(self, e->textures[1].surface);
    gfx_draw_mesh(n->mesh, &n->draw);
    if (n->cull && !caster)
        HRS(self, GFX_RS_CULLMODE, cull);
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
        if (e->mesh) {
            draw_mesh_entry(self, e, 0);
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
        if (l->items[i].mesh) {
            draw_mesh_entry(self, &l->items[i], 0);
            continue;
        }
        if (skipNull && !n)
            continue;
        set_tex0(self, l->items[i].textures[0].surface);
        draw_quad(n);
    }
}

static int blended_mode = -1;

static void set_blended_mode(struct Renderer *self, int mode)
{
    if (mode != blended_mode) {
        renderer_set_blend_stages(self, mode);
        blended_mode = mode;
    }
}

static void flush_blended(struct Renderer *self, const struct RenderEntry *entry)
{
    const struct RPQuad *n = entry->primitive;
    set_tex0(self, entry->textures[0].surface);
    set_blended_mode(self, n->stageMode);
    HRS(self, GFX_RS_SRCBLEND, n->srcBlend);
    HRS(self, GFX_RS_DESTBLEND, n->destBlend);
    draw_quad(n);
}

/* custom: the ground's meshes (render/gfx.h), drawn with the states the original drew its
   two-texture ground quads with and sorted by texture and lightmap as those were */
static void flush_ground_meshes(struct Renderer *self)
{
    struct RenderQueue *l = &self->queues[RENDER_GROUND_MESH];
    queue_sort(l, two_tex_less);
    for (size_t i = 0; i < l->count; ++i)
        draw_mesh_entry(self, &l->items[i], 0);
}

static void flush_alpha_keyed_lists(struct Renderer *self)
{
    HTSS(self, 0, GFX_TSS_ADDRESS, GFX_ADDRESS_CLAMP);
    flush_keyed(self, RENDER_ALPHA_CLAMP, 1, 0);
    HTSS(self, 0, GFX_TSS_ADDRESS, GFX_ADDRESS_WRAP);
    flush_keyed(self, RENDER_ALPHA_WRAP, 1, 0);
    HTSS(self, 0, GFX_TSS_ADDRESS, GFX_ADDRESS_CLAMP);
}

static void flush_blended_wrap(struct Renderer *self)
{
    struct RenderQueue *l = &self->queues[RENDER_BLENDED_WRAP];
    HTSS(self, 0, GFX_TSS_ADDRESS, GFX_ADDRESS_WRAP);
    blended_mode = -1;
    for (size_t i = 0; i < l->count; ++i) {
        if (l->items[i].primitive)
            flush_blended(self, &l->items[i]);
    }
    renderer_set_blend_stages(self, 0);
    HTSS(self, 0, GFX_TSS_ADDRESS, GFX_ADDRESS_CLAMP);
}

/* custom: draws a queue's meshes into the shadow map cascade chosen (render/gfx.h), with their
   first textures bound, which tests them for alpha, where they have one */
static void draw_shadow_casters(struct Renderer *self, int list)
{
    struct RenderQueue *l = &self->queues[list];

    for (size_t i = 0; i < l->count; ++i)
        draw_mesh_entry(self, &l->items[i], 1);
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

static void flush_opaque_lists(struct Renderer *self)
{
    draw_shadow_maps(self);
    gfx_shadow_receive(self->shadowDrawn);
    HTSS(self, 0, GFX_TSS_ADDRESS, GFX_ADDRESS_CLAMP);
    flush_plain(self, RENDER_OPAQUE_CLAMP, 1, 1);
    HTSS(self, 0, GFX_TSS_ADDRESS, GFX_ADDRESS_WRAP);
    flush_plain(self, RENDER_OPAQUE_WRAP, 1, 1);
    HTSS(self, 0, GFX_TSS_ADDRESS, GFX_ADDRESS_CLAMP);
}

static void flush_blended_overlay(struct Renderer *self)
{
    struct RenderQueue *l = &self->queues[RENDER_BLENDED_OVERLAY];
    struct RenderEntry *e;
    queue_sort(l, keyed_less);
    blended_mode = -1;
    for (size_t i = 0; i < l->count; ++i) {
        e = l->items + i;
        struct RPQuad *n = e->primitive;
        if (!n)
            continue;
        set_tex0(self, e->textures[0].surface);
        set_blended_mode(self, n->stageMode);
        if (e->key >= 0.99f) {
            HRS(self, GFX_RS_SRCBLEND, GFX_BLEND_SRCALPHA);
            HRS(self, GFX_RS_DESTBLEND, GFX_BLEND_INVSRCALPHA);
        } else {
            HRS(self, GFX_RS_SRCBLEND, n->srcBlend);
            HRS(self, GFX_RS_DESTBLEND, n->destBlend);
        }
        draw_quad(n);
    }
    renderer_set_blend_stages(self, 0);
}

static void flush_multitexture(struct Renderer *self)
{
    struct RenderQueue *l = &self->queues[RENDER_MULTITEXTURE];
    if (!l->count)
        return;
    queue_sort(l, multitexture_less);
    HTSS(self, 0, GFX_TSS_COLOROP, GFX_TOP_SELECTARG1);
    HTSS(self, 0, GFX_TSS_COLORARG1, GFX_TA_TEXTURE);
    HTSS(self, 0, GFX_TSS_ALPHAOP, GFX_TOP_SELECTARG1);
    HTSS(self, 0, GFX_TSS_ALPHAARG1, GFX_TA_TEXTURE);
    HTSS(self, 1, GFX_TSS_COLOROP, GFX_TOP_MODULATE);
    HTSS(self, 1, GFX_TSS_COLORARG1, GFX_TA_TEXTURE);
    HTSS(self, 1, GFX_TSS_COLORARG2, GFX_TA_CURRENT);
    HTSS(self, 1, GFX_TSS_ALPHAOP, GFX_TOP_MODULATE);
    HTSS(self, 1, GFX_TSS_ALPHAARG1, GFX_TA_TEXTURE);
    HTSS(self, 1, GFX_TSS_ALPHAARG2, GFX_TA_CURRENT);
    HTSS(self, 2, GFX_TSS_COLOROP, GFX_TOP_DISABLE);
    HTSS(self, 2, GFX_TSS_ALPHAOP, GFX_TOP_DISABLE);
    HTSS(self, 0, GFX_TSS_ADDRESS, GFX_ADDRESS_WRAP);
    HTSS(self, 1, GFX_TSS_ADDRESS, GFX_ADDRESS_CLAMP);
    for (size_t i = 0; i < l->count; ++i) {
        struct RPTwoTex *n = l->items[i].primitive;
        if (n->nverts <= 0)
            continue;
        set_tex0(self, l->items[i].textures[0].surface);
        set_tex1(self, l->items[i].textures[1].surface);
        if (n->nverts > 0)
            draw_twotex(n);
    }
    HTSS(self, 0, GFX_TSS_COLOROP, GFX_TOP_MODULATE);
    HTSS(self, 0, GFX_TSS_COLORARG1, GFX_TA_TEXTURE);
    HTSS(self, 0, GFX_TSS_ALPHAOP, GFX_TOP_MODULATE);
    HTSS(self, 0, GFX_TSS_ALPHAARG1, GFX_TA_TEXTURE);
    HTSS(self, 1, GFX_TSS_COLOROP, GFX_TOP_DISABLE);
    HTSS(self, 1, GFX_TSS_ALPHAOP, GFX_TOP_DISABLE);
    HTSS(self, 0, GFX_TSS_ADDRESS, GFX_ADDRESS_CLAMP);
    HTSS(self, 1, GFX_TSS_ADDRESS, GFX_ADDRESS_BORDER);
    gfx_set_texture(1, NULL);
    self->boundTextures[1] = NULL;
}

static void flush_blended_no_depth(struct Renderer *self)
{
    struct RenderQueue *l = &self->queues[RENDER_BLENDED_NO_DEPTH];
    blended_mode = -1;
    for (size_t i = 0; i < l->count; ++i) {
        if (l->items[i].primitive)
            flush_blended(self, &l->items[i]);
    }
    renderer_set_blend_stages(self, 0);
}

struct Renderer *g_renderer;
DWORD g_fogVertexMode;
DWORD g_fogColor;
DWORD g_fogUseRange;
float g_fogDensity;
float g_fogNear;
float g_fogFar;

static void renderer_do_fog_(struct Renderer *self)
{
    struct GfxMatrix identity, proj;
    DWORD caps;
    float fogStart, fogEnd;
    if (!self->fogDirty)
        return;
    if (!self->fogEnable) {
        self->fogDirty = 0;
        return;
    }
    fogStart = g_fogNear * 1490.0f + 10.0f;
    fogEnd = g_fogFar * 1490.0f + 10.0f;
    memset(&identity, 0, sizeof(identity));
    identity.m[0][0] = identity.m[1][1] = identity.m[2][2] = identity.m[3][3] = 1.0f;
    mat4_perspective_lh(proj.m[0], 0.26179939f, self->aspect, 10.0f, 1500.0f);
    gfx_set_transform(GFX_TS_WORLD, &identity);
    gfx_set_transform(GFX_TS_PROJECTION, &proj);
    gfx_set_transform(GFX_TS_VIEW, &identity);
    HRS(self, GFX_RS_FOGCOLOR, g_fogColor);
    caps = g_gfxCaps.vertexFog;
    if (g_fogVertexMode == 3) {
        if (!caps) {
            HRS(self, GFX_RS_FOGENABLE, FALSE);
            self->fogDirty = 0;
            return;
        }
        HRS(self, GFX_RS_FOGSTART, *(DWORD *)&fogStart);
        HRS(self, GFX_RS_FOGEND, *(DWORD *)&fogEnd);
        HRS(self, GFX_RS_FOGDENSITY, *(DWORD *)&g_fogDensity);
        HRS(self, GFX_RS_FOGTABLEMODE, GFX_FOG_NONE);
        HRS(self, GFX_RS_FOGVERTEXMODE, g_fogVertexMode);
    } else {
        if (caps) {
            HRS(self, GFX_RS_FOGTABLEMODE, GFX_FOG_NONE);
            HRS(self, GFX_RS_FOGVERTEXMODE, g_fogVertexMode);
        } else {
            error_msg("Your graphic accelerator does not support fog effect.", 0);
        }
        HRS(self, GFX_RS_FOGDENSITY, *(DWORD *)&g_fogDensity);
    }
    if (caps && g_fogUseRange)
        HRS(self, GFX_RS_RANGEFOGENABLE, 1);
    self->fogDirty = 0;
}

void renderer_enable_fog(struct Renderer *self, int enable)
{
    self->fogVertexCaps = g_gfxCaps.vertexFog;
    self->fogEnable = enable;
    HRS(self, GFX_RS_FOGENABLE, enable);
    self->fogDirty = 1;
    g_fogUseRange = 1;
    g_fogVertexMode = 3;
}

void renderer_set_light(struct Renderer *self, const float *dir, const float *diffuse, const float *ambient,
                        float shadowOpacity)
{
    float darkness = 1.0f - (shadowOpacity == shadowOpacity ? shadowOpacity : 0.5f);

    memcpy(self->lightDir, dir, sizeof self->lightDir);
    memcpy(self->lightAmbient, ambient, sizeof self->lightAmbient);
    self->shadowDarkness = darkness < 0.0f ? 0.0f : darkness > 1.0f ? 1.0f : darkness;
    self->lightSet = 1;
    gfx_set_light(dir, diffuse, ambient);
}

void renderer_set_blend_stages(struct Renderer *self, int mode)
{
    #define STAGE1_TAIL(op) do { \
        HTSS(self, 1, GFX_TSS_COLOROP, (op)); \
        HTSS(self, 1, GFX_TSS_COLORARG2, GFX_TA_CURRENT); \
        HTSS(self, 1, GFX_TSS_ALPHAARG1, GFX_TA_TEXTURE); \
        HTSS(self, 1, GFX_TSS_ALPHAOP, GFX_TOP_SELECTARG1); } while (0)
    #define STAGE1_OFF() do { \
        HTSS(self, 1, GFX_TSS_COLOROP, GFX_TOP_DISABLE); \
        HTSS(self, 1, GFX_TSS_ALPHAOP, GFX_TOP_DISABLE); } while (0)

    switch (mode) {
    case 0:
        HTSS(self, 0, GFX_TSS_TEXCOORDINDEX, 0);
        HTSS(self, 0, GFX_TSS_COLOROP, GFX_TOP_MODULATE);
        HTSS(self, 0, GFX_TSS_COLORARG1, GFX_TA_TEXTURE);
        HTSS(self, 0, GFX_TSS_COLORARG2, GFX_TA_DIFFUSE);
        HTSS(self, 0, GFX_TSS_ALPHAOP, GFX_TOP_MODULATE);
        HTSS(self, 0, GFX_TSS_ALPHAARG1, GFX_TA_TEXTURE);
        HTSS(self, 0, GFX_TSS_ALPHAARG2, GFX_TA_DIFFUSE);
        STAGE1_OFF();
        break;
    case 1:
    case 2:
        HTSS(self, 0, GFX_TSS_TEXCOORDINDEX, 0);
        HTSS(self, 0, GFX_TSS_COLORARG1, GFX_TA_TEXTURE);
        HTSS(self, 0, GFX_TSS_COLOROP, GFX_TOP_MODULATE);
        HTSS(self, 0, GFX_TSS_COLORARG2, GFX_TA_DIFFUSE);
        HTSS(self, 0, GFX_TSS_ALPHAARG1, GFX_TA_TEXTURE);
        HTSS(self, 0, GFX_TSS_ALPHAOP, GFX_TOP_SELECTARG1);
        STAGE1_OFF();
        break;
    case 3:
    case 4:
        HTSS(self, 0, GFX_TSS_TEXCOORDINDEX, 0);
        HTSS(self, 0, GFX_TSS_COLORARG1, GFX_TA_TEXTURE);
        HTSS(self, 0, GFX_TSS_COLOROP, mode == 3 ? GFX_TOP_ADD : GFX_TOP_BLENDTEXTUREALPHA);
        HTSS(self, 0, GFX_TSS_COLORARG2, GFX_TA_DIFFUSE);
        HTSS(self, 0, GFX_TSS_ALPHAOP, GFX_TOP_SELECTARG2);
        HTSS(self, 0, GFX_TSS_ALPHAARG2, GFX_TA_DIFFUSE);
        STAGE1_OFF();
        break;
    case 5:
        HTSS(self, 0, GFX_TSS_TEXCOORDINDEX, 0);
        HTSS(self, 0, GFX_TSS_COLORARG1, GFX_TA_TEXTURE);
        HTSS(self, 0, GFX_TSS_COLOROP, GFX_TOP_MODULATE);
        HTSS(self, 0, GFX_TSS_COLORARG2, GFX_TA_DIFFUSE);
        HTSS(self, 0, GFX_TSS_ALPHAOP, GFX_TOP_SELECTARG2);
        HTSS(self, 0, GFX_TSS_ALPHAARG2, GFX_TA_DIFFUSE);
        HTSS(self, 1, GFX_TSS_TEXCOORDINDEX, 1);
        HTSS(self, 1, GFX_TSS_COLORARG1, GFX_TA_TEXTURE);
        STAGE1_TAIL(GFX_TOP_MODULATE);
        break;
    case 6:
        HTSS(self, 0, GFX_TSS_TEXCOORDINDEX, 0);
        HTSS(self, 0, GFX_TSS_COLORARG1, GFX_TA_TEXTURE | GFX_TA_COMPLEMENT);
        HTSS(self, 0, GFX_TSS_COLOROP, GFX_TOP_MODULATE);
        HTSS(self, 0, GFX_TSS_COLORARG2, GFX_TA_DIFFUSE);
        HTSS(self, 0, GFX_TSS_ALPHAOP, GFX_TOP_SELECTARG2);
        HTSS(self, 0, GFX_TSS_ALPHAARG2, GFX_TA_DIFFUSE);
        HTSS(self, 1, GFX_TSS_TEXCOORDINDEX, 1);
        HTSS(self, 1, GFX_TSS_COLORARG1, GFX_TA_TEXTURE | GFX_TA_COMPLEMENT);
        STAGE1_TAIL(GFX_TOP_MODULATE);
        break;
    case 7:
        HTSS(self, 0, GFX_TSS_TEXCOORDINDEX, 0);
        HTSS(self, 0, GFX_TSS_COLORARG1, GFX_TA_TEXTURE);
        HTSS(self, 0, GFX_TSS_COLOROP, GFX_TOP_SELECTARG1);
        HTSS(self, 0, GFX_TSS_ALPHAARG1, GFX_TA_TEXTURE);
        HTSS(self, 0, GFX_TSS_ALPHAOP, GFX_TOP_SELECTARG1);
        HTSS(self, 1, GFX_TSS_TEXCOORDINDEX, 1);
        HTSS(self, 1, GFX_TSS_COLORARG1, GFX_TA_TEXTURE | GFX_TA_ALPHAREPLICATE);
        STAGE1_TAIL(GFX_TOP_MODULATE);
        break;
    case 8:
        HTSS(self, 0, GFX_TSS_TEXCOORDINDEX, 0);
        HTSS(self, 0, GFX_TSS_COLORARG1, GFX_TA_TEXTURE);
        HTSS(self, 0, GFX_TSS_COLOROP, GFX_TOP_MODULATE);
        HTSS(self, 0, GFX_TSS_COLORARG2, GFX_TA_DIFFUSE);
        HTSS(self, 0, GFX_TSS_ALPHAARG1, GFX_TA_TEXTURE);
        HTSS(self, 0, GFX_TSS_ALPHAOP, GFX_TOP_SELECTARG1);
        HTSS(self, 1, GFX_TSS_TEXCOORDINDEX, 1);
        HTSS(self, 1, GFX_TSS_COLORARG1, GFX_TA_TEXTURE);
        STAGE1_TAIL(GFX_TOP_ADD);
        break;
    case 9:
    case 10:
    case 14:
    case 15:
        HTSS(self, 0, GFX_TSS_TEXCOORDINDEX, 0);
        HTSS(self, 0, GFX_TSS_COLORARG1, GFX_TA_TEXTURE);
        HTSS(self, 0, GFX_TSS_COLOROP, GFX_TOP_MODULATE);
        HTSS(self, 0, GFX_TSS_COLORARG2, GFX_TA_DIFFUSE);
        HTSS(self, 0, GFX_TSS_ALPHAOP, GFX_TOP_SELECTARG2);
        HTSS(self, 0, GFX_TSS_ALPHAARG2, GFX_TA_DIFFUSE);
        HTSS(self, 1, GFX_TSS_TEXCOORDINDEX, 1);
        HTSS(self, 1, GFX_TSS_COLORARG1, GFX_TA_TEXTURE);
        STAGE1_TAIL(mode == 9 ? GFX_TOP_BLENDTEXTUREALPHA : mode == 10 ? GFX_TOP_BLENDDIFFUSEALPHA
                    : mode == 14 ? GFX_TOP_MODULATE2X : GFX_TOP_ADDSIGNED);
        break;
    case 11:
        HTSS(self, 0, GFX_TSS_TEXCOORDINDEX, 0);
        HTSS(self, 0, GFX_TSS_COLORARG1, GFX_TA_TEXTURE);
        HTSS(self, 0, GFX_TSS_COLOROP, GFX_TOP_MODULATE);
        HTSS(self, 0, GFX_TSS_COLORARG2, GFX_TA_DIFFUSE);
        HTSS(self, 0, GFX_TSS_ALPHAOP, GFX_TOP_SELECTARG2);
        HTSS(self, 0, GFX_TSS_ALPHAARG2, GFX_TA_DIFFUSE);
        HTSS(self, 1, GFX_TSS_TEXCOORDINDEX, 1);
        HTSS(self, 1, GFX_TSS_COLORARG1, GFX_TA_TEXTURE);
        STAGE1_TAIL(GFX_TOP_ADD);
        break;
    case 12:
        HTSS(self, 0, GFX_TSS_TEXCOORDINDEX, 0);
        HTSS(self, 0, GFX_TSS_COLORARG1, GFX_TA_TEXTURE);
        HTSS(self, 0, GFX_TSS_COLOROP, GFX_TOP_MODULATE);
        HTSS(self, 0, GFX_TSS_COLORARG2, GFX_TA_DIFFUSE);
        HTSS(self, 0, GFX_TSS_ALPHAOP, GFX_TOP_SELECTARG2);
        HTSS(self, 0, GFX_TSS_ALPHAARG2, GFX_TA_DIFFUSE);
        HTSS(self, 1, GFX_TSS_TEXCOORDINDEX, 1);
        HTSS(self, 1, GFX_TSS_COLORARG1, GFX_TA_TEXTURE | GFX_TA_COMPLEMENT);
        STAGE1_TAIL(GFX_TOP_ADD);
        break;
    case 13:
        HTSS(self, 0, GFX_TSS_TEXCOORDINDEX, 0);
        HTSS(self, 0, GFX_TSS_COLORARG1, GFX_TA_TEXTURE);
        HTSS(self, 0, GFX_TSS_COLOROP, GFX_TOP_ADD);
        HTSS(self, 0, GFX_TSS_COLORARG2, GFX_TA_DIFFUSE);
        HTSS(self, 0, GFX_TSS_ALPHAARG1, GFX_TA_TEXTURE);
        HTSS(self, 0, GFX_TSS_ALPHAOP, GFX_TOP_SELECTARG1);
        HTSS(self, 1, GFX_TSS_TEXCOORDINDEX, 1);
        HTSS(self, 1, GFX_TSS_COLORARG1, GFX_TA_TEXTURE);
        STAGE1_TAIL(GFX_TOP_MODULATE);
        break;
    default:
        break;
    }
    #undef STAGE1_TAIL
    #undef STAGE1_OFF
}

void rp_quad_draw(struct RPQuad *self)
{
    draw_quad(self);
}

static void renderer_draw_lists(struct Renderer *self)
{
    DWORD fog, zwrite, alpha;
    self->meshViewSet = 0;
    if (self->cameraSet) {
        struct GfxWorldView camera;
        renderer_world_view(self, self->cameraView, &camera);
        gfx_set_world_camera(&camera);
        self->cameraSet = 0;
    }
    HTSS(self, 0, GFX_TSS_MIPFILTER, GFX_MIPFILTER_LINEAR);
    renderer_do_fog_(self);
    HTSS(self, 0, GFX_TSS_COLOROP, GFX_TOP_MODULATE);
    HTSS(self, 0, GFX_TSS_ALPHAOP, GFX_TOP_MODULATE);
    HTSS(self, 1, GFX_TSS_COLOROP, GFX_TOP_DISABLE);
    HTSS(self, 1, GFX_TSS_ALPHAOP, GFX_TOP_DISABLE);
    HRS(self, GFX_RS_FOGENABLE, self->fogEnable);
    HRS(self, GFX_RS_ALPHAREF, 0xcf);
    HRS(self, GFX_RS_ALPHATESTENABLE, TRUE);
    HRS(self, GFX_RS_ALPHABLENDENABLE, FALSE);
    HRS(self, GFX_RS_ZENABLE, TRUE);
    HRS(self, GFX_RS_ZWRITEENABLE, TRUE);
    TIMED(PERF_L_GROUND, flush_opaque_lists(self));
    HRS(self, GFX_RS_AMBIENT, 0xffffffff);
    HRS(self, GFX_RS_CULLMODE, GFX_CULL_CW);
    perf_begin(PERF_L_WATER);
    /* custom: the ground (world/ground_draw.c); without lightmaps stage 1 stays off, and the
       ground is its texture times its colour, as the original's ground quads without them */
    HRS(self, GFX_RS_COLORKEYENABLE, FALSE);
    if (g_lightmap)
        HTSS(self, 1, GFX_TSS_COLOROP, GFX_TOP_MODULATEALPHA_ADDCOLOR);
    flush_ground_meshes(self);
    HRS(self, GFX_RS_COLORKEYENABLE, TRUE);
    HTSS(self, 1, GFX_TSS_COLOROP, GFX_TOP_DISABLE);
    perf_end(PERF_L_WATER);
    HRS(self, GFX_RS_COLORKEYENABLE, FALSE);
    HRS(self, GFX_RS_AMBIENT, 0);
    HRS(self, GFX_RS_CULLMODE, GFX_CULL_NONE);
    HRS(self, GFX_RS_ALPHAREF, 1);
    HRS(self, GFX_RS_ALPHABLENDENABLE, TRUE);
    TIMED(PERF_L_MULTITEXTURE, flush_multitexture(self));
    fog = get_rs(self, GFX_RS_FOGENABLE);
    zwrite = get_rs(self, GFX_RS_ZWRITEENABLE);
    HRS(self, GFX_RS_FOGENABLE, FALSE);
    HRS(self, GFX_RS_ZWRITEENABLE, FALSE);
    HRS(self, GFX_RS_SRCBLEND, GFX_BLEND_ONE);
    HRS(self, GFX_RS_DESTBLEND, GFX_BLEND_ONE);
    TIMED(PERF_L_BLENDED_NO_DEPTH, flush_blended_no_depth(self));
    HRS(self, GFX_RS_FOGENABLE, fog);
    HRS(self, GFX_RS_ZWRITEENABLE, zwrite);
    HRS(self, GFX_RS_SRCBLEND, GFX_BLEND_SRCALPHA);
    HRS(self, GFX_RS_DESTBLEND, GFX_BLEND_INVSRCALPHA);
    HRS(self, GFX_RS_ZENABLE, FALSE);
    TIMED(PERF_L_ALPHA_NO_DEPTH, flush_keyed(self, RENDER_ALPHA_NO_DEPTH, 1, 0));
    HRS(self, GFX_RS_ZENABLE, TRUE);
    TIMED(PERF_L_ALPHA_DEPTH, flush_keyed(self, RENDER_ALPHA_DEPTH, 1, 0));
    HRS(self, GFX_RS_SRCBLEND, GFX_BLEND_ONE);
    HRS(self, GFX_RS_DESTBLEND, GFX_BLEND_ONE);
    TIMED(PERF_L_ADDITIVE_FOG, flush_keyed(self, RENDER_ADDITIVE_FOG, 1, 0));
    HRS(self, GFX_RS_ZWRITEENABLE, FALSE);
    HRS(self, GFX_RS_SRCBLEND, GFX_BLEND_SRCALPHA);
    HRS(self, GFX_RS_DESTBLEND, GFX_BLEND_INVSRCALPHA);
    perf_begin(PERF_L_ALPHATEST);
    if (self->twoPassAlpha == 1) {
        DWORD saved_zwrite_enable = get_rs(self, GFX_RS_ZWRITEENABLE);
        DWORD saved_alpha_test_enable = get_rs(self, GFX_RS_ALPHATESTENABLE);
        DWORD saved_alpha_ref = get_rs(self, GFX_RS_ALPHAREF);
        DWORD saved_alpha_func = get_rs(self, GFX_RS_ALPHAFUNC);
        HRS(self, GFX_RS_ALPHATESTENABLE, TRUE);
        HRS(self, GFX_RS_ZWRITEENABLE, TRUE);
        HRS(self, GFX_RS_ALPHAREF, 0xf0);
        HRS(self, GFX_RS_ALPHAFUNC, GFX_CMP_GREATEREQUAL);
        flush_alpha_keyed_lists(self);
        HRS(self, GFX_RS_ZWRITEENABLE, FALSE);
        HRS(self, GFX_RS_ALPHAREF, 0xf0);
        HRS(self, GFX_RS_ALPHAFUNC, GFX_CMP_LESS);
        flush_alpha_keyed_lists(self);
        HRS(self, GFX_RS_ALPHAREF, 1);
        HRS(self, GFX_RS_ALPHAFUNC, GFX_CMP_GREATEREQUAL);
        flush_keyed(self, RENDER_ALPHA_REMAINDER, 1, 0);
        HRS(self, GFX_RS_ZWRITEENABLE, saved_zwrite_enable);
        HRS(self, GFX_RS_ALPHATESTENABLE, saved_alpha_test_enable);
        HRS(self, GFX_RS_ALPHAREF, saved_alpha_ref);
        HRS(self, GFX_RS_ALPHAFUNC, saved_alpha_func);
    } else {
        flush_alpha_keyed_lists(self);
    }
    perf_end(PERF_L_ALPHATEST);
    zwrite = get_rs(self, GFX_RS_ZWRITEENABLE);
    HRS(self, GFX_RS_ZWRITEENABLE, TRUE);
    TIMED(PERF_L_ALPHA_WRITE_DEPTH, flush_keyed(self, RENDER_ALPHA_WRITE_DEPTH, 1, 0));
    HRS(self, GFX_RS_ZWRITEENABLE, zwrite);
    alpha = get_rs(self, GFX_RS_ALPHAREF);
    HRS(self, GFX_RS_ALPHAREF, 0xcf);
    TIMED(PERF_L_ALPHA_CUTOUT, flush_keyed(self, RENDER_ALPHA_CUTOUT, 1, 0));
    HRS(self, GFX_RS_ALPHAREF, alpha);
    HRS(self, GFX_RS_FOGENABLE, FALSE);
    HRS(self, GFX_RS_SRCBLEND, GFX_BLEND_ONE);
    HRS(self, GFX_RS_DESTBLEND, GFX_BLEND_ONE);
    TIMED(PERF_L_ADDITIVE, flush_keyed(self, RENDER_ADDITIVE, 1, 0));
    TIMED(PERF_L_BLENDED_WRAP, flush_blended_wrap(self));
    HRS(self, GFX_RS_ZENABLE, FALSE);
    HRS(self, GFX_RS_SRCBLEND, GFX_BLEND_ONE);
    HRS(self, GFX_RS_DESTBLEND, GFX_BLEND_ONE);
    TIMED(PERF_L_ADDITIVE_OVERLAY, flush_keyed(self, RENDER_ADDITIVE_OVERLAY, 1, 0));
    HRS(self, GFX_RS_SRCBLEND, GFX_BLEND_SRCALPHA);
    HRS(self, GFX_RS_DESTBLEND, GFX_BLEND_INVSRCALPHA);
    TIMED(PERF_L_OVERLAY_ORDERED, flush_keyed(self, RENDER_OVERLAY_ORDERED, 0, 0));
    TIMED(PERF_L_BLENDED_OVERLAY, flush_blended_overlay(self));
    HRS(self, GFX_RS_SRCBLEND, GFX_BLEND_SRCALPHA);
    HRS(self, GFX_RS_DESTBLEND, GFX_BLEND_INVSRCALPHA);
    HTSS(self, 0, GFX_TSS_MIPFILTER, GFX_MIPFILTER_NONE);
    gfx_shadow_receive(0);
}

int renderer_draw_scene(struct Renderer *self)
{
    if (gfx_begin_scene() < 0) {
        renderer_destroy_all_rp_list(self);
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
    return 1;
}

void renderer_flip(struct Renderer *self, char vsync)
{
    int hr;
    self->frame++;
    if ((float)(unsigned)(platform_coarse_ticks_ms() - self->fpsTick) >= 1000.0f) {
        self->fpsTick = platform_coarse_ticks_ms();
        self->fps = self->fpsCount;
        self->fpsCount = 0;
    } else {
        self->fpsCount++;
    }
    if (c3d_device_check_lost(&g_3dDevice) == 1)
        return;
    perf_begin(PERF_TARGETLOCK);
    self->bpp = g_3dDevice.bpp;
    perf_end(PERF_TARGETLOCK);
    TIMED(PERF_PRESENT, hr = c3d_device_show_frame(&g_3dDevice, vsync));
    if (hr == GFX_E_SURFACELOST)
        c3d_device_on_surfaces_lost(&g_3dDevice);
    perf_frame_end(self);
}
