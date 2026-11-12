// Float arithmetic that agrees with the C code's to the bit (the rules of plans/gpu-renderer.md,
// "Sprites, actors and effects"). It rests on what Vulkan promises of every device (and Metal of
// its own): an addition, subtraction or multiplication that is not contracted is rounded correctly,
// to nearest, ties to even. Vulkan lets a device flush denormal inputs and results to zero, where
// the C keeps them: the world's matrices, bounds and the water's heights are far from that range,
// and tests/test_static_world_maps.c finds no difference on the maps it runs.
// - Every operation is `precise` (SPIR-V NoContraction): the GPU neither fuses a multiply and an
//   add the CPU rounds apart nor reorders a sum. The C code is compiled without contraction (MSVC
//   /fp:precise, -ffp-contract=off elsewhere), so each of its float operations rounds once, in the
//   order written. dxc marks the operations that feed a precise variable within its function, not
//   an expression passed as an argument, so every value is computed into a precise variable first.
// - Division is IEEE division (div_rn): Vulkan lets a division be 2.5 units off.
// - No fused multiply-add (mad()), matrix or dot product (mul(), dot()): Vulkan does not promise to
//   fuse the one, and leaves the order of the others' sums to the GPU.
// tests/test_static_world_maps.c reads the SPIR-V of the shaders that include this and fails on an
// operation without the mark.

// a + b as s + e exactly (Knuth's two-sum)
struct Sum
{
    float s, e;
};

Sum two_sum(float a, float b)
{
    precise float s = a + b;
    precise float bb = s - a;
    precise float e = (a - (s - bb)) + (b - bb);
    Sum r;
    r.s = s;
    r.e = e;
    return r;
}

// a * b as s + e exactly (Dekker's product: each factor split into halves of 12 bits by
// Veltkamp's method, whose products are exact)
Sum two_product(float a, float b)
{
    precise float p = a * b;
    precise float ca = 4097.0 * a;
    precise float ah = ca - (ca - a);
    precise float al = a - ah;
    precise float cb = 4097.0 * b;
    precise float bh = cb - (cb - b);
    precise float bl = b - bh;
    precise float e = (((ah * bh - p) + ah * bl) + al * bh) + al * bl;
    Sum r;
    r.s = p;
    r.e = e;
    return r;
}

// a - b q as s + e exactly, for q within a few units of a / b: b q then lies within a factor of
// two of a, so their difference is a float
Sum remainder(float a, float b, float q)
{
    Sum p = two_product(b, q);
    precise float d = a - p.s;
    precise float e = -p.e;
    return two_sum(d, e);
}

// q > 0 moved to the float nearest a / b for b > 0, from the exact remainder a - b q = rs + re,
// for q within a little over half a unit of a / b: up a unit when the remainder lies beyond b
// times half the unit above q, down when beyond b times half the unit below, ties to even. b times
// a power of two is exact, and rs less it is exact where it matters, near it.
float nearest(float q, float b, float rs, float re)
{
    precise float up = asfloat(asuint(q) + 1) - q;
    precise float down = q - asfloat(asuint(q) - 1);
    precise float halfUp = up * 0.5;
    precise float halfDown = down * 0.5;
    precise float reachUp = b * halfUp;
    precise float reachDown = b * halfDown;
    precise float above = (rs - reachUp) + re;
    precise float below = (rs + reachDown) + re;
    bool odd = (asuint(q) & 1) != 0;
    precise float r = q;
    if (above > 0.0 || (above == 0.0 && odd))
        r = q + up;
    else if (below < 0.0 || (below == 0.0 && odd))
        r = q - down;
    return r;
}

// a / b rounded to nearest, as IEEE 754 divides: the GPU's quotient, corrected once by its exact
// remainder, then moved to the nearest float. For a / b of 0 or infinite (b = 0) the GPU's
// quotient; otherwise for a and b far enough inside the float range that the parts of
// two_product stay normal (from 2^-30 to 2^30, and every divisor from 1 to 2).
float div_rn(float a, float b)
{
    bool negative = ((asuint(a) ^ asuint(b)) & 0x80000000u) != 0;
    float na = abs(a), nb = abs(b);
    precise float q0 = na / nb;
    if (q0 == 0.0 || isinf(q0)) {
        precise float plain = negative ? -q0 : q0;
        return plain;
    }
    Sum r0 = remainder(na, nb, q0);
    precise float c = r0.s / nb;
    precise float q1 = q0 + c;
    Sum r1 = remainder(na, nb, q1);
    precise float q = nearest(q1, nb, r1.s, r1.e);
    precise float result = negative ? -q : q;
    return result;
}

// The client's 4x3 row-vector matrices (render/gfx.h) as three float4 of its twelve floats in
// order: m[0..3], m[4..7], m[8..11].
struct Matrix43
{
    float4 r0, r1, r2;
};

float m43(Matrix43 m, uint i)
{
    float4 r = i < 4 ? m.r0 : i < 8 ? m.r1 : m.r2;
    return r[i & 3];
}

// render/math3d.c, mat_multiply: a then b, out[r][c] = a[r][0] b[0][c] + a[r][1] b[1][c] +
// a[r][2] b[2][c], the translation row adding b[3][c] last
Matrix43 mat_multiply(Matrix43 a, Matrix43 b)
{
    float o[12];
    [unroll] for (uint col = 0; col < 3; col++) {
        [unroll] for (uint row = 0; row < 3; row++) {
            precise float x = m43(a, row * 3) * m43(b, col);
            precise float y = m43(a, row * 3 + 1) * m43(b, 3 + col);
            precise float z = m43(a, row * 3 + 2) * m43(b, 6 + col);
            precise float xy = x + y;
            precise float s = xy + z;
            o[row * 3 + col] = s;
        }
        precise float tx = m43(a, 9) * m43(b, col);
        precise float ty = m43(a, 10) * m43(b, 3 + col);
        precise float tz = m43(a, 11) * m43(b, 6 + col);
        precise float txy = tx + ty;
        precise float txyz = txy + tz;
        precise float t = txyz + m43(b, 9 + col);
        o[9 + col] = t;
    }
    Matrix43 m;
    m.r0 = float4(o[0], o[1], o[2], o[3]);
    m.r1 = float4(o[4], o[5], o[6], o[7]);
    m.r2 = float4(o[8], o[9], o[10], o[11]);
    return m;
}

// render/gfx_gpu.c: matrix_4x3, a 4x3 matrix as a row-vector 4x4 one, then multiply: a times b,
// r[i][j] = a[i][0] b[0][j] + a[i][1] b[1][j] + a[i][2] b[2][j] + a[i][3] b[3][j] in that order
float4x4 m44(Matrix43 m)
{
    return float4x4(m43(m, 0), m43(m, 1), m43(m, 2), 0.0,
                    m43(m, 3), m43(m, 4), m43(m, 5), 0.0,
                    m43(m, 6), m43(m, 7), m43(m, 8), 0.0,
                    m43(m, 9), m43(m, 10), m43(m, 11), 1.0);
}

float4x4 multiply44(float4x4 a, float4x4 b)
{
    float4x4 r;
    [unroll] for (uint i = 0; i < 4; i++) {
        [unroll] for (uint j = 0; j < 4; j++) {
            precise float p0 = a[i][0] * b[0][j];
            precise float p1 = a[i][1] * b[1][j];
            precise float p2 = a[i][2] * b[2][j];
            precise float p3 = a[i][3] * b[3][j];
            precise float s01 = p0 + p1;
            precise float s012 = s01 + p2;
            precise float s = s012 + p3;
            r[i][j] = s;
        }
    }
    return r;
}
