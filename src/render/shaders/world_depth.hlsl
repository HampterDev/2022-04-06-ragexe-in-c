// The receivers' depth pass (render/gpu_world.c): the static world's records the view's cull kept
// (world_cull.hlsl), drawn single-sampled into two targets of view depth, where the shadow fit
// reads them (shadow_fit.hlsli): the first takes the nearest receiver of every pass, the second, in
// the pipelines of the passes up to the ground, the nearest one that is not blended, through which
// nothing behind shows. A texel the draw's mode drops by its alpha shows what lies behind it, so it
// is dropped here too: the opaque passes' and the ground's below 0xcf, the blended passes' at 0.
#include "records.hlsli"
#include "world.hlsli"

StructuredBuffer<WorldRecord> records : register(t0, space0);
StructuredBuffer<WorldView> views : register(t1, space0);
StructuredBuffer<float4> uvs : register(t2, space0);

[[vk::combinedImageSampler]] Texture2DArray<float4> colourMaps : register(t0, space2);
[[vk::combinedImageSampler]] SamplerState colourSampler : register(s0, space2);

struct DepthIn
{
    [[vk::location(0)]] float3 position : TEXCOORD0;
    [[vk::location(1)]] float2 uv : TEXCOORD1;
    [[vk::location(2)]] uint record : TEXCOORD2;
};

struct DepthOut
{
    float4 position : SV_Position;
    [[vk::location(0)]] float depth : TEXCOORD0;
    [[vk::location(1)]] float2 uv : TEXCOORD1;
    // the texture's layer, whether the record has a texture, whether its pass is blended
    [[vk::location(2)]] nointerpolation uint3 layer : TEXCOORD2;
};

struct Depths
{
    float front : SV_Target0;
    float back : SV_Target1;
};

DepthOut vs_main(DepthIn i)
{
    WorldRecord rec = records[i.record];
    DepthOut o;
    o.position = transform(i.position, views[record_slot(rec)].toClip);
    o.depth = o.position.w;
    uint uvSlot = record_uv(rec);
    float4 uvU = uvSlot != WORLD_NO_UV ? uvs[uvSlot * 2] : float4(1, 0, 0, 0);
    float4 uvV = uvSlot != WORLD_NO_UV ? uvs[uvSlot * 2 + 1] : float4(0, 1, 0, 0);
    o.uv = world_turn(i.uv, uvU, uvV);
    o.layer = uint3(rec.data.x & 0xffff, record_textured(rec) ? 1u : 0u, record_pass(rec) > WORLD_PASS_GROUND ? 1u : 0u);
    return o;
}

Depths fs_main(DepthOut i)
{
    if (i.layer.y != 0) {
        float a = floor(colourMaps.Sample(colourSampler, float3(i.uv, i.layer.x)).a * 255 + 0.5);
        if (a < (i.layer.z != 0 ? 1.0 : 207.0))
            discard;
    }
    Depths o;
    o.front = i.depth;
    o.back = i.depth;
    return o;
}
