#include "effects/effect_allocator.h"
#include "render/math3d.h"
#include "game/context.h"
#include <stdlib.h>
#include <string.h>
#include "effects/particle.h"
#include "effects/skills/skill_view.h"
#include "std/vec.h"

#define HEAL_TYPE_FD  0xfd
#define HEAL_TYPE_HEAL 0x138
#define HEAL_TYPE_HEAL2 0x139
#define HEAL_TYPE_HEAL3 0x140
#define HEAL_TYPE_HEAL4 0x145
#define HEAL_TYPE_PURPLE 0x459
#define HEAL_TYPE_CYAN 0x477
#define HEAL_TYPE_YELLOW 0x478

#define STRIP_QUADS 21
#define SPARKLES_PER_SPAWN 4
#define HEAL_LIST 5
#define HEAL_DURATION 100

struct HealEffect {
    struct Effect base;
    Vec3 column_ground;
    Vec3 column_top;
    Vec3 cone_ground;
    Vec3 cone_top;
    int column_fade_frame;
    int cone_fade_frame;
    int spawn_frame;
    struct Vec columns;
    struct Vec cones;
    struct Vec sparkles;
};

struct String;
void heal_effect_init(struct Effect *self);
void heal_effect_process(struct Effect *self);
void heal_effect_render(struct Effect *self);
void heal_effect_release(struct Effect *self);
void heal_effect_after_init(struct Effect *self);
static void heal_effect_destroy(struct Effect *effect);

static const struct EffectOps heal_effect_ops = {
    .set_submit = effect_set_submit,
    .init = heal_effect_init,
    .update_range = effect_update_range,
    .process = heal_effect_process,
    .render = heal_effect_render,
    .release = heal_effect_release,
    .is_alive = effect_is_within_duration,
    .restart = effect_restart,
    .set_color = effect_set_color,
    .set_texture = effect_set_texture,
    .after_init = heal_effect_after_init,
    .skip = effect_skip,
    .set_stop_frame = effect_set_stop_frame,
    .set_start_delay = effect_set_start_delay,
    .set_spin = effect_set_spin,
    .set_str_name = effect_set_str_name,
    .set_list_flags = effect_set_list_flags,
    .set_zoom = effect_set_zoom,
    .set_alpha_scale = effect_set_alpha_scale,
    .destroy = heal_effect_destroy,
    .fade_out = effect_fade_out,
};
#define SIN_DEG(d) (g_sinTable[d])
#define COS_DEG(d) (g_cosTable[d])

#define PART_EFFECT_TYPE(p) ((p)->effect_type)
struct HealParticle {
    int            strip_tilt;
    union {
        int        strip_sweep;
        int        sparkle_spin;
    };
    union {
        int        strip_lift;
        float      sparkle_rise;
    };
    float          strip_height;
};
PARTICLE_EFFECT_DATA_FITS(HealParticle);
static __inline struct HealParticle *heal_particle(const struct Particle *p)
{
    return (struct HealParticle *)p->effect_data;
}
#define PART_INDEX(p)       ((p)->index)
#define STRIP_START(p)      ((p)->roll)
#define SPARKLE_OFFSET(p)   (((struct Particle *)(p))->offset)

static void set_texture(struct Particle *p, const char *name)
{
    struct String texture;
    string_init(&texture);
    string_assign_cstr(&texture, name);
    particle_set_texture(p, texture);
}

static int rand_below(int n)
{
    int r = rand() % n;
    return r < 0 ? 0 : r;
}

struct HealEffect *heal_effect_construct(struct HealEffect *self)
{
    effect_construct(&self->base, -1);
    self->base.ops = &heal_effect_ops;
    self->column_ground.x = self->column_ground.y = self->column_ground.z = 0.0f;
    self->column_top.x = self->column_top.y = self->column_top.z = 0.0f;
    self->cone_ground.x = self->cone_ground.y = self->cone_ground.z = 0.0f;
    self->cone_top.x = self->cone_top.y = self->cone_top.z = 0.0f;
    self->columns.first = self->columns.last = self->columns.end = 0;
    self->cones.first = self->cones.last = self->cones.end = 0;
    self->sparkles.first = self->sparkles.last = self->sparkles.end = 0;
    self->spawn_frame = 0;
    return self;
}

struct Effect *heal_effect_new(void)
{
    struct HealEffect *self = mem_try_alloc(sizeof *self);

    return self ? &heal_effect_construct(self)->base : NULL;
}

void heal_effect_destruct(struct HealEffect *self)
{
    self->base.ops = &heal_effect_ops;
    vec_free(&self->sparkles);
    vec_free(&self->cones);
    vec_free(&self->columns);
    effect_destruct(&self->base);
}

static void heal_effect_destroy(struct Effect *effect)
{
    struct HealEffect *self = (struct HealEffect *)effect;
    heal_effect_destruct(self);
    mem_free(self);
}

void heal_effect_spawn_sparkles(struct HealEffect *self, int kind)
{
    int i;

    for (i = 0; i < SPARKLES_PER_SPAWN; i++) {
        struct Particle *p = particle_new(self->base.pos, HEAL_LIST);
        int dz, dx;

        if (!p)
            continue;
        PART_EFFECT_TYPE(p) = self->base.type;
        PART_INDEX(p) = i;
        p->group = 0;
        p->owner_tag = 1;
        set_texture(p, "effect\\pok1.tga");
        p->alpha = 0;
        p->roll = 0.0f;
        heal_particle(p)->sparkle_spin = 0;
        dz = rand() % 13;
        dx = rand() % 13;
        SPARKLE_OFFSET(p).y = 0.0f;
        SPARKLE_OFFSET(p).x = (float)((dx < 0 ? 0 : dx) - 6);
        SPARKLE_OFFSET(p).z = (float)((dz < 0 ? 0 : dz) - 6);
        particle_set_color(p, 220, 250, 220);
        particle_quad_unit_corners(p->corners);
        vec_push(&self->sparkles, &p, sizeof p);

        switch (kind) {
        case 0:
        case 3:
        case 6:
            p->scale = (float)rand_below(10) * 0.1f + 2.0f;
            heal_particle(p)->sparkle_rise = (float)rand_below(40) * 0.01f + 0.4f;
            break;
        case 1:
            p->scale = (float)rand_below(10) * 0.1f + 2.0f;
            heal_particle(p)->sparkle_rise = (float)rand_below(40) * 0.01f + 0.6f;
            break;
        case 2:
            p->scale = (float)rand_below(40) * 0.1f + 0.4f;
            heal_particle(p)->sparkle_rise = (float)rand_below(40) * 0.01f + 0.4f;
            break;
        case 4:
            p->scale = (float)rand_below(10) * 0.1f + 2.0f;
            heal_particle(p)->sparkle_rise = (float)rand_below(40) * 0.01f + 0.3f;
            break;
        case 7:
            p->scale = (float)rand_below(10) * 0.1f + 2.0f;
            heal_particle(p)->sparkle_rise = (float)rand_below(40) * 0.01f + 0.4f;
            particle_set_color(p, 0, 255, 255);
            break;
        case 8:
            p->scale = (float)rand_below(10) * 0.1f + 2.0f;
            heal_particle(p)->sparkle_rise = (float)rand_below(40) * 0.01f + 0.4f;
            particle_set_color(p, 255, 255, 0);
            break;
        default:
            break;
        }
    }
}

void heal_effect_spawn_columns(struct HealEffect *self, int kind)
{
    float random_height = (float)rand_below(20) + 40.0f;
    int strip = 0, start;

    for (start = 0; start < 360; start += 180, strip++) {
        int random_start = rand_below(360);
        int quad;

        for (quad = 0; quad < STRIP_QUADS; quad++) {
            struct Particle *p = particle_new(self->base.pos, HEAL_LIST);
            if (!p)
                continue;
            PART_INDEX(p) = quad;
            p->group = strip;
            PART_EFFECT_TYPE(p) = self->base.type;
            p->owner_tag = 1;
            heal_particle(p)->strip_sweep = 360;
            p->alpha = 0;
            heal_particle(p)->strip_tilt = 90;
            heal_particle(p)->strip_lift = 0;

            switch (kind) {
            case 0:
                set_texture(p, "effect\\ring_white.tga");
                heal_particle(p)->strip_height = 60.0f;
                STRIP_START(p) = (float)start;
                p->scale = (float)strip * 0.4f + 4.6f;
                particle_set_color(p, 140, 210, 140);
                self->column_fade_frame = 60;
                break;
            case 1:
                set_texture(p, "effect\\ring_white.tga");
                heal_particle(p)->strip_height = 60.0f;
                STRIP_START(p) = (float)start;
                p->scale = (float)strip * 0.4f + 4.6f;
                particle_set_color(p, 140, 210, 140);
                self->column_fade_frame = 40;
                break;
            case 2:
                set_texture(p, "effect\\alpha_down.tga");
                heal_particle(p)->strip_height = 90.0f;
                STRIP_START(p) = (float)start;
                p->scale = (float)strip * 0.1f + 4.2f;
                particle_set_color(p, 140, 210, 140);
                self->column_fade_frame = 40;
                break;
            case 3: {
                int r;
                set_texture(p, "effect\\alpha_down.tga");
                heal_particle(p)->strip_height = random_height;
                STRIP_START(p) = (float)random_start;
                r = rand_below(100);
                p->scale = (float)r * 0.005f + 4.5f + (float)strip * 0.2f;
                particle_set_color(p, 255, 255, 255);
                self->column_fade_frame = 30;
                break;
            }
            case 4:
            case 7:
            case 8:
                set_texture(p, "effect\\alpha_down.tga");
                if (kind == 4)
                    particle_set_color(p, 140, 210, 140);
                else if (kind == 7)
                    particle_set_color(p, 0, 255, 255);
                else
                    particle_set_color(p, 255, 255, 0);
                heal_particle(p)->strip_height = (float)(rand_below(20) * strip) + 40.0f;
                STRIP_START(p) = (float)random_start;
                p->scale = (float)strip * 0.2f + 4.6f;
                self->column_fade_frame = 60;
                break;
            case 5:
                set_texture(p, "effect\\ring_blue.tga");
                heal_particle(p)->strip_height = 60.0f;
                STRIP_START(p) = (float)start;
                p->scale = (float)strip * 0.4f + 4.6f;
                particle_set_color(p, 100, 100, 255);
                self->column_fade_frame = 40;
                break;
            case 6:
                set_texture(p, "effect\\ring_white.tga");
                heal_particle(p)->strip_height = 60.0f;
                STRIP_START(p) = (float)start;
                p->scale = (float)strip * 0.4f + 4.6f;
                particle_set_color(p, 115, 25, 115);
                self->column_fade_frame = 60;
                break;
            default:
                break;
            }
            particle_quad_unit_corners(p->corners);
            vec_push(&self->columns, &p, sizeof p);
        }
    }
}

void heal_effect_spawn_cones(struct HealEffect *self, int kind)
{
    int strip = 0, tilt = 50;

    do {
        int random_start = rand_below(360);
        int quad;

        for (quad = 0; quad < STRIP_QUADS; quad++) {
            struct Particle *p = particle_new(self->base.pos, HEAL_LIST);
            if (!p)
                continue;
            p->group = strip;
            PART_EFFECT_TYPE(p) = self->base.type;
            PART_INDEX(p) = quad;
            p->owner_tag = 1;
            heal_particle(p)->strip_sweep = 360;
            p->alpha = 0;
            heal_particle(p)->strip_lift = 0;

            switch (kind) {
            case 0:
                set_texture(p, "effect\\ring_white.tga");
                STRIP_START(p) = (float)random_start;
                heal_particle(p)->strip_height = 12.0f;
                p->scale = (float)strip * 0.2f + 5.0f;
                heal_particle(p)->strip_tilt = tilt;
                particle_set_color(p, 140, 210, 140);
                self->cone_fade_frame = 60;
                break;
            case 1:
            case 5:
                if (kind == 1) {
                    set_texture(p, "effect\\ring_white.tga");
                    particle_set_color(p, 140, 210, 140);
                } else {
                    set_texture(p, "effect\\ring_blue.tga");
                    particle_set_color(p, 100, 100, 255);
                }
                STRIP_START(p) = (float)random_start;
                heal_particle(p)->strip_height = 18.0f;
                p->scale = (float)strip * 0.2f + 5.0f;
                heal_particle(p)->strip_tilt = tilt;
                self->cone_fade_frame = 40;
                break;
            case 2:
                set_texture(p, "effect\\alpha_down.tga");
                STRIP_START(p) = (float)random_start;
                heal_particle(p)->strip_height = 10.0f;
                p->scale = (float)strip * 0.1f + 5.3f;
                heal_particle(p)->strip_tilt = tilt - 4;
                particle_set_color(p, 140, 210, 140);
                self->cone_fade_frame = 30;
                break;
            default:
                break;
            }
            particle_quad_unit_corners(p->corners);
            vec_push(&self->cones, &p, sizeof p);
        }
        strip++;
        tilt -= 2;
    } while (tilt > 46);
}

static void lay_strip_quad(struct Particle *p, float height, Vec3 *ground, Vec3 *top)
{
    int index = PART_INDEX(p);
    float lift = 0.0f > height ? 0.0f : height;
    float scale = p->scale;
    int angle = (int)((float)((int)((float)heal_particle(p)->strip_sweep / 20.0f) * index) + STRIP_START(p)) % 360;
    int tilt = heal_particle(p)->strip_tilt;
    float c = COS_DEG(angle), s = SIN_DEG(angle);
    float ground_x = scale * c, ground_z = scale * s;
    float along = COS_DEG(tilt) * lift;
    float top_x = along * c + ground_x;
    float top_y = 0.0f - SIN_DEG(tilt) * lift;
    float top_z = along * s + ground_z;

    p->corners[1].x = top_x;
    p->corners[1].y = top_y;
    p->corners[1].z = top_z;
    p->corners[3].x = ground_x;
    p->corners[3].y = 0.0f;
    p->corners[3].z = ground_z;
    if (index != 0) {
        p->corners[0].x = top->x;
        p->corners[0].y = top->y;
        p->corners[0].z = top->z;
        p->corners[2].x = ground->x;
        p->corners[2].y = ground->y;
        p->corners[2].z = ground->z;
    }
    ground->x = ground_x;
    ground->y = 0.0f;
    ground->z = ground_z;
    top->x = top_x;
    top->y = top_y;
    top->z = top_z;
}

static float strip_height(const struct Particle *p, int wave)
{
    float height = heal_particle(p)->strip_height;
    float h = height * 0.25f * SIN_DEG(wave % 360) + height * 0.75f;
    if (p->age <= 90)
        h = h * SIN_DEG(p->age);
    return h;
}

void heal_effect_shape_column(struct HealEffect *self, struct Particle *p)
{
    if (!p)
        return;
    lay_strip_quad(p, strip_height(p, (4 - p->group) * p->age + PART_INDEX(p) * 34),
                   &self->column_ground, &self->column_top);
}

void heal_effect_shape_cone(struct HealEffect *self, struct Particle *p)
{
    if (!p)
        return;
    lay_strip_quad(p, strip_height(p, (PART_INDEX(p) * 17 + p->age) * 2), &self->cone_ground, &self->cone_top);
}

void heal_effect_face_camera(Vec3 *corners)
{
    int heading = 360 - (int)game_camera_heading();
    int pitch, i;
    pitch = (450 - (int)game_camera_pitch()) % 360;
    for (i = 0; i < 4; i++) {
        float y = corners[i].y, z = corners[i].z;
        corners[i].z = COS_DEG(pitch) * z - SIN_DEG(pitch) * y;
        corners[i].y = COS_DEG(pitch) * y + SIN_DEG(pitch) * z;
    }
    for (i = 0; i < 4; i++) {
        float x = corners[i].x, z = corners[i].z;
        corners[i].x = x * COS_DEG(heading) - z * SIN_DEG(heading);
        corners[i].z = z * COS_DEG(heading) + x * SIN_DEG(heading);
    }
}

void heal_effect_shape_sparkle(struct HealEffect *self, struct Particle *p)
{
    Vec3 v[4];
    float scale;
    int spin, i;

    if (!p)
        return;
    if (strcmp(game_current_map_name(), "ecl_fild01.rsw") == 0 && p->alpha > 100)
        p->alpha = 100;

    spin = heal_particle(p)->sparkle_spin;
    scale = p->scale;
    for (i = 0; i < 4; i++) {
        int a = (spin + i * 90) % 360;
        v[i].x = COS_DEG(a) * scale;
        v[i].y = 0.0f;
        v[i].z = SIN_DEG(a) * scale;
    }
    heal_effect_face_camera(v);

    p->corners[0].x = SPARKLE_OFFSET(p).x + v[3].x;
    p->corners[0].y = SPARKLE_OFFSET(p).y + v[3].y;
    p->corners[0].z = SPARKLE_OFFSET(p).z + v[3].z;
    p->corners[1].x = SPARKLE_OFFSET(p).x + v[0].x;
    p->corners[1].y = SPARKLE_OFFSET(p).y + v[0].y;
    p->corners[1].z = SPARKLE_OFFSET(p).z + v[0].z;
    p->corners[2].x = SPARKLE_OFFSET(p).x + v[2].x;
    p->corners[2].y = SPARKLE_OFFSET(p).y + v[2].y;
    p->corners[2].z = SPARKLE_OFFSET(p).z + v[2].z;
    p->corners[3].x = SPARKLE_OFFSET(p).x + v[1].x;
    p->corners[3].y = SPARKLE_OFFSET(p).y + v[1].y;
    p->corners[3].z = SPARKLE_OFFSET(p).z + v[1].z;
}

static void strip_fade(struct Particle *p, int fade_frame)
{
    int alpha = p->alpha;
    if (fade_frame >= 0 && fade_frame <= p->age) {
        alpha -= 2;
        if (alpha < 0)
            alpha = 0;
    } else if (p->age < 16) {
        alpha += 5;
        if (alpha > 180)
            alpha = 180;
    }
    p->alpha = alpha;
}

void heal_effect_update_column(struct HealEffect *self, struct Particle *p)
{
    if (!p)
        return;
    strip_fade(p, self->column_fade_frame);
    heal_effect_shape_column(self, p);
}

void heal_effect_update_cone(struct HealEffect *self, struct Particle *p)
{
    if (!p)
        return;
    strip_fade(p, self->cone_fade_frame);
    heal_effect_shape_cone(self, p);
}

void heal_effect_update_sparkle(struct HealEffect *self, struct Particle *p)
{
    if (p) {
        int spin = heal_particle(p)->sparkle_spin;
        int age = p->age;
        int alpha = p->alpha;

        heal_particle(p)->sparkle_spin = spin + (spin - 5 >= 0 ? -5 : 355);
        if (age <= 10) {
            alpha += 15;
        } else if (self->base.type != HEAL_TYPE_HEAL) {
            int flash = -(PART_INDEX(p) * 7);
            if (flash + 35 == age || flash + 36 == age)
                set_texture(p, "effect\\pok3.tga");
            else
                set_texture(p, "effect\\pok1.tga");
        }
        alpha -= 3;
        p->alpha = alpha < 0 ? 0 : alpha;
        SPARKLE_OFFSET(p).x = SPARKLE_OFFSET(p).x - 0.0f;
        SPARKLE_OFFSET(p).y = SPARKLE_OFFSET(p).y - heal_particle(p)->sparkle_rise;
        SPARKLE_OFFSET(p).z = SPARKLE_OFFSET(p).z - 0.0f;
    }
    heal_effect_shape_sparkle(self, p);
}

void heal_effect_step_spawns(struct HealEffect *self)
{
    int columns = -1, cones = -1, sparkles = -1, sparkle_until = 50;

    switch (self->base.type) {
    case HEAL_TYPE_FD:  columns = 5; cones = 5; break;
    case HEAL_TYPE_HEAL: columns = 4; sparkles = 4; break;
    case HEAL_TYPE_HEAL2: columns = 0; cones = 0; sparkles = 0; break;
    case HEAL_TYPE_HEAL3: columns = 3; sparkles = 3; sparkle_until = 25; break;
    case HEAL_TYPE_PURPLE: columns = 6; sparkles = 6; break;
    case HEAL_TYPE_CYAN: columns = 7; sparkles = 7; break;
    case HEAL_TYPE_YELLOW: columns = 8; sparkles = 8; break;
    case HEAL_TYPE_HEAL4:
        if (self->spawn_frame == 0) {
            heal_effect_spawn_columns(self, 1);
            heal_effect_spawn_cones(self, 1);
        }
        if (self->spawn_frame == 1) {
            heal_effect_spawn_columns(self, 2);
            heal_effect_spawn_cones(self, 2);
        }
        if (self->spawn_frame <= 50) {
            if (self->spawn_frame % 6 == 0)
                heal_effect_spawn_sparkles(self, 1);
            if (self->spawn_frame % 9 == 2)
                heal_effect_spawn_sparkles(self, 2);
        }
        break;
    default:
        break;
    }

    if (self->spawn_frame == 0) {
        if (columns >= 0)
            heal_effect_spawn_columns(self, columns);
        if (cones >= 0)
            heal_effect_spawn_cones(self, cones);
    }
    if (sparkles >= 0 && self->spawn_frame <= sparkle_until && self->spawn_frame % 6 == 0)
        heal_effect_spawn_sparkles(self, sparkles);
    self->spawn_frame++;
}

void heal_render_strip_quad(struct Particle *p)
{
    struct ParticleRenderArgs args;
    int index;

    if (!p)
        return;
    index = PART_INDEX(p);
    if (index <= 0)
        return;
    args.facing_fixed = 0;
    args.v_top = 0.0f;
    args.v_bottom = 1.0f;
    args.u_left = (float)(index - 1) / 21.0f;
    args.u_right = (float)index / 21.0f;
    args.map_chain_flag = 0;
    args.with_depth = 1;
    particle_render_with(p, &args);
}

void heal_render_sparkle(struct Particle *p)
{
    if (p)
        particle_render(p, 0, 0.0f, 1.0f, 0.0f, 1.0f, 1);
}

void heal_effect_init(struct Effect *effect)
{
    effect->duration = HEAL_DURATION;
}

void heal_effect_after_init(struct Effect *effect)
{
    switch (effect->type) {
    case HEAL_TYPE_FD:
        effect_add_sound_cue(effect, SOUND_CUE_WAVE, "effect\\흡기.wav", 0, 0.0f);
        break;
    case HEAL_TYPE_HEAL:
    case HEAL_TYPE_HEAL2:
    case HEAL_TYPE_HEAL3:
    case HEAL_TYPE_HEAL4:
        effect_add_sound_cue(effect, SOUND_CUE_WAVE, "_heal_effect.wav", 0, 0.0f);
        break;
    default:
        break;
    }
}

void heal_effect_process(struct Effect *effect)
{
    struct HealEffect *self = (struct HealEffect *)effect;
    struct Particle **it;

    heal_effect_step_spawns(self);
    for (it = self->columns.first; it != (struct Particle **)self->columns.last; it++) {
        struct Particle *p = *it;
        p->pos = self->base.pos;
        heal_effect_update_column(self, p);
        p->age++;
    }
    for (it = self->cones.first; it != (struct Particle **)self->cones.last; it++) {
        struct Particle *p = *it;
        p->pos = self->base.pos;
        heal_effect_update_cone(self, p);
        p->age++;
    }
    for (it = self->sparkles.first; it != (struct Particle **)self->sparkles.last; it++) {
        struct Particle *p = *it;
        p->pos = self->base.pos;
        heal_effect_update_sparkle(self, p);
        p->age++;
    }
}

void heal_effect_render(struct Effect *effect)
{
    struct HealEffect *self = (struct HealEffect *)effect;
    struct Particle **it;
    for (it = self->columns.first; it != (struct Particle **)self->columns.last; it++)
        heal_render_strip_quad(*it);
    for (it = self->cones.first; it != (struct Particle **)self->cones.last; it++)
        heal_render_strip_quad(*it);
    for (it = self->sparkles.first; it != (struct Particle **)self->sparkles.last; it++)
        heal_render_sparkle(*it);
}

void heal_effect_release(struct Effect *effect)
{
    struct HealEffect *self = (struct HealEffect *)effect;
    struct Particle **it;
    for (it = self->columns.first; it != (struct Particle **)self->columns.last; it++)
        particle_free(*it);
    self->columns.last = self->columns.first;
    for (it = self->cones.first; it != (struct Particle **)self->cones.last; it++)
        particle_free(*it);
    self->cones.last = self->cones.first;
    for (it = self->sparkles.first; it != (struct Particle **)self->sparkles.last; it++)
        particle_free(*it);
    self->sparkles.last = self->sparkles.first;
}
