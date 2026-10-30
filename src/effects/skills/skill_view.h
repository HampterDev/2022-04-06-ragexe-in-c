#ifndef EFFECTS_SKILLS_SKILL_VIEW_H
#define EFFECTS_SKILLS_SKILL_VIEW_H

#include "render/math3d.h"

#include "effects/effect.h"
#include "game/context.h"

#include <string.h>

#define DEG_TO_RAD(d) ((d) / 180.0f * 3.141592f)

static __inline float degrees_to_radians(float deg)
{
    return deg / g_halfTurnDegrees * g_pi;
}

struct CaneOfEvilEyeEffect;
void cane_of_evil_eye_effect_end(struct CaneOfEvilEyeEffect *self);

struct EffectMaker *effect_maker_new(void);

struct FallenAngelEffect;
void fallen_angel_effect_set_position(struct FallenAngelEffect *self, Vec3 pos);

struct GroundScope *ground_scope_new(void);

struct ShowEffect;
void show_effect_add(struct ShowEffect *self, int show_id, int effect_type);
int show_effect_count(struct ShowEffect *self);
void show_effect_remove(struct ShowEffect *self, int show_id);

struct TalismanMarkEffect;
void talisman_mark_effect_finish(struct TalismanMarkEffect *self);

struct TetraBallEffect;
void tetra_ball_effect_add_ball(struct TetraBallEffect *self, int element, int slot);
int tetra_ball_effect_count(struct TetraBallEffect *self);
void tetra_ball_effect_move_first_to(struct TetraBallEffect *self, int slot);
void tetra_ball_effect_remove_ball(struct TetraBallEffect *self, int slot);

#endif
