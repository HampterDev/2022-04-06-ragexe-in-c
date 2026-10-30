#include "render/rp_pool.h"
#ifndef EFFECTS_EFFECT_MATH_H
#define EFFECTS_EFFECT_MATH_H

#include "render/projection.h"

#include "render/math3d.h"

#include "effects/effect.h"

float effect_random_tenths(float lo, float hi);

float effect_heading_deg(float x, float z);

double effect_screen_heading(Vec3 from, Vec3 to);

#endif
