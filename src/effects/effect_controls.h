#ifndef EFFECTS_EFFECT_CONTROLS_H
#define EFFECTS_EFFECT_CONTROLS_H

#include "platform/win_types.h"
#include "render/vec3.h"
#include <stdint.h>

struct Effect;

void curse_of_cube_effect_shock(struct Effect *effect);
void energy_orb_effect_set_ring_count(struct Effect *effect, int count);

void warning_plane_effect_add_plane(struct Effect *effect, Vec3 pos);
void warning_plane_effect_set_end_time(struct Effect *effect, DWORD end_time);

#endif
