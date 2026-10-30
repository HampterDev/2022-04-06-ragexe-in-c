#include "game/camera.h"
#include "effects/effect_allocator.h"
#include "session/state.h"
#include <math.h>
#include <string.h>
#include "effects/particle.h"
#include "effects/skills/skill_view.h"

#define FLIGHT_FRAMES 8
#define MIN_FLIGHT 40.0f
#define ARROW_HEIGHT 10.0f
#define START_OFFSET 4.0f
#define FRAME_MS 16

struct TriangleShotArrow {
    struct Effect base;
    struct Ez2StrParticle *arrow;
    int flight_frame;
    int delay_ms;
    float speed_scale;
    Vec3 pos;
    Vec3 step;
    Vec3 start;
};

void triangle_shot_arrow_init(struct Effect *self);
void triangle_shot_arrow_process(struct Effect *self);
void triangle_shot_arrow_render(struct Effect *self);
void triangle_shot_arrow_release(struct Effect *self);
unsigned char triangle_shot_arrow_is_alive(struct Effect *self);
void triangle_shot_arrow_set_delay(struct Effect *self, int delay_ms);
static void triangle_shot_arrow_destroy(struct Effect *effect);

static const struct EffectOps triangle_shot_arrow_ops = {
    .set_submit = effect_set_submit,
    .init = triangle_shot_arrow_init,
    .update_range = effect_update_range,
    .process = triangle_shot_arrow_process,
    .render = triangle_shot_arrow_render,
    .release = triangle_shot_arrow_release,
    .is_alive = triangle_shot_arrow_is_alive,
    .restart = effect_restart,
    .set_color = effect_set_color,
    .set_texture = effect_set_texture,
    .after_init = effect_after_init,
    .skip = effect_skip,
    .set_stop_frame = effect_set_stop_frame,
    .set_start_delay = triangle_shot_arrow_set_delay,
    .set_spin = effect_set_spin,
    .set_str_name = effect_set_str_name,
    .set_list_flags = effect_set_list_flags,
    .set_zoom = effect_set_zoom,
    .set_alpha_scale = effect_set_alpha_scale,
    .destroy = triangle_shot_arrow_destroy,
    .fade_out = effect_fade_out,
};

struct TriangleShotArrow *triangle_shot_arrow_construct(struct TriangleShotArrow *self)
{
    effect_construct(&self->base, -1);
    self->base.ops = &triangle_shot_arrow_ops;
    self->pos.x = self->pos.y = self->pos.z = 0;
    self->step.x = self->step.y = self->step.z = 0;
    self->start.x = self->start.y = self->start.z = 0;
    self->base.params.rise_angle = 0;
    return self;
}

struct Effect *triangle_shot_arrow_new(void)
{
    struct TriangleShotArrow *self = mem_try_alloc(sizeof *self);

    return self ? &triangle_shot_arrow_construct(self)->base : NULL;
}

void triangle_shot_arrow_destruct(struct TriangleShotArrow *self)
{
    self->base.ops = &triangle_shot_arrow_ops;
    effect_destruct(&self->base);
}

static void triangle_shot_arrow_destroy(struct Effect *effect)
{
    struct TriangleShotArrow *self = (struct TriangleShotArrow *)effect;
    triangle_shot_arrow_destruct(self);
    mem_free(self);
}

void triangle_shot_arrow_init(struct Effect *effect)
{
    struct TriangleShotArrow *self = (struct TriangleShotArrow *)effect;
    const char *str;
    struct Ez2StrParticle *p;
    Vec3 target;
    float target_y, dx, dy, dz, distance, inv, degrees;
    double heading;

    self->pos = self->base.pos;
    p = ez2str_particle_new(self->pos, 5);
    str = g_session1.map_rule_flags[0] == 1 ? "mineffect\\new_triangleshot\\new_triangleshot_arrow\\new_triangleshot_arrow.str"
                            : "new_triangleshot\\new_triangleshot_arrow\\new_triangleshot_arrow.str";
    ez2_str_particle_load(p, str);
    p->loop_forever = 1;

    self->pos.y = self->pos.y - ARROW_HEIGHT;
    self->arrow = p;
    self->speed_scale = 1.0f;
    self->delay_ms = 0;
    self->flight_frame = 0;

    target = self->base.target_pos;
    target_y = target.y - ARROW_HEIGHT;
    self->step.x = target.x - self->pos.x;
    self->step.y = target_y - self->pos.y;
    self->step.z = target.z - self->pos.z;
    distance = float_sqrt(self->step.y * self->step.y + self->step.x * self->step.x + self->step.z * self->step.z);
    self->step.x = self->step.x * 0.125f;
    self->step.y = self->step.y * 0.125f;
    self->step.z = self->step.z * 0.125f;
    if (MIN_FLIGHT > distance) {
        self->speed_scale = MIN_FLIGHT / distance;
        self->step.x = self->speed_scale * self->step.x;
        self->step.y = self->speed_scale * self->step.y;
        self->step.z = self->speed_scale * self->step.z;
    }

    dx = target.x - self->pos.x;
    dy = target_y - self->pos.y;
    dz = target.z - self->pos.z;
    inv = 1.0f / float_sqrt(dx * dx + dy * dy + dz * dz);
    self->pos.y = dy * inv * START_OFFSET + self->pos.y;
    self->pos.x = inv * dx * START_OFFSET + self->pos.x;
    self->pos.z = inv * dz * START_OFFSET + self->pos.z;
    self->start = self->pos;
    self->base.pos = self->pos;

    heading = atan2((double)(target.z - self->pos.z), (double)(target.x - self->pos.x));
    degrees = (float)heading / 3.141592f * 180.0f;
    degrees = game_camera_target_heading() + degrees;
    if (0.0f > degrees)
        degrees = degrees + 360.0f;
    self->arrow->spin_degrees = 360.0f - degrees;
}

unsigned char triangle_shot_arrow_is_alive(struct Effect *effect)
{
    struct TriangleShotArrow *self = (struct TriangleShotArrow *)effect;
    float last = (float)FLIGHT_FRAMES / self->speed_scale;
    return !((float)self->flight_frame > last);
}

void triangle_shot_arrow_update_arrow(struct TriangleShotArrow *self,
                                                struct Ez2StrParticle *p)
{
    Vec3 none = { 0.0f, 0.0f, 0.0f };
    float t;

    if (!p)
        return;
    t = (float)self->flight_frame;
    self->pos.x = self->step.x * t + self->start.x;
    self->pos.y = self->step.y * t + self->start.y;
    self->pos.z = self->step.z * t + self->start.z;
    self->base.pos = self->pos;
    p->base.pos = self->pos;
    ez2_str_particle_process(p, none);
}

void triangle_shot_arrow_process(struct Effect *effect)
{
    struct TriangleShotArrow *self = (struct TriangleShotArrow *)effect;
    if (self->delay_ms > 0) {
        self->delay_ms -= FRAME_MS;
        return;
    }
    triangle_shot_arrow_update_arrow(self, self->arrow);
    self->flight_frame++;
}

void triangle_shot_arrow_render_particle(struct Ez2StrParticle *p)
{
    if (!p)
        return;
    ez2_str_particle_render_current(p);
}

void triangle_shot_arrow_render(struct Effect *effect)
{
    struct TriangleShotArrow *self = (struct TriangleShotArrow *)effect;
    triangle_shot_arrow_render_particle(self->arrow);
}

void triangle_shot_arrow_release(struct Effect *effect)
{
    struct TriangleShotArrow *self = (struct TriangleShotArrow *)effect;
    particle_free(&self->arrow->base);
}

void triangle_shot_arrow_set_delay(struct Effect *effect, int delay_ms)
{
    struct TriangleShotArrow *self = (struct TriangleShotArrow *)effect;
    self->delay_ms = delay_ms;
}
