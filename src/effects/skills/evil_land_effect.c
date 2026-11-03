#include "game/cells.h"
#include "effects/effect_allocator.h"
#include "render/texture_cache.h"
#include "effects/particle.h"
#include "render/texture_types.h"
#include "std/vec.h"

struct Effect;
struct EvilLandEffect;
void evil_land_effect_init(struct Effect *self);
void evil_land_effect_process(struct Effect *self);
void evil_land_effect_render(struct Effect *self);
void evil_land_effect_release(struct Effect *self);
static void evil_land_effect_destroy(struct Effect *effect);

static const struct EffectOps evil_land_effect_ops = {
    .set_submit = effect_set_submit,
    .init = evil_land_effect_init,
    .update_range = effect_update_range,
    .process = evil_land_effect_process,
    .render = evil_land_effect_render,
    .release = evil_land_effect_release,
    .is_alive = effect_is_owner_alive,
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
    .destroy = evil_land_effect_destroy,
    .fade_out = effect_fade_out,
};

#define LIST_GROUND (-1)
#define SCROLL_PER_FRAME 0.001f

#define particle_owner_type(p) ((p)->base.effect_type)
#define particle_tag(p)        ((p)->base.owner_tag)
#define particle_col(p)        ((p)->base.index)
#define particle_row(p)        ((p)->base.group)

#define grid_size(self) ((self)->base.level)

struct EvilLandEffect {
    struct Effect base;
    struct Vec cells;
};

#define EACH_CELL(self, it) \
    for (it = (struct MultiTexParticle **)(self)->cells.first; it != (struct MultiTexParticle **)(self)->cells.last; it++)

struct EvilLandEffect *evil_land_effect_construct(struct EvilLandEffect *self)
{
    effect_construct(&self->base, 0x613);
    self->base.ops = &evil_land_effect_ops;
    self->cells.first = self->cells.last = self->cells.end = 0;
    return self;
}

struct Effect *evil_land_effect_new(void)
{
    struct EvilLandEffect *self = mem_try_alloc(sizeof *self);

    return self ? &evil_land_effect_construct(self)->base : NULL;
}

/* Effect types created as EvilLandEffect, registered at startup through g_effectTypes
   (effects/effect_registry.c). A type missing here is not created. */
static const struct EffectTypeRow evil_land_effect_type_rows[] = {
    EFFECT_TYPE(0x613, evil_land_effect_new),
};
const struct EffectTypeTable evil_land_effect_types = EFFECT_TYPE_TABLE(evil_land_effect_type_rows);

void evil_land_effect_destruct(struct EvilLandEffect *self)
{
    self->base.ops = &evil_land_effect_ops;
    vec_free(&self->cells);
    effect_destruct(&self->base);
}

static void evil_land_effect_destroy(struct Effect *effect)
{
    struct EvilLandEffect *self = (struct EvilLandEffect *)effect;
    evil_land_effect_destruct(self);
    mem_free(self);
}

void evil_land_effect_release(struct Effect *effect)
{
    struct EvilLandEffect *self = (struct EvilLandEffect *)effect;
    struct MultiTexParticle **it;
    EACH_CELL(self, it)
        particle_free(&(*it)->base);
    self->cells.last = self->cells.first;
}

void evil_land_effect_init(struct Effect *effect)
{
    struct EvilLandEffect *self = (struct EvilLandEffect *)effect;
    struct Texture *ground, *alpha;
    float u0, v0, u1, v1, n;
    int row, col;

    ground = texture_cache_get(&g_textureCache, "effect\\EVILLAND\\demonground.tga", 0, 0);
    n = (float)grid_size(self);
    u0 = (float)(unsigned)ground->reqWidth / (float)(unsigned)ground->base.width / n;
    v0 = (float)(unsigned)ground->reqHeight / (float)(unsigned)ground->base.height / n;
    alpha = texture_cache_get(&g_textureCache, "effect\\EVILLAND\\demonground_alpha_translucent.tga", 0, 0);
    n = (float)grid_size(self);
    u1 = (float)(unsigned)alpha->reqWidth / (float)(unsigned)alpha->base.width / n;
    v1 = (float)(unsigned)alpha->reqHeight / (float)(unsigned)alpha->base.height / n;

    for (row = 0; row < grid_size(self); row++) {
        for (col = 0; col < grid_size(self); col++) {
            struct MultiTexParticle *p = multitex_particle_new(self->base.pos, LIST_GROUND);
            float left, top, right, bottom;
            if (!p)
                continue;
            particle_owner_type(p) = self->base.type;
            particle_row(p) = row;
            particle_col(p) = col;
            particle_tag(p) = 1;
            p->texture0 = ground;
            p->texture1 = alpha;
            left = (float)col;
            top = (float)row;
            right = (float)(col + 1);
            bottom = (float)(row + 1);
            p->uv0[0].u = left * u0;   p->uv0[0].v = top * v0;
            p->uv1[0].u = left * u1;   p->uv1[0].v = top * v1;
            p->uv0[1].u = right * u0;  p->uv0[1].v = top * v0;
            p->uv1[1].u = right * u1;  p->uv1[1].v = top * v1;
            p->uv0[2].u = left * u0;   p->uv0[2].v = bottom * v0;
            p->uv1[2].u = left * u1;   p->uv1[2].v = bottom * v1;
            p->uv0[3].u = right * u0;  p->uv0[3].v = bottom * v0;
            p->uv1[3].u = right * u1;  p->uv1[3].v = bottom * v1;
            vec_push(&self->cells, &p, sizeof(struct MultiTexParticle *));
        }
    }
}

void evil_land_scroll_cell(struct MultiTexParticle *p)
{
    if (p)
        p->v_scroll += SCROLL_PER_FRAME;
}

void evil_land_effect_process(struct Effect *effect)
{
    struct EvilLandEffect *self = (struct EvilLandEffect *)effect;
    struct MultiTexParticle **first = self->cells.first, **last = self->cells.last;
    size_t count = first > last ? 0 : (size_t)(last - first), i;
    for (i = 0; i < count; i++)
        evil_land_scroll_cell(first[i]);
}

void evil_land_effect_render_cell(struct EvilLandEffect *self,
                                            struct MultiTexParticle *p)
{
    int x, y, half;

    if (!p || !game_world_to_cell(self->base.pos.x, self->base.pos.z, &x, &y))
        return;
    half = grid_size(self) / 2;
    multi_tex_particle_render_on_cell(p, particle_col(p) - half + x, particle_row(p) - half + y);
}

void evil_land_effect_render(struct Effect *effect)
{
    struct EvilLandEffect *self = (struct EvilLandEffect *)effect;
    struct MultiTexParticle **it;
    EACH_CELL(self, it)
        evil_land_effect_render_cell(self, *it);
}
