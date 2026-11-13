#include "render/math3d.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>

static void near_value(float actual, float expected)
{
    if (fabsf(actual - expected) >= 0.00002f)
        fprintf(stderr, "actual %.9g, expected %.9g, error %.9g\n", actual, expected, actual - expected);
    assert(fabsf(actual - expected) < 0.00002f);
}

static void test_transform_inverse(void)
{
    float m[12], inverse[12], product[12];
    struct Vec3 translation = { 17.0f, -31.0f, 4.0f };
    int axis, step, i;
    for (axis = 0; axis < 3; ++axis) {
        for (step = -360; step <= 360; step += 5) {
            float angle = (float)step * 3.14159265f / 180.0f;
            mat_set_scale(m, 0.5f, 2.0f, 4.0f);
            if (axis == 0) mat_rotate_x(m, angle);
            if (axis == 1) mat_rotate_y(m, angle);
            if (axis == 2) mat_rotate_z(m, angle);
            mat_translate(m, &translation);
            mat_invert(inverse, m);
            mat_multiply(product, m, inverse);
            for (i = 0; i < 12; ++i)
                near_value(product[i], g_identityMatrix.m[i]);
        }
    }
}

static void test_quaternions_and_degrees(void)
{
    struct KeyQuat q;
    float m[12];
    int i;
    quat_identity(&q);
    quat_to_matrix(m, &q);
    assert(mat_is_identity(m));
    math_init_degree_tables();
    for (i = 0; i <= 360; ++i) {
        float s = get_sin_deg(i), c = get_cos_deg(i);
        near_value(s * s + c * c, 1.0f);
    }
    near_value(get_sin_deg(90), 1.0f);
    near_value(get_cos_deg(180), -1.0f);
    near_value(get_sin_deg(-90), get_sin_deg(271));
    near_value(get_cos_deg(-90), get_cos_deg(271));
    near_value(deg_to_rad(-90), deg_to_rad(271));
    near_value(get_cos_deg(-1), get_cos_deg(360));
    near_value(get_sin_deg(-360), get_sin_deg(1));
    near_value(get_sin_deg(-361), get_sin_deg(360));
    near_value(get_cos_deg(450), 0.0f);
}

static int address_index(int deg)
{
    return (int)((unsigned)deg << 2) >> 2;
}

static int turn(int deg)
{
    deg = address_index(deg) % 360;
    return deg < 0 ? deg + 360 : deg;
}

static void test_degree_table_reads(void)
{
    static const unsigned wraps[] = { 0x40000000u, 0x80000000u, 0xc0000000u };
    static const int outside[] = { 765, 2209, 8190, 11565, 32767, -1445, -1806, -32768, 0x1fffffff, -0x20000000 };
    int i, w;

    math_init_degree_tables();
    for (i = -2 * 361; i < 4 * 361; ++i) {
        assert(&SIN_DEG(i) == &g_sinTable[i]);
        for (w = 0; w < 3; ++w)
            assert(&SIN_DEG((int)((unsigned)i + wraps[w])) == &g_sinTable[i]);
    }
    for (i = -4 * 361; i < 2 * 361; ++i) {
        assert(&COS_DEG(i) == &g_cosTable[i]);
        for (w = 0; w < 3; ++w)
            assert(&COS_DEG((int)((unsigned)i + wraps[w])) == &g_cosTable[i]);
    }
    assert(&SIN_DEG((int)0x80000000u) == &g_sinTable[0] && &COS_DEG((int)0x80000000u) == &g_cosTable[0]);
    assert(&SIN_DEG(0x40000000) == &g_sinTable[0] && &COS_DEG(0x7fffffff) == &g_cosTable[-1]);
    assert(&SIN_DEG(-723) == &g_sinTable[turn(-723)]);
    assert(&SIN_DEG(1444) == &g_sinTable[turn(1444)]);
    assert(&COS_DEG(722) == &g_cosTable[2]);
    assert(&COS_DEG(-1445) == &g_cosTable[turn(-1445)]);
    for (i = 0; i < (int)(sizeof outside / sizeof *outside); ++i) {
        assert(&COS_DEG(outside[i]) == &g_cosTable[turn(outside[i])]);
        if (outside[i] >= 4 * 361 || outside[i] < -2 * 361)
            assert(&SIN_DEG(outside[i]) == &g_sinTable[turn(outside[i])]);
        for (w = 0; w < 3; ++w)
            assert(&COS_DEG((int)((unsigned)outside[i] + wraps[w])) == &g_cosTable[turn(outside[i])]);
    }
}

static void test_picking(void)
{
    struct OrientedBox box = {
        { 1, 1, 1 }, { 0, 0, 0 }, { { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } }
    };
    struct Ray3 ray = { { 0, 0, -5 }, { 0, 0, 1 }, 10 };
    struct Vec3 a = { -1, -1, 0 }, b = { 1, -1, 0 }, c = { 0, 1, 0 };
    float u, v, t;
    assert(ray_hits_obb(&ray, &box));
    assert(ray_triangle_hit(&ray, &a, &b, &c, &u, &v, &t));
    near_value(t, 5.0f);
    near_value(u, 0.25f);
    near_value(v, 0.5f);
    ray.length = 3;
    assert(!ray_hits_obb(&ray, &box));
    ray.origin.x = 5;
    assert(!ray_hits_obb(&ray, &box));
    assert(!ray_triangle_hit(&ray, &a, &b, &c, &u, &v, &t));
}

int main(void)
{
    test_transform_inverse();
    test_quaternions_and_degrees();
    test_degree_table_reads();
    test_picking();
    puts("math: rotated/scaled inverse transforms, degree tables, reads within the degree block, quaternions "
         "and picking passed");
    return 0;
}
