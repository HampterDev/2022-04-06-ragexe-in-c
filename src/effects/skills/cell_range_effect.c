#include "effects/effect_allocator.h"
#include "render/math3d.h"
#include "resources/attr_queries.h"
#include <stdio.h>
#include <string.h>
#include "effects/particle.h"
#include "effects/skills/skill_view.h"
#include "effects/skills/skill.h"
#include "std/vec.h"

struct CellRangeEffect;
struct Effect;
struct String;
void cell_range_effect_init(struct Effect *self);
void cell_range_effect_process(struct Effect *self);
void cell_range_effect_render(struct Effect *self);
void cell_range_effect_release(struct Effect *self);
static void cell_range_effect_destroy(struct Effect *effect);

static const struct EffectOps cell_range_effect_ops = {
    .set_submit = effect_set_submit,
    .init = cell_range_effect_init,
    .update_range = effect_update_range,
    .process = cell_range_effect_process,
    .render = cell_range_effect_render,
    .release = cell_range_effect_release,
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
    .destroy = cell_range_effect_destroy,
    .fade_out = effect_fade_out,
};

enum {
    CELL_RANGE_WATER = 992,
    CELL_RANGE_STONE = 993,
    CELL_RANGE_FIRE = 994,
    CELL_RANGE_LIGHTNING = 995,
    CELL_RANGE_MAKIBISHI = 1007,
};

#define ACTOR_CELL_RANGE(actor) ((const struct Skill *)(actor))->range

struct CellRangeEffect {
    struct Effect base;
    float cell_size;
    struct Vec tiles;
};

static void set_particle_texture(struct Particle *p, const char *name)
{
    struct String s;
    string_init(&s);
    string_assign_cstr(&s, name);
    particle_set_texture(p, s);
}

struct CellRangeEffect *cell_range_effect_construct(struct CellRangeEffect *self)
{
    effect_construct(&self->base, -1);
    self->base.ops = &cell_range_effect_ops;
    self->cell_size = 5.0f;
    self->tiles.first = self->tiles.last = self->tiles.end = 0;
    return self;
}

struct Effect *cell_range_effect_new(void)
{
    struct CellRangeEffect *self = mem_try_alloc(sizeof *self);

    return self ? &cell_range_effect_construct(self)->base : NULL;
}

/* Effect types created as CellRangeEffect, registered at startup through g_effectTypes
   (effects/effect_registry.c). A type missing here is not created. */
static const struct EffectTypeRow cell_range_effect_type_rows[] = {
    EFFECT_TYPE(0x3ef, cell_range_effect_new),
};
const struct EffectTypeTable cell_range_effect_types = EFFECT_TYPE_TABLE(cell_range_effect_type_rows);

void cell_range_effect_destruct(struct CellRangeEffect *self)
{
    self->base.ops = &cell_range_effect_ops;
    vec_free(&self->tiles);
    effect_destruct(&self->base);
}

static void cell_range_effect_destroy(struct Effect *effect)
{
    struct CellRangeEffect *self = (struct CellRangeEffect *)effect;
    cell_range_effect_destruct(self);
    mem_free(self);
}

void cell_range_effect_release(struct Effect *effect)
{
    struct CellRangeEffect *self = (struct CellRangeEffect *)effect;
    struct Particle **it;
    for (it = self->tiles.first; it != (struct Particle **)self->tiles.last; it++)
        particle_free(*it);
    self->tiles.last = self->tiles.first;
}

void cell_range_effect_init(struct Effect *effect)
{
    struct CellRangeEffect *self = (struct CellRangeEffect *)effect;
    void *owner = effect_find_actor(self->base.owner_aid);
    int range, x, z;

    if (!owner)
        return;
    range = ACTOR_CELL_RANGE(owner);
    for (z = -range; z <= range; z++) {
        for (x = -range; x <= range; x++) {
            struct Particle *p = particle_new(self->base.pos, 5);
            if (!p)
                continue;
            p->effect_type = self->base.type;
            p->index = x;
            p->group = z;
            p->owner_tag = 1;
            p->alpha = 0xff;
            particle_set_color(p, 0xff, 0xff, 0xff);
            set_particle_texture(p, "effect\\torch_violet08.bmp");
            p->scale = 8.0f;
            vec_push(&self->tiles, &p, sizeof(struct Particle *));
        }
    }
}

void cell_range_effect_place_tile(struct CellRangeEffect *self, struct Particle *p)
{
    Vec3 pos;
    float x, z, y;
    float turn[12], scale[12];
    void *ground;

    if (!p)
        return;
    pos = self->base.pos;
    x = (float)p->index * self->cell_size + pos.x;
    z = (float)p->group * self->cell_size + pos.z;
    ground = game_current_map_attr();
    if (!ground)
        return;
    y = c3d_attr_get_height(ground, x, z) - 1.2f;
    p->pos.x = x;
    p->pos.y = y;
    p->pos.z = z;

    mat_set_rotate_x(turn, degrees_to_radians(game_camera_pitch()));
    mat_rotate_x(turn, 3.141592f);
    mat_set_scale(scale, p->scale, p->scale, 1.0f);
    mat_multiply(p->matrix, scale, turn);
}

void cell_range_effect_process(struct Effect *effect)
{
    struct CellRangeEffect *self = (struct CellRangeEffect *)effect;
    struct Particle **it;
    for (it = self->tiles.first; it != (struct Particle **)self->tiles.last; it++) {
        struct Particle *p = *it;
        p->pos = self->base.pos;
        cell_range_effect_place_tile(self, p);
        p->age++;
    }
}

void cell_range_effect_set_tile_texture(struct CellRangeEffect *self,
                                                 struct Particle *p)
{
    char name[0x1000];
    int frame;

    if (!p)
        return;
    frame = (p->age + p->index) / 10 % 3 + 1;
    name[0] = 0;
    switch (self->base.type) {
    case CELL_RANGE_WATER:
        sprintf(name, "effect\\water-%d.bmp", frame);
        break;
    case CELL_RANGE_STONE:
        sprintf(name, "effect\\stone-%d.bmp", frame);
        break;
    case CELL_RANGE_FIRE:
        sprintf(name, "effect\\fire-%d.bmp", frame);
        break;
    case CELL_RANGE_LIGHTNING:
        sprintf(name, "effect\\lightning-%d.bmp", frame);
        break;
    case CELL_RANGE_MAKIBISHI:
        strcpy(name, "effect\\makibishi.bmp");
        p->scale = 2.5f;
        p->dest_blend = 6;
        break;
    }
    if (name[0])
        set_particle_texture(p, name);
}

void cell_range_effect_draw_tile(struct CellRangeEffect *self, struct Particle *p)
{
    if (!p)
        return;
    cell_range_effect_set_tile_texture(self, p);
    particle_render(p, 0, 0, 1.0f, 0, 1.0f, 1);
}

void cell_range_effect_render(struct Effect *effect)
{
    struct CellRangeEffect *self = (struct CellRangeEffect *)effect;
    struct Particle **it;
    for (it = self->tiles.first; it != (struct Particle **)self->tiles.last; it++)
        cell_range_effect_draw_tile(self, *it);
}
