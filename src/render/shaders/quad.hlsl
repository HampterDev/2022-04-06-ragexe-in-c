// The client's quads and triangles (render/gfx.h, gfx_draw_quad and gfx_draw_vertices): every
// one is an instance, a record of the frame's quads (render/quads.c, instances.hlsli), from which
// this vertex shader makes the vertices the client's C code made for it before, bit for bit
// (exact.hlsli, quad_vertex); the fragment shader draws the texture stages' combination of the
// draw's mode (render/draw_modes.h, enum DrawCombine), the vertex fog and the alpha test, as
// Direct3D 9 drew them.
// Vertices on the screen keep Direct3D 9's pixels: the rasterizer gets x and y snapped as
// Direct3D 9 snapped them (snapped_ndc), and covers pixels with Direct3D's top-left rule. Their w
// stays 1 so that nothing divides the position; the varyings are interpolated perspective-correct
// by hand, with the rhw, as Direct3D 9 interpolates them. QUAD_CAMERA's world points go through the
// camera's view to the same pixels, which the GPU divides and moves by the viewport as Direct3D 9
// did.
#include "records.hlsli"
#include "exact.hlsli"

StructuredBuffer<Draw> draws : register(t0, space0);
StructuredBuffer<View> views : register(t1, space0);
StructuredBuffer<float4> fogTables : register(t2, space0);
StructuredBuffer<uint4> quads : register(t3, space0);

[[vk::combinedImageSampler]] Texture2D<float4> texture0 : register(t0, space2);
[[vk::combinedImageSampler]] SamplerState sampler0 : register(s0, space2);
[[vk::combinedImageSampler]] Texture2D<float4> texture1 : register(t1, space2);
[[vk::combinedImageSampler]] SamplerState sampler1 : register(s1, space2);
[[vk::combinedImageSampler]] Texture2DArray<float4> atlas : register(t2, space2);
[[vk::combinedImageSampler]] SamplerState sampler2 : register(s2, space2);
StructuredBuffer<Draw> fragmentDraws : register(t3, space2);

#include "instances.hlsli"

// Each varying times q, and q: 1 / w, interpolated in screen space and divided per pixel.
struct Varyings
{
    float4 position : SV_Position;
    [[vk::location(0)]] noperspective float4 diffuse : TEXCOORD0;
    [[vk::location(1)]] noperspective float4 uv : TEXCOORD1;
    [[vk::location(2)]] noperspective float2 fogQ : TEXCOORD2;  // the fog's share kept, times q; q
    [[vk::location(3)]] nointerpolation uint draw : TEXCOORD3;
    [[vk::location(4)]] nointerpolation uint layer : TEXCOORD4; // the layer of the atlas array, where
                                                                // the draw reads one (Draw.stages.y)
};

struct QuadIn
{
    [[vk::location(0)]] uint corner : TEXCOORD0;  // per vertex: 0 top left, 1 top right, 2 bottom left, 3 bottom right
    [[vk::location(1)]] uint at : TEXCOORD1;      // per instance: the row its record starts at in quads
};

// The share of its colour a vertex of this rhw keeps from the fog (render/gfx.h, GfxFog): the step
// the original's vertex fog computed on the CPU, looked up in the fog's table of the rhw at which
// each step begins (render/vertex_fog.h, vertex_fog_lookup), which comparisons find exactly.
float vertex_fog(float rhw, float4 fog, uint table)
{
    uint at = 0;
    [unroll] for (uint step = 128; step > 0; step >>= 1) {
        uint i = at + step - 1;
        if (fogTables[table * 64 + i / 4][i % 4] <= rhw)
            at += step;
    }
    return (fog.x + fog.y * min((float)at, fog.z)) / 255;
}

Varyings vs_main(QuadIn i)
{
    uint4 h = quads[i.at];
    Draw d = draws[h.y];
    QuadVertex v = quad_vertex(i.at, i.corner);
    Varyings o;
    float q;
    if ((h.x & QUAD_KIND) == QUAD_CAMERA) {
        o.position = transform(float3(v.x, v.y, v.z), views[d.mode.w].toClip);
        q = 1 / o.position.w;
    } else {
        o.position = float4(snapped_ndc(float2(v.x, v.y), d.target.yz), v.z, 1);
        q = v.rhw > 0 ? v.rhw : 1;
    }
    o.diffuse = v.colour * q;
    o.uv = v.uv * q;
    // the fog by the rhw: a world point's the one the CPU's projection gave it, as the original's
    o.fogQ = float2(d.mode.z != 0 ? vertex_fog(v.rhw, d.fog, d.stages.w) * q : q, q);
    o.draw = h.y;
    o.layer = h.x >> 16;
    return o;
}

// What the stages make of the first texture (t), the second (u) and the vertex colour (d)
// (render/draw_modes.h, enum DrawCombine), each step clamped to 0..1 as Direct3D 9 clamped each
// stage's result.
float4 combine(uint which, float4 t, float4 u, float4 d)
{
    float3 c;
    float a;
    switch (which) {
    case 0:  c = t.rgb * d.rgb; a = t.a * d.a; break;
    case 1:  c = t.rgb * d.rgb; a = t.a; break;
    case 3:  c = saturate(t.rgb + d.rgb); a = d.a; break;
    case 4:  c = lerp(d.rgb, t.rgb, t.a); a = d.a; break;
    case 5:  c = u.rgb * (t.rgb * d.rgb); a = u.a; break;
    case 6:  c = (1 - u.rgb) * ((1 - t.rgb) * d.rgb); a = u.a; break;
    case 7:  c = u.a * t.rgb; a = u.a; break;
    case 8:
    case 11: c = saturate(u.rgb + t.rgb * d.rgb); a = u.a; break;
    case 9:  c = lerp(t.rgb * d.rgb, u.rgb, u.a); a = u.a; break;
    case 10: c = lerp(t.rgb * d.rgb, u.rgb, d.a); a = u.a; break;
    case 12: c = saturate((1 - u.rgb) + t.rgb * d.rgb); a = u.a; break;
    case 13: c = u.rgb * saturate(t.rgb + d.rgb); a = u.a; break;
    case 14: c = saturate(u.rgb * (t.rgb * d.rgb) * 2); a = u.a; break;
    case 15: c = saturate(u.rgb + t.rgb * d.rgb - 0.5); a = u.a; break;
    case 16: c = t.rgb; a = t.a; break;
    case 17: c = d.rgb; a = d.a; break;
    default: c = u.rgb * t.rgb; a = u.a * t.a; break;
    }
    return float4(c, a);
}

float4 fs_main(Varyings i) : SV_Target0
{
    Draw d = fragmentDraws[i.draw];
    float4 diffuse = i.diffuse / i.fogQ.y, uv = i.uv / i.fogQ.y;
    float2 at0 = stage_uv(uv.xy, d.uvScale.xy, d.uvLimit.xy);
    // stage 0 reads a layer of an atlas array where the draw's record says so, its texture
    // otherwise; the same within a draw, so every pixel of it takes the same branch
    float4 read0 = (d.stages.y & 4) != 0 ? atlas.Sample(sampler2, float3(at0, (float)i.layer))
                                         : texture0.Sample(sampler0, at0);
    float4 t = bordered(read0, uv.xy, (d.stages.y & 1) != 0);
    float4 u = bordered(texture1.Sample(sampler1, stage_uv(uv.zw, d.uvScale.zw, d.uvLimit.zw)), uv.zw,
                        (d.stages.y & 2) != 0);
    float4 c = combine(d.stages.x, t, u, diffuse);
    if (d.mode.z != 0)
        c.rgb = lerp(d.fogColour.rgb, c.rgb, i.fogQ.x / i.fogQ.y);
    if (!alpha_test(c.a, d))
        discard;
    return c;
}
