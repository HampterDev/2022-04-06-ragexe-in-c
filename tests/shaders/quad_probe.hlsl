// tests/test_quad_instances.c: runs render/shaders/instances.hlsli's quad_vertex, the vertex the quad
// shader draws, and exact.hlsli's operations on the GPU's vertex stage and hands back what they
// computed for each vertex, bit for bit. Each vertex of each record is drawn as a point on its own
// pixel of four 32-bit float targets, its results in flat varyings.
#include "../../src/render/shaders/exact.hlsli"

StructuredBuffer<uint4> quads : register(t0, space0);

#include "../../src/render/shaders/instances.hlsli"

cbuffer Probe : register(b0, space1)
{
    uint4 probe;  // the targets' width in pixels; which results (0: quad_vertex's, 1: the exact
                  // operations alone); the index of the first instance of the draw, which numbers
                  // the pixels; the float 2 / the width, so that the probe divides nothing itself
};

struct ProbeIn
{
    [[vk::location(0)]] uint corner : TEXCOORD0;  // per vertex: 0 to 3
    [[vk::location(1)]] uint at : TEXCOORD1;      // per instance: the row its record starts at in quads
    [[vk::location(2)]] uint index : TEXCOORD2;   // per instance: its number
};

struct ProbeOut
{
    float4 position : SV_Position;
    [[vk::builtin("PointSize")]] float size : PSIZE;
    [[vk::location(0)]] nointerpolation float4 r0 : TEXCOORD0;
    [[vk::location(1)]] nointerpolation float4 r1 : TEXCOORD1;
    [[vk::location(2)]] nointerpolation float4 r2 : TEXCOORD2;
    [[vk::location(3)]] nointerpolation float4 r3 : TEXCOORD3;
};

struct ProbeTargets
{
    float4 t0 : SV_Target0;
    float4 t1 : SV_Target1;
    float4 t2 : SV_Target2;
    float4 t3 : SV_Target3;
};

ProbeOut vs_main(ProbeIn i)
{
    uint pixel = (i.index - probe.z) * 4 + i.corner, w = probe.x;
    precise float px = (float(pixel % w) + 0.5) * asfloat(probe.w) - 1.0;
    precise float py = 1.0 - (float(pixel / w) + 0.5) * asfloat(probe.w);
    ProbeOut o;
    o.position = float4(px, py, 0.5, 1.0);
    o.size = 1.0;
    o.r3 = 0.0;
    if (probe.y == 1) {
        // the exact operations on the record's floats: a / b, renderer_project_depth, and
        // renderer_project_vertex of a point through the matrix whose first row the header's w is
        uint4 h = quads[i.at];
        float4 a = quad_float4(i.at + 1);
        float4 p = quad_float4(i.at + 2);
        Projected pr = project_point(p.xyz, quad_float4(h.w), quad_float4(h.w + 1), quad_float4(h.w + 2),
                                     quad_float4(h.w + 3));
        o.r0 = float4(div_rn(a.x, a.y), project_depth(a.z), 0.0, 0.0);
        o.r1 = float4(pr.x, pr.y, project_depth(pr.rhw), pr.rhw);
        o.r2 = 0.0;
        return o;
    }
    QuadVertex v = quad_vertex(i.at, i.corner);
    o.r0 = float4(v.x, v.y, v.z, v.rhw);
    o.r1 = v.colour;
    o.r2 = v.uv;
    return o;
}

ProbeTargets fs_main(ProbeOut i)
{
    ProbeTargets o;
    o.t0 = i.r0;
    o.t1 = i.r1;
    o.t2 = i.r2;
    o.t3 = i.r3;
    return o;
}
