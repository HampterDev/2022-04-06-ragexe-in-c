#include "effects/effect_allocator.h"
#include <windows.h>
#include <mmsystem.h>
#include <math.h>
#include <string.h>
#include "effects/particle.h"
#include "effects/skills/skill_view.h"
#include "std/list.h"

static const double k_twoPiAsFloat = 6.2831840515136719;

struct HawkVumerangEffect {
    struct Effect base;
    struct List trail;
    struct List landed;
    __int64 copies;
    double full_turn;
    Vec3 offset;
    unsigned start_ms;
    Vec3 from;
    Vec3 to;
    Vec3 centre;
    double radius;
    double start_angle;
};

#define EFFECT_HAWK_BOOMERANG 0x7b0
#define HAWK_COPIES 10

void hawk_vumerang_effect_init(struct Effect *self);
void hawk_vumerang_effect_process(struct Effect *self);
void hawk_vumerang_effect_render(struct Effect *self);
void hawk_vumerang_effect_release(struct Effect *self);
unsigned char hawk_vumerang_effect_is_alive(struct Effect *self);
static void hawk_vumerang_effect_destroy(struct Effect *effect);

static const struct EffectOps hawk_vumerang_effect_ops = {
    .set_submit = effect_set_submit,
    .init = hawk_vumerang_effect_init,
    .update_range = effect_update_range,
    .process = hawk_vumerang_effect_process,
    .render = hawk_vumerang_effect_render,
    .release = hawk_vumerang_effect_release,
    .is_alive = hawk_vumerang_effect_is_alive,
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
    .destroy = hawk_vumerang_effect_destroy,
    .fade_out = effect_fade_out,
};

static void render_anim(struct Ez2StrParticle *p)
{
    ez2_str_particle_render_current(p);
}

static struct Ez2StrParticle *pop_last_particle(struct List *list)
{
    struct ListNode *node = list->head->prev;
    struct Ez2StrParticle *p = *LIST_VALUE(node, struct Ez2StrParticle *);

    list_erase(list, node, NULL);
    return p;
}

static void free_particles(struct List *list)
{
    struct ListNode *node;

    for (node = list->head->next; node != list->head; node = node->next)
        particle_free(&(*LIST_VALUE(node, struct Ez2StrParticle *))->base);
    list_clear(list, NULL);
}

static float spin_for(float angle)
{
    return 360.0f - angle / 3.141592f * 180.0f;
}

struct HawkVumerangEffect *hawk_vumerang_effect_construct(struct HawkVumerangEffect *self)
{
    effect_construct(&self->base, EFFECT_HAWK_BOOMERANG);
    self->base.ops = &hawk_vumerang_effect_ops;
    list_init(&self->trail);
    list_init(&self->landed);
    self->copies = HAWK_COPIES;
    self->full_turn = k_twoPiAsFloat;
    self->offset.x = 0.0f;
    self->offset.y = -17.0f;
    self->offset.z = 0.0f;
    self->from.x = self->from.y = self->from.z = 0.0f;
    self->to.x = self->to.y = self->to.z = 0.0f;
    self->centre.x = self->centre.y = self->centre.z = 0.0f;
    return self;
}

struct Effect *hawk_vumerang_effect_new(void)
{
    struct HawkVumerangEffect *self = mem_try_alloc(sizeof *self);

    return self ? &hawk_vumerang_effect_construct(self)->base : NULL;
}

void hawk_vumerang_effect_destruct(struct HawkVumerangEffect *self)
{
    self->base.ops = &hawk_vumerang_effect_ops;
    list_free(&self->landed, NULL);
    list_free(&self->trail, NULL);
    effect_destruct(&self->base);
}

static void hawk_vumerang_effect_destroy(struct Effect *effect)
{
    struct HawkVumerangEffect *self = (struct HawkVumerangEffect *)effect;
    hawk_vumerang_effect_destruct(self);
    mem_free(self);
}

void hawk_vumerang_effect_release(struct Effect *effect)
{
    struct HawkVumerangEffect *self = (struct HawkVumerangEffect *)effect;
    free_particles(&self->landed);
    free_particles(&self->trail);
}

void hawk_vumerang_effect_init(struct Effect *effect)
{
    struct HawkVumerangEffect *self = (struct HawkVumerangEffect *)effect;
    float dx, dy, dz;
    __int64 i;

    self->start_ms = timeGetTime();
    self->from = self->base.pos;
    self->to = self->base.target_pos;
    self->centre.y = (self->from.y + self->to.y) * 0.5f;
    self->centre.z = (self->from.z + self->to.z) * 0.5f;
    self->centre.x = (self->to.x + self->from.x) * 0.5f;
    dx = self->to.x - self->from.x;
    dy = self->to.y - self->from.y;
    dz = self->to.z - self->from.z;
    self->radius = (double)float_sqrt(dy * dy + dx * dx + dz * dz) * 0.5;
    self->start_angle = acos((double)(self->from.x - self->centre.x) / self->radius);
    if (self->centre.z > self->from.z)
        self->start_angle = k_twoPiAsFloat - self->start_angle;

    for (i = 0; i < self->copies; i++) {
        struct Ez2StrParticle *p = ez2str_particle_new(self->base.pos, 5);

        ez2_str_particle_load(p, "hawkvumerang\\hawkvumerang\\hawkvumerang.str");
        p->spin_degrees = spin_for((float)self->start_angle);
        p->loop_forever = 1;
        list_push_back(&self->trail, &p, sizeof p);
    }
    effect_add_sound_cue(&self->base, SOUND_CUE_WAVE, "effect\\WH_HAWKBOOMERANG.wav", 0, 0.0f);
}

unsigned char hawk_vumerang_effect_is_alive(struct Effect *effect)
{
    struct HawkVumerangEffect *self = (struct HawkVumerangEffect *)effect;
    return self->trail.size != 0;
}

void hawk_vumerang_effect_process(struct Effect *effect)
{
    struct HawkVumerangEffect *self = (struct HawkVumerangEffect *)effect;
    struct Ez2StrParticle *p;
    struct ListNode *node;
    double turned, angle;
    __int64 n;

    if (self->trail.size == 0)
        return;
    p = pop_last_particle(&self->trail);
    turned = (double)(unsigned)(timeGetTime() - self->start_ms) / (double)(self->base.duration * 2) * self->full_turn;
    if (self->full_turn > turned) {
        float a, x, z;

        angle = self->start_angle + turned;
        if (angle > k_twoPiAsFloat)
            angle = angle - k_twoPiAsFloat;
        a = (float)angle;
        p->spin_degrees = spin_for(a);
        x = (float)((double)float_cos(a) * self->radius);
        z = (float)((double)float_sin((float)angle) * self->radius);
        p->base.pos.x = x + self->centre.x + self->offset.x;
        p->base.pos.y = self->centre.y + 0.0f + self->offset.y;
        p->base.pos.z = z + self->centre.z + self->offset.z;
        list_push_front(&self->trail, &p, sizeof p);
    } else {
        list_push_back(&self->landed, &p, sizeof p);
    }

    n = self->copies - (__int64)self->trail.size;
    for (node = self->trail.head->next; node != self->trail.head; node = node->next, n++) {
        struct Ez2StrParticle *copy = *LIST_VALUE(node, struct Ez2StrParticle *);
        Vec3 zero = { 0.0f, 0.0f, 0.0f };
        float step, scale;

        if (!copy)
            continue;
        step = 1.0f / (float)(double)self->copies;
        scale = 1.0f - step * (float)(double)n;
        copy->zoom = scale;
        copy->alpha_scale = scale;
        ez2_str_particle_process(copy, zero);
    }
}

void hawk_vumerang_effect_render(struct Effect *effect)
{
    struct HawkVumerangEffect *self = (struct HawkVumerangEffect *)effect;
    struct ListNode *node;

    for (node = self->trail.head->next; node != self->trail.head; node = node->next) {
        struct Ez2StrParticle *p = *LIST_VALUE(node, struct Ez2StrParticle *);
        if (p)
            render_anim(p);
    }
}
