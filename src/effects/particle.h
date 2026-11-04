#include "render/rp_pool.h"
#ifndef EFFECTS_PARTICLE_H
#define EFFECTS_PARTICLE_H

#include "effects/effect.h"
#include "effects/str_animation.h"
#include <stdint.h>

#define PARTICLE_TINT_BY_CLIP 0x200

#define PARTICLE_KEEP_CHANNEL ((short)-1)

#define PARTICLE_EFFECT_DATA_SIZE 32

enum ParticleKind {
    PARTICLE_KIND_PLAIN,
    PARTICLE_KIND_SPRITE,
    PARTICLE_KIND_EZ2STR,
    PARTICLE_KIND_MULTITEX,
    PARTICLE_KIND_COUNT
};

struct Particle {
    enum ParticleKind pool_kind;
    int effect_type;
    int owner_tag;
    int index;
    int group;
    int owner_extra;
    void *texture;
    int list_flags;
    float scale;
    Vec3 pos;
    Vec3 velocity;
    short r, g, b;
    int alpha;
    unsigned char effect_data[PARTICLE_EFFECT_DATA_SIZE];
    float roll;
    int roll_extra;
    float life_seconds;
    Vec3 offset;
    Vec3 second_offset;
    int age;
    unsigned flags;
    struct TexDrawPrim batch;
    float orientation[12];
    float matrix[12];
    struct TexDrawVertex corners[4];
    Vec3 corner_points[4];
    int src_blend;
    int dest_blend;
    float depth_bias;
    ParticleSubmitFn submit;
};
#define PARTICLE_EFFECT_DATA_FITS(type) \
    STATIC_ASSERT(type##_fits_effect_data, sizeof(struct type) <= PARTICLE_EFFECT_DATA_SIZE)

struct ParticleRenderArgs {
    unsigned char facing_fixed;
    float v_top, v_bottom;
    float u_left, u_right;
    unsigned char map_chain_flag;
    unsigned char with_depth;
};

struct Particle *particle_construct(struct Particle *self, Vec3 pos, int list_flags);
void particle_reset(struct Particle *self);
void particle_set_color(struct Particle *self, short r, short g, short b);
void particle_set_texture(struct Particle *self, struct String name);
void particle_render(struct Particle *self, unsigned char facing_fixed, float v_top, float v_bottom,
                                  float u_left, float u_right, unsigned char with_depth);
void particle_render_with(struct Particle *self, const struct ParticleRenderArgs *args);
void particle_quad_unit_corners(struct TexDrawVertex *quad);
void particle_quad_paint(struct Particle *self, struct TexDrawVertex *quad, float v_top, float v_bottom,
                                    float u_left, float u_right);
void particle_submit(struct TexDrawPrim *batch, int list_flags);

enum SpriteLoop {
    SPRITE_LOOP = 0,
    SPRITE_ONCE = 1,
    SPRITE_HOLD = 2,
    SPRITE_STILL = 3,
};

struct SpriteParticle {
    struct Particle base;
    Vec3 sprite_offset;
    float roll_step;
    float frame_delay;
    float frame_delay_step;
    void *spr;
    void *act;
    int action;
    int motion;
    int loop;
};

struct SpriteParticle *sprite_particle_construct(struct SpriteParticle *self, Vec3 pos, int list_flags);
void sprite_particle_reset(struct SpriteParticle *self);
void sprite_particle_set_act(struct SpriteParticle *self, const char *name);
void sprite_particle_set_spr(struct SpriteParticle *self, const char *name);
void sprite_particle_render(struct SpriteParticle *self);

#define EZ2STR_LAYERS 128

struct Ez2StrParticle {
    struct Particle base;
    struct EzEffectRes *res;
    struct StrAnim *anim;
    struct StrKeyState state[EZ2STR_LAYERS];
    int layers_done;
    int layers_to_play;
    unsigned char drawn[EZ2STR_LAYERS];
    int key_index[EZ2STR_LAYERS];
    Vec3 screen_pos;
    int frame;
    unsigned char loop_forever;
    struct String path;
    struct String texture_set;
    int min_frames;
    unsigned char stopping;
    int stop_frame;
    int loops;
    int start_delay;
    float spin_degrees;
    float zoom;
    float alpha_scale;
    int window_id;
    unsigned char anchor_to_window;
    unsigned char visible;
    Vec3 ui_offset;
    unsigned char scale_by_actor;
    int actor_aid;
    float screen_dx, screen_dy;
    int64_t lift;
    unsigned char flip_v;
    unsigned char flip_h;
    unsigned char sound_loops;
};

#define PARTICLE_LIST_UI_SCREEN 0x205
#define PARTICLE_LIST_UI_WINDOW 0x225

static __inline int ez2str_particle_is_ui(const struct Ez2StrParticle *self)
{
    return self->base.list_flags == PARTICLE_LIST_UI_SCREEN ||
           self->base.list_flags == PARTICLE_LIST_UI_WINDOW;
}

struct Ez2StrParticle *ez2_str_particle_construct(struct Ez2StrParticle *self, Vec3 pos, int list_flags);
void ez2_str_particle_reset(struct Ez2StrParticle *self);
void ez2_str_particle_destruct(struct Ez2StrParticle *self);
void ez2_str_particle_open(struct Ez2StrParticle *self, const char *path);
void ez2_str_particle_load(struct Ez2StrParticle *self, const char *path);
void ez2_str_particle_rewind(struct Ez2StrParticle *self);
unsigned char ez2_str_particle_process(struct Ez2StrParticle *self, Vec3 offset);
void ez2_str_particle_skip(struct Ez2StrParticle *self, float fraction);
void ez2_str_particle_render(struct Ez2StrParticle *self, const float *view);
void ez2_str_particle_render_current(struct Ez2StrParticle *self);

struct UV { float u, v; };

struct MultiTexParticle {
    struct Particle base;
    struct Texture *texture0, *texture1;
    struct UV uv0[4];
    struct UV uv1[4];
    float u_scroll, v_scroll;
};

struct MultiTexParticle *multi_tex_particle_construct(struct MultiTexParticle *self, Vec3 pos,
                                                                          int list_flags);
void multi_tex_particle_reset(struct MultiTexParticle *self);
void multi_tex_particle_render_on_cell(struct MultiTexParticle *self, int cell_x, int cell_y);

struct Particle *particle_new(Vec3 pos, int list_flags);
struct SpriteParticle *sprite_particle_new(Vec3 pos, int list_flags);
struct Ez2StrParticle *ez2str_particle_new(Vec3 pos, int list_flags);
struct MultiTexParticle *multitex_particle_new(Vec3 pos, int list_flags);
void particle_free(struct Particle *p);
unsigned char effect_memory_create(void);
void effect_memory_destroy(void);

#endif
