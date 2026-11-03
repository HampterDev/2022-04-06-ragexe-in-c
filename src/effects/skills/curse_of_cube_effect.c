#include "effects/effect_allocator.h"
#include "render/math3d.h"
#include <string.h>
#include "effects/particle.h"
#include "effects/skills/skill_view.h"
#include "effects/effect_controls.h"
#include "std/vec.h"

enum {
    EFFECT_CURSE_OF_RED_CUBE = 0x901,
    EFFECT_CURSE_OF_BLUE_CUBE = 0x902,
};

enum CubeStage {
    CUBE_NONE = 0,
    CUBE_CLOSING = 1,
    CUBE_LOOPING = 2,
    CUBE_SHOCKED = 3,
};

struct CurseOfCubeEffect {
    struct Effect base;
    struct Vec parts;
    int stage;
    Vec3 view_offset;
};

void curse_of_cube_effect_init(struct Effect *self);
void curse_of_cube_effect_process(struct Effect *self);
void curse_of_cube_effect_render(struct Effect *self);
void curse_of_cube_effect_release(struct Effect *self);
unsigned char curse_of_cube_effect_is_alive(struct Effect *self);
static void curse_of_cube_effect_destroy(struct Effect *effect);
static void curse_of_cube_effect_add_start(struct CurseOfCubeEffect *self);
static void curse_of_cube_effect_add_loop(struct CurseOfCubeEffect *self);
static void curse_of_cube_effect_add_shock(struct CurseOfCubeEffect *self);

static const struct EffectOps curse_of_cube_effect_ops = {
    .set_submit = effect_set_submit,
    .init = curse_of_cube_effect_init,
    .update_range = effect_update_range,
    .process = curse_of_cube_effect_process,
    .render = curse_of_cube_effect_render,
    .release = curse_of_cube_effect_release,
    .is_alive = curse_of_cube_effect_is_alive,
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
    .destroy = curse_of_cube_effect_destroy,
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

static int still_playing(const struct Ez2StrParticle *p)
{
    return p->base.life_seconds == 0.0f || p->base.life_seconds * 60.0f > (float)p->base.age;
}

static void add_part(struct CurseOfCubeEffect *self, int list_flags, const char *path, unsigned char loop)
{
    struct Ez2StrParticle *p = ez2str_particle_new(self->base.pos, list_flags);

    if (!p)
        return;
    ez2_str_particle_load(p, path);
    if (loop)
        p->loop_forever = 1;
    vec_push(&self->parts, &p, sizeof p);
}

struct CurseOfCubeEffect *curse_of_cube_effect_construct(struct CurseOfCubeEffect *self)
{
    effect_construct(&self->base, -1);
    self->base.ops = &curse_of_cube_effect_ops;
    self->parts.first = self->parts.last = self->parts.end = 0;
    self->stage = CUBE_NONE;
    self->view_offset.x = 0.0f;
    self->view_offset.y = -10.0f;
    self->view_offset.z = 0.0f;
    return self;
}

struct Effect *curse_of_cube_effect_new(void)
{
    struct CurseOfCubeEffect *self = mem_try_alloc(sizeof *self);

    return self ? &curse_of_cube_effect_construct(self)->base : NULL;
}

/* Effect types created as CurseOfCubeEffect, registered at startup through g_effectTypes
   (effects/effect_registry.c). A type missing here is not created. */
static const struct EffectTypeRow curse_of_cube_effect_type_rows[] = {
    EFFECT_TYPES(0x901, 0x902, curse_of_cube_effect_new),
};
const struct EffectTypeTable curse_of_cube_effect_types = EFFECT_TYPE_TABLE(curse_of_cube_effect_type_rows);

void curse_of_cube_effect_destruct(struct CurseOfCubeEffect *self)
{
    self->base.ops = &curse_of_cube_effect_ops;
    vec_free(&self->parts);
    effect_destruct(&self->base);
}

static void curse_of_cube_effect_destroy(struct Effect *effect)
{
    struct CurseOfCubeEffect *self = (struct CurseOfCubeEffect *)effect;
    curse_of_cube_effect_destruct(self);
    mem_free(self);
}

void curse_of_cube_effect_release(struct Effect *effect)
{
    struct CurseOfCubeEffect *self = (struct CurseOfCubeEffect *)effect;
    struct Ez2StrParticle **it;
    for (it = self->parts.first; it != (struct Ez2StrParticle **)self->parts.last; it++)
        particle_free(&(*it)->base);
    self->parts.last = self->parts.first;
}

static void curse_of_cube_effect_add_start(struct CurseOfCubeEffect *self)
{
    switch (self->base.type) {
    case EFFECT_CURSE_OF_RED_CUBE:
        add_part(self, 13, "NPC_CURSE_OF_RED_CUBE\\npc_cur_r_cube\\npc_cur_r_cube_bottom_start.str", 0);
        add_part(self, 5, "NPC_CURSE_OF_RED_CUBE\\npc_cur_r_cube\\npc_cur_r_cube_start.str", 0);
        break;
    case EFFECT_CURSE_OF_BLUE_CUBE:
        add_part(self, 13, "NPC_CURSE_OF_BLUE_CUBE\\npc_cur_b_cube\\npc_cur_b_cube_bottom_start.str", 0);
        add_part(self, 5, "NPC_CURSE_OF_BLUE_CUBE\\npc_cur_b_cube\\npc_cur_b_cube_start.str", 0);
        break;
    }
}

static void curse_of_cube_effect_add_loop(struct CurseOfCubeEffect *self)
{
    switch (self->base.type) {
    case EFFECT_CURSE_OF_RED_CUBE:
        add_part(self, 13, "NPC_CURSE_OF_RED_CUBE\\npc_cur_r_cube\\npc_cur_r_cube_bottom_loop.str", 1);
        add_part(self, 5, "NPC_CURSE_OF_RED_CUBE\\npc_cur_r_cube\\npc_cur_r_cube_loop.str", 1);
        break;
    case EFFECT_CURSE_OF_BLUE_CUBE:
        add_part(self, 13, "NPC_CURSE_OF_BLUE_CUBE\\npc_cur_b_cube\\npc_cur_b_cube_bottom_loop.str", 1);
        add_part(self, 5, "NPC_CURSE_OF_BLUE_CUBE\\npc_cur_b_cube\\npc_cur_b_cube_loop.str", 1);
        break;
    }
}

static void curse_of_cube_effect_add_shock(struct CurseOfCubeEffect *self)
{
    switch (self->base.type) {
    case EFFECT_CURSE_OF_RED_CUBE:
        add_part(self, 5, "NPC_CURSE_OF_RED_CUBE\\npc_cur_r_shock\\npc_cur_r_shock.str", 0);
        break;
    case EFFECT_CURSE_OF_BLUE_CUBE:
        add_part(self, 5, "NPC_CURSE_OF_BLUE_CUBE\\npc_cur_b_shock\\npc_cur_b_shock.str", 0);
        break;
    }
}

void curse_of_cube_effect_shock(struct Effect *effect)
{
    struct CurseOfCubeEffect *self = (struct CurseOfCubeEffect *)effect;
    curse_of_cube_effect_release(&self->base);
    curse_of_cube_effect_add_shock(self);
    self->stage = CUBE_SHOCKED;
}

void curse_of_cube_effect_init(struct Effect *effect)
{
    struct CurseOfCubeEffect *self = (struct CurseOfCubeEffect *)effect;
    self->base.has_instance_key = 1;
    curse_of_cube_effect_add_start(self);
    if (self->parts.first != self->parts.last)
        self->stage = CUBE_CLOSING;
}

unsigned char curse_of_cube_effect_is_alive(struct Effect *effect)
{
    struct CurseOfCubeEffect *self = (struct CurseOfCubeEffect *)effect;
    if (effect_is_owner_alive(&self->base) && self->parts.first != self->parts.last)
        return 1;
    return 0;
}

void curse_of_cube_effect_place_in_view(struct CurseOfCubeEffect *self, struct Ez2StrParticle *p)
{
    float view[12], from_view[12];
    Vec3 seen, zero = { 0, 0, 0 };

    if (!p)
        return;
    if (!game_copy_view_matrix(view))
        return;
    seen = transform(view, self->base.pos);
    mat_invert(from_view, view);
    seen.x = self->view_offset.x + seen.x;
    seen.y = self->view_offset.y + seen.y;
    seen.z = self->view_offset.z + seen.z;
    p->base.pos = transform(from_view, seen);
    ez2_str_particle_process(p, zero);
}

void curse_of_cube_effect_process(struct Effect *effect)
{
    struct CurseOfCubeEffect *self = (struct CurseOfCubeEffect *)effect;
    struct Ez2StrParticle **it = self->parts.first;

    while (it != (struct Ez2StrParticle **)self->parts.last) {
        struct Ez2StrParticle *p = *it;
        if (!still_playing(p)) {
            particle_free(&p->base);
            memmove(it, it + 1, (char *)self->parts.last - (char *)(it + 1));
            self->parts.last = (struct Ez2StrParticle **)self->parts.last - 1;
            continue;
        }
        if (p)
            curse_of_cube_effect_place_in_view(self, p);
        it++;
    }
    if (self->stage == CUBE_CLOSING && self->parts.first == self->parts.last) {
        curse_of_cube_effect_add_loop(self);
        if (self->parts.first != self->parts.last)
            self->stage = CUBE_LOOPING;
    }
}

void curse_of_cube_effect_render(struct Effect *effect)
{
    struct CurseOfCubeEffect *self = (struct CurseOfCubeEffect *)effect;
    struct Ez2StrParticle **it;

    for (it = self->parts.first; it != (struct Ez2StrParticle **)self->parts.last; it++) {
        ez2_str_particle_render_current(*it);
    }
}
