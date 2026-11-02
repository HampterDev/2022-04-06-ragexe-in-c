#include "actor/actor_ops.h"
#include "contents/sound.h"
#include "session/state.h"
#include <math.h>
#include <string.h>
#include "effects/effect_mgr.h"
#include "effects/render_object.h"
#include "game/context.h"
#include "session/session_skills.h"

struct Effect;
struct String;

static void effect_destroy(struct Effect *effect);

static const struct EffectOps effect_ops = {
    .set_submit = effect_set_submit,
    .init = effect_init,
    .update_range = effect_update_range,
    .process = effect_process,
    .render = effect_render,
    .release = effect_release,
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
    .destroy = effect_destroy,
    .fade_out = effect_fade_out,
};

#define GAME_MSG_QUAKE 0x7a

#define ACTOR_IS_PC(actor) ((actor)->is_pc)

static void sound_cues_free(struct Vec *cues)
{
    struct SoundCue *cue;

    for (cue = cues->first; cue != cues->last; cue++)
        string_free(&cue->wave);
    vec_free(cues);
}

struct Effect *effect_construct(struct Effect *self, int type)
{
    self->ops = &effect_ops;
    self->type = type;
    self->pos.x = self->pos.y = self->pos.z = 0;
    self->target_pos.x = self->target_pos.y = self->target_pos.z = 0;
    self->owner_aid = 0;
    self->target_aid = 0;
    self->frame = 0;
    self->duration = 60;
    self->variant = 1;
    self->out_of_range = 0;
    self->view_range = 130;
    self->level = -1;
    self->sound_cues.first = self->sound_cues.last = self->sound_cues.end = 0;
    self->has_instance_key = 0;
    self->always_draw = 0;
    self->instance_key = 0;
    return self;
}

void effect_destruct(struct Effect *self)
{
    self->ops = &effect_ops;
    sound_cues_free(&self->sound_cues);
}

static void effect_destroy(struct Effect *self)
{
    effect_destruct(self);
    mem_free(self);
}

void effect_set_owner(struct Effect *self, int aid)
{
    self->owner_aid = aid;
}

void effect_set_target(struct Effect *self, int aid)
{
    void *actor = effect_find_actor(aid);
    if (actor) {
        self->target_aid = aid;
        self->target_pos = *ACTOR_POS(actor);
    }
}

unsigned char effect_is_owner_alive(struct Effect *self)
{
    if (self->owner_aid && !effect_find_actor(self->owner_aid))
        return 0;
    return 1;
}

unsigned char effect_is_finished(struct Effect *self)
{
    return self->frame >= self->duration;
}

unsigned char effect_is_within_duration(struct Effect *self)
{
    return self->frame < self->duration;
}

void effect_add_sound_cue(struct Effect *self, int kind, const char *wave,
                                     int frame, float height)
{
    struct SoundCue cue, copy;
    const char *text = wave ? wave : "";

    string_init(&cue.wave);
    string_assign_cstr(&cue.wave, text);
    cue.kind = kind;
    cue.frame = frame;
    cue.height = height;
    copy.kind = cue.kind;
    copy.frame = cue.frame;
    copy.height = cue.height;
    string_copy(&copy.wave, &cue.wave);
    vec_push(&self->sound_cues, &copy, sizeof copy);
    string_free(&cue.wave);
}

void effect_set_sound_cue_frame(struct Effect *self, int kind, int frame)
{
    struct SoundCue *cue;
    for (cue = self->sound_cues.first; cue != (struct SoundCue *)self->sound_cues.last; cue++)
        if (cue->kind == kind) {
            cue->frame = frame;
            return;
        }
}

void effect_play_sound_cues(struct Effect *self)
{
    const Vec3 here = self->pos;
    struct SoundCue *cue;

    for (cue = self->sound_cues.first; cue != (struct SoundCue *)self->sound_cues.last; cue++) {
        if (cue->frame != self->frame)
            continue;
        if (cue->kind == SOUND_CUE_WAVE) {
            if (cue->wave.size)
                effect_play_wave_at(string_cstr(&cue->wave), &here, cue->height);
        } else if (cue->kind == SOUND_CUE_GAME_MESSAGE) {
            game_send_message(GAME_MSG_QUAKE, 0, 0, 0, 0);
        }
    }
}

void effect_update_range(struct Effect *self)
{
    Vec3 here = self->pos, player;
    float dx, dy, dz, distance;

    effect_player_position(&player);
    dx = player.x - here.x;
    dy = player.y - here.y;
    dz = player.z - here.z;
    distance = sqrtf(dy * dy + dx * dx + dz * dz);
    self->out_of_range = !(distance < (float)self->view_range);
}

unsigned char effect_is_in_range(struct Effect *self)
{
    Vec3 here = self->pos, player;
    float dx, dy, dz, distance;

    effect_player_position(&player);
    dx = player.x - here.x;
    dy = player.y - here.y;
    dz = player.z - here.z;
    distance = sqrtf(dy * dy + dx * dx + dz * dz);
    return distance < (float)self->view_range;
}

void effect_step(struct Effect *self, int owner_aid)
{
    void *owner = effect_find_actor(owner_aid);
    if (owner)
        self->pos = *ACTOR_POS(owner);
    self->ops->update_range(self);
    if (self->out_of_range)
        return;
    effect_play_sound_cues(self);
    self->ops->process(self);
    self->frame++;
}

unsigned char effect_remove_if_dead(struct Effect *self)
{
    if (self->ops->is_alive(self))
        return 0;
    effect_mgr_remove(g_effectMgr, self, 0);
    return 1;
}

void effect_render_if_visible(struct Effect *self)
{
    struct RenderObject *owner;

    if (!g_session1.effectOn && !self->always_draw)
        return;
    owner = effect_find_actor(self->owner_aid);
    if (owner && self->owner_aid != g_session1.ownAid && ACTOR_IS_PC(owner) == 1) {
        if (!owner->visible || !owner->revealed)
            return;
        if (is_effect_state_star2(actor_get_effect_state(owner)))
            return;
    }
    if (!self->out_of_range)
        self->ops->render(self);
}

void effect_set_submit(struct Effect *self, ParticleSubmitFn fn) {}
void effect_init(struct Effect *self) {}
void effect_process(struct Effect *self) {}
void effect_render(struct Effect *self) {}
void effect_release(struct Effect *self) {}
void effect_restart(struct Effect *self) {}
void effect_set_color(struct Effect *self, int r, int g, int b) {}
void effect_set_texture(struct Effect *self, struct String name)
{
    string_free(&name);
}
void effect_after_init(struct Effect *self) {}
void effect_skip(struct Effect *self, float fraction) {}
void effect_set_stop_frame(struct Effect *self, float frame) {}
void effect_set_start_delay(struct Effect *self, int ms) {}
void effect_set_spin(struct Effect *self, float degrees) {}
void effect_set_str_name(struct Effect *self, const char *name) {}
void effect_set_list_flags(struct Effect *self, int list_flags) {}
void effect_set_zoom(struct Effect *self, float zoom) {}
void effect_set_alpha_scale(struct Effect *self, float scale) {}
void effect_fade_out(struct Effect *self) {}
