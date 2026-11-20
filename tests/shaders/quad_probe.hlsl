// tests/test_quad_instances.c: runs render/shaders/instances.hlsli's quad_vertex, the vertex the quad
// shader draws, on the GPU's vertex stage and hands back what it computed for each vertex. Each
// vertex of each record is drawn as a point on its own pixel of three 32-bit float targets, its
// results in flat varyings.
#include "../../src/render/shaders/projection.hlsli"

StructuredBuffer<uint4> quads : register(t0, space0);

#include "../../src/render/shaders/instances.hlsli"

cbuffer Probe : register(b0, space1)
{
    uint4 probe;  // the targets' width in pixels; the float 2 / the width, so that the probe divides
                  // nothing itself
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
};

struct ProbeTargets
{
    float4 t0 : SV_Target0;
    float4 t1 : SV_Target1;
    float4 t2 : SV_Target2;
};

ProbeOut vs_main(ProbeIn i)
{
    uint pixel = i.index * 4 + i.corner, w = probe.x;
    ProbeOut o;
    o.position = float4((float(pixel % w) + 0.5) * asfloat(probe.y) - 1.0,
                        1.0 - (float(pixel / w) + 0.5) * asfloat(probe.y), 0.5, 1.0);
    o.size = 1.0;
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
    return o;
}
