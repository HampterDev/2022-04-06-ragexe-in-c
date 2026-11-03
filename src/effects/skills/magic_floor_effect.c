#include "effects/effect_allocator.h"
#include "render/math3d.h"
#include <stdlib.h>
#include <string.h>
#include "effects/particle.h"
#include "std/vec.h"

struct MagicFloorEffect {
    struct Effect base;
    Vec3 prev_inner;
    Vec3 prev_outer;
    int prev_tile_corners[6];
    struct Vec tiles;
    struct Vec clouds;
};

struct String;
void magic_floor_effect_init(struct Effect *self);
void magic_floor_effect_process(struct Effect *self);
void magic_floor_effect_render(struct Effect *self);
void magic_floor_effect_release(struct Effect *self);
void magic_floor_effect_reset_tiles(struct Effect *self);
static void magic_floor_effect_destroy(struct Effect *effect);

static const struct EffectOps magic_floor_effect_ops = {
    .set_submit = effect_set_submit,
    .init = magic_floor_effect_init,
    .update_range = effect_update_range,
    .process = magic_floor_effect_process,
    .render = magic_floor_effect_render,
    .release = magic_floor_effect_release,
    .is_alive = effect_is_owner_alive,
    .restart = magic_floor_effect_reset_tiles,
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
    .destroy = magic_floor_effect_destroy,
    .fade_out = effect_fade_out,
};
#define SIN_DEG(d) (g_sinTable[d])
#define COS_DEG(d) (g_cosTable[d])

#define MAGIC_FLOOR_LIST 5
#define CLOUD_SEGMENTS 21

#define PART_EFFECT_TYPE(p) ((p)->effect_type)
struct MagicFloorParticle {
    int cloud_tilt;
    int cloud_twist;
    float cloud_width;
    float cloud_height;
};
PARTICLE_EFFECT_DATA_FITS(MagicFloorParticle);
static __inline struct MagicFloorParticle *magic_floor_particle(const struct Particle *p)
{
    return (struct MagicFloorParticle *)p->effect_data;
}
#define PART_INDEX(p)       ((p)->index)

static const char *const tile_textures[6] = {
    "effect\\hero_011.tga", "effect\\hero_001.tga",
    "effect\\hero_021.tga", "effect\\hero_002.tga",
    "effect\\hero_031.tga", "effect\\hero_003.tga",
};

static int random_below(int n)
{
    int r = rand() % n;
    return r < 0 ? 0 : r;
}

static void set_texture(struct Particle *p, const char *name)
{
    struct String texture;
    string_init(&texture);
    string_assign_cstr(&texture, name);
    particle_set_texture(p, texture);
}

void magic_floor_tile_spin(struct Particle *tile)
{
    float turn[12], scale[12], m[12];
    int deg = 0;

    if (!tile)
        return;
    switch (PART_INDEX(tile)) {
    case 0: case 1: deg = (int)(tile->roll + 1.0f); break;
    case 2: case 3: deg = (int)(tile->roll + 2.0f); break;
    case 4: case 5: deg = (int)(tile->roll - 1.0f); break;
    }
    if (deg >= 360)
        deg -= 360;
    else if (deg < 0)
        deg += 360;
    tile->roll = (float)deg;
    mat_set_rotate_z(turn, tile->roll / 180.0f * 3.141592f);
    mat_rotate_x(turn, 1.570796f);
    mat_set_scale(scale, tile->scale, tile->scale, 1.0f);
    mat_multiply(m, scale, turn);
    memcpy(tile->matrix, m, sizeof(m));
}

void magic_floor_tile_render(struct Particle *tile)
{
    if (tile)
        particle_render(tile, 0, 0.0f, 1.0f, 0.0f, 1.0f, 1);
}

void magic_floor_effect_reset_tile(struct MagicFloorEffect *self, struct Particle *tile)
{
    tile->list_flags = MAGIC_FLOOR_LIST;
    tile->pos = self->base.pos;
    tile->scale = self->base.params.scale;
    tile->roll = (float)self->base.params.roll;
    magic_floor_particle(tile)->cloud_tilt = (int)((float)tile->group * self->base.params.rise_angle);
    tile->alpha = self->base.params.alpha;
    tile->src_blend = 5;
    tile->dest_blend = 7;
    particle_quad_unit_corners(tile->corners);
    tile->age = 0;
}

void magic_floor_effect_spawn_tiles(struct MagicFloorEffect *self)
{
    int i;

    for (i = 0; i < 6; i++) {
        struct Particle *tile = particle_new(self->base.pos, MAGIC_FLOOR_LIST);
        if (!tile)
            continue;
        set_texture(tile, tile_textures[i]);
        PART_EFFECT_TYPE(tile) = self->base.type;
        PART_INDEX(tile) = i;
        tile->group = 0;
        tile->owner_tag = 1;
        tile->alpha = 255;
        tile->roll = 0.0f;
        magic_floor_particle(tile)->cloud_tilt = 0;
        tile->scale = 30.0f;
        particle_quad_unit_corners(tile->corners);
        vec_push(&self->tiles, &tile, sizeof tile);
    }
}

void magic_floor_effect_spawn_clouds(struct MagicFloorEffect *self, int set)
{
    int band, segment;

    for (band = 0; band < 4; band++) {
        int spin = random_below(360);
        for (segment = 0; segment < CLOUD_SEGMENTS; segment++) {
            struct Particle *cloud = particle_new(self->base.pos, MAGIC_FLOOR_LIST);
            if (!cloud)
                continue;
            set_texture(cloud, "effect\\cloud11.tga");
            PART_EFFECT_TYPE(cloud) = self->base.type;
            PART_INDEX(cloud) = segment;
            cloud->group = band;
            cloud->owner_tag = 1;
            cloud->alpha = 200;
            magic_floor_particle(cloud)->cloud_twist = 30;
            cloud->roll = (float)spin;
            magic_floor_particle(cloud)->cloud_tilt = random_below(21) + 80;
            magic_floor_particle(cloud)->cloud_width = 1.0f;
            particle_set_color(cloud, 255, 100, 100);
            if (set == 0) {
                magic_floor_particle(cloud)->cloud_height = 48.0f - (float)band * 5.0f;
                cloud->scale = 11.0f;
            } else if (set == 1) {
                magic_floor_particle(cloud)->cloud_height = 28.0f - (float)band * 5.0f;
                cloud->scale = 22.0f;
            }
            particle_quad_unit_corners(cloud->corners);
            vec_push(&self->clouds, &cloud, sizeof cloud);
        }
    }
}

void magic_floor_effect_update_cloud(struct MagicFloorEffect *self, struct Particle *cloud)
{
    int spin, twist, alpha, phase, segment, a, tilt;
    float width, inner, y_in, tilted, x_in, z_in, x_out, y_out, z_out;

    if (!cloud)
        return;
    spin = (int)(cloud->roll + 5.0f) % 360;
    twist = magic_floor_particle(cloud)->cloud_twist + 3;
    if (twist > 180)
        twist = 180;
    magic_floor_particle(cloud)->cloud_twist = twist;
    alpha = cloud->alpha;
    phase = cloud->age % 300;
    cloud->roll = (float)spin;
    if (phase == 0) {
        alpha = 50;
    } else if (phase > 100) {
        alpha = alpha - 1;
        if (alpha < 0)
            alpha = 0;
    }
    cloud->alpha = phase < 12 ? alpha + 1 : alpha;

    segment = PART_INDEX(cloud);
    width = magic_floor_particle(cloud)->cloud_width;
    inner = self->base.params.scale;
    a = (int)((float)((int)((float)twist / 20.0f) * segment) + (float)spin) % 360;
    tilt = magic_floor_particle(cloud)->cloud_tilt;
    y_in = -magic_floor_particle(cloud)->cloud_height;
    tilted = COS_DEG(tilt) * width;
    x_in = inner * COS_DEG(a);
    z_in = inner * SIN_DEG(a);
    x_out = tilted * COS_DEG(a) + x_in;
    y_out = y_in - width * SIN_DEG(tilt);
    z_out = tilted * SIN_DEG(a) + z_in;

    cloud->corners[1].x = x_out; cloud->corners[1].y = y_out; cloud->corners[1].z = z_out;
    cloud->corners[3].x = x_in;  cloud->corners[3].y = y_in;  cloud->corners[3].z = z_in;
    if (segment != 0) {
        cloud->corners[0].x = self->prev_outer.x;
        cloud->corners[0].y = self->prev_outer.y;
        cloud->corners[0].z = self->prev_outer.z;
        cloud->corners[2].x = self->prev_inner.x;
        cloud->corners[2].y = self->prev_inner.y;
        cloud->corners[2].z = self->prev_inner.z;
    }
    self->prev_inner.x = x_in;  self->prev_inner.y = y_in;  self->prev_inner.z = z_in;
    self->prev_outer.x = x_out; self->prev_outer.y = y_out; self->prev_outer.z = z_out;
}

void magic_floor_cloud_render(struct Particle *cloud)
{
    struct ParticleRenderArgs args;
    int segment;

    if (!cloud || (segment = PART_INDEX(cloud)) <= 0)
        return;
    args.facing_fixed = 0;
    args.v_top = 0.0f;
    args.v_bottom = 1.0f;
    args.u_left = (float)(segment - 1) / 21.0f;
    args.u_right = (float)segment / 21.0f;
    args.map_chain_flag = 0;
    args.with_depth = 1;
    particle_render_with(cloud, &args);
}

struct MagicFloorEffect *magic_floor_effect_construct(struct MagicFloorEffect *self)
{
    effect_construct(&self->base, 0x401);
    self->base.ops = &magic_floor_effect_ops;
    self->prev_inner.x = self->prev_inner.y = self->prev_inner.z = 0;
    self->prev_outer.x = self->prev_outer.y = self->prev_outer.z = 0;
    memset(self->prev_tile_corners, 0, sizeof(self->prev_tile_corners));
    self->tiles.first = self->tiles.last = self->tiles.end = 0;
    self->clouds.first = self->clouds.last = self->clouds.end = 0;
    return self;
}

struct Effect *magic_floor_effect_new(void)
{
    struct MagicFloorEffect *self = mem_try_alloc(sizeof *self);

    return self ? &magic_floor_effect_construct(self)->base : NULL;
}

/* Effect types created as MagicFloorEffect, registered at startup through g_effectTypes
   (effects/effect_registry.c). A type missing here is not created. */
static const struct EffectTypeRow magic_floor_effect_type_rows[] = {
    EFFECT_TYPE(0x401, magic_floor_effect_new),
};
const struct EffectTypeTable magic_floor_effect_types = EFFECT_TYPE_TABLE(magic_floor_effect_type_rows);

void magic_floor_effect_destruct(struct MagicFloorEffect *self)
{
    self->base.ops = &magic_floor_effect_ops;
    vec_free(&self->clouds);
    vec_free(&self->tiles);
    effect_destruct(&self->base);
}

static void magic_floor_effect_destroy(struct Effect *effect)
{
    struct MagicFloorEffect *self = (struct MagicFloorEffect *)effect;
    magic_floor_effect_destruct(self);
    mem_free(self);
}

void magic_floor_effect_init(struct Effect *effect)
{
    struct MagicFloorEffect *self = (struct MagicFloorEffect *)effect;
    magic_floor_effect_spawn_tiles(self);
    magic_floor_effect_spawn_clouds(self, 0);
    magic_floor_effect_spawn_clouds(self, 1);
}

void magic_floor_effect_process(struct Effect *effect)
{
    struct MagicFloorEffect *self = (struct MagicFloorEffect *)effect;
    struct Particle **it;

    for (it = self->tiles.first; it != (struct Particle **)self->tiles.last; it++) {
        struct Particle *tile = *it;
        tile->pos = self->base.pos;
        magic_floor_tile_spin(tile);
        tile->age++;
    }
    for (it = self->clouds.first; it != (struct Particle **)self->clouds.last; it++) {
        struct Particle *cloud = *it;
        cloud->pos = self->base.pos;
        magic_floor_effect_update_cloud(self, cloud);
        cloud->age++;
    }
}

void magic_floor_effect_render(struct Effect *effect)
{
    struct MagicFloorEffect *self = (struct MagicFloorEffect *)effect;
    struct Particle **it;

    for (it = self->tiles.first; it != (struct Particle **)self->tiles.last; it++)
        magic_floor_tile_render(*it);
    for (it = self->clouds.first; it != (struct Particle **)self->clouds.last; it++)
        magic_floor_cloud_render(*it);
}

void magic_floor_effect_release(struct Effect *effect)
{
    struct MagicFloorEffect *self = (struct MagicFloorEffect *)effect;
    struct Particle **it;

    for (it = self->tiles.first; it != (struct Particle **)self->tiles.last; it++)
        particle_free(*it);
    self->tiles.last = self->tiles.first;
    for (it = self->clouds.first; it != (struct Particle **)self->clouds.last; it++)
        particle_free(*it);
    self->clouds.last = self->clouds.first;
}

void magic_floor_effect_reset_tiles(struct Effect *effect)
{
    struct MagicFloorEffect *self = (struct MagicFloorEffect *)effect;
    struct Particle **it;

    for (it = self->tiles.first; it != (struct Particle **)self->tiles.last; it++)
        magic_floor_effect_reset_tile(self, *it);
}
