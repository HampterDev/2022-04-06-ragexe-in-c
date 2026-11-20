// A shadow caster's depth from the light: the static world's records a cascade's cull kept
// (world_cull.hlsl), drawn into that cascade's map through their slot's place in the world
// (world_transform.hlsl) and the cascade's light space as the shadow fit left it on the GPU
// (cascades.hlsli), their depth negative for a model's. The vertex shader serves both of the
// shadow pass's pipelines (world_caster.hlsli); the fragment shader here is the one of the casters
// whose texels the opaque lists' alpha test (0xcf) drops, the models' textured records. One
// pipeline draws every record of a run, whatever its pass and cull mode: casters are drawn from
// both sides.
#include "records.hlsli"
#include "world.hlsli"
#include "cascades.hlsli"
#include "world_caster.hlsli"

StructuredBuffer<WorldRecord> records : register(t0, space0);
StructuredBuffer<float4> placed : register(t1, space0);  // a slot's 4x3 in three float4 (render/gfx.h)
StructuredBuffer<float4> uvs : register(t2, space0);
StructuredBuffer<Cascades> fit : register(t3, space0);

cbuffer VertexDraw : register(b0, space1)
{
    uint4 vertexDraw;  // y: the cascade
};

[[vk::combinedImageSampler]] Texture2DArray<float4> colourMaps : register(t0, space2);
[[vk::combinedImageSampler]] SamplerState colourSampler : register(s0, space2);

struct CasterIn
{
    [[vk::location(0)]] float3 position : TEXCOORD0;
    [[vk::location(1)]] float2 uv : TEXCOORD1;
    [[vk::location(2)]] uint record : TEXCOORD2;
};

CasterOut vs_main(CasterIn i)
{
    WorldRecord rec = records[i.record];
    uint slot = record_slot(rec);
    float4 a = placed[slot * 3], b = placed[slot * 3 + 1], c = placed[slot * 3 + 2];
    // the client's 4x3: x * m[0..2] + y * m[3..5] + z * m[6..8] + m[9..11]
    float3 world = i.position.x * a.xyz + i.position.y * float3(a.w, b.xy) + i.position.z * float3(b.zw, c.x)
                   + c.yzw;
    CasterOut o;
    o.position = mul(float4(world, 1), fit[0].world[vertexDraw.y]);
    o.depth = o.position.z;
    uint uvSlot = record_uv(rec);
    float4 uvU = uvSlot != WORLD_NO_UV ? uvs[uvSlot * 2] : float4(1, 0, 0, 0);
    float4 uvV = uvSlot != WORLD_NO_UV ? uvs[uvSlot * 2 + 1] : float4(0, 1, 0, 0);
    o.uv = world_turn(i.uv, uvU, uvV);
    o.layer = uint2(rec.data.x & 0xffff, record_pass(rec) != WORLD_PASS_GROUND ? 1u : 0u);
    return o;
}

float4 fs_main(CasterOut i) : SV_Target0
{
    // the opaque lists' alpha test, on the alpha rounded to 0 to 255 as theirs (records.hlsli,
    // alpha_passes) and the depth pass's (world_depth.hlsl) round it
    if (floor(colourMaps.Sample(colourSampler, float3(i.uv, i.layer.x)).a * 255 + 0.5) < 207.0)
        discard;
    return caster_depth(i);
}
