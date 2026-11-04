#ifndef RENDER_MATH3D_H
#define RENDER_MATH3D_H

#include <stdint.h>
#include "core/layout.h"
#include "render/vec3.h"

struct Mat34 {
    float m[12];
};
SIZE_ASSERT(Mat34, 0x30);
SIZE_ASSERT(Vec3, 0xc);

struct KeyQuat {
    int frame;
    float x, y, z, w;
};
SIZE_ASSERT(KeyQuat, 0x14);

struct Ray3 {
    struct Vec3 origin;
    struct Vec3 dir;
    float length;
};
OFFSET_ASSERT(Ray3, length, 0x18);

struct OrientedBox {
    float half[3];
    struct Vec3 centre;
    struct Vec3 axis[3];
};
SIZE_ASSERT(OrientedBox, 0x3c);

extern struct Mat34 g_identityMatrix;
extern float g_radianTable[361], g_negRadianTable[361];
extern float g_sinTable[361], g_negSinTable[361];
extern float g_cosTable[361], g_negCosTable[361];
extern float *const g_degree_tables[6];
extern const float g_halfTurnDegrees;
extern const float g_pi;

float float_sin(float x);
float float_cos(float x);
float float_sqrt(float x);

void mat_multiply(float *out, const float *a, const float *b);

void mat_set_translation(float *m, float x, float y, float z);
void mat_set_scale(float *m, float x, float y, float z);
void mat_set_rotate_x(float *m, float rad);
void mat_set_rotate_y(float *m, float rad);
void mat_set_rotate_z(float *m, float rad);
void mat_scale(float *m, float x, float y, float z);
void mat_translate(float *m, const struct Vec3 *v);
void mat_rotate_x(float *m, float rad);
void mat_rotate_y(float *m, float rad);
void mat_rotate_z(float *m, float rad);
void mat_set_rotate_axis(float *m, const struct Vec3 *axis, float rad);
void mat_invert(float *out, const float *m);
int mat_is_identity(const float *m);

void quat_to_matrix(float *m, const struct KeyQuat *q);
void quat_to_matrix_b(float *m, const struct KeyQuat *q);
struct KeyQuat *matrix_to_quat(const float *m, struct KeyQuat *out);
struct KeyQuat *quat_identity(struct KeyQuat *q);

void math_init_degree_tables(void);
float get_cos_deg(int deg);
float deg_to_rad(int deg);
float get_sin_deg(int deg);

int ray_hits_obb(const struct Ray3 *ray, const struct OrientedBox *box);
int ray_triangle_hit(const struct Ray3 *ray, const struct Vec3 *v0, const struct Vec3 *v1,
                         const struct Vec3 *v2, float *u, float *v, float *t);
void mat_look_at(float *m, const struct Vec3 *eye,
                               const struct Vec3 *target, const struct Vec3 *up);

/* Retains the signed 32-bit result convention on LP64 platforms. */
int32_t mat4_perspective_lh(float *m, float fov, float aspect, float zNear, float zFar);

#endif
