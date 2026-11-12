// The screen wobble (render/gfx.h, gfx_wobble): one triangle over the whole screen, each of whose
// pixels reads the screen's picture as it was before the wobble (a copy of it): row y from the row
// and the shift its draw's table gives, the pixels left of the shift from row y itself. Every
// sample of a pixel takes that texel, so a multisampled screen resolves to it exactly.
#include "records.hlsli"

StructuredBuffer<Draw> draws : register(t0, space0);

[[vk::combinedImageSampler]] Texture2D<float4> frame : register(t0, space2);
[[vk::combinedImageSampler]] SamplerState frameSampler : register(s0, space2);
// a row's source (low 16 bits) and shift in pixels (high 16), from the draw's first row
StructuredBuffer<uint> rows : register(t1, space2);

struct RowsIn
{
    [[vk::location(0)]] uint draw : TEXCOORD0;
    uint vertex : SV_VertexID;
};

struct RowsOut
{
    float4 position : SV_Position;
    [[vk::location(0)]] nointerpolation uint first : TEXCOORD0;
};

RowsOut vs_main(RowsIn i)
{
    RowsOut o;
    // (-1, 1), (3, 1), (-1, -3): a triangle over the viewport, which is the whole target
    o.position = float4((i.vertex & 1) * 4.0 - 1.0, 1.0 - (i.vertex >> 1) * 4.0, 0, 1);
    o.first = draws[i.draw].mode.w;
    return o;
}

float4 fs_main(RowsOut i) : SV_Target0
{
    int2 pixel = int2(i.position.xy);
    uint row = rows[i.first + (uint)pixel.y];
    int shift = (int)(row >> 16);
    int2 from = pixel.x >= shift ? int2(pixel.x - shift, (int)(row & 0xffff)) : pixel;
    return frame.Load(int3(from, 0));
}
