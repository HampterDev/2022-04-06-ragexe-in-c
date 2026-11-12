// The ground's and the models' meshes (render/gfx.h, gfx_draw_mesh), as the Direct3D 9 client
// drew them: the stages' work for a model's or the ground's quads, the fog by view depth, and the
// shadow maps while receiving (GfxShadow), with the alpha test of the render states.
#include "records.hlsli"

StructuredBuffer<Draw> draws : register(t0, space0);
StructuredBuffer<View> views : register(t1, space0);
StructuredBuffer<Shadow> shadows : register(t2, space0);

[[vk::combinedImageSampler]] Texture2D<float4> colourMap : register(t0, space2);
[[vk::combinedImageSampler]] SamplerState colourSampler : register(s0, space2);
[[vk::combinedImageSampler]] Texture2D<float4> lightMap : register(t1, space2);
[[vk::combinedImageSampler]] SamplerState lightSampler : register(s1, space2);
[[vk::combinedImageSampler]] Texture2DArray<float4> shadowMaps : register(t2, space2);
[[vk::combinedImageSampler]] SamplerState shadowSampler : register(s2, space2);
StructuredBuffer<Draw> fragmentDraws : register(t3, space2);
StructuredBuffer<Shadow> fragmentShadows : register(t4, space2);

struct MeshIn
{
    [[vk::location(0)]] float3 position : TEXCOORD0;
    [[vk::location(1)]] float3 normal : TEXCOORD1;
    [[vk::location(2)]] float4 lit : TEXCOORD2;    // D3DCOLOR bytes: b, g, r, a
    [[vk::location(3)]] float4 lit2 : TEXCOORD3;
    [[vk::location(4)]] float2 uv : TEXCOORD4;
    [[vk::location(5)]] float2 uv1 : TEXCOORD5;
    [[vk::location(6)]] uint draw : TEXCOORD6;
};

struct MeshOut
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
    [[vk::location(8)]] nointerpolation uint draw : TEXCOORD8;
};

float2 turn(float2 uv, Draw d)
{
    return float2(dot(float3(uv, 1), d.uvU.xyz), dot(float3(uv, 1), d.uvV.xyz));
}

MeshOut vs_main(MeshIn i)
{
    Draw d = draws[i.draw];
    View v = views[d.mode.w];
    float4 lit = i.lit.zyxw, lit2 = i.lit2.zyxw;
    MeshOut o;
    o.position = transform(i.position, v.toClip);
    float3 own = floor(lit.rgb * 255 + 0.5);
    bool receive = d.mesh.w != 0;
    Shadow s = (Shadow)0;
    if (receive)
        s = shadows[d.mesh.w - 1];
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
    o.uv = float4(turn(i.uv, d), i.uv1);
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
        tangent = min(sqrt(saturate(1 - c * c)) / max(abs(c), 1e-3), 8);
    }
    o.depthFog = float3(o.position.w, saturate((v.fog.y - o.position.w) * v.fog.z), tangent);
    o.draw = i.draw;
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
// maps are `size` texels square in a texture a texel wider and taller (render/gfx_gpu.c, MARGIN),
// so they are read by texel, the one point sampling with the coordinates clamped to a map picks.
float2 shadow_seen(uint cascade, float4 l, float radius, float slant, float bias, float size)
{
    float d = l.z - bias - slant, lit = 0, contact = 0;
    [unroll] for (int k = 0; k < 16; k++) {
        int2 texel = clamp(int2(floor((l.xy + disk[k] * radius) * size)), 0, int(size) - 1);
        float s = shadowMaps.Load(int4(texel, cascade, 0)).r;
        lit += abs(s) >= d ? 1 : 0;
        contact += s < 0 && -s >= d && -s <= l.z + slant ? 1 : 0;
    }
    float2 edge = abs(l.xy - 0.5);
    return max(edge.x, edge.y) < 0.5 && l.z < 1 ? float2(lit, contact) / 16 : float2(1, 0);
}

float2 seen_at(MeshOut i, Shadow s)
{
    float2 seen = float2(1, 0);
    float bias = s.lightView.w;
    float size = s.mapSize.x;
    if (i.depthFog.x < s.farDepth.x) seen = shadow_seen(0, i.l0, s.blur.x, i.depthFog.z * s.slant.x, bias, size);
    else if (i.depthFog.x < s.farDepth.y) seen = shadow_seen(1, i.l1, s.blur.y, i.depthFog.z * s.slant.y, bias, size);
    else if (i.depthFog.x < s.farDepth.z) seen = shadow_seen(2, i.l2, s.blur.z, i.depthFog.z * s.slant.z, bias, size);
    else if (i.depthFog.x < s.farDepth.w) seen = shadow_seen(3, i.l3, s.blur.w, i.depthFog.z * s.slant.w, bias, size);
    return seen;
}

float4 fs_main(MeshOut i) : SV_Target0
{
    Draw d = fragmentDraws[i.draw];
    float4 t = colourMap.Sample(colourSampler, stage_uv(i.uv.xy, d.uvScale.xy, d.uvLimit.xy));
    float4 lightTexel = lightMap.Sample(lightSampler, stage_uv(i.uv.zw, d.uvScale.zw, d.uvLimit.zw));
    bool receive = d.mesh.w != 0;
    Shadow s = (Shadow)0;
    float2 seen = float2(1, 0);
    float darkness = 1;
    if (receive) {
        s = fragmentShadows[d.mesh.w - 1];
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
        // darker texels keep their own darkness
        if ((d.stage1b.w & DRAW_BORDER) != 0 && any(abs(i.uv.zw - 0.5) > 0.5))
            lightTexel = d.border1;
        float4 lm = d.mesh.z != 0 ? lightTexel : float4(0, 0, 0, 1);
        float keep = lerp(saturate(lm.a / darkness) * lerp(darkness, 1, seen.x), lm.a, seen.y);
        c = float4(saturate(lm.rgb + keep * saturate(t.rgb * i.lit.rgb)), t.a * i.lit.a);
    }
    if (d.mode.z != 0)
        c.rgb = lerp(d.fogColour.rgb, c.rgb, i.depthFog.y);
    if (d.target.w != 0) {
        c.a = saturate((c.a * 255 - d.target.x) / max(fwidth(c.a * 255), 1.0 / 16) + 0.5);
        if (c.a <= 0)
            discard;
    } else if (!alpha_passes(d.mode.y, c.a, d.target.x)) {
        discard;
    }
    return c;
}
