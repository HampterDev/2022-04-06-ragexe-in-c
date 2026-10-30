#ifndef GAMEMODE_CAMERA_H
#define GAMEMODE_CAMERA_H

struct GameMode;

#include "world/view_types.h"

void game_mode_init_view(struct GameMode *self);
float game_mode_get_view_heading(struct GameMode *self);
void game_mode_point_mouse_at(struct GameMode *self, float x, float z, int red);

extern float g_fixedViewDistance;
extern float g_fixedViewDistanceMax;
extern float g_fixedViewPitch;
extern float g_freeViewDistance;
extern float g_freeViewPitch;
extern float g_viewDistance;
extern float g_viewDistanceMin;
extern float g_viewPitch;

#endif
