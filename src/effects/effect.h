#include "render/rp_pool.h"
#include "session/state.h"
#include "world/mode_manager.h"
#include "game/context.h"
#ifndef EFFECTS_EFFECT_H
#define EFFECTS_EFFECT_H

#include "render/math3d.h"

#include "base.h"
#include "render/renderer.h"

#include "render/vec3.h"
#include "actor/types.h"
#include <stdint.h>

enum SoundCueKind {
    SOUND_CUE_WAVE = 0,
    SOUND_CUE_GAME_MESSAGE = 1,
};
struct SoundCue {
    int kind;
    int frame;
    float height;
    struct String wave;
};

struct Effect;

struct TexDrawPrim;
typedef void (*ParticleSubmitFn)(struct TexDrawPrim *batch, int list_flags);

struct EffectOps {
    void (*set_submit)(struct Effect *, ParticleSubmitFn fn);
    void (*init)(struct Effect *);
    void (*update_range)(struct Effect *);
    void (*process)(struct Effect *);
    void (*render)(struct Effect *);
    void (*release)(struct Effect *);
    unsigned char (*is_alive)(struct Effect *);
    void (*restart)(struct Effect *);
    void (*set_color)(struct Effect *, int r, int g, int b);
    void (*set_texture)(struct Effect *, struct String name);
    void (*after_init)(struct Effect *);
    void (*skip)(struct Effect *, float fraction);
    void (*set_stop_frame)(struct Effect *, float frame);
    void (*set_start_delay)(struct Effect *, int ms);
    void (*set_spin)(struct Effect *, float degrees);
    void (*set_str_name)(struct Effect *, const char *name);
    void (*set_list_flags)(struct Effect *, int list_flags);
    void (*set_zoom)(struct Effect *, float zoom);
    void (*set_alpha_scale)(struct Effect *, float scale);
    void (*destroy)(struct Effect *);
    void (*fade_out)(struct Effect *);
};

struct EffectParams {
    float area;
    float scale;
    int roll;
    float rise_angle;
    int alpha;
    int full_display_angle;
    float values[21];
};
SIZE_ASSERT(EffectParams, 0x6c);

struct Effect {
    const struct EffectOps *ops;
    int type;
    Vec3 pos;
    Vec3 target_pos;
    int owner_aid;
    int target_aid;
    int frame;
    int duration;
    int variant;
    unsigned char out_of_range;
    int view_range;
    struct EffectParams params;
    int level;
    struct Vec sound_cues;
    unsigned char has_instance_key;
    unsigned char paused;
    unsigned char always_draw;
    int instance_key;
};

struct Effect *effect_construct(struct Effect *self, int type);
void effect_destruct(struct Effect *self);
void effect_set_owner(struct Effect *self, int aid);
void effect_set_target(struct Effect *self, int aid);
void effect_add_sound_cue(struct Effect *self, int kind, const char *wave, int frame, float height);
unsigned char effect_is_owner_alive(struct Effect *self);
unsigned char effect_is_finished(struct Effect *self);
unsigned char effect_is_within_duration(struct Effect *self);
void effect_update_range(struct Effect *self);
unsigned char effect_is_in_range(struct Effect *self);

void effect_set_submit(struct Effect *self, ParticleSubmitFn fn);
void effect_init(struct Effect *self);
void effect_process(struct Effect *self);
void effect_render(struct Effect *self);
void effect_release(struct Effect *self);
void effect_restart(struct Effect *self);
void effect_set_color(struct Effect *self, int r, int g, int b);
void effect_set_texture(struct Effect *self, struct String name);
void effect_after_init(struct Effect *self);
void effect_skip(struct Effect *self, float fraction);
void effect_set_stop_frame(struct Effect *self, float frame);
void effect_set_start_delay(struct Effect *self, int ms);
void effect_set_spin(struct Effect *self, float degrees);
void effect_set_str_name(struct Effect *self, const char *name);
void effect_set_list_flags(struct Effect *self, int list_flags);
void effect_set_zoom(struct Effect *self, float zoom);
void effect_set_alpha_scale(struct Effect *self, float scale);
void effect_fade_out(struct Effect *self);

struct Renderer;

extern struct EffectMgr *g_effectMgr;

void *effect_find_actor(int aid);
void effect_play_wave_at(const char *wave, const Vec3 *position, float height);
struct World;
struct RenderObject;
int effect_add_world_object(struct World *expected_world, struct RenderObject *object);
Vec3 *effect_player_position(Vec3 *out);

float *effect_random_point_in_disc(float *out, float cx, float cy, int64_t radius);

#define ACTOR_POS(actor) (&((const struct RenderObject *)(actor))->pos)

unsigned char effect_remove_if_dead(struct Effect *self);
void effect_render_if_visible(struct Effect *self);
void effect_set_sound_cue_frame(struct Effect *self, int kind, int frame);
void effect_step(struct Effect *self, int owner_aid);

#endif
