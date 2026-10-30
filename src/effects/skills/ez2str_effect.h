#ifndef EFFECTS_SKILLS_EZ2STR_EFFECT_H
#define EFFECTS_SKILLS_EZ2STR_EFFECT_H

#include "effects/particle.h"

struct Ez2StrEffect {
    struct Effect base;
    struct Ez2StrParticle *particle;
    struct String str_name;
    unsigned char adjust_position_when_shrunk;
    unsigned screen_shade;
    unsigned char turn_with_camera;
    double camera_angle;
    unsigned char attached_to_head;
};

#define EZ2STR_LIST_DEFAULT        5
#define EZ2STR_LIST_BEFORE_ACTORS  0xd
#define EZ2STR_LIST_SCREEN_FILL    0x205

struct Ez2StrEffect *ez2_str_effect_construct(struct Ez2StrEffect *self);
void ez2_str_effect_destruct(struct Ez2StrEffect *self);
void ez2_str_effect_set_submit(struct Effect *self, ParticleSubmitFn fn);
void ez2_str_effect_init(struct Effect *self);
void ez2_str_effect_process(struct Effect *self);
void ez2_str_effect_render(struct Effect *self);
void ez2_str_effect_release(struct Effect *self);
unsigned char ez2_str_effect_is_alive(struct Effect *self);
void ez2_str_effect_after_init(struct Effect *self);
void ez2_str_effect_skip(struct Effect *self, float fraction);
void ez2_str_effect_set_stop_frame(struct Effect *self, float frame);
void ez2_str_effect_set_start_delay(struct Effect *self, int ms);
void ez2_str_effect_set_spin(struct Effect *self, float degrees);
void ez2_str_effect_set_str_name(struct Effect *self, const char *name);
void ez2_str_effect_set_list_flags(struct Effect *self, int list_flags);
void ez2_str_effect_set_zoom(struct Effect *self, float zoom);
void ez2_str_effect_set_alpha_scale(struct Effect *self, float scale);

unsigned char ez2_str_effect_is_headgear_type(struct Ez2StrEffect *self);
unsigned char ez2_str_effect_is_head_type(struct Ez2StrEffect *self);
unsigned char ez2_str_effect_place_at_head(struct Ez2StrEffect *self, struct Ez2StrParticle *p);
void ez2_str_effect_step_particle(struct Ez2StrEffect *self, struct Ez2StrParticle *p);
void ez2_str_effect_draw_particle(struct Ez2StrEffect *self, struct Ez2StrParticle *p);
void ez2_str_effect_follow_camera(struct Ez2StrEffect *self);
float ez2str_head_x_offset(int type, const char *str_path);
int ez2str_head_height(int type, void *actor);

void ez2_str_effect_open_for_type(struct Ez2StrEffect *self, struct Ez2StrParticle *p, int type);

#endif
