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

/* custom: how far the camera may zoom out, on every map; the original stops at 400 (480 with the
   wide zoom option) outdoors and 300 indoors */
#define VIEW_DISTANCE_LIMIT 10000.0f
/* custom: how far the camera may tilt, on every map, from nearly straight down to nearly level;
   the original keeps it within -65..-25 outdoors (-50 without Ctrl) and -55..-35 indoors. The view
   divides by the sine and cosine of the tilt, so it stays clear of 0 and -90. */
#define VIEW_PITCH_STEEPEST -85.0f
#define VIEW_PITCH_FLATTEST -5.0f
extern float g_viewPitch;

#endif
