// tests/test_skin_instances.c: runs render/shaders/skin.hlsli's skin_vertex and skin_clip, what
// the skinning shader computes of a vertex, on the GPU's vertex stage and hands back, for each
// vertex, its clip position on a screen of the probe's size, its colour, texture coordinates and
// place in view space. Each vertex is drawn as a point on its own pixel of four 32-bit float
// targets, its results in flat varyings. The vertices come as skin.hlsl takes them, with the row of
// the pose each is skinned in beside them.
StructuredBuffer<uint4> quads : register(t0, space0);

#include "../../src/render/shaders/skin.hlsli"

cbuffer Probe : register(b0, space1)
{
    uint4 probe;    // the targets' width in pixels; the float 2 / the width; the pixel of the draw's
                    // first vertex
    float4 screen;  // half the width and height of the screen the vertices are projected onto
};

struct ProbeIn
{
    [[vk::location(0)]] float3 position : TEXCOORD0;
    [[vk::location(1)]] float3 normal : TEXCOORD1;
    [[vk::location(2)]] float2 uv : TEXCOORD2;
    [[vk::location(3)]] float4 weights : TEXCOORD3;
    [[vk::location(4)]] uint4 bones : TEXCOORD4;
    [[vk::location(5)]] uint flags : TEXCOORD5;
    [[vk::location(6)]] uint pose : TEXCOORD6;  // the row its pose starts at among the quads
    uint index : SV_VertexID;
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
    uint w = probe.x, pixel = probe.z + i.index;
    float px = (float(pixel % w) + 0.5) * asfloat(probe.y) - 1.0;
    float py = 1.0 - (float(pixel / w) + 0.5) * asfloat(probe.y);
    SkinVertex v;
    v.position = i.position;
    v.normal = i.normal;
    v.uv = i.uv;
    v.weights = i.weights;
    v.bones = i.bones;
    v.flags = i.flags;
    Skinned s = skin_vertex(i.pose, v);
    ProbeOut o;
    o.position = float4(px, py, 0.5, 1.0);
    o.size = 1.0;
    o.r0 = skin_clip(i.pose, s.view, screen.xy);
    o.r1 = s.colour;
    o.r2 = float4(s.uv, 0.0, 0.0);
    o.r3 = float4(s.view, 0.0);
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
