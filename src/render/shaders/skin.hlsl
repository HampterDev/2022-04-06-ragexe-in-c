// Granny's actors (render/gfx.h, gfx_draw_skin): a model's vertices, which stay on the device,
// skinned, lit and projected in an actor's pose (skin.hlsli) to the pixel and depth the CPU gave
// them, with the original's vertex fog by their rhw. The pixels are the quads' (quad.hlsli,
// quad_pixel), as Granny's triangles were drawn through the quad shader before.
#include "quad.hlsli"

StructuredBuffer<uint4> quads : register(t1, space0);

#include "skin.hlsli"

struct SkinIn
{
    [[vk::location(0)]] float3 position : TEXCOORD0;
    [[vk::location(1)]] float3 normal : TEXCOORD1;
    [[vk::location(2)]] float2 uv : TEXCOORD2;
    [[vk::location(3)]] float4 weights : TEXCOORD3;
    [[vk::location(4)]] uint4 bones : TEXCOORD4;
    [[vk::location(5)]] uint flags : TEXCOORD5;
    // per instance: the row of its header among the frame's quads (the pose's first row, the
    // draw's record)
    [[vk::location(6)]] uint at : TEXCOORD6;
};

Varyings vs_main(SkinIn i)
{
    uint4 h = quads[i.at];
    Draw d = draws[h.y];
    SkinVertex v;
    v.position = i.position;
    v.normal = i.normal;
    v.uv = i.uv;
    v.weights = i.weights;
    v.bones = i.bones;
    v.flags = i.flags;
    Skinned s = skin_vertex(h.x, v);
    Varyings o;
    o.position = skin_clip(h.x, s.view, d.target.yz);
    float q = 1 / o.position.w;
    o.diffuse = s.colour * q;
    o.uv = float4(s.uv, s.uv) * q;
    o.fogQ = float2(d.mode.z != 0 ? vertex_fog(q, d.fog) * q : q, q);
    o.draw = h.y;
    o.layer = 0;
    return o;
}

float4 fs_main(Varyings i) : SV_Target0
{
    return quad_pixel(i);
}
