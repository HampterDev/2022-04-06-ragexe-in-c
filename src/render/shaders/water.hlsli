// The water's quads as the original built them on the CPU each frame (projection.hlsli): water.hlsl
// draws them, and tests/test_static_world_maps.c holds them to a tolerance of that code, which it
// keeps (ref_water), through tests/shaders/water_probe.hlsl.
#include "projection.hlsli"

// A cell of the map's water (struct GpuWaterCell): its x and y, its water's block, and the bits of
// the greatest y of its ground's corners, the lowest, y growing downward (-infinity where none is a
// number); its corners' x and z.
struct WaterCell
{
    uint4 cell;
    float4 corners01;  // corner 0's x and z, corner 1's
    float4 corners23;
};

// A block of the map's water (struct GpuWater, render/gfx.h GfxWaterWave): its wave's phase this
// frame, pitch, height and level.
struct Water
{
    float4 wave;
};

// The view the CPU projected the quads with: the camera's 4x3 view (render/gfx.h), then
// Renderer.proj[0], proj[1], (float)centreX, (float)centreY (projection.hlsli, project_point).
struct WaterCamera
{
    Matrix43 view;
    float4 screen;
};

// world/ground_water.c, ground_water_height: the whole degree of the wave at a diagonal that
// get_sin_deg (render/math3d.c) looks up, 0 to 359. An angle that is no number or leaves the int
// range is INT_MIN degrees, as x86's conversion makes it.
int wave_degrees(Water w, int diagonal)
{
    float angle = (float)diagonal * w.wave.y + w.wave.x;
    uint degrees = 0x80000000u;
    if (angle >= -2147483648.0 && angle < 2147483648.0)
        degrees = (uint)(int)angle;
    // the C's % and + 360, taken of the magnitude: Vulkan leaves SPIR-V's remainder of a negative
    // operand (OpSRem, OpSMod) undefined, and NVIDIA's driver does not give C's
    bool negative = (degrees & 0x80000000u) != 0;
    uint magnitude = negative ? 0u - degrees : degrees;
    uint r = magnitude % 360u;
    return negative && r != 0 ? 360 - (int)r : (int)r;
}

// the height of the wave whose sine at its degree is `sine`
float wave_height(Water w, float sine)
{
    if (w.wave.z == 0.0)
        return w.wave.w;
    return sine * w.wave.z + w.wave.w;
}

// A cell's quad as the original built it: its corners in the world, raised by the wave at
// the cell's diagonal (corners 1 and 2) and the ones on either side (corner 0 the one before,
// corner 3 the one after), their rhw, and whether it is drawn: where a corner of the cell's ground
// lies below the wave (y grows downward), every corner lies in front of the eye and its first
// triangle faces the screen.
struct WaterQuad
{
    float3 p[4];
    float rhw[4];
    float2 screen[4];
    bool drawn;
};

// heights: the wave at the diagonal before the cell's, at it and after it (wave_height)
WaterQuad water_quad(WaterCell c, WaterCamera camera, float3 heights)
{
    WaterQuad q;
    q.p[0] = float3(c.corners01.x, heights.x, c.corners01.y);
    q.p[1] = float3(c.corners01.z, heights.y, c.corners01.w);
    q.p[2] = float3(c.corners23.x, heights.y, c.corners23.y);
    q.p[3] = float3(c.corners23.z, heights.z, c.corners23.w);
    Projected s0 = project_point(q.p[0], camera.view, camera.screen);
    Projected s1 = project_point(q.p[1], camera.view, camera.screen);
    Projected s2 = project_point(q.p[2], camera.view, camera.screen);
    Projected s3 = project_point(q.p[3], camera.view, camera.screen);
    float turnA = (s2.x - s0.x) * (s1.y - s0.y);
    float turnB = (s1.x - s0.x) * (s2.y - s0.y);
    q.rhw[0] = s0.rhw;
    q.rhw[1] = s1.rhw;
    q.rhw[2] = s2.rhw;
    q.rhw[3] = s3.rhw;
    q.screen[0] = float2(s0.x, s0.y);
    q.screen[1] = float2(s1.x, s1.y);
    q.screen[2] = float2(s2.x, s2.y);
    q.screen[3] = float2(s3.x, s3.y);
    q.drawn = asfloat(c.cell.w) > heights.y && s0.rhw > 0.0 && s1.rhw > 0.0 && s2.rhw > 0.0 && s3.rhw > 0.0
              && turnA > turnB;
    return q;
}
