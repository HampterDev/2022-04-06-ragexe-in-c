// What Direct3D 9's fixed function does for the client's vertices (render/gfx.h, gfx_draw): two
// texture stages with the operations and arguments render/gfx.h names, the vertex fog in the
// specular alpha and the alpha test. Pre-transformed vertices keep Direct3D 9's pixels: the
// rasterizer gets x and y snapped as Direct3D 9 snapped them (snapped_ndc), and covers pixels with
// Direct3D's top-left rule. Their w stays 1 so that nothing divides the position; the varyings are
// interpolated perspective-correct by hand, with the rhw, as Direct3D 9 interpolates them.
// GFX_VTX_WORLD vertices go through the camera's view to the same pixels, which the GPU divides
// and moves by the viewport as Direct3D 9 did.
#include "records.hlsli"

StructuredBuffer<Draw> draws : register(t0, space0);
StructuredBuffer<View> views : register(t1, space0);

[[vk::combinedImageSampler]] Texture2D<float4> texture0 : register(t0, space2);
[[vk::combinedImageSampler]] SamplerState sampler0 : register(s0, space2);
[[vk::combinedImageSampler]] Texture2D<float4> texture1 : register(t1, space2);
[[vk::combinedImageSampler]] SamplerState sampler1 : register(s1, space2);
StructuredBuffer<Draw> fragmentDraws : register(t2, space2);

struct VertexIn
{
    [[vk::location(0)]] float4 position : TEXCOORD0;  // x, y, z, rhw; or world x, y, z
    [[vk::location(1)]] float4 diffuse : TEXCOORD1;   // the bytes of a D3DCOLOR: b, g, r, a
    [[vk::location(2)]] float4 specular : TEXCOORD2;
    [[vk::location(3)]] float2 uv0 : TEXCOORD3;
    [[vk::location(4)]] float2 uv1 : TEXCOORD4;
    [[vk::location(5)]] uint draw : TEXCOORD5;        // per instance: the draw's record
};

// Each varying times q, and q: 1 / w, interpolated in screen space and divided per pixel.
struct Varyings
{
    float4 position : SV_Position;
    [[vk::location(0)]] noperspective float4 diffuse : TEXCOORD0;
    [[vk::location(1)]] noperspective float4 specular : TEXCOORD1;
    [[vk::location(2)]] noperspective float4 uv : TEXCOORD2;
    [[vk::location(3)]] noperspective float q : TEXCOORD3;
    [[vk::location(4)]] nointerpolation uint draw : TEXCOORD4;
};

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
    o.specular = i.specular.zyxw * q;
    o.uv = float4(i.uv0, i.uv1) * q;
    o.q = q;
    o.draw = i.draw;
    return o;
}

// A stage's argument (GFX_TA_*): the diffuse colour, the current one or the texel, its alpha
// replicated, complemented.
float4 argument(uint a, float4 diffuse, float4 current, float4 tex)
{
    uint selector = a & 15;
    float4 r = selector == 0 ? diffuse : selector == 2 ? tex : current;
    if ((a & 32) != 0)
        r = r.aaaa;
    if ((a & 16) != 0)
        r = 1 - r;
    return r;
}

// A stage's operation (GFX_TOP_*) on its two arguments.
float4 combine(uint op, float4 a1, float4 a2, float4 current, float4 tex, float4 diffuse)
{
    switch (op) {
    case 2: return a1;
    case 3: return a2;
    case 4: return a1 * a2;
    case 5: return a1 * a2 * 2;
    case 7: return a1 + a2;
    case 8: return a1 + a2 - 0.5;
    case 12: return lerp(a2, a1, diffuse.a);
    case 13: return lerp(a2, a1, tex.a);
    case 18: return float4(a1.rgb + a1.a * a2.rgb, a1.a);
    default: return current;
    }
}

float4 stage(uint4 s, uint4 sb, float4 current, float4 tex, float4 diffuse)
{
    float4 c = combine(s.x, argument(s.y, diffuse, current, tex), argument(s.z, diffuse, current, tex), current,
                       tex, diffuse);
    float a = s.w == TOP_DISABLE ? current.a
            : combine(s.w, argument(sb.x, diffuse, current, tex), argument(sb.y, diffuse, current, tex), current,
                      tex, diffuse).a;
    return saturate(float4(c.rgb, a));
}

// A stage's texel: its texture at its coordinate set, opaque white without a texture, and with
// GFX_ADDRESS_BORDER the border colour outside 0 to 1.
float4 texel(float4 sampled, uint4 sb, float2 uv, float4 border)
{
    if ((sb.w & DRAW_TEXTURE) == 0)
        return 1;
    if ((sb.w & DRAW_BORDER) != 0 && any(abs(uv - 0.5) > 0.5))
        return border;
    return sampled;
}

float4 fs_main(Varyings i) : SV_Target0
{
    Draw d = fragmentDraws[i.draw];
    float4 diffuse = i.diffuse / i.q, specular = i.specular / i.q, uv = i.uv / i.q;
    float2 uv0 = (d.stage0b.z & 7) == 1 ? uv.zw : uv.xy;
    float2 uv1 = (d.stage1b.z & 7) == 1 ? uv.zw : uv.xy;
    float4 sampled0 = texture0.Sample(sampler0, stage_uv(uv0, d.uvScale.xy, d.uvLimit.xy));
    float4 sampled1 = texture1.Sample(sampler1, stage_uv(uv1, d.uvScale.zw, d.uvLimit.zw));
    float4 current = diffuse;
    if (d.stage0.x != TOP_DISABLE) {
        current = stage(d.stage0, d.stage0b, current, texel(sampled0, d.stage0b, uv0, d.border0), diffuse);
        if (d.stage1.x != TOP_DISABLE)
            current = stage(d.stage1, d.stage1b, current, texel(sampled1, d.stage1b, uv1, d.border1), diffuse);
    }
    if (d.mode.z != 0)
        current.rgb = lerp(d.fogColour.rgb, current.rgb, specular.a);
    if (d.target.w != 0) {
        // alpha to coverage instead of the test: the edge where alpha crosses the reference, a
        // pixel wide, spread over the samples
        float a = current.a * 255;
        current.a = saturate((a - d.target.x) / max(fwidth(a), 1.0 / 16) + 0.5);
        if (current.a <= 0)
            discard;
    } else if (!alpha_passes(d.mode.y, current.a, d.target.x)) {
        discard;
    }
    return current;
}
