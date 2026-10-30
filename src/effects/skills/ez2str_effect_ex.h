#ifndef EFFECTS_SKILLS_EZ2STR_EFFECT_EX_H
#define EFFECTS_SKILLS_EZ2STR_EFFECT_EX_H

#include "effects/particle.h"

struct Ez2StrEffectEx {
    struct Effect base;
    struct Ez2StrParticle *particle;
    unsigned char fading;
    int start_frame;
};

struct Ez2StrEffectEx *ez2_str_effect_ex_construct(struct Ez2StrEffectEx *self);
void ez2_str_effect_ex_destruct(struct Ez2StrEffectEx *self);
void ez2_str_effect_ex_init(struct Effect *self);
void ez2_str_effect_ex_process(struct Effect *self);
void ez2_str_effect_ex_render(struct Effect *self);
void ez2_str_effect_ex_release(struct Effect *self);
unsigned char ez2_str_effect_ex_is_alive(struct Effect *self);
void ez2_str_effect_ex_set_stop_frame(struct Effect *self, float frame);
void ez2_str_effect_ex_set_start_delay(struct Effect *self, int ms);
void ez2_str_effect_ex_set_spin(struct Effect *self, float degrees);
void ez2_str_effect_ex_set_zoom(struct Effect *self, float zoom);
void ez2_str_effect_ex_set_alpha_scale(struct Effect *self, float scale);
void ez2_str_effect_ex_fade_out(struct Effect *self);

float effect_screen_angle(Vec3 from, Vec3 to, const float *view);

float effect_screen_angle_quadrant(Vec3 from, Vec3 to);

#endif
