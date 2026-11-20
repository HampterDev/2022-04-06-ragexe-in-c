// The client's quads and triangles (render/gfx.h, gfx_draw_quad and gfx_draw_vertices): every
// one is an instance, a record of the frame's quads (render/quads.c, instances.hlsli), from which
// this vertex shader makes the vertices the client's C code made for it before (quad_vertex); the
// fragment shader draws the texture stages' combination of the draw's mode (render/draw_modes.h,
// enum DrawCombine), the vertex fog and the alpha test, as Direct3D 9 drew them (quad.hlsli,
// quad_pixel).
// Vertices on the screen keep Direct3D 9's pixels: the rasterizer gets x and y snapped as
// Direct3D 9 snapped them (snapped_ndc), and covers pixels with Direct3D's top-left rule. Their w
// stays 1 so that nothing divides the position; the varyings are interpolated perspective-correct
// by hand, with the rhw, as Direct3D 9 interpolates them.
#include "quad.hlsli"
#include "projection.hlsli"

StructuredBuffer<uint4> quads : register(t1, space0);

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
    float q = v.rhw > 0 ? v.rhw : 1;
    o.position = float4(snapped_ndc(float2(v.x, v.y), d.target.yz), v.z, 1);
    o.diffuse = v.colour * q;
    o.uv = v.uv * q;
    o.fogQ = float2(d.mode.z != 0 ? vertex_fog(v.rhw, d.fog) * q : q, q);
    o.draw = h.y;
    o.layer = h.x >> 16;
    return o;
}

float4 fs_main(Varyings i) : SV_Target0
{
    return quad_pixel(i);
}
