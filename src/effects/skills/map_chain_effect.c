#include "effects/effect_allocator.h"
#include "render/math3d.h"
#include <string.h>
#include "effects/particle.h"
#include "effects/skills/skill_view.h"
#include "std/vec.h"

struct MapChainEffect {
    struct Effect base;
    Vec3 prev_corner3;
    Vec3 prev_corner1;
    struct Vec links;
    int chain_set_modes[5];
};

struct String;
void map_chain_effect_init(struct Effect *self);
void map_chain_effect_process(struct Effect *self);
void map_chain_effect_render(struct Effect *self);
void map_chain_effect_release(struct Effect *self);
void map_chain_effect_reset_links(struct Effect *self);
static void map_chain_effect_destroy(struct Effect *effect);

static const struct EffectOps map_chain_effect_ops = {
    .set_submit = effect_set_submit,
    .init = map_chain_effect_init,
    .update_range = effect_update_range,
    .process = map_chain_effect_process,
    .render = map_chain_effect_render,
    .release = map_chain_effect_release,
    .is_alive = effect_is_owner_alive,
    .restart = map_chain_effect_reset_links,
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
    .destroy = map_chain_effect_destroy,
    .fade_out = effect_fade_out,
};
#define SIN_DEG(d) (g_sinTable[d])
#define COS_DEG(d) (g_cosTable[d])

#define MAP_CHAIN_LIST 5
#define CHAIN_SEGMENTS 21

#define PART_EFFECT_TYPE(p) ((p)->effect_type)
struct MapChainParticle {
    int link_tilt;
    int link_twist;
    float link_height;
    float link_spin_step;
    float link_offset;
};
PARTICLE_EFFECT_DATA_FITS(MapChainParticle);
static __inline struct MapChainParticle *map_chain_particle(const struct Particle *p)
{
    return (struct MapChainParticle *)p->effect_data;
}
#define LINK_SEGMENT(p)     ((p)->index)

static void set_texture(struct Particle *p, const char *name)
{
    struct String texture;
    string_init(&texture);
    string_assign_cstr(&texture, name);
    particle_set_texture(p, texture);
}

void map_chain_effect_shape_link(struct MapChainEffect *self, struct Particle *link)
{
    int segment, a, tilt;
    float r, offset, ring_x, ring_z, ring_y, along_x, along_z;
    Vec3 c1, c3;

    if (!link)
        return;
    segment = LINK_SEGMENT(link);
    r = link->scale;
    a = (int)((float)((int)((float)map_chain_particle(link)->link_twist / 20.0f) * segment) + link->roll) % 360;
    tilt = map_chain_particle(link)->link_tilt;
    offset = map_chain_particle(link)->link_offset;
    ring_x = SIN_DEG(tilt) * r * COS_DEG(a);
    ring_z = COS_DEG(tilt) * r * COS_DEG(a);
    ring_y = SIN_DEG(a) * r;
    along_x = COS_DEG(tilt) * offset;
    along_z = -(SIN_DEG(tilt) * offset);

    c1.x = (ring_x - along_x) + 0.0f;
    c3.x = (along_x + ring_x) + 0.0f;
    c1.y = (ring_y - 0.0f) + map_chain_particle(link)->link_height;
    c1.z = (ring_z - along_z) + 0.0f;
    c3.z = (along_z + ring_z) + 0.0f;
    c3.y = (ring_y + 0.0f) + map_chain_particle(link)->link_height;
    link->corners[1].x = c1.x; link->corners[1].y = c1.y; link->corners[1].z = c1.z;
    link->corners[3].x = c3.x; link->corners[3].y = c3.y; link->corners[3].z = c3.z;
    if (segment != 0) {
        link->corners[0].x = self->prev_corner1.x;
        link->corners[0].y = self->prev_corner1.y;
        link->corners[0].z = self->prev_corner1.z;
        link->corners[2].x = self->prev_corner3.x;
        link->corners[2].y = self->prev_corner3.y;
        link->corners[2].z = self->prev_corner3.z;
    }
    self->prev_corner3 = c3;
    self->prev_corner1 = c1;
}

void map_chain_effect_update_link(struct MapChainEffect *self, struct Particle *link)
{
    float spin, m[12];

    if (!link)
        return;
    spin = map_chain_particle(link)->link_spin_step + link->roll;
    if (spin >= 360.0f)
        spin -= 360.0f;
    link->roll = spin;
    if (link->group > 5) {
        int alpha = (link->age % 600 <= 250 ? 3 : -3) + link->alpha;
        if (alpha >= 255)
            alpha = 255;
        link->alpha = alpha > 0 ? alpha : 0;
    }
    mat_set_rotate_y(m, game_camera_heading() / -180.0f * 3.141592f);
    memcpy(link->matrix, m, sizeof(m));
    map_chain_effect_shape_link(self, link);
}

void map_chain_link_render(struct Particle *link)
{
    struct ParticleRenderArgs args;
    int segment;

    if (!link || (segment = LINK_SEGMENT(link)) <= 0)
        return;
    if (link->group < 6)
        link->dest_blend = 6;
    args.facing_fixed = 0;
    args.v_top = 0.0f;
    args.v_bottom = 1.0f;
    args.u_left = (float)(segment - 1) / 21.0f;
    args.u_right = (float)segment / 21.0f;
    args.map_chain_flag = 1;
    args.with_depth = 1;
    particle_render_with(link, &args);
}

void map_chain_effect_reset_link(struct MapChainEffect *self, struct Particle *link)
{
    link->list_flags = MAP_CHAIN_LIST;
    link->pos = self->base.pos;
    map_chain_particle(link)->link_offset = self->base.params.area;
    link->scale = self->base.params.scale;
    link->alpha = self->base.params.alpha;
    map_chain_particle(link)->link_height = self->base.params.values[0];
    map_chain_particle(link)->link_spin_step = self->base.params.values[1];
    link->src_blend = 5;
    link->dest_blend = 7;
    particle_quad_unit_corners(link->corners);
    link->age = 0;
}

void map_chain_effect_spawn_chains(struct MapChainEffect *self, int tilt)
{
    int strip, angle, segment;

    for (strip = 0, angle = 0; angle < 360; strip++, angle += 60)
        for (segment = 0; segment < CHAIN_SEGMENTS; segment++) {
            struct Particle *link = particle_new(self->base.pos, MAP_CHAIN_LIST);
            if (!link)
                continue;
            link->group = strip;
            PART_EFFECT_TYPE(link) = self->base.type;
            LINK_SEGMENT(link) = segment;
            link->owner_tag = 1;
            map_chain_particle(link)->link_offset = 10.0f;
            link->scale = 14.0f;
            link->roll = (float)angle;
            map_chain_particle(link)->link_tilt = tilt;
            link->alpha = 255;
            map_chain_particle(link)->link_twist = 60;
            map_chain_particle(link)->link_height = -14.0f;
            map_chain_particle(link)->link_spin_step = 1.0f;
            link->dest_blend = 6;
            particle_quad_unit_corners(link->corners);
            set_texture(link, "effect\\hero_005.tga");
            vec_push(&self->links, &link, sizeof link);
        }

    for (strip = 6, angle = 0; angle < 360; strip++, angle += 60)
        for (segment = 0; segment < CHAIN_SEGMENTS; segment++) {
            struct Particle *link = particle_new(self->base.pos, MAP_CHAIN_LIST);
            if (!link)
                continue;
            link->group = strip;
            PART_EFFECT_TYPE(link) = self->base.type;
            LINK_SEGMENT(link) = segment;
            link->owner_tag = 1;
            map_chain_particle(link)->link_offset = 10.0f;
            link->scale = 14.0f;
            link->roll = (float)angle;
            map_chain_particle(link)->link_tilt = tilt;
            link->alpha = 255;
            map_chain_particle(link)->link_twist = 60;
            map_chain_particle(link)->link_height = -14.0f;
            map_chain_particle(link)->link_spin_step = 1.0f;
            link->src_blend = 5;
            link->dest_blend = 7;
            particle_quad_unit_corners(link->corners);
            set_texture(link, "effect\\hero_005.tga");
            vec_push(&self->links, &link, sizeof link);
        }
}

struct MapChainEffect *map_chain_effect_construct(struct MapChainEffect *self)
{
    int i;

    effect_construct(&self->base, 0x400);
    self->base.ops = &map_chain_effect_ops;
    self->prev_corner3.x = self->prev_corner3.y = self->prev_corner3.z = 0;
    self->prev_corner1.x = self->prev_corner1.y = self->prev_corner1.z = 0;
    self->links.first = self->links.last = self->links.end = 0;
    for (i = 0; i < 5; i++)
        self->chain_set_modes[i] = 2;
    return self;
}

struct Effect *map_chain_effect_new(void)
{
    struct MapChainEffect *self = mem_try_alloc(sizeof *self);

    return self ? &map_chain_effect_construct(self)->base : NULL;
}

void map_chain_effect_destruct(struct MapChainEffect *self)
{
    self->base.ops = &map_chain_effect_ops;
    vec_free(&self->links);
    effect_destruct(&self->base);
}

static void map_chain_effect_destroy(struct Effect *effect)
{
    struct MapChainEffect *self = (struct MapChainEffect *)effect;
    map_chain_effect_destruct(self);
    mem_free(self);
}

void map_chain_effect_init(struct Effect *effect)
{
    struct MapChainEffect *self = (struct MapChainEffect *)effect;
    map_chain_effect_spawn_chains(self, 25);
    map_chain_effect_spawn_chains(self, 50);
    map_chain_effect_spawn_chains(self, 95);
    map_chain_effect_spawn_chains(self, 120);
    map_chain_effect_spawn_chains(self, 165);
}

void map_chain_effect_process(struct Effect *effect)
{
    struct MapChainEffect *self = (struct MapChainEffect *)effect;
    struct Particle **it;

    for (it = self->links.first; it != (struct Particle **)self->links.last; it++) {
        struct Particle *link = *it;
        link->pos = self->base.pos;
        map_chain_effect_update_link(self, link);
        link->age++;
    }
}

void map_chain_effect_render(struct Effect *effect)
{
    struct MapChainEffect *self = (struct MapChainEffect *)effect;
    struct Particle **it;

    for (it = self->links.first; it != (struct Particle **)self->links.last; it++)
        map_chain_link_render(*it);
}

void map_chain_effect_release(struct Effect *effect)
{
    struct MapChainEffect *self = (struct MapChainEffect *)effect;
    struct Particle **it;

    for (it = self->links.first; it != (struct Particle **)self->links.last; it++)
        particle_free(*it);
    self->links.last = self->links.first;
}

void map_chain_effect_reset_links(struct Effect *effect)
{
    struct MapChainEffect *self = (struct MapChainEffect *)effect;
    struct Particle **it;

    for (it = self->links.first; it != (struct Particle **)self->links.last; it++)
        map_chain_effect_reset_link(self, *it);
}
