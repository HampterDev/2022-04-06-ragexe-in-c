#include "effects/effect_allocator.h"
#include "render/math3d.h"
#include <string.h>
#include "effects/particle.h"
#include "effects/skills/skill_view.h"
#include "effects/effect_controls.h"
#include "std/tree.h"
#include <stdint.h>

struct BallSlot {
    float zoom;
    Vec3 offset;
};
struct SlotEntry {
    int64_t key;
    struct BallSlot slot;
};

enum BallState {
    BALL_GONE = 0,
    BALL_APPEARING = 1,
    BALL_HOVERING = 2,
    BALL_DYING = 3,
};
struct Ball {
    int64_t key;
    int state;
    struct Ez2StrParticle *anim;
};

struct FromTheAbyssEffect {
    struct Effect base;
    int64_t max_balls;
    int64_t ball_count;
    struct Tree slots;
    struct Tree balls;
};

#define FROM_THE_ABYSS_MAX_BALLS 5

#define PARTICLE_OFFSET(p) ((p)->base.offset)

void from_the_abyss_effect_init(struct Effect *self);
void from_the_abyss_effect_process(struct Effect *self);
void from_the_abyss_effect_render(struct Effect *self);
void from_the_abyss_effect_release(struct Effect *self);
static void from_the_abyss_effect_destroy(struct Effect *effect);

static const struct EffectOps from_the_abyss_effect_ops = {
    .set_submit = effect_set_submit,
    .init = from_the_abyss_effect_init,
    .update_range = effect_update_range,
    .process = from_the_abyss_effect_process,
    .render = from_the_abyss_effect_render,
    .release = from_the_abyss_effect_release,
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
    .destroy = from_the_abyss_effect_destroy,
    .fade_out = effect_fade_out,
};

static Vec3 transform(const float *m, Vec3 v)
{
    Vec3 out;
    out.x = v.x * m[0] + v.y * m[3] + v.z * m[6] + m[9];
    out.y = v.x * m[1] + v.y * m[4] + v.z * m[7] + m[10];
    out.z = v.x * m[2] + v.y * m[5] + v.z * m[8] + m[11];
    return out;
}

static int anim_alive(const struct Ez2StrParticle *p)
{
    float life = p->base.life_seconds;
    return life == 0.0f || life * 60.0f > (float)p->base.age;
}

static struct Ez2StrParticle *open_anim(struct FromTheAbyssEffect *self, const char *name)
{
    struct Ez2StrParticle *p = ez2str_particle_new(self->base.pos, 5);

    if (!p)
        return 0;
    ez2_str_particle_open(p, name);
    return p;
}

static struct BallSlot *slot_at(struct FromTheAbyssEffect *self, int64_t key)
{
    struct SlotEntry *entry = tree_get(&self->slots, &key, sizeof key, sizeof *entry, tree_less_int64);

    return &entry->slot;
}

static struct Ball *ball_at(struct FromTheAbyssEffect *self, int64_t key)
{
    return tree_get(&self->balls, &key, sizeof key, sizeof(struct Ball), tree_less_int64);
}

struct FromTheAbyssEffect *from_the_abyss_effect_construct(struct FromTheAbyssEffect *self)
{
    static const struct BallSlot layout[FROM_THE_ABYSS_MAX_BALLS] = {
        { 0.8f, { -4.8f, -9.1f, 0.0f } },
        { 0.6f, { -3.4f, -12.8f, 0.0f } },
        { 0.5f, { -0.7f, -14.0f, 0.0f } },
        { 0.7f, { 2.7f, -14.0f, 0.0f } },
        { 0.4f, { 4.3f, -10.0f, 0.0f } },
    };
    int64_t key;

    effect_construct(&self->base, -1);
    self->base.ops = &from_the_abyss_effect_ops;
    self->max_balls = FROM_THE_ABYSS_MAX_BALLS;
    self->ball_count = 0;
    tree_init(&self->slots);
    tree_init(&self->balls);
    for (key = 0; key < FROM_THE_ABYSS_MAX_BALLS; key++)
        *slot_at(self, key) = layout[key];
    return self;
}

struct Effect *from_the_abyss_effect_new(void)
{
    struct FromTheAbyssEffect *self = mem_try_alloc(sizeof *self);

    return self ? &from_the_abyss_effect_construct(self)->base : NULL;
}

void from_the_abyss_effect_destruct(struct FromTheAbyssEffect *self)
{
    self->base.ops = &from_the_abyss_effect_ops;
    tree_free(&self->balls, NULL);
    tree_free(&self->slots, NULL);
    effect_destruct(&self->base);
}

static void from_the_abyss_effect_destroy(struct Effect *effect)
{
    struct FromTheAbyssEffect *self = (struct FromTheAbyssEffect *)effect;
    from_the_abyss_effect_destruct(self);
    mem_free(self);
}

void from_the_abyss_effect_create_ball(struct FromTheAbyssEffect *self, int64_t key)
{
    struct Ball *ball = tree_find(&self->balls, &key, tree_less_int64);
    struct Ez2StrParticle *p;

    if (ball && ball->anim)
        particle_free(&ball->anim->base);
    p = open_anim(self, "from_the_abyss\\from_the_abyss_shadowball_create\\from_the_abyss_shadowball_create.str");
    if (!p)
        return;
    p->zoom = slot_at(self, key)->zoom;
    PARTICLE_OFFSET(p) = slot_at(self, key)->offset;
    ball = ball_at(self, key);
    ball->state = BALL_APPEARING;
    ball->anim = p;
}

void from_the_abyss_effect_kill_ball(struct FromTheAbyssEffect *self, int64_t key)
{
    struct Ball *ball = tree_find(&self->balls, &key, tree_less_int64);
    struct Ez2StrParticle *old, *p;
    float zoom;
    Vec3 offset;

    if (!ball || !ball->anim)
        return;
    old = ball->anim;
    zoom = old->zoom;
    offset = PARTICLE_OFFSET(old);
    particle_free(&old->base);
    p = open_anim(self, "from_the_abyss\\from_the_abyss_shadowball_die\\from_the_abyss_shadowball_die.str");
    if (p) {
        p->zoom = zoom;
        PARTICLE_OFFSET(p) = offset;
    }
    ball->state = BALL_DYING;
    ball->anim = p;
}

void from_the_abyss_effect_set_ball_count(struct Effect *effect, int64_t count)
{
    struct FromTheAbyssEffect *self = (struct FromTheAbyssEffect *)effect;
    int64_t key;

    if (count < 0 || count > self->max_balls)
        return;
    if (count > self->ball_count) {
        for (key = self->ball_count; key < count; key++)
            from_the_abyss_effect_create_ball(self, key);
    } else if (count < self->ball_count) {
        for (key = self->ball_count - 1; key > count - 1; key--)
            from_the_abyss_effect_kill_ball(self, key);
    }
    self->ball_count = count;
}

void from_the_abyss_effect_place_ball(struct FromTheAbyssEffect *self, struct Ez2StrParticle *p)
{
    float view[12], from_view[12];
    Vec3 at, seen, zero = { 0.0f, 0.0f, 0.0f };

    if (!p)
        return;
    at = self->base.pos;
    if (!game_copy_view_matrix(view))
        return;
    seen = transform(view, at);
    mat_invert(from_view, view);
    seen.x = PARTICLE_OFFSET(p).x + seen.x;
    seen.y = PARTICLE_OFFSET(p).y + seen.y;
    seen.z = PARTICLE_OFFSET(p).z + seen.z;
    p->base.pos = transform(from_view, seen);
    ez2_str_particle_process(p, zero);
}

void from_the_abyss_effect_update_ball(struct FromTheAbyssEffect *self, int64_t key, int state,
                                                struct Ez2StrParticle *anim)
{
    struct Ez2StrParticle *p;
    struct Ball *ball;
    float zoom;
    Vec3 offset;

    if (!anim)
        return;
    switch (state) {
    case BALL_APPEARING:
        if (anim_alive(anim))
            break;
        zoom = anim->zoom;
        offset = PARTICLE_OFFSET(anim);
        particle_free(&anim->base);
        p = open_anim(self, "from_the_abyss\\from_the_abyss_shadowball\\from_the_abyss_shadowball.str");
        if (p) {
            p->loop_forever = 1;
            p->zoom = zoom;
            PARTICLE_OFFSET(p) = offset;
        }
        ball = ball_at(self, key);
        ball->state = BALL_HOVERING;
        ball->anim = p;
        return;
    case BALL_HOVERING:
        break;
    case BALL_DYING:
        if (anim_alive(anim))
            break;
        particle_free(&anim->base);
        ball = ball_at(self, key);
        ball->state = BALL_GONE;
        ball->anim = 0;
        return;
    default:
        return;
    }
    from_the_abyss_effect_place_ball(self, anim);
}

void from_the_abyss_effect_release(struct Effect *effect)
{
    struct FromTheAbyssEffect *self = (struct FromTheAbyssEffect *)effect;
    struct TreeNode *node;

    for (node = tree_first(&self->balls); node != self->balls.head; node = tree_next(node)) {
        struct Ball *ball = TREE_VALUE(node, struct Ball);

        if (ball->anim)
            particle_free(&ball->anim->base);
    }
    tree_clear(&self->balls, NULL);
}

void from_the_abyss_effect_init(struct Effect *effect)
{
    effect->has_instance_key = 1;
}

void from_the_abyss_effect_process(struct Effect *effect)
{
    struct FromTheAbyssEffect *self = (struct FromTheAbyssEffect *)effect;
    struct TreeNode *node;

    for (node = tree_first(&self->balls); node != self->balls.head; node = tree_next(node)) {
        struct Ball *ball = TREE_VALUE(node, struct Ball);

        from_the_abyss_effect_update_ball(self, ball->key, ball->state, ball->anim);
    }
}

void from_the_abyss_effect_render(struct Effect *effect)
{
    struct FromTheAbyssEffect *self = (struct FromTheAbyssEffect *)effect;
    struct TreeNode *node;

    for (node = tree_first(&self->balls); node != self->balls.head; node = tree_next(node)) {
        struct Ball *ball = TREE_VALUE(node, struct Ball);

        if (ball->anim) {
            ez2_str_particle_render_current(ball->anim);
        }
    }
}
