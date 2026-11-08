#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <math.h>
#include "render/math3d.h"

const float g_pi = 3.141592f;
const float g_halfTurnDegrees = 180.0f;

struct Mat34 g_identityMatrix = { { 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0 } };

float g_radianTable[361], g_negRadianTable[361];
float g_sinTable[361], g_negSinTable[361];
float g_cosTable[361], g_negCosTable[361];
float *const g_degree_tables[6] = {
    g_radianTable, g_negRadianTable, g_sinTable, g_negSinTable, g_cosTable, g_negCosTable,
};

#define PI_F 3.141592f

float float_sin(float x)
{
    return sinf(x);
}

float float_cos(float x)
{
    return cosf(x);
}

float float_sqrt(float x)
{
    return sqrtf(x);
}

static void mat34_product(struct Mat34 *r, const float *a, const float *b)
{
    int row, col;

    for (row = 0; row < 3; row++)
        for (col = 0; col < 3; col++)
            r->m[row * 3 + col] = a[row * 3] * b[col] + a[row * 3 + 1] * b[3 + col]
                                + a[row * 3 + 2] * b[6 + col];
    for (col = 0; col < 3; col++)
        r->m[9 + col] = a[9] * b[col] + a[10] * b[3 + col] + a[11] * b[6 + col] + b[9 + col];
}

static void mat34_post_multiply(float *m, const struct Mat34 *b)
{
    struct Mat34 r;

    mat34_product(&r, m, b->m);
    *(struct Mat34 *)m = r;
}

void mat_multiply(float *out, const float *a, const float *b)
{
    int row, col;

    for (col = 0; col < 3; col++) {
        for (row = 0; row < 3; row++)
            out[row * 3 + col] = a[row * 3] * b[col] + a[row * 3 + 1] * b[3 + col]
                               + a[row * 3 + 2] * b[6 + col];
        out[9 + col] = a[9] * b[col] + a[10] * b[3 + col] + a[11] * b[6 + col] + b[9 + col];
    }
}

void mat_scale(float *m, float x, float y, float z)
{
    struct Mat34 b = g_identityMatrix;

    b.m[0] = x;
    b.m[4] = y;
    b.m[8] = z;
    mat34_post_multiply(m, &b);
}

void mat_translate(float *m, const struct Vec3 *v)
{
    struct Mat34 b = g_identityMatrix;

    b.m[9] = v->x;
    b.m[10] = v->y;
    b.m[11] = v->z;
    mat34_post_multiply(m, &b);
}

void mat_rotate_x(float *m, float rad)
{
    struct Mat34 b = g_identityMatrix;
    float s = float_sin(rad), c = float_cos(rad);

    b.m[4] = c;
    b.m[5] = s;
    b.m[7] = -s;
    b.m[8] = c;
    mat34_post_multiply(m, &b);
}

static __declspec(thread) DWORD sincos_last_angle;
static __declspec(thread) float sincos_last_sin, sincos_last_cos;
static __declspec(thread) int sincos_valid;

static void cached_sincos(float rad, float *s, float *c)
{
    DWORD bits = *(DWORD *)&rad;

    if (bits != sincos_last_angle || !sincos_valid) {
        sincos_last_angle = bits;
        sincos_last_sin = float_sin(rad);
        sincos_last_cos = float_cos(rad);
        sincos_valid = 1;
    }
    *s = sincos_last_sin;
    *c = sincos_last_cos;
}

void mat_rotate_y(float *m, float rad)
{
    struct Mat34 b = g_identityMatrix;
    float s, c;

    cached_sincos(rad, &s, &c);
    b.m[0] = c;
    b.m[2] = -s;
    b.m[6] = s;
    b.m[8] = c;
    mat34_post_multiply(m, &b);
}

void mat_rotate_z(float *m, float rad)
{
    struct Mat34 b = g_identityMatrix;
    float s = float_sin(rad), c = float_cos(rad);

    b.m[0] = c;
    b.m[1] = s;
    b.m[3] = -s;
    b.m[4] = c;
    mat34_post_multiply(m, &b);
}

static int ray_slab(float e, float f, float h, float *tmin, float *tmax)
{
    if ((float)fabs(f) > 1.0e-5f) {
        float inv = 1.0f / f;
        float t1 = (h + e) * inv;
        float t2 = (e - h) * inv;

        if (t1 > t2) {
            *tmin = t2 > *tmin ? t2 : *tmin;
            *tmax = t1 < *tmax ? t1 : *tmax;
        } else {
            *tmin = t1 > *tmin ? t1 : *tmin;
            *tmax = t2 < *tmax ? t2 : *tmax;
        }
        if (*tmin > *tmax)
            return 0;
        if (0.0f > *tmax)
            return 0;
    } else {
        if (-e - h > 0.0f)
            return 0;
        if (0.0f > h - e)
            return 0;
    }
    return 1;
}

int ray_hits_obb(const struct Ray3 *ray, const struct OrientedBox *box)
{
    float tmin = -999999.0f, tmax = 999999.0f;
    float dx = box->centre.x - ray->origin.x;
    float dy = box->centre.y - ray->origin.y;
    float dz = box->centre.z - ray->origin.z;
    int i;

    for (i = 0; i < 3; i++) {
        const struct Vec3 *a = &box->axis[i];
        float e = dy * a->y + dx * a->x + dz * a->z;
        float f = ray->dir.x * a->x + ray->dir.y * a->y + ray->dir.z * a->z;

        if (!ray_slab(e, f, box->half[i], &tmin, &tmax))
            return 0;
    }
    if (tmin > 0.0f)
        return !(tmin > ray->length);
    return !(tmax > ray->length);
}

int ray_triangle_hit(const struct Ray3 *ray, const struct Vec3 *v0, const struct Vec3 *v1,
                         const struct Vec3 *v2, float *u, float *v, float *t)
{
    float e1x = v1->x - v0->x, e1y = v1->y - v0->y, e1z = v1->z - v0->z;
    float e2x = v2->x - v0->x, e2y = v2->y - v0->y, e2z = v2->z - v0->z;
    float px = e2z * ray->dir.y - e2y * ray->dir.z;
    float py = e2x * ray->dir.z - e2z * ray->dir.x;
    float pz = e2y * ray->dir.x - e2x * ray->dir.y;
    float det = py * e1y + px * e1x + pz * e1z;
    float inv, sx, sy, sz, qx, qy, qz;

    if (det > -1.0e-5f && 1.0e-5f > det)
        return 0;
    inv = 1.0f / det;
    sx = ray->origin.x - v0->x;
    sy = ray->origin.y - v0->y;
    sz = ray->origin.z - v0->z;
    *u = (sy * py + sx * px + sz * pz) * inv;
    if (0.0f > *u || *u > 1.0f)
        return 0;
    qx = sy * e1z - sz * e1y;
    qy = e1x * sz - sx * e1z;
    qz = sx * e1y - e1x * sy;
    *v = (ray->dir.y * qy + ray->dir.x * qx + ray->dir.z * qz) * inv;
    if (0.0f > *v || *v + *u > 1.0f)
        return 0;
    *t = (qy * e2y + qx * e2x + qz * e2z) * inv;
    return 1;
}

void math_init_degree_tables(void)
{
    int deg;

    for (deg = 0; deg <= 360; deg++) {
        float s, c;

        g_degree_tables[0][deg] = (float)deg / 180.0f * PI_F;
        s = float_sin(g_degree_tables[0][deg]);
        g_degree_tables[2][deg] = s;
        c = float_cos(g_degree_tables[0][deg]);
        g_degree_tables[1][deg] = -g_degree_tables[0][deg];
        g_degree_tables[4][deg] = c;
        g_degree_tables[3][deg] = -g_degree_tables[2][deg];
        g_degree_tables[5][deg] = -c;
    }
}

static float degree_lookup(int table, int deg)
{
    if (deg > 360 || deg < -360)
        deg %= 360;
    if (deg < 0)
        return g_degree_tables[table][361 + deg];
    return g_degree_tables[table][deg];
}

float get_cos_deg(int deg)
{
    return degree_lookup(4, deg);
}

float deg_to_rad(int deg)
{
    return degree_lookup(0, deg);
}

float get_sin_deg(int deg)
{
    return degree_lookup(2, deg);
}

struct KeyQuat *quat_identity(struct KeyQuat *q)
{
    q->frame = 0;
    q->x = 0.0f;
    q->y = 0.0f;
    q->z = 0.0f;
    q->w = 1.0f;
    return q;
}

struct KeyQuat *matrix_to_quat(const float *m, struct KeyQuat *out)
{
    float w4;

    quat_identity(out);
    out->w = float_sqrt(m[0] + 1.0f + m[4] + m[8]) * 0.5f;
    w4 = out->w * 4.0f;
    out->x = (m[7] - m[5]) / w4;
    out->y = (m[2] - m[6]) / w4;
    out->z = (m[3] - m[1]) / w4;
    return out;
}

int mat_is_identity(const float *m)
{
    static const int ones[3] = { 0, 4, 8 };
    static const int zeros[9] = { 1, 2, 3, 5, 6, 7, 9, 10, 11 };
    int i;

    for (i = 0; i < 3; i++)
        if ((float)fabs(m[ones[i]] - 1.0f) > 1.0e-5f)
            return 0;
    for (i = 0; i < 9; i++)
        if ((float)fabs(m[zeros[i]]) > 1.0e-5f)
            return 0;
    return 1;
}

void mat_invert(float *out, const float *m)
{
    float c00 = m[4] * m[8] - m[5] * m[7];
    float det = c00 * m[0] - (m[3] * m[8] - m[6] * m[5]) * m[1] + (m[3] * m[7] - m[6] * m[4]) * m[2];
    float inv = 1.0f / det;
    float i0, i1, i2, i3, i4, i5, i6, i7, i8;

    out[0] = i0 = c00 * inv;
    out[1] = i1 = -((m[1] * m[8] - m[7] * m[2]) * inv);
    out[2] = i2 = (m[1] * m[5] - m[2] * m[4]) * inv;
    out[3] = i3 = -((m[3] * m[8] - m[6] * m[5]) * inv);
    out[4] = i4 = (m[0] * m[8] - m[2] * m[6]) * inv;
    out[5] = i5 = -((m[0] * m[5] - m[2] * m[3]) * inv);
    out[6] = i6 = (m[7] * m[3] - m[6] * m[4]) * inv;
    out[7] = i7 = -((m[7] * m[0] - m[1] * m[6]) * inv);
    out[8] = i8 = (m[4] * m[0] - m[1] * m[3]) * inv;
    out[9] = -(m[9] * i0 + m[10] * i3 + i6 * m[11]);
    out[10] = -(m[10] * i4 + m[9] * i1 + i7 * m[11]);
    out[11] = -(m[10] * i5 + m[9] * i2 + i8 * m[11]);
}

void mat_set_scale(float *m, float x, float y, float z)
{
    *(struct Mat34 *)m = g_identityMatrix;
    m[0] = x;
    m[4] = y;
    m[8] = z;
}

static void quat_rows(float *m, float x, float y, float z, float w)
{
    float x2 = x + x, y2 = y + y, z2 = z + z;
    float wx = w * x2, xx = x * x2, xz = x * z2, xy = x * y2;
    float yz = y * z2, yy = y * y2, zz = z * z2, wy = w * y2, wz = w * z2;

    m[0] = 1.0f - (zz + yy);
    m[1] = xy + wz;
    m[3] = xy - wz;
    m[2] = xz - wy;
    m[4] = 1.0f - (zz + xx);
    m[8] = 1.0f - (yy + xx);
    m[5] = yz + wx;
    m[7] = yz - wx;
    m[6] = xz + wy;
}

void quat_to_matrix(float *m, const struct KeyQuat *q)
{
    m[9] = 0.0f;
    m[10] = 0.0f;
    m[11] = 0.0f;
    quat_rows(m, q->x, q->y, q->z, q->w);
}

void quat_to_matrix_b(float *m, const struct KeyQuat *q)
{
    m[9] = 0.0f;
    m[10] = 0.0f;
    m[11] = 0.0f;
    quat_rows(m, q->x, q->y, q->z, q->w);
}

static void axis_angle_rows(float *m, const struct Vec3 *axis, float rad)
{
    float half = rad * 0.5f;
    float s = float_sin(half);
    float x = axis->x * s, y = axis->y * s, z = axis->z * s;

    quat_rows(m, x, y, z, float_cos(half));
}

void mat_set_rotate_axis(float *m, const struct Vec3 *axis, float rad)
{
    m[9] = 0.0f;
    m[10] = 0.0f;
    m[11] = 0.0f;
    axis_angle_rows(m, axis, rad);
}

void mat_set_translation(float *m, float x, float y, float z)
{
    *(struct Mat34 *)m = g_identityMatrix;
    m[9] = x;
    m[10] = y;
    m[11] = z;
}

void mat_look_at(float *m, const struct Vec3 *eye,
                               const struct Vec3 *target, const struct Vec3 *up)
{
    float fx = target->x - eye->x, fy = target->y - eye->y, fz = target->z - eye->z;
    float inv, sx, sy, sz, ux, uy, uz;

    *(struct Mat34 *)m = g_identityMatrix;
    inv = 1.0f / float_sqrt(fy * fy + fx * fx + fz * fz);
    fy *= inv;
    fz *= inv;
    fx *= inv;
    sx = fz * up->y - fy * up->z;
    sy = fx * up->z - fz * up->x;
    sz = fy * up->x - fx * up->y;
    inv = 1.0f / float_sqrt(sy * sy + sx * sx + sz * sz);
    sy *= inv;
    sz *= inv;
    sx *= inv;
    ux = sz * fy - sy * fz;
    uy = sx * fz - sz * fx;
    uz = sy * fx - sx * fy;
    inv = 1.0f / float_sqrt(uy * uy + ux * ux + uz * uz);
    m[0] = sx;
    m[2] = fx;
    m[7] = uz * inv;
    m[8] = fz;
    m[1] = ux * inv;
    m[3] = sy;
    m[6] = sz;
    m[4] = uy * inv;
    m[5] = fy;
    m[9] = -(sx * eye->x + sy * eye->y + sz * eye->z);
    m[10] = -(m[1] * eye->x + m[4] * eye->y + m[7] * eye->z);
    m[11] = -(fx * eye->x + eye->y * fy + fz * eye->z);
}

void mat_set_rotate_x(float *m, float rad)
{
    float s, c;

    *(struct Mat34 *)m = g_identityMatrix;
    s = float_sin(rad);
    c = float_cos(rad);
    m[4] = c;
    m[8] = c;
    m[5] = s;
    m[7] = -s;
}

void mat_set_rotate_y(float *m, float rad)
{
    float s, c;

    *(struct Mat34 *)m = g_identityMatrix;
    s = float_sin(rad);
    c = float_cos(rad);
    m[0] = c;
    m[8] = c;
    m[2] = -s;
    m[6] = s;
}

void mat_set_rotate_z(float *m, float rad)
{
    float s, c;

    *(struct Mat34 *)m = g_identityMatrix;
    s = float_sin(rad);
    c = float_cos(rad);
    m[0] = c;
    m[4] = c;
    m[3] = -s;
    m[1] = s;
}

long mat4_perspective_lh(float *m, float fov, float aspect, float zNear, float zFar)
{
    float depth = zFar - zNear, cosHalf, w, q;
    double h;

    if (0.01f > (float)fabs((double)depth))
        return (long)0x80070057;
    fov = fov * 0.5f;
    if (0.01f > (float)fabs((double)float_sin(fov)))
        return (long)0x80070057;
    cosHalf = float_cos(fov);
    w = (float)((double)cosHalf / (double)float_sin(fov) * (double)aspect);
    cosHalf = float_cos(fov);
    h = (double)cosHalf / (double)float_sin(fov);
    m[1] = 0.0f;
    m[2] = 0.0f;
    m[3] = 0.0f;
    m[4] = 0.0f;
    m[6] = 0.0f;
    m[7] = 0.0f;
    m[8] = 0.0f;
    m[9] = 0.0f;
    m[12] = 0.0f;
    m[13] = 0.0f;
    m[15] = 0.0f;
    m[11] = 1.0f;
    q = zFar / depth;
    m[10] = q;
    m[0] = w;
    m[5] = (float)h;
    m[14] = -(q * zNear);
    return 0;
}
