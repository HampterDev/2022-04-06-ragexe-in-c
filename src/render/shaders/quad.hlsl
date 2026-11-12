// The client's quads (render/gfx.h, gfx_draw): the texture stages' combination of the draw's mode
// (render/draw_modes.h, enum DrawCombine), the vertex fog and the alpha test, as Direct3D 9 drew
// them.
// Pre-transformed vertices keep Direct3D 9's pixels: the rasterizer gets x and y snapped as
// Direct3D 9 snapped them (snapped_ndc), and covers pixels with Direct3D's top-left rule. Their w
// stays 1 so that nothing divides the position; the varyings are interpolated perspective-correct
// by hand, with the rhw, as Direct3D 9 interpolates them. GFX_VTX_WORLD vertices go through the
// camera's view to the same pixels, which the GPU divides and moves by the viewport as Direct3D 9
// did.
#include "records.hlsli"

StructuredBuffer<Draw> draws : register(t0, space0);
StructuredBuffer<View> views : register(t1, space0);
StructuredBuffer<float4> fogTables : register(t2, space0);

[[vk::combinedImageSampler]] Texture2D<float4> texture0 : register(t0, space2);
[[vk::combinedImageSampler]] SamplerState sampler0 : register(s0, space2);
[[vk::combinedImageSampler]] Texture2D<float4> texture1 : register(t1, space2);
[[vk::combinedImageSampler]] SamplerState sampler1 : register(s1, space2);
StructuredBuffer<Draw> fragmentDraws : register(t2, space2);

// One layout for every quad: a vertex with one texture's coordinates takes them for both.
struct VertexIn
{
    [[vk::location(0)]] float4 position : TEXCOORD0;  // x, y, z, rhw; or world x, y, z
    [[vk::location(1)]] float4 diffuse : TEXCOORD1;   // the bytes of a D3DCOLOR: b, g, r, a
    [[vk::location(2)]] float2 uv0 : TEXCOORD2;
    [[vk::location(3)]] float2 uv1 : TEXCOORD3;
    [[vk::location(4)]] uint draw : TEXCOORD4;        // per instance: the draw's record
};

// Each varying times q, and q: 1 / w, interpolated in screen space and divided per pixel.
struct Varyings
{
    float4 position : SV_Position;
    [[vk::location(0)]] noperspective float4 diffuse : TEXCOORD0;
    [[vk::location(1)]] noperspective float4 uv : TEXCOORD1;
    [[vk::location(2)]] noperspective float2 fogQ : TEXCOORD2;  // the fog's share kept, times q; q
    [[vk::location(3)]] nointerpolation uint draw : TEXCOORD3;
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

Varyings vs_main(VertexIn i)
{
    Draw d = draws[i.draw];
    Varyings o;
    float q;
    if (d.mode.x == KIND_WORLD) {
        o.position = transform(i.position.xyz, views[d.mode.w].toClip);
        q = 1 / o.position.w;
    } else {
        o.position = float4(snapped_ndc(i.position.xy, d.target.yz), i.position.z, 1);
        q = i.position.w > 0 ? i.position.w : 1;
    }
    o.diffuse = i.diffuse.zyxw * q;
    o.uv = float4(i.uv0, i.uv1) * q;
    // the fog of a vertex in world coordinates by the rhw the CPU's projection gave it, as the
    // original's
    o.fogQ = float2(d.mode.z != 0 ? vertex_fog(i.position.w, d.fog, d.stages.w) * q : q, q);
    o.draw = i.draw;
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
    float4 t = bordered(texture0.Sample(sampler0, stage_uv(uv.xy, d.uvScale.xy, d.uvLimit.xy)), uv.xy,
                        (d.stages.y & 1) != 0);
    float4 u = bordered(texture1.Sample(sampler1, stage_uv(uv.zw, d.uvScale.zw, d.uvLimit.zw)), uv.zw,
                        (d.stages.y & 2) != 0);
    float4 c = combine(d.stages.x, t, u, diffuse);
    if (d.mode.z != 0)
        c.rgb = lerp(d.fogColour.rgb, c.rgb, i.fogQ.x / i.fogQ.y);
    if (!alpha_test(c.a, d))
        discard;
    return c;
}
