// The static world's draw records (render/gfx.h, gfx_world_draw and gfx_world_draw_record): the
// ground's and the models' triangles from the map's shared buffers, as the Direct3D 9 client drew
// them: the stages' work for a model's or the ground's quads, the fog by view depth, and the shadow
// maps while receiving, as the shadow fit left them on the GPU (cascades.hlsli), with the alpha
// test of the draw's mode. A vertex carries its record's id, which gives its transform slot, its
// texture-coordinate matrix and its textures' layers; the draw's record (records.hlsli, Draw) comes
// as a uniform, the same for every record of an indirect call.
#include "records.hlsli"
#include "world.hlsli"
#include "cascades.hlsli"

StructuredBuffer<WorldRecord> records : register(t0, space0);
StructuredBuffer<WorldView> views : register(t1, space0);
StructuredBuffer<Cascades> fit : register(t2, space0);
StructuredBuffer<float4> uvs : register(t3, space0);

cbuffer VertexDraw : register(b0, space1)
{
    Draw vertexDraw;
};

[[vk::combinedImageSampler]] Texture2DArray<float4> colourMaps : register(t0, space2);
[[vk::combinedImageSampler]] SamplerState colourSampler : register(s0, space2);
[[vk::combinedImageSampler]] Texture2DArray<float4> lightMaps : register(t1, space2);
[[vk::combinedImageSampler]] SamplerState lightSampler : register(s1, space2);
[[vk::combinedImageSampler]] Texture2DArray<float4> shadowMaps : register(t2, space2);
[[vk::combinedImageSampler]] SamplerState shadowSampler : register(s2, space2);
StructuredBuffer<Cascades> fragmentFit : register(t3, space2);

cbuffer FragmentDraw : register(b0, space3)
{
    Draw fragmentDraw;
};

struct WorldIn
{
    [[vk::location(0)]] float3 position : TEXCOORD0;
    [[vk::location(1)]] float3 normal : TEXCOORD1;
    [[vk::location(2)]] float4 lit : TEXCOORD2;    // D3DCOLOR bytes: b, g, r, a
    [[vk::location(3)]] float4 lit2 : TEXCOORD3;
    [[vk::location(4)]] float2 uv : TEXCOORD4;
    [[vk::location(5)]] float2 uv1 : TEXCOORD5;
    [[vk::location(6)]] uint record : TEXCOORD6;   // the vertex's record id
};

struct WorldOut
{
    float4 position : SV_Position;
    [[vk::location(0)]] float4 lit : TEXCOORD0;
    [[vk::location(1)]] float4 shade : TEXCOORD1;
    [[vk::location(2)]] float4 uv : TEXCOORD2;
    [[vk::location(3)]] float4 l0 : TEXCOORD3;
    [[vk::location(4)]] float4 l1 : TEXCOORD4;
    [[vk::location(5)]] float4 l2 : TEXCOORD5;
    [[vk::location(6)]] float4 l3 : TEXCOORD6;
    [[vk::location(7)]] float3 depthFog : TEXCOORD7;  // view depth, fog factor, the slant's tangent
    // the texture's layer, the lightmap's, and whether the record has each (bit 0, bit 1)
    [[vk::location(8)]] nointerpolation uint3 layers : TEXCOORD8;
};

// Whether the draw receives shadows: the frame has them (Draw.mesh.w) and something on screen
// receives them; where nothing does, the ground keeps its baked shadows.
bool receiving(Draw d, Cascades s)
{
    return d.mesh.w != 0 && s.count.x != 0;
}

WorldOut vs_main(WorldIn i)
{
    Draw d = vertexDraw;
    WorldRecord rec = records[i.record];
    WorldView v = views[record_slot(rec)];
    float4 lit = i.lit.zyxw, lit2 = i.lit2.zyxw;
    WorldOut o;
    o.position = transform(i.position, v.toClip);
    float3 own = floor(lit.rgb * 255 + 0.5);
    Cascades s = (Cascades)0;
    if (d.mesh.w != 0)
        s = fit[0];
    bool receive = receiving(d, s);
    float4 shading = receive ? float4(s.ambient.rgb, s.ambient.w) : float4(1, 1, 1, 1);
    if (d.mesh.x != 0) {
        float3 ground = d.mesh.y != 0 ? floor(lit2.rgb * 255 + 0.5) : 0;
        float3 dark = min(max(floor(shading.rgb * floor(lit.a * 255 + 0.5)), floor(own * shading.w)), own);
        o.lit = float4(floor(min(own + ground, 255) * d.colour.rgb) / 255, d.colour.a);
        o.shade = float4(floor(min(dark + ground, 255) * d.colour.rgb) / 255, d.colour.a);
    } else {
        o.lit = float4(floor(own * d.colour.rgb) / 255, lit.a * d.colour.a);
        o.shade = o.lit;
    }
    uint uvSlot = record_uv(rec);
    float4 uvU = uvSlot != WORLD_NO_UV ? uvs[uvSlot * 2] : float4(1, 0, 0, 0);
    float4 uvV = uvSlot != WORLD_NO_UV ? uvs[uvSlot * 2 + 1] : float4(0, 1, 0, 0);
    o.uv = float4(world_turn(i.uv, uvU, uvV), i.uv1);
    float tangent = 0;
    o.l0 = o.l1 = o.l2 = o.l3 = 0;
    if (receive) {
        float3 p = transform(i.position, v.toView).xyz;
        float3 n = v.toView[1].xyz * i.normal.y;
        n = mad(i.normal.x, v.toView[0].xyz, n);
        n = mad(i.normal.z, v.toView[2].xyz, n);
        n *= rsqrt(max(dot(n, n), 1e-12));
        float c = dot(n, s.lightView.xyz);
        float3 away = n * sign(c) * sqrt(saturate(1 - c * c));
        o.l0 = transform(p + away * s.offset.x, s.fromView[0]);
        o.l1 = transform(p + away * s.offset.y, s.fromView[1]);
        o.l2 = transform(p + away * s.offset.z, s.fromView[2]);
        o.l3 = transform(p + away * s.offset.w, s.fromView[3]);
        o.l0.xy = map_uv(o.l0.xy);
        o.l1.xy = map_uv(o.l1.xy);
        o.l2.xy = map_uv(o.l2.xy);
        o.l3.xy = map_uv(o.l3.xy);
        tangent = min(sqrt(saturate(1 - c * c)) / max(abs(c), 1e-3), 8);
    }
    // the meshes' fog in the draw's record: its start and end in view depth, 1 / (end - start)
    o.depthFog = float3(o.position.w, saturate((d.fog.y - o.position.w) * d.fog.z), tangent);
    o.layers = uint3(rec.data.x & 0xffff, rec.data.x >> 16,
                     (record_textured(rec) ? 1u : 0u) | (record_lightmapped(rec) ? 2u : 0u));
    return o;
}

static const float2 disk[16] = {
    float2(-0.942, -0.399), float2(0.946, -0.769), float2(-0.094, -0.929), float2(0.345, 0.294),
    float2(-0.916, 0.458), float2(-0.815, -0.879), float2(-0.383, 0.277), float2(0.975, 0.756),
    float2(0.443, -0.975), float2(0.537, -0.474), float2(-0.265, -0.419), float2(0.792, 0.191),
    float2(-0.242, 0.997), float2(-0.814, 0.914), float2(0.200, 0.786), float2(0.144, -0.141)
};

// The share of the disc's points that are lit, and of those that a model's surface, within the
// bias in front of the receiver or as far behind as the slant reaches, may or may not shade. The
// maps are MAP_SIZE texels square in a texture a texel wider and taller (render/gfx_gpu.c, MARGIN),
// so they are read by texel, the one point sampling with the coordinates clamped to a map picks.
float2 shadow_seen(uint cascade, float4 l, float radius, float slant, float bias)
{
    float d = l.z - bias - slant, lit = 0, contact = 0;
    [unroll] for (int k = 0; k < 16; k++) {
        int2 texel = clamp(int2(floor((l.xy + disk[k] * radius) * MAP_SIZE)), 0, MAP_SIZE - 1);
        float s = shadowMaps.Load(int4(texel, cascade, 0)).r;
        lit += abs(s) >= d ? 1 : 0;
        contact += s < 0 && -s >= d && -s <= l.z + slant ? 1 : 0;
    }
    float2 edge = abs(l.xy - 0.5);
    return max(edge.x, edge.y) < 0.5 && l.z < 1 ? float2(lit, contact) / 16 : float2(1, 0);
}

float2 seen_at(WorldOut i, Cascades s)
{
    float2 seen = float2(1, 0);
    float bias = s.lightView.w;
    if (i.depthFog.x < s.farDepth.x) seen = shadow_seen(0, i.l0, s.blur.x, i.depthFog.z * s.slant.x, bias);
    else if (i.depthFog.x < s.farDepth.y) seen = shadow_seen(1, i.l1, s.blur.y, i.depthFog.z * s.slant.y, bias);
    else if (i.depthFog.x < s.farDepth.z) seen = shadow_seen(2, i.l2, s.blur.z, i.depthFog.z * s.slant.z, bias);
    else if (i.depthFog.x < s.farDepth.w) seen = shadow_seen(3, i.l3, s.blur.w, i.depthFog.z * s.slant.w, bias);
    return seen;
}

float4 fs_main(WorldOut i) : SV_Target0
{
    Draw d = fragmentDraw;
    float4 t = colourMaps.Sample(colourSampler, float3(stage_uv(i.uv.xy, d.uvScale.xy, d.uvLimit.xy), i.layers.x));
    float4 lightTexel = lightMaps.Sample(lightSampler, float3(stage_uv(i.uv.zw, d.uvScale.zw, d.uvLimit.zw), i.layers.y));
    Cascades s = (Cascades)0;
    if (d.mesh.w != 0)
        s = fragmentFit[0];
    bool receive = receiving(d, s);
    float2 seen = float2(1, 0);
    float darkness = 1;
    if (receive) {
        seen = seen_at(i, s);
        darkness = s.ambient.w;
    }
    float4 c;
    if (d.mesh.x != 0) {
        c = t * float4(lerp(i.shade.rgb, i.lit.rgb, seen.x), i.lit.a);
    } else {
        // the ground's two stages, texture times colour times the lightmap's intensity plus its
        // colour, with the shadow maps in place of the shadows baked into that intensity: down to
        // the darkness of a shadow the intensity counts as lit, and the shadow maps darken to it;
        // darker texels keep their own darkness. A stage reads with a border only where the record
        // has its texture, as a stage without one read opaque white.
        bool border = (d.stages.y & 2) != 0 && (i.layers.z & 2) != 0;
        float4 lm = d.stages.z != 0 ? bordered(lightTexel, i.uv.zw, border) : float4(0, 0, 0, 1);
        float keep = lerp(saturate(lm.a / darkness) * lerp(darkness, 1, seen.x), lm.a, seen.y);
        c = float4(saturate(lm.rgb + keep * saturate(t.rgb * i.lit.rgb)), t.a * i.lit.a);
    }
    if (d.mode.z != 0)
        c.rgb = lerp(d.fogColour.rgb, c.rgb, i.depthFog.y);
    if (!alpha_test(c.a, d))
        discard;
    return c;
}
