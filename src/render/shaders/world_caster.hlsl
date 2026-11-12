// A shadow caster's depth from the light (render/gfx.h, gfx_world_cast): triangles of the static
// world drawn into one cascade's map, through the cascade's view of their transform slot
// (world_transform.hlsl), their depth negative for a model's; texels the opaque lists' alpha test
// (0xcf) drops cast nothing, and the ground casts without its texture.
#include "records.hlsli"
#include "world.hlsli"

StructuredBuffer<WorldRecord> records : register(t0, space0);
StructuredBuffer<float4x4> casterViews : register(t1, space0);  // four a slot, one a cascade
StructuredBuffer<float4> uvs : register(t2, space0);

cbuffer VertexDraw : register(b0, space1)
{
    uint4 vertexDraw;  // x: unused, y: the cascade
};

[[vk::combinedImageSampler]] Texture2DArray<float4> colourMaps : register(t0, space2);
[[vk::combinedImageSampler]] SamplerState colourSampler : register(s0, space2);
StructuredBuffer<Draw> fragmentDraws : register(t1, space2);

cbuffer FragmentDraw : register(b0, space3)
{
    uint4 fragmentDraw;  // x: the draw's record
};

struct CasterIn
{
    [[vk::location(0)]] float3 position : TEXCOORD0;
    [[vk::location(1)]] float2 uv : TEXCOORD1;
    [[vk::location(2)]] uint record : TEXCOORD2;
};

struct CasterOut
{
    float4 position : SV_Position;
    [[vk::location(0)]] float depth : TEXCOORD0;
    [[vk::location(1)]] float2 uv : TEXCOORD1;
    // the texture's layer, whether the record has a texture, whether it is a model's
    [[vk::location(2)]] nointerpolation uint3 layer : TEXCOORD2;
};

CasterOut vs_main(CasterIn i)
{
    WorldRecord rec = records[i.record];
    CasterOut o;
    o.position = transform(i.position, casterViews[record_slot(rec) * 4 + vertexDraw.y]);
    o.depth = o.position.z;
    uint uvSlot = record_uv(rec);
    float4 uvU = uvSlot != WORLD_NO_UV ? uvs[uvSlot * 2] : float4(1, 0, 0, 0);
    float4 uvV = uvSlot != WORLD_NO_UV ? uvs[uvSlot * 2 + 1] : float4(0, 1, 0, 0);
    o.uv = world_turn(i.uv, uvU, uvV);
    bool ground = record_pass(rec) == WORLD_PASS_GROUND;
    o.layer = uint3(rec.data.x & 0xffff, record_textured(rec) && !ground ? 1u : 0u, ground ? 0u : 1u);
    return o;
}

float4 fs_main(CasterOut i) : SV_Target0
{
    Draw d = fragmentDraws[fragmentDraw.x];
    float alpha = colourMaps.Sample(colourSampler, float3(stage_uv(i.uv, d.uvScale.xy, d.uvLimit.xy), i.layer.x)).a;
    if (i.layer.y == 0)
        alpha = 1.0;
    if (alpha < 207.0 / 255)
        discard;
    return float4(i.depth * (i.layer.z != 0 ? -1 : 1), 0, 0, 1);
}
