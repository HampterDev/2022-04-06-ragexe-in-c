#include "effects/effect_allocator.h"
#include "render/rp_pool.h"
#include <string.h>
#include "effects/particle.h"
#include "render/texture_types.h"
#include "render/render_api.h"
#include "std/vec.h"

struct FullScreenEffect {
    struct Effect base;
    struct Vec quads;
    unsigned char active;
};

#define EFFECT_FROSTBITE_SCREEN 0x8e0

enum ScreenQuarter {
    QUARTER_TOP_LEFT = 0,
    QUARTER_TOP_RIGHT = 1,
    QUARTER_BOTTOM_LEFT = 2,
    QUARTER_BOTTOM_RIGHT = 3,
};

struct String;
void full_screen_effect_init(struct Effect *self);
void full_screen_effect_render(struct Effect *self);
void full_screen_effect_release(struct Effect *self);
unsigned char full_screen_effect_is_alive(struct Effect *self);
static void full_screen_effect_destroy(struct Effect *effect);

static const struct EffectOps full_screen_effect_ops = {
    .set_submit = effect_set_submit,
    .init = full_screen_effect_init,
    .update_range = effect_update_range,
    .process = effect_process,
    .render = full_screen_effect_render,
    .release = full_screen_effect_release,
    .is_alive = full_screen_effect_is_alive,
    .restart = effect_restart,
    .set_color = effect_set_color,
    .set_texture = effect_set_texture,
    .after_init = effect_after_init,
    .skip = effect_skip,
    .set_stop_frame = effect_set_stop_frame,
    .set_start_delay = effect_set_start_delay,
    .set_spin = effect_set_spin,
    .set_str_name = effect_set_str_name,
    .set_list_flags = effect_set_list_flags,
    .set_zoom = effect_set_zoom,
    .set_alpha_scale = effect_set_alpha_scale,
    .destroy = full_screen_effect_destroy,
    .fade_out = effect_fade_out,
};

#define SCREEN_LIST 0x201
#define SCREEN_Z 1e-6f
#define SCREEN_RHW 0.999999f

struct FullScreenEffect *full_screen_effect_construct(struct FullScreenEffect *self)
{
    effect_construct(&self->base, -1);
    self->base.ops = &full_screen_effect_ops;
    self->quads.first = self->quads.last = self->quads.end = 0;
    self->active = 1;
    return self;
}

struct Effect *full_screen_effect_new(void)
{
    struct FullScreenEffect *self = mem_try_alloc(sizeof *self);

    return self ? &full_screen_effect_construct(self)->base : NULL;
}

void full_screen_effect_destruct(struct FullScreenEffect *self)
{
    self->base.ops = &full_screen_effect_ops;
    vec_free(&self->quads);
    effect_destruct(&self->base);
}

static void full_screen_effect_destroy(struct Effect *effect)
{
    struct FullScreenEffect *self = (struct FullScreenEffect *)effect;
    full_screen_effect_destruct(self);
    mem_free(self);
}

void full_screen_effect_release(struct Effect *effect)
{
    struct FullScreenEffect *self = (struct FullScreenEffect *)effect;
    struct Particle **it;

    for (it = self->quads.first; it != (struct Particle **)self->quads.last; it++)
        particle_free(*it);
    self->quads.last = self->quads.first;
}

void full_screen_effect_init(struct Effect *effect)
{
    struct FullScreenEffect *self = (struct FullScreenEffect *)effect;
    static const char frostbite[] = "effect\\state_effect\\frostbite\\frostbite_screen\\frostbite_screen.tga";
    struct String texture;
    int quarter;

    string_init(&texture);
    if (self->base.type != EFFECT_FROSTBITE_SCREEN) {
        self->active = 0;
        string_free(&texture);
        return;
    }
    string_assign(&texture, frostbite, sizeof(frostbite) - 1);
    self->base.has_instance_key = 1;
    for (quarter = QUARTER_TOP_LEFT; quarter <= QUARTER_BOTTOM_RIGHT; quarter++) {
        struct Particle *p = particle_new(self->base.pos, SCREEN_LIST);
        struct String copy;
        const char *name;

        if (!p) {
            self->active = 0;
            break;
        }
        p->effect_type = self->base.type;
        p->index = quarter;
        p->group = 0;
        p->owner_tag = 1;
        name = string_cstr(&texture);
        string_init(&copy);
        string_assign_cstr(&copy, name);
        particle_set_texture(p, copy);
        p->src_blend = 5;
        p->dest_blend = 6;
        p->alpha = 255;
        vec_push(&self->quads, &p, sizeof(struct Particle *));
    }
    string_free(&texture);
}

unsigned char full_screen_effect_is_alive(struct Effect *effect)
{
    struct FullScreenEffect *self = (struct FullScreenEffect *)effect;
    if (self->active == 1 && effect_is_owner_alive(&self->base))
        return 1;
    return 0;
}

static void screen_vertex(struct TexDrawVertex *v, float x, float y, unsigned color, float u, float tv)
{
    v->x = x;
    v->y = y;
    v->z = SCREEN_Z;
    v->rhw = SCREEN_RHW;
    v->diffuse = color;
    v->specular = 0xff000000u;
    v->u = u;
    v->v = tv;
}

void full_screen_effect_render_quad(struct FullScreenEffect *self, struct Particle *p)
{
    struct Texture *tex;
    struct TexDrawPrim *batch;
    float u0, u1, v0, v1, w, h, left, right, top, bottom;
    unsigned color;
    const struct Renderer *r;

    if (!p)
        return;
    tex = p->texture;
    batch = &p->batch;
    batch->base.texture = tex;
    w = (float)tex->base.width;
    u0 = g_texelOffsetU / w;
    u1 = ((float)(double)(unsigned)tex->reqWidth + g_texelOffsetU) / w;
    h = (float)tex->base.height;
    v1 = (float)(double)(unsigned)tex->reqHeight + g_texelOffsetV;
    v0 = g_texelOffsetV / h;
    v1 = v1 / h;
    color = (unsigned)((((p->alpha << 8 | (int)p->r) << 8 | (int)p->g) << 8) | (int)p->b);

    r = g_renderer;
    switch ((unsigned)p->index) {
    case QUARTER_TOP_LEFT:
        left = 0.0f; right = (float)r->centreX; top = 0.0f; bottom = (float)r->centreY;
        screen_vertex(&batch->quad[0], left, top, color, u1, v0);
        screen_vertex(&batch->quad[1], right, top, color, u0, v0);
        screen_vertex(&batch->quad[2], left, bottom, color, u1, v1);
        screen_vertex(&batch->quad[3], right, bottom, color, u0, v1);
        break;
    case QUARTER_TOP_RIGHT:
        left = (float)r->centreX; right = (float)r->width; top = 0.0f; bottom = (float)r->centreY;
        screen_vertex(&batch->quad[0], left, top, color, u0, v0);
        screen_vertex(&batch->quad[1], right, top, color, u1, v0);
        screen_vertex(&batch->quad[2], left, bottom, color, u0, v1);
        screen_vertex(&batch->quad[3], right, bottom, color, u1, v1);
        break;
    case QUARTER_BOTTOM_LEFT:
        left = 0.0f; right = (float)r->centreX; top = (float)r->centreY; bottom = (float)r->height;
        screen_vertex(&batch->quad[0], left, top, color, u1, v1);
        screen_vertex(&batch->quad[1], right, top, color, u0, v1);
        screen_vertex(&batch->quad[2], left, bottom, color, u1, v0);
        screen_vertex(&batch->quad[3], right, bottom, color, u0, v0);
        break;
    case QUARTER_BOTTOM_RIGHT:
        left = (float)r->centreX; right = (float)r->width; top = (float)r->centreY; bottom = (float)r->height;
        screen_vertex(&batch->quad[0], left, top, color, u0, v1);
        screen_vertex(&batch->quad[1], right, top, color, u1, v1);
        screen_vertex(&batch->quad[2], left, bottom, color, u0, v0);
        screen_vertex(&batch->quad[3], right, bottom, color, u1, v0);
        break;
    }
    batch->base.srcBlend = p->src_blend;
    batch->base.destBlend = p->dest_blend;
    renderer_add_prim(g_renderer, &batch->base, p->list_flags);
}

void full_screen_effect_render(struct Effect *effect)
{
    struct FullScreenEffect *self = (struct FullScreenEffect *)effect;
    struct Particle **it;

    for (it = self->quads.first; it != (struct Particle **)self->quads.last; it++)
        full_screen_effect_render_quad(self, *it);
}
