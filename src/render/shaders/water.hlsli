// The water's quads as the original built them on the CPU each frame, to the bit (exact.hlsli):
// water.hlsl draws them, and tests/test_static_world_maps.c checks them against that code, which it
// keeps (ref_water), through tests/shaders/water_probe.hlsl.
#include "exact.hlsli"

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

// The view the CPU projected the quads with: the camera's 4x3 view (render/gfx.h) as three float4
// of its twelve floats, then Renderer.proj[0], proj[1], (float)centreX, (float)centreY.
struct WaterCamera
{
    float4 view0, view1, view2;
    float4 screen;
};

float camera_m(WaterCamera c, uint i)
{
    float4 r = i < 4 ? c.view0 : i < 8 ? c.view1 : c.view2;
    return r[i & 3];
}

// world/ground_water.c, ground_water_height: the whole degree of the wave at a diagonal that
// get_sin_deg (render/math3d.c) looks up, 0 to 359. An angle that is no number or leaves the int
// range is INT_MIN degrees, as x86's conversion makes it.
int wave_degrees(Water w, int diagonal)
{
    precise float fd = (float)diagonal;
    precise float along = fd * w.wave.y;
    precise float angle = along + w.wave.x;
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
    precise float s = sine * w.wave.z;
    precise float h = s + w.wave.w;
    return h;
}

// render/rp_project.c, project_vertex: x, y and rhw on the screen
float3 water_project(WaterCamera c, float3 v)
{
    precise float z0 = camera_m(c, 2) * v.x;
    precise float z1 = camera_m(c, 5) * v.y;
    precise float z2 = camera_m(c, 8) * v.z;
    precise float z01 = z0 + z1;
    precise float z012 = z01 + z2;
    precise float depth = z012 + camera_m(c, 11);
    precise float rhw = div_rn(1.0, depth);
    precise float x0 = camera_m(c, 3) * v.y;
    precise float x1 = v.x * camera_m(c, 0);
    precise float x2 = camera_m(c, 6) * v.z;
    precise float x01 = x0 + x1;
    precise float x012 = x01 + x2;
    precise float vx = x012 + camera_m(c, 9);
    precise float y0 = camera_m(c, 4) * v.y;
    precise float y1 = camera_m(c, 1) * v.x;
    precise float y2 = camera_m(c, 7) * v.z;
    precise float y01 = y0 + y1;
    precise float y012 = y01 + y2;
    precise float vy = y012 + camera_m(c, 10);
    precise float xr = vx * rhw;
    precise float xs = xr * c.screen.x;
    precise float x = xs + c.screen.z;
    precise float yr = vy * rhw;
    precise float ys = yr * c.screen.y;
    precise float y = ys + c.screen.w;
    return float3(x, y, rhw);
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
    float3 s0 = water_project(camera, q.p[0]), s1 = water_project(camera, q.p[1]);
    float3 s2 = water_project(camera, q.p[2]), s3 = water_project(camera, q.p[3]);
    precise float ax = s2.x - s0.x;
    precise float ay = s1.y - s0.y;
    precise float bx = s1.x - s0.x;
    precise float by = s2.y - s0.y;
    precise float turnA = ax * ay;
    precise float turnB = bx * by;
    q.rhw[0] = s0.z;
    q.rhw[1] = s1.z;
    q.rhw[2] = s2.z;
    q.rhw[3] = s3.z;
    q.screen[0] = s0.xy;
    q.screen[1] = s1.xy;
    q.screen[2] = s2.xy;
    q.screen[3] = s3.xy;
    q.drawn = asfloat(c.cell.w) > heights.y && s0.z > 0.0 && s1.z > 0.0 && s2.z > 0.0 && s3.z > 0.0
              && turnA > turnB;
    return q;
}
