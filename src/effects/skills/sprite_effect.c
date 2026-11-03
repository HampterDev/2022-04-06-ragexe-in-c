#include "effects/effect_allocator.h"
#include "world/object_lookup.h"
#include <string.h>
#include "effects/particle.h"
#include "effects/skills/skill_view.h"
#include "actor/game_actor.h"

struct SpriteEffect {
    struct Effect base;
    struct Vec sprites;
};

void sprite_effect_init(struct Effect *self);
void sprite_effect_process(struct Effect *self);
void sprite_effect_render(struct Effect *self);
void sprite_effect_release(struct Effect *self);
unsigned char sprite_effect_is_alive(struct Effect *self);
static void sprite_effect_destroy(struct Effect *effect);

static const struct EffectOps sprite_effect_ops = {
    .set_submit = effect_set_submit,
    .init = sprite_effect_init,
    .update_range = effect_update_range,
    .process = sprite_effect_process,
    .render = sprite_effect_render,
    .release = sprite_effect_release,
    .is_alive = sprite_effect_is_alive,
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
    .destroy = sprite_effect_destroy,
    .fade_out = effect_fade_out,
};

#define SPRITE_LIST 0x200
#define LIST_WORLD_SORTED 0x101
#define SECONDS_TO_FRAMES 60.0f

#define EFFECT_DIR "이팩트\\"
#define SET_SPRITE(p, name)                                    \
    do {                                                       \
        sprite_particle_set_spr((p), EFFECT_DIR name ".spr");  \
        sprite_particle_set_act((p), EFFECT_DIR name ".act");  \
    } while (0)

#define particle_owner_type(p) ((p)->base.effect_type)
#define particle_tag(p)        ((p)->base.owner_tag)
#define particle_index(p)      ((p)->base.index)
#define particle_row(p)        ((p)->base.group)

#define EACH_SPRITE(self, it) \
    for (it = (struct SpriteParticle **)(self)->sprites.first; it != (struct SpriteParticle **)(self)->sprites.last; it++)

struct SpriteEffect *sprite_effect_construct(struct SpriteEffect *self)
{
    effect_construct(&self->base, -1);
    self->base.ops = &sprite_effect_ops;
    self->sprites.first = self->sprites.last = self->sprites.end = 0;
    return self;
}

struct Effect *sprite_effect_new(void)
{
    struct SpriteEffect *self = mem_try_alloc(sizeof *self);

    return self ? &sprite_effect_construct(self)->base : NULL;
}

/* Effect types created as SpriteEffect, registered at startup through g_effectTypes
   (effects/effect_registry.c). A type missing here is not created. */
static const struct EffectTypeRow sprite_effect_type_rows[] = {
    EFFECT_TYPE(0x3de, sprite_effect_new),
    EFFECT_TYPE(0x46a, sprite_effect_new),
    EFFECT_TYPE(0x47b, sprite_effect_new),
    EFFECT_TYPES(0x47f, 0x488, sprite_effect_new),
    EFFECT_TYPE(0x4a0, sprite_effect_new),
    EFFECT_TYPE(0x4a8, sprite_effect_new),
    EFFECT_TYPES(0x4bb, 0x4bd, sprite_effect_new),
    EFFECT_TYPES(0x4cf, 0x4d5, sprite_effect_new),
    EFFECT_TYPE(0x4da, sprite_effect_new),
    EFFECT_TYPE(0x561, sprite_effect_new),
    EFFECT_TYPE(0x58b, sprite_effect_new),
    EFFECT_TYPES(0x651, 0x653, sprite_effect_new),
};
const struct EffectTypeTable sprite_effect_types = EFFECT_TYPE_TABLE(sprite_effect_type_rows);

void sprite_effect_destruct(struct SpriteEffect *self)
{
    self->base.ops = &sprite_effect_ops;
    vec_free(&self->sprites);
    effect_destruct(&self->base);
}

static void sprite_effect_destroy(struct Effect *effect)
{
    struct SpriteEffect *self = (struct SpriteEffect *)effect;
    sprite_effect_destruct(self);
    mem_free(self);
}

void sprite_effect_release(struct Effect *effect)
{
    struct SpriteEffect *self = (struct SpriteEffect *)effect;
    struct SpriteParticle **it;

    EACH_SPRITE(self, it)
        particle_free(&(*it)->base);
    self->sprites.last = self->sprites.first;
}

void sprite_effect_setup_sprite(struct SpriteEffect *self, struct SpriteParticle *p)
{
    int type;

    if (!p)
        return;
    p->base.flags |= PARTICLE_TINT_BY_CLIP;
    p->frame_delay = 2.0f;
    p->base.alpha = 255;
    type = self->base.type;
    self->base.duration = 100;
    switch (type) {
    case 0x3de:
        SET_SPRITE(p, "m_ef03");
        p->frame_delay = 4.0f;
        p->base.scale = 1.5f;
        break;
    case 0x46a:
        SET_SPRITE(p, "bakuretsu_hadou\\bakuretsu_hadou");
        p->base.flags |= PARTICLE_TINT_BY_CLIP;
        p->frame_delay = 8.0f;
        p->base.depth_bias = 0.0003f;
        p->base.list_flags = LIST_WORLD_SORTED;
        p->loop = SPRITE_LOOP;
        break;
    case 0x47b:
        SET_SPRITE(p, "EF_POPCORN\\EF_POPCORN");
        p->frame_delay = 8.0f;
        p->loop = SPRITE_ONCE;
        p->base.depth_bias = 0.0003f;
        break;
    case 0x47f: case 0x480: case 0x481: case 0x482: case 0x483:
    case 0x484: case 0x485: case 0x486: case 0x487: case 0x488:
        switch (type) {
        case 0x47f: SET_SPRITE(p, "EF_IMMUNE_PROPERTY\\EF_IMMUNE_PROPERTY_NOTHING"); break;
        case 0x480: SET_SPRITE(p, "EF_IMMUNE_PROPERTY\\EF_IMMUNE_PROPERTY_WATER"); break;
        case 0x481: SET_SPRITE(p, "EF_IMMUNE_PROPERTY\\EF_IMMUNE_PROPERTY_GROUND"); break;
        case 0x482: SET_SPRITE(p, "EF_IMMUNE_PROPERTY\\EF_IMMUNE_PROPERTY_FIRE"); break;
        case 0x483: SET_SPRITE(p, "EF_IMMUNE_PROPERTY\\EF_IMMUNE_PROPERTY_WIND"); break;
        case 0x484: SET_SPRITE(p, "EF_IMMUNE_PROPERTY\\EF_IMMUNE_PROPERTY_POISON"); break;
        case 0x485: SET_SPRITE(p, "EF_IMMUNE_PROPERTY\\EF_IMMUNE_PROPERTY_SAINT"); break;
        case 0x486: SET_SPRITE(p, "EF_IMMUNE_PROPERTY\\EF_IMMUNE_PROPERTY_DARKNESS"); break;
        case 0x487: SET_SPRITE(p, "EF_IMMUNE_PROPERTY\\EF_IMMUNE_PROPERTY_TELEKINESIS"); break;
        default:    SET_SPRITE(p, "EF_IMMUNE_PROPERTY\\EF_IMMUNE_PROPERTY_UNDEAD"); break;
        }
        p->base.depth_bias = 0.0003f;
        p->loop = SPRITE_LOOP;
        break;
    case 0x4a8:
        SET_SPRITE(p, "random_opt_spr\\yellow");
        p->frame_delay = 4.0f;
        p->loop = SPRITE_LOOP;
        break;
    case 0x4bb: case 0x4bc: case 0x4bd:
        SET_SPRITE(p, "subject_aura\\subject_aura");
        p->frame_delay = 8.0f;
        p->loop = SPRITE_LOOP;
        p->base.depth_bias = 0.0003f;
        p->base.list_flags = LIST_WORLD_SORTED;
        if (self->base.type == 0x4bc)
            p->action = 0;
        else if (self->base.type == 0x4bd)
            p->action = 1;
        else if (self->base.type == 0x4bb)
            p->action = 2;
        break;
    case 0x4cf: case 0x4d0: case 0x4d1: case 0x4d2:
        if (type == 0x4cf)
            SET_SPRITE(p, "book_of_dimension\\book_of_dimension");
        else if (type == 0x4d0)
            SET_SPRITE(p, "book_of_creatingstar\\book_of_creatingstar");
        else if (type == 0x4d1)
            SET_SPRITE(p, "curse_explosion\\curse_explosion");
        else
            SET_SPRITE(p, "soul_reaper\\soul_reaper");
        p->frame_delay = 4.0f;
        p->loop = SPRITE_ONCE;
        p->base.list_flags = LIST_WORLD_SORTED;
        break;
    case 0x4d3: case 0x4d4: case 0x4d5: case 0x4da:
        if (type == 0x4d3)
            SET_SPRITE(p, "soul_shadow\\soul_shadow");
        else if (type == 0x4d4)
            SET_SPRITE(p, "soul_fairy\\soul_fairy");
        else if (type == 0x4d5)
            SET_SPRITE(p, "soul_falcon\\soul_falcon");
        else
            SET_SPRITE(p, "soul_explosion\\soul_explosion");
        p->base.depth_bias = 0.0003f;
        p->frame_delay = 4.0f;
        p->loop = SPRITE_ONCE;
        p->base.list_flags = LIST_WORLD_SORTED;
        break;
    case 0x561:
        SET_SPRITE(p, "valkyrie_wing\\valkyrie_wing");
        p->base.depth_bias = 0.001f;
        p->base.list_flags = LIST_WORLD_SORTED;
        p->frame_delay = 6.0f;
        p->loop = SPRITE_LOOP;
        break;
    case 0x58b: case 0x651: case 0x652: case 0x653:
        if (type == 0x58b)
            SET_SPRITE(p, "c_amdarais_effect");
        else if (type == 0x651)
            SET_SPRITE(p, "black_bubble\\small\\blackbubble_small");
        else if (type == 0x652)
            SET_SPRITE(p, "black_bubble\\midium\\blackbubble_midium");
        else
            SET_SPRITE(p, "black_bubble\\large\\blackbubble_large");
        p->base.list_flags = 1;
        p->base.depth_bias = 0.0001f;
        p->frame_delay = 6.0f;
        p->loop = SPRITE_LOOP;
        break;
    }
}

void sprite_effect_init(struct Effect *effect)
{
    struct SpriteEffect *self = (struct SpriteEffect *)effect;
    struct SpriteParticle *p = sprite_particle_new(self->base.pos, SPRITE_LIST);

    if (!p)
        return;
    particle_owner_type(p) = self->base.type;
    particle_index(p) = 1;
    particle_row(p) = 0;
    particle_tag(p) = 1;
    sprite_effect_setup_sprite(self, p);
    vec_push(&self->sprites, &p, sizeof(struct SpriteParticle *));
}

unsigned char sprite_effect_is_alive(struct Effect *effect)
{
    struct SpriteEffect *self = (struct SpriteEffect *)effect;
    struct SpriteParticle **it;

    switch (self->base.type) {
    case 0x3de:
        return !effect_is_finished(&self->base);
    case 0x4a8: {
        void *world;
        if (self->base.owner_aid == 0)
            return 0;
        world = game_current_world();
        if (!world || !world_get_item_by_aid(world, self->base.owner_aid))
            return 0;
        return 1;
    }
    case 0x46a: case 0x47f: case 0x480: case 0x481: case 0x482: case 0x483:
    case 0x484: case 0x485: case 0x486: case 0x487: case 0x488:
    case 0x4bb: case 0x4bc: case 0x4bd:
    case 0x561: case 0x58b: case 0x651: case 0x652: case 0x653:
        return effect_is_owner_alive(&self->base);
    }
    EACH_SPRITE(self, it) {
        float spent = (*it)->base.life_seconds;
        if (0.0f == spent)
            return 1;
        if (spent * SECONDS_TO_FRAMES > (float)(*it)->base.age)
            return 1;
    }
    return 0;
}

static int owner_height(const struct SpriteEffect *self, float above, int fallback)
{
    void *owner = effect_find_actor(self->base.owner_aid);
    if (!owner)
        return fallback;
    return (int)(float)(game_actor_get_mount_lift(owner) + above);
}

void sprite_effect_process(struct Effect *effect)
{
    struct SpriteEffect *self = (struct SpriteEffect *)effect;
    struct SpriteParticle **it;

    EACH_SPRITE(self, it) {
        struct SpriteParticle *p = *it;
        Vec3 here;
        int height;
        void *owner;

        p->base.age++;
        switch (self->base.type) {
        case 0x46a:
            height = owner_height(self, 8.0f, 8);
            here = self->base.pos;
            p->base.pos.x = here.x + 0.0f;
            p->base.pos.y = here.y - (float)height;
            p->base.pos.z = here.z + 0.0f;
            break;
        case 0x47f: case 0x480: case 0x481: case 0x482: case 0x483:
        case 0x484: case 0x485: case 0x486: case 0x487: case 0x488:
            height = owner_height(self, 25.0f, 25);
            here = self->base.pos;
            p->base.pos.x = here.x + 0.0f;
            p->base.pos.y = here.y - (float)height;
            p->base.pos.z = here.z + 0.0f;
            break;
        case 0x47b: case 0x4a8:
            here = self->base.pos;
            p->base.pos.x = here.x + 0.0f;
            p->base.pos.y = here.y - 10.0f;
            p->base.pos.z = here.z + 0.0f;
            break;
        case 0x4bb: case 0x4bc: case 0x4bd: case 0x561:
            owner = effect_find_actor(self->base.owner_aid);
            if (!owner)
                break;
            height = (int)(float)(game_actor_get_mount_lift(owner) + (self->base.type == 0x561 ? 3.0f : 8.0f));
            here = self->base.pos;
            p->base.pos.x = here.x + 0.0f;
            p->base.pos.y = here.y + (float)-height;
            p->base.pos.z = here.z + 0.0f;
            break;
        default:
            p->base.pos = self->base.pos;
            break;
        }
    }
}

void sprite_effect_render(struct Effect *effect)
{
    struct SpriteEffect *self = (struct SpriteEffect *)effect;
    struct SpriteParticle **it;

    EACH_SPRITE(self, it)
        if ((*it)->base.age > 0)
            sprite_particle_render(*it);
}
