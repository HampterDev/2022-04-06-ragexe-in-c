// The client's quads and triangles (render/gfx.h, gfx_draw_quad and gfx_draw_vertices): every
// one is an instance, a record of the frame's quads (render/quads.c, instances.hlsli), from which
// this vertex shader makes the vertices the client's C code made for it before, bit for bit
// (exact.hlsli, quad_vertex); the fragment shader draws the texture stages' combination of the
// draw's mode (render/draw_modes.h, enum DrawCombine), the vertex fog and the alpha test, as
// Direct3D 9 drew them (quad.hlsli, quad_pixel).
// Vertices on the screen keep Direct3D 9's pixels: the rasterizer gets x and y snapped as
// Direct3D 9 snapped them (snapped_ndc), and covers pixels with Direct3D's top-left rule. Their w
// stays 1 so that nothing divides the position; the varyings are interpolated perspective-correct
// by hand, with the rhw, as Direct3D 9 interpolates them. QUAD_CAMERA's world points go through the
// camera's view to the same pixels, which the GPU divides and moves by the viewport as Direct3D 9
// did.
#include "quad.hlsli"
#include "exact.hlsli"

StructuredBuffer<View> views : register(t2, space0);
StructuredBuffer<uint4> quads : register(t3, space0);

#include "instances.hlsli"

struct QuadIn
{
    [[vk::location(0)]] uint corner : TEXCOORD0;  // per vertex: 0 top left, 1 top right, 2 bottom left, 3 bottom right
    [[vk::location(1)]] uint at : TEXCOORD1;      // per instance: the row its record starts at in quads
};

Varyings vs_main(QuadIn i)
{
    uint4 h = quads[i.at];
    Draw d = draws[h.y];
    QuadVertex v = quad_vertex(i.at, i.corner);
    Varyings o;
    float q;
    if ((h.x & QUAD_KIND) == QUAD_CAMERA) {
        o.position = transform(float3(v.x, v.y, v.z), views[d.mode.w].toClip);
        q = 1 / o.position.w;
    } else {
        o.position = float4(snapped_ndc(float2(v.x, v.y), d.target.yz), v.z, 1);
        q = v.rhw > 0 ? v.rhw : 1;
    }
    o.diffuse = v.colour * q;
    o.uv = v.uv * q;
    // the fog by the rhw: a world point's the one the CPU's projection gave it, as the original's
    o.fogQ = float2(d.mode.z != 0 ? vertex_fog(v.rhw, d.fog, d.stages.w) * q : q, q);
    o.draw = h.y;
    o.layer = h.x >> 16;
    return o;
}

float4 fs_main(Varyings i) : SV_Target0
{
    return quad_pixel(i);
}
