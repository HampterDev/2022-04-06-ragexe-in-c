#include "effects/effect_allocator.h"
#include "render/math3d.h"
#include "contents/sound.h"
#include <math.h>
#include <string.h>
#include "effects/particle.h"
#include "effects/render_object.h"
#include "effects/skills/skill_view.h"

enum {
    ACIDIFIED_FIRE = 2206,
    ACIDIFIED_WATER = 2214,
    ACIDIFIED_WIND = 2222,
    ACIDIFIED_GROUND = 2230,
};

#define ACIDIFIED_LAYERS 3

struct AcidifiedThrow {
    struct Effect base;
    struct Ez2StrParticle *bottle;
    struct Ez2StrParticle *layers[ACIDIFIED_LAYERS];
    unsigned char alive;
    float flight_left;
    Vec3 land_pos;
};

void acidified_throw_init(struct Effect *self);
void acidified_throw_process(struct Effect *self);
void acidified_throw_render(struct Effect *self);
void acidified_throw_release(struct Effect *self);
unsigned char acidified_throw_is_alive(struct Effect *self);
static void acidified_throw_destroy(struct Effect *effect);

static const struct EffectOps acidified_throw_ops = {
    .set_submit = effect_set_submit,
    .init = acidified_throw_init,
    .update_range = effect_update_range,
    .process = acidified_throw_process,
    .render = acidified_throw_render,
    .release = acidified_throw_release,
    .is_alive = acidified_throw_is_alive,
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
    .destroy = acidified_throw_destroy,
    .fade_out = effect_fade_out,
};

static const Vec3 no_offset = { 0, 0, 0 };

static const Vec3 *player_position(void)
{
    const struct RenderObject *player = (const struct RenderObject *)game_current_player();
    return player ? &player->pos : NULL;
}

static void transform(Vec3 *out, const float *m, const Vec3 *p)
{
    float x = p->x, y = p->y, z = p->z;
    out->x = y * m[3] + x * m[0] + z * m[6] + m[9];
    out->y = y * m[4] + x * m[1] + z * m[7] + m[10];
    out->z = y * m[5] + x * m[2] + z * m[8] + m[11];
}

static void load_str(struct Ez2StrParticle *p, const char *file)
{
    ez2_str_particle_open(p, file);
}

static void play_wave(const char *wave, float x, float z)
{
    sound_play_wave(wave, x, 0.0f, z, 250, 40, 1.0f);
}

void acidified_bezier(Vec3 *out, Vec3 p0, Vec3 p1, Vec3 p2, float t)
{
    float s = 1.0f - t;
    float a = (float)pow((double)s, 2.0);
    float b = (float)pow((double)t, 2.0);
    float c = (t + t) * s;
    out->x = c * p1.x + b * p2.x + a * p0.x;
    out->y = c * p1.y + b * p2.y + a * p0.y;
    out->z = c * p1.z + b * p2.z + a * p0.z;
}

static void acidified_throw_destroy(struct Effect *effect)
{
    struct AcidifiedThrow *self = (struct AcidifiedThrow *)effect;
    self->base.ops = &acidified_throw_ops;
    effect_destruct(&self->base);
    mem_free(self);
}

void acidified_throw_release(struct Effect *effect)
{
    struct AcidifiedThrow *self = (struct AcidifiedThrow *)effect;
    int i;
    if (self->bottle) {
        particle_free(&self->bottle->base);
        self->bottle = 0;
    }
    for (i = 0; i < ACIDIFIED_LAYERS; i++)
        if (self->layers[i]) {
            particle_free(&self->layers[i]->base);
            self->layers[i] = 0;
        }
}

void acidified_throw_init(struct Effect *effect)
{
    struct AcidifiedThrow *self = (struct AcidifiedThrow *)effect;
    self->bottle = ez2str_particle_new(self->base.pos, 5);
    self->layers[0] = ez2str_particle_new(self->base.pos, 5);
    self->layers[1] = ez2str_particle_new(self->base.pos, 0xd);
    self->layers[2] = ez2str_particle_new(self->base.pos, 5);
    if (!self->bottle || !self->layers[0] || !self->layers[1] || !self->layers[2])
        return;

    switch (self->base.type) {
    case ACIDIFIED_FIRE:
        load_str(self->bottle, "acidified_zone_fire\\acidified_zone_fire_c\\acidified_zone_fire_c.str");
        load_str(self->layers[0], "acidified_zone_fire\\acidified_zone_fire_b\\acidified_zone_fire_b.str");
        load_str(self->layers[1], "acidified_zone_fire\\acidified_zone_fire_b_bottom\\acidified_zone_fire_b_bottom.str");
        load_str(self->layers[2], "acidified_zone_fire\\acidified_zone_fire_water\\acidified_zone_fire_water.str");
        break;
    case ACIDIFIED_WATER:
        load_str(self->bottle, "acidified_zone_water\\acidified_zone_water_c\\acidified_zone_water_c.str");
        load_str(self->layers[0], "acidified_zone_water\\acidified_zone_water_b\\acidified_zone_water_b.str");
        load_str(self->layers[1], "acidified_zone_water\\acidified_zone_water_b_bottom\\acidified_zone_water_b_bottom.str");
        load_str(self->layers[2], "acidified_zone_water\\acidified_zone_water_water\\acidified_zone_water_water.str");
        break;
    case ACIDIFIED_WIND:
        load_str(self->bottle, "acidified_zone_wind\\acidified_zone_wind_c\\acidified_zone_wind_c.str");
        load_str(self->layers[0], "acidified_zone_wind\\acidified_zone_wind_b\\acidified_zone_wind_b.str");
        load_str(self->layers[1], "acidified_zone_wind\\acidified_zone_wind_b_bottom\\acidified_zone_wind_b_bottom.str");
        load_str(self->layers[2], "acidified_zone_wind\\acidified_zone_wind_water\\acidified_zone_wind_water.str");
        break;
    case ACIDIFIED_GROUND:
        load_str(self->bottle, "acidified_zone_ground\\acidified_zone_ground_c\\acidified_zone_ground_c.str");
        load_str(self->layers[0], "acidified_zone_ground\\acidified_zone_ground_b\\acidified_zone_ground_b.str");
        load_str(self->layers[1],
                 "acidified_zone_ground\\acidified_zone_ground_b_bottom\\acidified_zone_ground_b_bottom.str");
        load_str(self->layers[2], "acidified_zone_ground\\acidified_zone_ground_water\\acidified_zone_ground_water.str");
        break;
    }
    self->bottle->loop_forever = 1;
    self->land_pos.x = self->land_pos.y = self->land_pos.z = 0;
    self->flight_left = 1.0f;
    self->alive = 1;
}

unsigned char acidified_throw_is_alive(struct Effect *effect)
{
    struct AcidifiedThrow *self = (struct AcidifiedThrow *)effect;
    return self->alive;
}

static void fly(struct AcidifiedThrow *self, const Vec3 *caster)
{
    float view[12], inverse[12], turn[12];
    Vec3 from, top, way, ctrl, land;
    float land_view_x, dx, dy, dz, len_sq, len, inv_len;
    const Vec3 *player;

    if (!game_copy_view_matrix(view))
        return;
    transform(&from, view, caster);
    land_view_x = self->land_pos.y * view[3] + self->land_pos.x * view[0] + self->land_pos.z * view[6] + view[9];
    mat_set_rotate_y(turn, land_view_x >= from.x ? 4.1887894f : 2.0943947f);
    from.y -= 10.0f;
    mat_invert(inverse, view);
    transform(&top, inverse, &from);

    self->flight_left -= 0.07f;
    dx = self->land_pos.x - top.x;
    dy = self->land_pos.y - top.y;
    dz = self->land_pos.z - top.z;
    len_sq = dy * dy + dx * dx + dz * dz;
    len = float_sqrt(len_sq);
    inv_len = 1.0f / float_sqrt(len_sq);
    way.x = inv_len * dx * len;
    way.y = inv_len * dy * len;
    way.z = inv_len * dz * len;
    transform(&ctrl, turn, &way);
    land = self->land_pos;
    ctrl.x += land.x;
    ctrl.y += land.y;
    ctrl.z += land.z;
    acidified_bezier(&self->bottle->base.pos, land, ctrl, top, self->flight_left);
    ez2_str_particle_process(self->bottle, no_offset);

    if (!(self->flight_left <= 0.0f))
        return;
    particle_free(&self->bottle->base);
    self->bottle = 0;
    player = player_position();
    if (!player)
        return;
    dx = player->x - self->land_pos.x;
    dz = player->z - self->land_pos.z;
    play_wave("effect\\BO_ACIDIFIED_ZONE.wav", dx, dz);
    switch (self->base.type) {
    case ACIDIFIED_FIRE:   play_wave("effect\\BO_ACIDIFIED_ZONE_FIRE.wav", dx, dz); break;
    case ACIDIFIED_WATER:  play_wave("effect\\BO_ACIDIFIED_ZONE_WATER.wav", dx, dz); break;
    case ACIDIFIED_WIND:   play_wave("effect\\BO_ACIDIFIED_ZONE_WIND.wav", dx, dz); break;
    case ACIDIFIED_GROUND: play_wave("effect\\BO_ACIDIFIED_ZONE_GROUND.wav", dx, dz); break;
    }
}

void acidified_throw_process(struct Effect *effect)
{
    struct AcidifiedThrow *self = (struct AcidifiedThrow *)effect;
    void *caster = effect_find_actor(self->base.owner_aid);
    void *target;
    int i, playing = 0;

    if (!caster) {
        self->alive = 0;
        return;
    }
    target = effect_find_actor(self->base.target_aid);
    if (target)
        self->land_pos = *ACTOR_POS(target);
    if (self->bottle) {
        fly(self, ACTOR_POS(caster));
        return;
    }

    for (i = 0; i < ACIDIFIED_LAYERS; i++) {
        struct Particle *layer = &self->layers[i]->base;
        if (!layer)
            continue;
        if (layer->life_seconds != 0.0f && !(layer->life_seconds * 60.0f > (float)layer->age)) {
            particle_free(layer);
            self->layers[i] = 0;
            continue;
        }
        layer->pos.x = self->land_pos.x - 1.0f;
        layer->pos.y = self->land_pos.y - 14.0f;
        layer->pos.z = self->land_pos.z;
        ez2_str_particle_process(self->layers[i], no_offset);
        playing++;
    }
    if (!playing)
        self->alive = 0;
}

void acidified_throw_render(struct Effect *effect)
{
    struct AcidifiedThrow *self = (struct AcidifiedThrow *)effect;
    int i;

    if (self->bottle) {
        ez2_str_particle_render_current(self->bottle);
        return;
    }
    for (i = 0; i < ACIDIFIED_LAYERS; i++)
        if (self->layers[i]) {
            ez2_str_particle_render_current(self->layers[i]);
        }
}

struct AcidifiedThrow *acidified_throw_construct(struct AcidifiedThrow *self)
{
    effect_construct(&self->base, -1);
    self->base.ops = &acidified_throw_ops;
    memset(&self->land_pos, 0, sizeof(self->land_pos));
    return self;
}

struct Effect *acidified_throw_new(void)
{
    struct AcidifiedThrow *self = mem_try_alloc(sizeof *self);

    return self ? &acidified_throw_construct(self)->base : NULL;
}
