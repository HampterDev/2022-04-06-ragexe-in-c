#include "effects/effect_allocator.h"
#include "render/math3d.h"
#include <string.h>
#include "effects/particle.h"
#include "effects/skills/skill_view.h"
#include "std/tree.h"
#include "effects/effect_controls.h"

#define A_MACHINE_LIST 5

enum AMachineState {
    A_MACHINE_ATTACK_WAITING = 0,
    A_MACHINE_ATTACK = 1,
    A_MACHINE_WAITING = 2,
};

struct StateAnim {
    int state;
    struct Ez2StrParticle *anim;
};

struct AMachineEffect {
    struct Effect base;
    struct Tree anims;
    int state;
};

void a_machine_effect_init(struct Effect *self);
void a_machine_effect_process(struct Effect *self);
void a_machine_effect_render(struct Effect *self);
void a_machine_effect_release(struct Effect *self);
static void a_machine_effect_destroy(struct Effect *effect);

static const struct EffectOps a_machine_effect_ops = {
    .set_submit = effect_set_submit,
    .init = a_machine_effect_init,
    .update_range = effect_update_range,
    .process = a_machine_effect_process,
    .render = a_machine_effect_render,
    .release = a_machine_effect_release,
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
    .destroy = a_machine_effect_destroy,
    .fade_out = effect_fade_out,
};

static struct Ez2StrParticle *open_anim(struct AMachineEffect *self, const char *str)
{
    struct Ez2StrParticle *p = ez2str_particle_new(self->base.pos, A_MACHINE_LIST);
    if (p) {
        ez2_str_particle_open(p, str);
    }
    return p;
}

struct AMachineEffect *a_machine_effect_construct(struct AMachineEffect *self)
{
    effect_construct(&self->base, -1);
    self->base.ops = &a_machine_effect_ops;
    tree_init(&self->anims);
    self->state = A_MACHINE_ATTACK_WAITING;
    return self;
}

struct Effect *a_machine_effect_new(void)
{
    struct AMachineEffect *self = mem_try_alloc(sizeof *self);

    return self ? &a_machine_effect_construct(self)->base : NULL;
}

void a_machine_effect_destruct(struct AMachineEffect *self)
{
    self->base.ops = &a_machine_effect_ops;
    tree_free(&self->anims, NULL);
    effect_destruct(&self->base);
}

static void a_machine_effect_destroy(struct Effect *effect)
{
    struct AMachineEffect *self = (struct AMachineEffect *)effect;
    a_machine_effect_destruct(self);
    mem_free(self);
}

struct Ez2StrParticle *a_machine_effect_get_anim(struct AMachineEffect *self, int state)
{
    struct StateAnim *found = tree_find(&self->anims, &state, tree_less_int);

    return found ? found->anim : 0;
}

static void set_anim(struct AMachineEffect *self, int state, struct Ez2StrParticle *p)
{
    struct StateAnim *entry = tree_get(&self->anims, &state, sizeof state, sizeof *entry, tree_less_int);

    entry->anim = p;
}

void a_machine_effect_play_attack(struct Effect *effect)
{
    struct AMachineEffect *self = (struct AMachineEffect *)effect;
    struct Ez2StrParticle *p = a_machine_effect_get_anim(self, A_MACHINE_ATTACK);
    if (p) {
        ez2_str_particle_rewind(p);
        self->state = A_MACHINE_ATTACK;
    }
}

void a_machine_effect_init(struct Effect *effect)
{
    struct AMachineEffect *self = (struct AMachineEffect *)effect;
    struct Ez2StrParticle *p;

    self->base.has_instance_key = 1;

    p = open_anim(self, "a_machine\\a_machine\\a_machine_attack_waiting.str");
    if (p)
        set_anim(self, A_MACHINE_ATTACK_WAITING, p);
    p = open_anim(self, "a_machine\\a_machine\\a_machine_attack.str");
    if (p)
        set_anim(self, A_MACHINE_ATTACK, p);
    p = open_anim(self, "a_machine\\a_machine\\a_machine_waiting.str");
    if (p) {
        p->loop_forever = 1;
        set_anim(self, A_MACHINE_WAITING, p);
    }
}

void a_machine_effect_process(struct Effect *effect)
{
    struct AMachineEffect *self = (struct AMachineEffect *)effect;
    static const Vec3 no_offset = { 0.0f, 0.0f, 0.0f };
    struct Ez2StrParticle *p = a_machine_effect_get_anim(self, self->state);
    Vec3 pos;
    float m[12], inv[12], vx, vy, vz;

    if (!p)
        return;
    pos = self->base.pos;
    if (!game_copy_view_matrix(m))
        return;
    vx = m[0] * pos.x + m[3] * pos.y + m[6] * pos.z + m[9];
    vy = m[1] * pos.x + m[4] * pos.y + m[7] * pos.z + m[10];
    vz = m[2] * pos.x + m[5] * pos.y + m[8] * pos.z + m[11];
    mat_invert(inv, m);
    vy = vy - 8.0f;
    vx = vx + 0.0f;
    vz = vz + 0.0f;
    p->base.pos.x = vx * inv[0] + vy * inv[3] + vz * inv[6] + inv[9];
    p->base.pos.y = vx * inv[1] + vy * inv[4] + vz * inv[7] + inv[10];
    p->base.pos.z = vx * inv[2] + vy * inv[5] + vz * inv[8] + inv[11];

    if (!ez2_str_particle_process(p, no_offset)) {
        struct Ez2StrParticle *waiting = a_machine_effect_get_anim(self, A_MACHINE_WAITING);
        if (waiting) {
            ez2_str_particle_rewind(waiting);
            self->state = A_MACHINE_WAITING;
        }
    }
}

void a_machine_effect_render(struct Effect *effect)
{
    struct AMachineEffect *self = (struct AMachineEffect *)effect;
    struct Ez2StrParticle *p = a_machine_effect_get_anim(self, self->state);
    if (p) {
        ez2_str_particle_render_current(p);
    }
}

void a_machine_effect_release(struct Effect *effect)
{
    struct AMachineEffect *self = (struct AMachineEffect *)effect;
    struct TreeNode *node;

    for (node = tree_first(&self->anims); node != self->anims.head; node = tree_next(node)) {
        struct StateAnim *entry = TREE_VALUE(node, struct StateAnim);

        if (entry->anim) {
            particle_free(&entry->anim->base);
            entry->anim = 0;
        }
    }
    tree_clear(&self->anims, NULL);
}
