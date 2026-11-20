// tests/test_static_world_maps.c: render/shaders/water.hlsli on the GPU, a thread a cell of a
// map's water, handing back what water.hlsl computes for its quad: the wave's heights at the
// cell's diagonal and on either side, whether the quad is drawn, and its corners' rhw and points
// on the screen, which the test compares with world/ground_water.c and render/rp_project.c.
#include "../../src/render/shaders/water.hlsli"

StructuredBuffer<WaterCell> cells : register(t0, space0);
StructuredBuffer<Water> waters : register(t1, space0);
StructuredBuffer<float> sines : register(t2, space0);
RWStructuredBuffer<float4> results : register(u0, space1);  // four a cell

cbuffer Probe : register(b0, space2)
{
    WaterCamera camera;
    uint4 count;  // x: the cells
};

[numthreads(64, 1, 1)]
void cs_main(uint3 id : SV_DispatchThreadID)
{
    if (id.x >= count.x)
        return;
    WaterCell c = cells[id.x];
    Water w = waters[c.cell.z];
    int diagonal = (int)c.cell.x + (int)c.cell.y;
    float3 heights = float3(wave_height(w, sines[wave_degrees(w, diagonal - 1)]),
                            wave_height(w, sines[wave_degrees(w, diagonal)]),
                            wave_height(w, sines[wave_degrees(w, diagonal + 1)]));
    WaterQuad q = water_quad(c, camera, heights);
    results[id.x * 4] = float4(heights, q.drawn ? 1.0 : 0.0);
    results[id.x * 4 + 1] = float4(q.rhw[0], q.rhw[1], q.rhw[2], q.rhw[3]);
    results[id.x * 4 + 2] = float4(q.screen[0], q.screen[1]);
    results[id.x * 4 + 3] = float4(q.screen[2], q.screen[3]);
}
