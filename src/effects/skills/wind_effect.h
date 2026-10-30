#ifndef EFFECTS_SKILLS_WIND_EFFECT_H
#define EFFECTS_SKILLS_WIND_EFFECT_H
#include "effects/particle.h"

struct WindEffect {
    struct Effect base;
    unsigned short rings;
    unsigned char max_alpha;
    unsigned char r, g, b;
    float radius;
    float band_width;
    float height;
    float spin;
    unsigned short arc;
    float tilt_x;
    float tilt_y;
    float tilt_z;
    int src_blend;
    int dest_blend;
    struct String texture;
    Vec3 prev_bottom;
    Vec3 prev_top;
    struct Vec segments;
};

#endif
