// Whether the GPU rounds as exact.hlsli assumes (render/gfx_gpu.c, check_rounding): each case of
// two floats a and b, drawn as a point on its own pixel of a 32-bit float target, gives a + b,
// a - b and a * b, each rounded once, and div_rn(a, b), which the CPU compares with its own.
#include "exact.hlsli"

StructuredBuffer<float4> cases : register(t0, space0);

struct Point
{
    float4 position : SV_Position;
    [[vk::builtin("PointSize")]] float size : PSIZE;
    [[vk::location(0)]] nointerpolation float4 result : TEXCOORD0;
};

// the cases' pixels: a row of CHECK_WIDTH (render/gfx_gpu.c)
#define CHECK_WIDTH 64

Point vs_main(uint id : SV_VertexID)
{
    float4 c = cases[id];
    precise float sum = c.x + c.y;
    precise float difference = c.x - c.y;
    precise float product = c.x * c.y;
    Point o;
    o.position = float4(((float)id + 0.5) * (2.0 / CHECK_WIDTH) - 1.0, 0.0, 0.5, 1.0);
    o.size = 1.0;
    o.result = float4(sum, difference, product, div_rn(c.x, c.y));
    return o;
}

float4 fs_main(Point i) : SV_Target0
{
    return i.result;
}
