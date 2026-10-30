#ifndef EFFECTS_SKILLS_EVILS_PAW_EFFECT_H
#define EFFECTS_SKILLS_EVILS_PAW_EFFECT_H

#include "effects/particle.h"

struct EvilsPawEffect {
    struct Effect base;
    struct Vec particles;
    int frame_delay;
    int particle_scale;
    int frame_countdown;
    Vec3 offset;
    struct Vec frames;
    int frame;
};

#endif
