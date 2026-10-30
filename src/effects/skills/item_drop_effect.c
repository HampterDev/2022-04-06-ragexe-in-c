#include "effects/effect_allocator.h"
#include "resources/sprite.h"
#include "resources/animation.h"
#include "world/object_lookup.h"
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include "effects/particle.h"
#include "effects/skills/skill_view.h"
#include "effects/render_object.h"
#include "std/std.h"

enum {
    EFFECT_DROP_YELLOW = 0x4a5,
    EFFECT_DROP_PURPLE = 0x4a6,
    EFFECT_DROP_BLUE = 0x74d,
    EFFECT_DROP_GREEN = 0x74e,
    EFFECT_DROP_RED = 0x74f,
};

#define LIST_WORLD 5
#define LIST_ITEM_DROP 0xd
#define DROP_LAYERS 2

struct ItemDropEffect {
    struct Effect base;
    int layer_count;
    struct Vec layers;
    struct String sound_dir;
    struct String wave;
    Vec3 item_pos;
};

void item_drop_effect_init(struct Effect *self);
void item_drop_effect_process(struct Effect *self);
void item_drop_effect_render(struct Effect *self);
void item_drop_effect_release(struct Effect *self);
unsigned char item_drop_effect_is_alive(struct Effect *self);
void item_drop_effect_after_init(struct Effect *self);
static void item_drop_effect_destroy(struct Effect *effect);

static const struct EffectOps item_drop_effect_ops = {
    .set_submit = effect_set_submit,
    .init = item_drop_effect_init,
    .update_range = effect_update_range,
    .process = item_drop_effect_process,
    .render = item_drop_effect_render,
    .release = item_drop_effect_release,
    .is_alive = item_drop_effect_is_alive,
    .restart = effect_restart,
    .set_color = effect_set_color,
    .set_texture = effect_set_texture,
    .after_init = item_drop_effect_after_init,
    .skip = effect_skip,
    .set_stop_frame = effect_set_stop_frame,
    .set_start_delay = effect_set_start_delay,
    .set_spin = effect_set_spin,
    .set_str_name = effect_set_str_name,
    .set_list_flags = effect_set_list_flags,
    .set_zoom = effect_set_zoom,
    .set_alpha_scale = effect_set_alpha_scale,
    .destroy = item_drop_effect_destroy,
    .fade_out = effect_fade_out,
};

static const Vec3 no_offset = { 0, 0, 0 };

static struct Ez2StrParticle **layer_at(struct ItemDropEffect *self, size_t i)
{
    return (struct Ez2StrParticle **)self->layers.first + i;
}

static size_t layer_total(const struct ItemDropEffect *self)
{
    return VEC_COUNT(&self->layers, struct Ez2StrParticle *);
}

static void load_str(struct Ez2StrParticle *p, const char *name)
{
    ez2_str_particle_load(p, name);
}

struct ItemDropEffect *item_drop_effect_construct(struct ItemDropEffect *self)
{
    effect_construct(&self->base, -1);
    self->base.ops = &item_drop_effect_ops;
    self->layer_count = DROP_LAYERS;
    self->layers.first = self->layers.last = self->layers.end = 0;
    string_init(&self->sound_dir);
    string_assign_cstr(&self->sound_dir, "wav\\effect\\");
    string_init(&self->wave);
    self->item_pos.x = self->item_pos.y = self->item_pos.z = 0;

    vec_reserve(&self->layers, (unsigned)self->layer_count, sizeof(struct Ez2StrParticle *));
    return self;
}

struct Effect *item_drop_effect_new(void)
{
    struct ItemDropEffect *self = mem_try_alloc(sizeof *self);

    return self ? &item_drop_effect_construct(self)->base : NULL;
}

void item_drop_effect_destruct(struct ItemDropEffect *self)
{
    self->base.ops = &item_drop_effect_ops;
    string_free(&self->wave);
    string_free(&self->sound_dir);
    vec_free(&self->layers);
    effect_destruct(&self->base);
}

static void item_drop_effect_destroy(struct Effect *effect)
{
    struct ItemDropEffect *self = (struct ItemDropEffect *)effect;
    item_drop_effect_destruct(self);
    mem_free(self);
}

void item_drop_effect_release(struct Effect *effect)
{
    struct ItemDropEffect *self = (struct ItemDropEffect *)effect;
    size_t i;
    for (i = 0; i < layer_total(self); i++)
        if (*layer_at(self, i))
            particle_free(&(*layer_at(self, i))->base);
}

void item_drop_effect_place_on_item(struct ItemDropEffect *self)
{
    size_t i;

    self->item_pos = self->base.pos;
    for (i = 0; i < layer_total(self); i++) {
        struct Ez2StrParticle *p = *layer_at(self, i);
        if (p)
            p->base.pos = self->item_pos;
    }
}

void item_drop_effect_set_colour(struct ItemDropEffect *self)
{
    struct Ez2StrParticle **layers = layer_at(self, 0);
    const char *wave;

    switch (self->base.type) {
    case EFFECT_DROP_PURPLE:
        load_str(layers[0], "new_dropitem\\dropitem_purple\\dropitem_purple\\dropitem_purple.str");
        load_str(layers[1], "new_dropitem\\dropitem_purple\\dropitem_purple_bottom\\dropitem_purple_bottom.str");
        wave = "drop_purple.wav";
        break;
    case EFFECT_DROP_YELLOW:
        load_str(layers[0], "new_dropitem\\dropitem_yellow\\dropitem_yellow\\dropitem_yellow.str");
        load_str(layers[1], "new_dropitem\\dropitem_yellow\\dropitem_yellow_bottom\\dropitem_yellow_bottom.str");
        wave = "drop_yellow.wav";
        break;
    case EFFECT_DROP_BLUE:
        load_str(layers[0], "new_dropitem\\dropitem_blue\\dropitem_blue\\dropitem_blue.str");
        load_str(layers[1], "new_dropitem\\dropitem_blue\\dropitem_blue_bottom\\dropitem_blue_bottom.str");
        wave = "drop_blue.wav";
        break;
    case EFFECT_DROP_GREEN:
        load_str(layers[0], "new_dropitem\\dropitem_green\\dropitem_green\\dropitem_green.str");
        load_str(layers[1], "new_dropitem\\dropitem_green\\dropitem_green_bottom\\dropitem_green_bottom.str");
        wave = "drop_green.wav";
        break;
    case EFFECT_DROP_RED:
        load_str(layers[0], "new_dropitem\\dropitem_red\\dropitem_red\\dropitem_red.str");
        load_str(layers[1], "new_dropitem\\dropitem_red\\dropitem_red_bottom\\dropitem_red_bottom.str");
        wave = "drop_red.wav";
        break;
    default:
        load_str(layers[0], "new_dropitem\\dropitem_pink\\dropitem_pink\\dropitem_pink.str");
        load_str(layers[1], "new_dropitem\\dropitem_pink\\dropitem_pink_bottom\\dropitem_pink_bottom.str");
        wave = "drop_pink.wav";
        break;
    }
    string_assign_cstr(&self->wave, wave);
}

void item_drop_effect_init(struct Effect *effect)
{
    struct ItemDropEffect *self = (struct ItemDropEffect *)effect;
    int i;

    if (!game_current_world())
        return;
    for (i = 0; i < self->layer_count; i++) {
        struct Ez2StrParticle *p = ez2str_particle_new(self->base.pos, LIST_WORLD);
        if (!p)
            return;
        particle_quad_unit_corners(p->base.corners);
        vec_push(&self->layers, &p, sizeof(struct Ez2StrParticle *));
    }
    item_drop_effect_set_colour(self);
    item_drop_effect_after_init(&self->base);
    item_drop_effect_place_on_item(self);
}

void item_drop_effect_after_init(struct Effect *effect)
{
    struct ItemDropEffect *self = (struct ItemDropEffect *)effect;
    struct String path;
    size_t i;

    for (i = 0; i < layer_total(self); i++) {
        struct Ez2StrParticle *p = *layer_at(self, i);
        if (p) {
            p->loop_forever = 1;
            p->base.list_flags = LIST_ITEM_DROP;
        }
    }
    string_copy(&path, &self->sound_dir);
    string_append(&path, string_cstr(&self->wave), self->wave.size);
    effect_add_sound_cue(&self->base, SOUND_CUE_WAVE, string_cstr(&path), 0, 0.0f);
    string_free(&path);
}

unsigned char item_drop_effect_is_alive(struct Effect *effect)
{
    struct ItemDropEffect *self = (struct ItemDropEffect *)effect;
    void *world = game_current_world();
    int aid = self->base.owner_aid;
    return world && aid != 0 && world_get_item_by_aid(world, aid) != NULL;
}

void item_drop_effect_process(struct Effect *effect)
{
    struct ItemDropEffect *self = (struct ItemDropEffect *)effect;
    size_t i;
    for (i = 0; i < layer_total(self); i++) {
        struct Ez2StrParticle *p = *layer_at(self, i);
        if (p) {
            ez2_str_particle_process(p, no_offset);
            (*layer_at(self, i))->base.age++;
        }
    }
}

void item_drop_effect_render(struct Effect *effect)
{
    struct ItemDropEffect *self = (struct ItemDropEffect *)effect;
    size_t i;

    for (i = 0; i < layer_total(self); i++) {
        if (*layer_at(self, i)) {
            ez2_str_particle_render_current(*layer_at(self, i));
        }
    }
}
