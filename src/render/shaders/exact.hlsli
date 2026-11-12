// Floats as the CPU computes them, bit for bit, in a vertex shader: what instances.hlsli needs to
// make the vertices render/gfx_gpu.c's instances stand for exactly where the client's C code made
// them (render_object_submit_spr_quad, renderer_project_vertex, the multisampled pass's move of a
// flat draw's vertices).
//
// It rests on one assumption: an addition, subtraction or multiplication that is not contracted is
// rounded to nearest, ties to even. Vulkan promises that it is rounded correctly but leaves the
// rounding mode to the device, unless a shader declares RoundingModeRTE (SPV_KHR_float_controls),
// which shaders for Vulkan 1.0 devices cannot; Metal rounds to nearest even. Every GPU these
// shaders have run on does: tests/test_quad_instances.c checks it on the one it runs on, and the
// client on its own when it makes the device (render/gfx_gpu.c, check_rounding). Bit for bit then
// takes three things the GPU does not do by itself:
// - Every operation is `precise` (SPIR-V NoContraction): the GPU neither fuses a multiply and an
//   add the CPU rounds apart nor reorders a sum. The C code rounds each of its float operations
//   once, in the order written, where its compiler does not contract them: MSVC's /fp:precise
//   does not on x64; GCC and Clang do unless compiled with -ffp-contract=off, which bit for bit
//   needs of them (docs/portability.md). dxc marks the operations that feed a precise
//   variable within its function, not an expression passed as an argument, so every value is
//   computed into a precise variable first (tests/test_quad_instances.c checks the SPIR-V for
//   operations without the mark).
// - Division is IEEE division (div_rn): Vulkan lets a division be 2.5 units off.
// - No fused multiply-add (mad(), GLSL.std.450 Fma): Vulkan does not promise to fuse it, and Mesa's
//   lavapipe rounds its product. No mul() or dot product either, which leave the order of their
//   sums to the GPU.
//
// Constants are given by their bits, the floats the C compiler made of the same expressions.

#define F_DEPTH_SCALE asfloat(0x3f800832u)   // RENDER_DEPTH_SCALE (render/render_api.h)
#define F_FLAT_DET asfloat(0x358637bdu)      // 1e-6f, the area under which the multisampled pass's move
                                             // of a flat draw's vertices left a triangle as it was

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
// two_product stay normal (tests/test_quad_instances.c: from 2^-30 to 2^30, and every divisor
// from 1 to 2). SDL compiles the Metal library with fast math, under which isinf need not find an
// infinite quotient: on Metal a division by 0 is unchecked (no Metal compiler has run these
// shaders).
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

// renderer_project_depth (render/rp_project.c): the depth of a pre-transformed vertex of this rhw
float project_depth(float rhw)
{
    precise float z = ((div_rn(1.0, rhw) - 10.0) * F_DEPTH_SCALE) * rhw;
    return z;
}

// renderer_project_vertex (render/rp_project.c): a point through the client's 4x3 view matrix m
// (rows m0 = m[0..3], m1 = m[4..7], m2 = m[8..11]), summed in the order the C code sums it, its
// rhw and its place on the screen; screen holds Renderer.proj[0], proj[1], centreX and centreY
struct Projected
{
    float rhw, x, y;
};

// the projection of a point in view space
Projected project_view(float3 v, float4 screen)
{
    precise float rhw = div_rn(1.0, v.z);
    precise float x = (v.x * rhw) * screen.x + screen.z;
    precise float y = (v.y * rhw) * screen.y + screen.w;
    Projected o;
    o.rhw = rhw;
    o.x = x;
    o.y = y;
    return o;
}

Projected project_point(float3 p, float4 m0, float4 m1, float4 m2, float4 screen)
{
    precise float vx = ((p.y * m0.w + p.x * m0.x) + m1.z * p.z) + m2.y;
    precise float vy = ((m0.y * p.x + m1.x * p.y) + m1.w * p.z) + m2.z;
    precise float vz = ((m0.z * p.x + m1.y * p.y) + m2.x * p.z) + m2.w;
    return project_view(float3(vx, vy, vz), screen);
}

// The pixel boundary before the first pixel centre at or after v, v snapped to 1/256 of a pixel
// rounding halves up: where a multisampled pass moves an edge of a flat draw's vertex
// (instances.hlsli, snapped), as the C code computed it in double. scaled + 0.5 may round in
// float, so the half is compared with what lies above the floor, which is exact below 2^23. From
// 2^23 / 256 on, v is a whole number of 256ths already (and v * 256 may not be a float), so it is
// its own snap.
float pixel_boundary(float v)
{
    precise float scaled = v * 256.0;
    precise float below = floor(scaled);
    precise float above = scaled - below;
    precise float up = below + 1.0;
    precise float whole = above >= 0.5 ? up : below;
    precise float snapped = abs(scaled) >= 8388608.0 ? v : whole * 0.00390625;
    precise float r = ceil(snapped) - 0.5;
    return r;
}

// The value at (ox, oy) from a triangle's first vertex on the plane through its values a, b and
// c, the triangle's edges from that vertex being (dx1, dy1) and (dx2, dy2) and det their cross
// product
float plane_value(float a, float b, float c, float dx1, float dy1, float dx2, float dy2, float det, float ox, float oy)
{
    precise float d1 = b - a, d2 = c - a;
    precise float nx = d1 * dy2 - d2 * dy1;
    precise float ny = d2 * dx1 - d1 * dx2;
    precise float gx = div_rn(nx, det);
    precise float gy = div_rn(ny, det);
    precise float value = (a + gx * ox) + gy * oy;
    return value;
}
