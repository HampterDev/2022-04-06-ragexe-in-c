// A shadow caster's depth from the light (render/gfx.h, gfx_shadow_begin): a mesh drawn into one
// cascade's map, its depth negative for a model's, and texels the opaque lists' alpha test (0xcf)
// drops cast nothing (without a texture the stage reads opaque white).
#include "records.hlsli"

StructuredBuffer<Draw> draws : register(t0, space0);
StructuredBuffer<View> views : register(t1, space0);

[[vk::combinedImageSampler]] Texture2D<float4> colourMap : register(t0, space2);
[[vk::combinedImageSampler]] SamplerState colourSampler : register(s0, space2);
StructuredBuffer<Draw> fragmentDraws : register(t1, space2);

struct CasterIn
{
    [[vk::location(0)]] float3 position : TEXCOORD0;
    [[vk::location(1)]] float2 uv : TEXCOORD1;
    [[vk::location(2)]] uint draw : TEXCOORD2;
};

struct CasterOut
{
    float4 position : SV_Position;
    [[vk::location(0)]] float depth : TEXCOORD0;
    [[vk::location(1)]] float2 uv : TEXCOORD1;
    [[vk::location(2)]] nointerpolation uint draw : TEXCOORD2;
};

CasterOut vs_main(CasterIn i)
{
    Draw d = draws[i.draw];
    CasterOut o;
    o.position = transform(i.position, views[d.mode.w].toClip);
    o.depth = o.position.z;
    o.uv = float2(dot(float3(i.uv, 1), d.uvU.xyz), dot(float3(i.uv, 1), d.uvV.xyz));
    o.draw = i.draw;
    return o;
}

float4 fs_main(CasterOut i) : SV_Target0
{
    Draw d = fragmentDraws[i.draw];
    float alpha = colourMap.Sample(colourSampler, stage_uv(i.uv, d.uvScale.xy, d.uvLimit.xy)).a;
    if (alpha < 207.0 / 255)
        discard;
    return float4(i.depth * (d.mesh.x != 0 ? -1 : 1), 0, 0, 1);
}
