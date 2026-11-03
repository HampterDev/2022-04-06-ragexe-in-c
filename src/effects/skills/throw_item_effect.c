#include "effects/effect_allocator.h"
#include <stdlib.h>
#include <string.h>
#include "effects/particle.h"
#include "effects/skills/skill_view.h"
#include "effects/effect_math.h"
#include "effects/render_object.h"
#include "std/vec.h"

enum {
    THROW_BOTTLE = 0x12a,
    THROW_POTION = 0x12b,
    THROW_STONE = 0x134,
    THROW_FIRE_BOTTLE = 0x21b,
    THROW_BERSERK_POTION = 0x21d,
    THROW_VENOM_KNIFE = 0x258,
    THROW_SHURIKEN = 0x265,
    THROW_POISON_KUNAI = 0x266,
    THROW_FUUMA = 0x267,
    THROW_COIN = 0x268,
    THROW_SPORE = 0x392,
    THROW_ACID_BOTTLE = 0x3d4,
    THROW_KUNAI_FAN = 0x3d5,
    THROW_COINS = 0x3d6,
    THROW_FIRE_KUNAI = 0x3d7,
    THROW_ORBS = 0x3dd,
};

#define THROW_LIST 5

#define ITEM_DIR "유저인터페이스\\item\\"

struct ThrowItemEffect {
    struct Effect base;
    int flight_frames;
    int start_frame;
    int volleys;
    int copies;
    float rise;
    float arc_height;
    int trail_alpha;
    float trail_roll;
    Vec3 trail_pos;
    struct Vec items;
};

#define EFFECT_POTION_COLOR(e) ((e)->variant)

struct String;
void throw_item_effect_init(struct Effect *self);
void throw_item_effect_process(struct Effect *self);
void throw_item_effect_render(struct Effect *self);
void throw_item_effect_release(struct Effect *self);
void throw_item_effect_after_init(struct Effect *self);
static void throw_item_effect_destroy(struct Effect *effect);

static const struct EffectOps throw_item_effect_ops = {
    .set_submit = effect_set_submit,
    .init = throw_item_effect_init,
    .update_range = effect_update_range,
    .process = throw_item_effect_process,
    .render = throw_item_effect_render,
    .release = throw_item_effect_release,
    .is_alive = effect_is_within_duration,
    .restart = effect_restart,
    .set_color = effect_set_color,
    .set_texture = effect_set_texture,
    .after_init = throw_item_effect_after_init,
    .skip = effect_skip,
    .set_stop_frame = effect_set_stop_frame,
    .set_start_delay = effect_set_start_delay,
    .set_spin = effect_set_spin,
    .set_str_name = effect_set_str_name,
    .set_list_flags = effect_set_list_flags,
    .set_zoom = effect_set_zoom,
    .set_alpha_scale = effect_set_alpha_scale,
    .destroy = throw_item_effect_destroy,
    .fade_out = effect_fade_out,
};

#define ACTOR_HEADING(actor)     ((struct RenderObject *)(actor))->roty
#define ACTOR_ORB_ELEMENT(actor) ((struct GameActor *)(actor))->orb_kind
#define ACTOR_ORB_COUNT(actor)   ((struct GameActor *)(actor))->orb_count

struct ThrowItemParticle {
    int item_heading;
    float item_distance;
    float item_speed;
};
PARTICLE_EFFECT_DATA_FITS(ThrowItemParticle);
static __inline struct ThrowItemParticle *throw_item_particle(const struct Particle *p)
{
    return (struct ThrowItemParticle *)p->effect_data;
}
#define ITEM_POS(p)       (&(p)->offset)

#define MATRIX_BYTES (12 * sizeof(float))

static void set_texture(struct Particle *p, const char *name)
{
    struct String s;
    string_init(&s);
    string_assign_cstr(&s, name);
    particle_set_texture(p, s);
}

static void start_at_effect(const struct ThrowItemEffect *self, struct Particle *p, float lift)
{
    ITEM_POS(p)->x = self->base.pos.x;
    ITEM_POS(p)->y = self->base.pos.y - lift;
    ITEM_POS(p)->z = self->base.pos.z;
}

struct ThrowItemEffect *throw_item_effect_construct(struct ThrowItemEffect *self)
{
    effect_construct(&self->base, -1);
    self->base.ops = &throw_item_effect_ops;
    self->flight_frames = 0;
    self->start_frame = 0;
    self->volleys = 1;
    self->copies = 4;
    self->rise = 0.0f;
    self->arc_height = 0.0f;
    self->trail_pos.x = self->trail_pos.y = self->trail_pos.z = 0.0f;
    self->items.first = self->items.last = self->items.end = 0;
    return self;
}

struct Effect *throw_item_effect_new(void)
{
    struct ThrowItemEffect *self = mem_try_alloc(sizeof *self);

    return self ? &throw_item_effect_construct(self)->base : NULL;
}

/* Effect types created as ThrowItemEffect, registered at startup through g_effectTypes
   (effects/effect_registry.c). A type missing here is not created. */
static const struct EffectTypeRow throw_item_effect_type_rows[] = {
    EFFECT_TYPES(0x12a, 0x12b, throw_item_effect_new),
    EFFECT_TYPE(0x134, throw_item_effect_new),
    EFFECT_TYPE(0x21b, throw_item_effect_new),
    EFFECT_TYPE(0x3d4, throw_item_effect_new),
    EFFECT_TYPE(0x21d, throw_item_effect_new),
    EFFECT_TYPE(0x258, throw_item_effect_new),
    EFFECT_TYPES(0x265, 0x268, throw_item_effect_new),
    EFFECT_TYPE(0x392, throw_item_effect_new),
    EFFECT_TYPE(0x3d7, throw_item_effect_new),
    EFFECT_TYPES(0x3d5, 0x3d6, throw_item_effect_new),
    EFFECT_TYPE(0x3dd, throw_item_effect_new),
};
const struct EffectTypeTable throw_item_effect_types = EFFECT_TYPE_TABLE(throw_item_effect_type_rows);

void throw_item_effect_destruct(struct ThrowItemEffect *self)
{
    self->base.ops = &throw_item_effect_ops;
    vec_free(&self->items);
    effect_destruct(&self->base);
}

static void throw_item_effect_destroy(struct Effect *effect)
{
    struct ThrowItemEffect *self = (struct ThrowItemEffect *)effect;
    throw_item_effect_destruct(self);
    mem_free(self);
}

unsigned short throw_item_effect_fan_angle(struct ThrowItemEffect *self, const struct Particle *p)
{
    if (self->base.type != THROW_KUNAI_FAN)
        return 0;
    return (unsigned short)((short)(360 / self->volleys) * (short)p->group);
}

void throw_item_effect_aim(struct ThrowItemEffect *self, struct Particle *p)
{
    float dx = self->base.target_pos.x - self->base.pos.x;
    float dz, distance, speed;
    unsigned short heading;
    int i;

    self->rise = self->base.target_pos.y - self->base.pos.y;
    dz = self->base.target_pos.z - self->base.pos.z;
    heading = (unsigned short)(int)effect_heading_deg(dx, dz);
    throw_item_particle(p)->item_heading = (short)(throw_item_effect_fan_angle(self, p) + heading) % 360;
    distance = float_sqrt(dz * dz + dx * dx);
    throw_item_particle(p)->item_distance = distance;
    speed = throw_item_particle(p)->item_speed;
    for (i = 0; i < 100; i++) {
        if ((float)i * speed > distance) {
            self->flight_frames = i;
            return;
        }
    }
}

void throw_item_effect_setup_bottle(struct ThrowItemEffect *self, struct Particle *p)
{
    self->base.duration = 300;
    self->arc_height = 30.0f;
    throw_item_effect_aim(self, p);
    set_texture(p, ITEM_DIR "빈병.bmp");
    p->scale = 1.5f;
    particle_set_color(p, 135, 255, 135);
    p->roll = 0.0f;
    throw_item_particle(p)->item_speed = throw_item_particle(p)->item_distance / 25.0f;
    p->alpha = 255;
    start_at_effect(self, p, 8.0f);
    p->dest_blend = 6;
}

void throw_item_effect_setup_flask(struct ThrowItemEffect *self, struct Particle *p)
{
    self->base.duration = 300;
    self->arc_height = 1.0f;
    throw_item_effect_aim(self, p);
    p->scale = 1.5f;
    particle_set_color(p, 255, 255, 255);
    p->roll = 0.0f;
    p->alpha = 255;
    throw_item_particle(p)->item_speed = throw_item_particle(p)->item_distance / 25.0f;
    start_at_effect(self, p, 8.0f);
    p->dest_blend = 6;
}

void throw_item_effect_setup_orb(struct ThrowItemEffect *self, struct Particle *p)
{
    self->base.duration = 200;
    p->scale = 2.0f;
    p->roll = 330.0f;
    throw_item_particle(p)->item_speed = 1.5f;
    p->alpha = 255;
    start_at_effect(self, p, 15.0f);
}

void throw_item_effect_setup_potion(struct ThrowItemEffect *self, struct Particle *p, int color)
{
    self->base.duration = 300;
    self->arc_height = 30.0f;
    throw_item_effect_aim(self, p);
    p->scale = 1.5f;
    p->roll = 0.0f;
    throw_item_particle(p)->item_speed = throw_item_particle(p)->item_distance / 25.0f;
    p->alpha = 255;
    start_at_effect(self, p, 8.0f);
    if (color != 6)
        p->dest_blend = 6;
    switch (color) {
    case 1:
        particle_set_color(p, 255, 135, 135);
        break;
    case 2:
        particle_set_color(p, 255, 205, 135);
        break;
    case 3:
        particle_set_color(p, 255, 255, 135);
        break;
    case 5:
        particle_set_color(p, 135, 135, 255);
        break;
    case 4:
    case 6:
        particle_set_color(p, 255, 255, 255);
        break;
    }
}

void throw_item_effect_setup_stone(struct ThrowItemEffect *self, struct Particle *p)
{
    self->base.duration = 300;
    self->arc_height = 10.0f;
    set_texture(p, ITEM_DIR "돌.bmp");
    p->scale = 1.0f;
    particle_set_color(p, 255, 255, 255);
    p->roll = 0.0f;
    throw_item_particle(p)->item_speed = 2.0f;
    p->alpha = 255;
    start_at_effect(self, p, 8.0f);
    p->dest_blend = 6;
}

void throw_item_effect_setup_poison_kunai(struct ThrowItemEffect *self, struct Particle *p)
{
    self->base.duration = 200;
    self->arc_height = 1.0f;
    set_texture(p, ITEM_DIR "쿠나이_독.bmp");
    p->scale = 3.0f;
    particle_set_color(p, 255, 200, 255);
    p->roll = 330.0f;
    throw_item_particle(p)->item_speed = 1.5f;
    p->alpha = 255;
    start_at_effect(self, p, 8.0f);
    p->dest_blend = 6;
    if (self->base.target_aid == 0) {
        self->base.target_pos.x = self->base.pos.x - 8.0f;
        self->base.target_pos.y = self->base.pos.y;
        self->base.target_pos.z = self->base.pos.z;
    }
}

void throw_item_effect_setup_coin(struct ThrowItemEffect *self, struct Particle *p)
{
    self->base.duration = 200;
    self->arc_height = 10.0f;
    set_texture(p, "effect\\coin_a.bmp");
    p->scale = 2.5f;
    particle_set_color(p, 255, 200, 0);
    p->roll = 330.0f;
    throw_item_particle(p)->item_speed = 2.0f;
    p->alpha = 255;
    start_at_effect(self, p, 8.0f);
    p->dest_blend = 6;
}

void throw_item_effect_setup_weapon(struct ThrowItemEffect *self, struct Particle *p)
{
    self->base.duration = 200;
    self->arc_height = 1.0f;
    p->scale = 2.0f;
    particle_set_color(p, 255, 200, 255);
    p->roll = 330.0f;
    throw_item_particle(p)->item_speed = 3.0f;
    p->alpha = 255;
    start_at_effect(self, p, 8.0f);
    p->dest_blend = 6;
}

static void setup_tinted(struct ThrowItemEffect *self, struct Particle *p)
{
    int color = EFFECT_POTION_COLOR(&self->base);
    throw_item_effect_setup_potion(self, p, color == -1 ? 1 : color);
}

void throw_item_effect_setup_item(struct ThrowItemEffect *self, struct Particle *p)
{
    void *owner;

    if (!p)
        return;
    switch (self->base.type) {
    case THROW_BOTTLE:
        throw_item_effect_setup_bottle(self, p);
        break;
    case THROW_POTION:
        set_texture(p, ITEM_DIR "빈포션병.bmp");
        setup_tinted(self, p);
        break;
    case THROW_BERSERK_POTION:
        set_texture(p, ITEM_DIR "버서크포션.bmp");
        setup_tinted(self, p);
        break;
    case THROW_SPORE:
        set_texture(p, ITEM_DIR "폭탄버섯포자.bmp");
        setup_tinted(self, p);
        break;
    case THROW_STONE:
        throw_item_effect_setup_stone(self, p);
        break;
    case THROW_FIRE_BOTTLE:
        self->start_frame = 5;
        set_texture(p, ITEM_DIR "화염병.bmp");
        throw_item_effect_setup_flask(self, p);
        break;
    case THROW_ACID_BOTTLE:
        self->start_frame = 10;
        set_texture(p, ITEM_DIR "염산병.bmp");
        throw_item_effect_setup_flask(self, p);
        break;
    case THROW_VENOM_KNIFE:
        set_texture(p, ITEM_DIR "베넘나이프.bmp");
        throw_item_effect_setup_weapon(self, p);
        break;
    case THROW_SHURIKEN:
        set_texture(p, ITEM_DIR "수리검.bmp");
        throw_item_effect_setup_weapon(self, p);
        break;
    case THROW_POISON_KUNAI:
        set_texture(p, ITEM_DIR "쿠나이_독.bmp");
        throw_item_effect_setup_weapon(self, p);
        break;
    case THROW_FUUMA:
        set_texture(p, ITEM_DIR "풍마_열화.bmp");
        throw_item_effect_setup_weapon(self, p);
        break;
    case THROW_COIN:
        set_texture(p, "effect\\coin_a.bmp");
        throw_item_effect_setup_weapon(self, p);
        break;
    case THROW_FIRE_KUNAI:
        set_texture(p, ITEM_DIR "쿠나이_화.bmp");
        throw_item_effect_setup_weapon(self, p);
        throw_item_particle(p)->item_speed = 1.0f;
    case THROW_KUNAI_FAN:
        throw_item_effect_setup_poison_kunai(self, p);
        break;
    case THROW_COINS:
        throw_item_effect_setup_coin(self, p);
        break;
    case THROW_ORBS:
        owner = effect_find_actor(self->base.owner_aid);
        if (!owner)
            break;
        switch (ACTOR_ORB_ELEMENT(owner)) {
        case 1:
            set_texture(p, "effect\\WaterOrb.bmp");
            break;
        case 2:
            set_texture(p, "effect\\StoneOrb.bmp");
            break;
        case 3:
            set_texture(p, "effect\\FireOrb.bmp");
            break;
        case 4:
            set_texture(p, "effect\\LightningOrb.bmp");
            break;
        }
        throw_item_effect_setup_orb(self, p);
        break;
    }
    throw_item_effect_aim(self, p);
}

void throw_item_effect_set_counts(struct ThrowItemEffect *self)
{
    void *owner;

    switch (self->base.type) {
    case THROW_KUNAI_FAN:
        self->volleys = 8;
        break;
    case THROW_COINS:
        self->volleys = 10;
        break;
    case THROW_ORBS:
        owner = effect_find_actor(self->base.owner_aid);
        if (owner) {
            self->volleys = ACTOR_ORB_COUNT(owner);
            self->copies = 1;
        }
        break;
    }
}

void throw_item_effect_spin_owner(struct ThrowItemEffect *self, const struct Particle *p)
{
    void *owner;

    if (self->base.type != THROW_KUNAI_FAN || p->group != 0 || p->index != 0)
        return;
    owner = effect_find_actor(self->base.owner_aid);
    if (owner)
        ACTOR_HEADING(owner) = (float)((int)(ACTOR_HEADING(owner) + 45.0f) % 360);
}

void throw_item_effect_step_item(struct ThrowItemEffect *self, struct Particle *p)
{
    float roll, t, lift = 0.0f;

    if (!p)
        return;
    if (self->start_frame > self->base.frame)
        return;

    if (p->index == 0) {
        roll = (float)((int)(p->roll + 45.0f) % 360);
    } else {
        self->trail_alpha += p->index == 1 ? -150 : -25;
        p->alpha = self->trail_alpha;
        *ITEM_POS(p) = self->trail_pos;
        roll = self->trail_roll;
    }
    p->roll = roll;

    t = (float)self->base.frame / (float)self->flight_frames;
    if (t > 1.05f) {
        p->alpha = p->alpha - 20 < 0 ? 0 : p->alpha - 20;
    } else {
        float speed = throw_item_particle(p)->item_speed;
        int arc = (int)(t * -180.0f);
        float y = speed / throw_item_particle(p)->item_distance * self->rise + ITEM_POS(p)->y;
        float z = g_sinTable[throw_item_particle(p)->item_heading] * speed + ITEM_POS(p)->z;
        float x = g_cosTable[throw_item_particle(p)->item_heading] * speed + ITEM_POS(p)->x;
        ITEM_POS(p)->y = y;
        ITEM_POS(p)->z = z;
        ITEM_POS(p)->x = x;
        lift = g_sinTable[-arc] * self->arc_height;
    }

    self->trail_pos = *ITEM_POS(p);
    self->trail_roll = p->roll;
    self->trail_alpha = p->alpha;

    if (p->alpha > 0) {
        float turn[12], size[12], move[12], m[12], out[12];

        memcpy(turn, g_identityMatrix.m, MATRIX_BYTES);
        mat_set_rotate_x(turn, DEG_TO_RAD(game_camera_pitch()));
        mat_rotate_y(turn, DEG_TO_RAD(p->roll));
        memcpy(size, g_identityMatrix.m, MATRIX_BYTES);
        mat_set_scale(size, p->scale, p->scale, 1.0f);
        memcpy(move, g_identityMatrix.m, MATRIX_BYTES);
        mat_set_translation(move, ITEM_POS(p)->x, ITEM_POS(p)->y - lift, ITEM_POS(p)->z);
        mat_multiply(m, size, turn);
        mat_multiply(out, m, move);
        memcpy(p->matrix, out, MATRIX_BYTES);
        throw_item_effect_spin_owner(self, p);
    }
}

void throw_item_draw(struct Particle *p)
{
    if (p)
        particle_render(p, 1, 0.0f, 1.0f, 0.0f, 1.0f, 1);
}

void throw_item_effect_init(struct Effect *effect)
{
    struct ThrowItemEffect *self = (struct ThrowItemEffect *)effect;
    int volley, copy;

    throw_item_effect_set_counts(self);
    for (volley = 0; volley < self->volleys; volley++) {
        for (copy = 0; copy < self->copies; copy++) {
            struct Particle *p = particle_new(self->base.pos, THROW_LIST);
            if (!p)
                continue;
            p->effect_type = self->base.type;
            p->index = copy;
            p->group = volley;
            p->owner_tag = 1;
            throw_item_effect_setup_item(self, p);
            vec_push(&self->items, &p, sizeof p);
        }
    }
}

void throw_item_effect_after_init(struct Effect *effect)
{
    int type = effect->type;

    if (type > THROW_COIN) {
        if (type == THROW_SPORE)
            effect_add_sound_cue(effect, SOUND_CUE_WAVE,
                                 "effect\\G스포어익스플로젼.wav", 0,
                                 0.0f);
    } else if (type >= THROW_SHURIKEN) {
        effect_add_sound_cue(effect, SOUND_CUE_WAVE,
                             "effect\\닌자_던지기.wav", 0, 0.0f);
    } else if (type == THROW_BOTTLE) {
        effect_add_sound_cue(effect, SOUND_CUE_WAVE, "_heal_effect.wav", 0, 0.0f);
    }
}

void throw_item_effect_process(struct Effect *effect)
{
    struct ThrowItemEffect *self = (struct ThrowItemEffect *)effect;
    struct Particle **it;

    for (it = self->items.first; it != (struct Particle **)self->items.last; it++) {
        (*it)->pos = self->base.pos;
        throw_item_effect_step_item(self, *it);
        (*it)->age++;
    }
}

void throw_item_effect_render(struct Effect *effect)
{
    struct ThrowItemEffect *self = (struct ThrowItemEffect *)effect;
    struct Particle **it;

    for (it = self->items.first; it != (struct Particle **)self->items.last; it++)
        throw_item_draw(*it);
}

void throw_item_effect_release(struct Effect *effect)
{
    struct ThrowItemEffect *self = (struct ThrowItemEffect *)effect;
    struct Particle **it;

    for (it = self->items.first; it != (struct Particle **)self->items.last; it++)
        particle_free(*it);
    self->items.last = self->items.first;
}
