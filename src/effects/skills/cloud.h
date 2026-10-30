#ifndef EFFECTS_SKILLS_CLOUD_H
#define EFFECTS_SKILLS_CLOUD_H

#include "effects/effect.h"

struct Cloud {
    struct Effect base;
    struct Particle *puff;
    int drift_x;
    int drift_x_goal;
    int drift_z;
    int drift_z_goal;
    int pulse_phase;
    float size;
    float alpha;
    int fade_in_until;
    int fade_out_after;
    unsigned char color[3];
    float size_min;
    float size_range;
    float pulse;
    float fade_in_step;
    float fade_out_step;
    int fade_in_min, fade_in_range;
    int fade_out_min, fade_out_range;
    float spread_y;
    float spread_x;
    float spread_z;
};

void cloud_respawn(struct Effect *self);

#endif
