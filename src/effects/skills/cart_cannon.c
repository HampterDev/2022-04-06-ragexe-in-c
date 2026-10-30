#include "game/camera.h"
#include "effects/effect_allocator.h"
#include "session/state.h"
#include <math.h>
#include <string.h>
#include "effects/particle.h"
#include "effects/skills/skill_view.h"

struct CartCannon {
    struct Effect base;
    struct Ez2StrParticle *ball;
    int frames;
    int delay_ms;
    float speed;
    Vec3 ball_pos;
    Vec3 step;
    Vec3 start;
};

void cart_cannon_init(struct Effect *self);
void cart_cannon_process(struct Effect *self);
void cart_cannon_render(struct Effect *self);
void cart_cannon_release(struct Effect *self);
unsigned char cart_cannon_is_alive(struct Effect *self);
void cart_cannon_set_delay(struct Effect *self, int ms);
static void cart_cannon_destroy(struct Effect *effect);

static const struct EffectOps cart_cannon_ops = {
    .set_submit = effect_set_submit,
    .init = cart_cannon_init,
    .update_range = effect_update_range,
    .process = cart_cannon_process,
    .render = cart_cannon_render,
    .release = cart_cannon_release,
    .is_alive = cart_cannon_is_alive,
    .restart = effect_restart,
    .set_color = effect_set_color,
    .set_texture = effect_set_texture,
    .after_init = effect_after_init,
    .skip = effect_skip,
    .set_stop_frame = effect_set_stop_frame,
    .set_start_delay = cart_cannon_set_delay,
    .set_spin = effect_set_spin,
    .set_str_name = effect_set_str_name,
    .set_list_flags = effect_set_list_flags,
    .set_zoom = effect_set_zoom,
    .set_alpha_scale = effect_set_alpha_scale,
    .destroy = cart_cannon_destroy,
    .fade_out = effect_fade_out,
};

static const Vec3 no_offset = { 0, 0, 0 };

static void render_with_view(struct Ez2StrParticle *p)
{
    ez2_str_particle_render_current(p);
}

struct CartCannon *cart_cannon_construct(struct CartCannon *self)
{
    effect_construct(&self->base, -1);
    self->base.ops = &cart_cannon_ops;
    self->ball_pos.x = self->ball_pos.y = self->ball_pos.z = 0;
    self->step.x = self->step.y = self->step.z = 0;
    self->start.x = self->start.y = self->start.z = 0;
    self->base.params.rise_angle = 0;
    return self;
}

struct Effect *cart_cannon_new(void)
{
    struct CartCannon *self = mem_try_alloc(sizeof *self);

    return self ? &cart_cannon_construct(self)->base : NULL;
}

void cart_cannon_destruct(struct CartCannon *self)
{
    self->base.ops = &cart_cannon_ops;
    effect_destruct(&self->base);
}

static void cart_cannon_destroy(struct Effect *effect)
{
    struct CartCannon *self = (struct CartCannon *)effect;
    cart_cannon_destruct(self);
    mem_free(self);
}

void cart_cannon_release(struct Effect *effect)
{
    struct CartCannon *self = (struct CartCannon *)effect;
    particle_free(&self->ball->base);
}

void cart_cannon_init(struct Effect *effect)
{
    struct CartCannon *self = (struct CartCannon *)effect;
    struct Ez2StrParticle *ball;
    const char *name;
    Vec3 target;
    float target_y, dist, dx, dy, dz, inv_len, angle;
    double heading;

    self->ball_pos = self->base.pos;
    ball = ez2str_particle_new(self->ball_pos, 5);
    if (g_session1.map_rule_flags[0] == 1)
        name = "mineffect\\new_cart_cannon\\new_cart_cannon\\new_cart_cannon.str";
    else
        name = "new_cart_cannon\\new_cart_cannon\\new_cart_cannon.str";
    ez2_str_particle_load(ball, name);
    ball->loop_forever = 1;

    self->ball = ball;
    self->speed = 1.0f;
    self->delay_ms = 0;
    self->ball_pos.y -= 10.0f;
    self->frames = 0;

    target = self->base.target_pos;
    target_y = target.y - 10.0f;
    self->step.x = target.x - self->ball_pos.x;
    self->step.y = target_y - self->ball_pos.y;
    self->step.z = target.z - self->ball_pos.z;
    dist = float_sqrt(self->step.y * self->step.y + self->step.x * self->step.x + self->step.z * self->step.z);
    self->step.y /= 10.0f;
    self->step.z /= 10.0f;
    self->step.x /= 10.0f;
    if (40.0f > dist) {
        self->speed = 40.0f / dist;
        self->step.x = self->speed * self->step.x;
        self->step.y = self->speed * self->step.y;
        self->step.z = self->speed * self->step.z;
    }

    dx = target.x - self->ball_pos.x;
    dy = target_y - self->ball_pos.y;
    dz = target.z - self->ball_pos.z;
    inv_len = 1.0f / float_sqrt(dx * dx + dy * dy + dz * dz);
    self->ball_pos.y = dy * inv_len * 4.0f + self->ball_pos.y;
    self->ball_pos.x = inv_len * dx * 4.0f + self->ball_pos.x;
    self->ball_pos.z = inv_len * dz * 4.0f + self->ball_pos.z;
    self->start = self->ball_pos;
    self->base.pos = self->ball_pos;

    heading = atan2((double)(target.z - self->ball_pos.z), (double)(target.x - self->ball_pos.x));
    angle = game_camera_target_heading() + (float)heading / 3.141592f * 180.0f;
    if (0.0f > angle)
        angle += 360.0f;
    angle = 270.0f - angle;
    if (0.0f > angle)
        angle += 360.0f;
    self->ball->spin_degrees = angle;
}

unsigned char cart_cannon_is_alive(struct Effect *effect)
{
    struct CartCannon *self = (struct CartCannon *)effect;
    return !((float)self->frames > 10.0f / self->speed);
}

static void place(struct CartCannon *self, struct Ez2StrParticle *ball)
{
    float n = (float)self->frames;
    self->ball_pos.x = self->step.x * n + self->start.x;
    self->ball_pos.y = self->step.y * n + self->start.y;
    self->ball_pos.z = self->step.z * n + self->start.z;
    self->base.pos = self->ball_pos;
    ball->base.pos = self->ball_pos;
    ez2_str_particle_process(ball, no_offset);
}

void cart_cannon_process(struct Effect *effect)
{
    struct CartCannon *self = (struct CartCannon *)effect;
    if (self->delay_ms > 0) {
        self->delay_ms -= 16;
        return;
    }
    if (self->ball)
        place(self, self->ball);
    self->frames++;
}

void cart_cannon_render(struct Effect *effect)
{
    struct CartCannon *self = (struct CartCannon *)effect;
    if (self->ball)
        render_with_view(self->ball);
}

void cart_cannon_set_delay(struct Effect *effect, int ms)
{
    struct CartCannon *self = (struct CartCannon *)effect;
    self->delay_ms = ms;
}
