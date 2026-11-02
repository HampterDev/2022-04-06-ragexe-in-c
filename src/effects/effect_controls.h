#ifndef EFFECTS_EFFECT_CONTROLS_H
#define EFFECTS_EFFECT_CONTROLS_H

#include <windows.h>
#include "render/vec3.h"

struct Effect;

void servant_weapon_effect_add_cast(struct Effect *effect);

void from_the_abyss_effect_set_ball_count(struct Effect *effect, __int64 count);
void charging_pierce_effect_set_charges(struct Effect *effect, unsigned __int64 count);
void curse_of_cube_effect_shock(struct Effect *effect);
void energy_orb_effect_set_ring_count(struct Effect *effect, int count);
void richs_coin_effect_set_coin_count(struct Effect *effect, int count);

void warning_plane_effect_add_plane(struct Effect *effect, Vec3 pos);
void warning_plane_effect_set_end_time(struct Effect *effect, DWORD end_time);

#endif
