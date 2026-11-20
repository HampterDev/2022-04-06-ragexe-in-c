// What the client's quads (quad.hlsl), the water (water.hlsl) and Granny's actors (skin.hlsl)
// share: the resource their vertex shaders read first (the draw records; what each reads after
// them, each declares), the varyings, the vertex fog, and the pixels
// (quad_pixel): the texture stages' combination of the draw's mode (render/draw_modes.h, enum
// DrawCombine), stage 0 read from a layer of an atlas array where the draw's record says so, the
// fog and the alpha test, as Direct3D 9 drew them.
#include "records.hlsli"

StructuredBuffer<Draw> draws : register(t0, space0);

[[vk::combinedImageSampler]] Texture2D<float4> texture0 : register(t0, space2);
[[vk::combinedImageSampler]] SamplerState sampler0 : register(s0, space2);
[[vk::combinedImageSampler]] Texture2D<float4> texture1 : register(t1, space2);
[[vk::combinedImageSampler]] SamplerState sampler1 : register(s1, space2);
[[vk::combinedImageSampler]] Texture2DArray<float4> atlas : register(t2, space2);
[[vk::combinedImageSampler]] SamplerState sampler2 : register(s2, space2);
StructuredBuffer<Draw> fragmentDraws : register(t3, space2);

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

// The share of its colour a vertex of this rhw keeps from the fog (render/gfx.h, GfxFog; fog holds
// its start, end, origin and span): the step the original's vertex fog computed on the CPU, 1 - t
// in 1/255 steps rounded down, t = ((1 / rhw - origin) / span - start) / (end - start) clamped to
// 0..1, without the division where end equals start. A vertex whose rhw is not positive takes the
// step of the smallest normal one, which lies beyond any fog's range.
float vertex_fog(float rhw, float4 fog)
{
    float t = (1.0 / max(rhw, 1.17549435e-38) - fog.z) / fog.w;
    if (fog.y != fog.x)
        t = (t - fog.x) / (fog.y - fog.x);
    return floor((1.0 - saturate(t)) * 255.0) / 255.0;
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

// The pixel shader of all three: fs_main in each source, which tools/shaders finds there.
float4 quad_pixel(Varyings i)
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
