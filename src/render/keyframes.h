#ifndef RENDER_KEYFRAMES_H
#define RENDER_KEYFRAMES_H

#include "render/math3d.h"
#include <stddef.h>

struct File;
struct KeyPos { int frame; float x, y, z; };
struct KeyVec { int frame; float x, y, z; int data; };
struct KeyScaleAxis { int frame; float x, y, z, qx, qy, qz, qw; };
struct KeyFloat { int frame; float value; };
SIZE_ASSERT(KeyPos, 16);
SIZE_ASSERT(KeyVec, 20);
SIZE_ASSERT(KeyScaleAxis, 32);
SIZE_ASSERT(KeyFloat, 8);

#define KEYFLOAT_CHANNELS 5
struct KeyFloatTrack { struct KeyFloat *keys; size_t count; };
struct UvKeyTrack { int texture; struct KeyFloatTrack channels[KEYFLOAT_CHANNELS]; };
struct UvAnimation { struct UvKeyTrack *tracks; size_t count; };

void key_pos_matrix(const struct KeyPos *keys, size_t count, int frame, float *out);
void key_quat_matrix(const struct KeyQuat *keys, size_t count, int frame, float *out);
void key_scale_matrix(const struct KeyScaleAxis *keys, size_t count, float *out);
void key_vec_translation(const struct KeyVec *keys, size_t count, float t, const float *base, float *out);
void key_quat_apply(const struct KeyQuat *keys, size_t count, float t, const float *base, float *out);
void key_vec_scale(const struct KeyVec *keys, size_t count, float t, float *out);
void key_uv_channels_eval(const struct KeyFloatTrack channels[KEYFLOAT_CHANNELS], float t, float out[KEYFLOAT_CHANNELS]);
void key_quat_from_axis_angle(struct KeyQuat *key, const float axis[3], float angle);

int key_vec_read(struct File *file, struct KeyVec **keys, size_t *count);
int key_quat_read(struct File *file, struct KeyQuat **keys, size_t *count);
int key_uv_read(struct File *file, struct UvAnimation *animation);
void key_uv_clear(struct UvAnimation *animation);

#endif
