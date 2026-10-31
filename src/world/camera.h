#include "world/view_api.h"
#ifndef WORLD_CAMERA_H
#define WORLD_CAMERA_H

#include "world/world.h"

#include "world/view_types.h"

#define VIEW_DEG_TO_RAD(d)  ((d) / 180.0f * 3.141592f)

void view_process_camera(struct View *self);
extern int g_drawnCellMinX, g_drawnCellMinY, g_drawnCellMaxX, g_drawnCellMaxY;

void view_add_longitude(struct View *self, float d);
void view_get_view_dir(struct View *self, Vec3 *dir);
float *view_get_view_matrix_at(struct View *self, float *out, float longitude);
void view_set_quake(struct View *self, int on, int duration, float sideways,
                               float distance, float latitude);
void view_set_quake_amplitude(struct View *self, float sideways, float distance,
                                        float latitude);

#endif
