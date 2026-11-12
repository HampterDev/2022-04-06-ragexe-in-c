// The client's quads (render/gfx.h, gfx_draw): the texture stages' combination of the draw's mode
// (render/draw_modes.h, enum DrawCombine), the vertex fog and the alpha test, as Direct3D 9 drew
// them.
// Pre-transformed vertices keep Direct3D 9's pixels: the rasterizer gets x and y snapped as
// Direct3D 9 snapped them (snapped_ndc), and covers pixels with Direct3D's top-left rule. Their w
// stays 1 so that nothing divides the position; the varyings are interpolated perspective-correct
// by hand, with the rhw, as Direct3D 9 interpolates them. GFX_VTX_WORLD vertices go through the
// camera's view to the same pixels, which the GPU divides and moves by the viewport as Direct3D 9
// did.
#include "quad.hlsli"

// One layout for every quad: a vertex with one texture's coordinates takes them for both.
struct VertexIn
{
    [[vk::location(0)]] float4 position : TEXCOORD0;  // x, y, z, rhw; or world x, y, z
    [[vk::location(1)]] float4 diffuse : TEXCOORD1;   // the bytes of a D3DCOLOR: b, g, r, a
    [[vk::location(2)]] float2 uv0 : TEXCOORD2;
    [[vk::location(3)]] float2 uv1 : TEXCOORD3;
    [[vk::location(4)]] uint draw : TEXCOORD4;        // per instance: the draw's record
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
    o.uv = float4(i.uv0, i.uv1) * q;
    // the fog of a vertex in world coordinates by the rhw the CPU's projection gave it, as the
    // original's
    o.fogQ = float2(d.mode.z != 0 ? vertex_fog(i.position.w, d.fog, d.stages.w) * q : q, q);
    o.draw = i.draw;
    return o;
}

float4 fs_main(Varyings i) : SV_Target0
{
    return quad_pixel(i);
}
