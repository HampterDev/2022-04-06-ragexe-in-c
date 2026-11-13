// The frame's shadow cascades fitted to the receivers' view depths, in three compute passes
// (shadow_range, shadow_extents, shadow_cascades), each the step of render/shadow_fit.c of the
// same name, which tests/test_shadow_fit_gpu.c checks them against (render/shadow_fit_gpu.h packs
// and reads what they take and leave).
//
// The receivers' depths are two textures the depth pass draws (world_depth.hlsl): front, the
// nearest receiver a pixel shows, and back, the nearest one that is not blended; 0 where none.
// The fit's state, a buffer of uints the CPU resets each frame: the nearest and the farthest
// depth (their bits; both positive), then per cascade the least and the greatest of its receivers
// across the light, x and y each, as ordered (order_float) so that uint atomics take them. Its
// history, which the CPU clears with the map: the squares' half sizes, then the range the frame
// before cut its slices from.
#include "cascades.hlsli"

#define SOFTNESS 0.5
#define OFFSET 1.5
// the range kept and the squares kept (render/shadow_fit.c, shadow_fit_keep and
// shadow_fit_cascades)
#define FAR_KEEP 1.1
#define FAR_AHEAD 1.01
#define NEAR_ABOVE 1.25
#define SQUARE_AHEAD 1.05
#define SQUARE_KEEP 1.2

#define STATE_NEAREST 0
#define STATE_FARTHEST 1
#define STATE_EXTENTS 2   // cascade c: STATE_EXTENTS + 4c + 0, 1 the least x, y; + 2, 3 the greatest
#define STATE_SIZE (STATE_EXTENTS + 4 * CASCADES)

// The frame's constants (render/shadow_fit_gpu.h, ShadowFitGpuFrame).
cbuffer Frame : register(b0, space2)
{
    float4 pixelX;     // ShadowFitFrame.pixel[0] and [1]
    float4 pixelY;
    float4 screen;     // the centre's x and y, the screen's width and height in pixels
    float4 depths;     // the near plane, the reach, the light's near depth and its range
    uint4 size;        // the depth textures' width and height in pixels
    float4 axisX;      // the light's axes, ShadowFitFrame.axis[0], [1] and [2]
    float4 axisY;
    float4 axisZ;
    float4 inverse[3]; // ShadowFitFrame.inverse, 12 floats
    float4 lightView;  // the receivers' constants, which shadow_cascades.hlsl hands on (Cascades)
    float4 ambient;
};

// a float as a uint that orders as the float does; infinity and minus infinity so
#define ORDERED_INFINITY 0xff800000
#define ORDERED_MINUS_INFINITY 0x007fffff

uint order_float(float x)
{
    uint u = asuint(x);
    return (u & 0x80000000) != 0 ? ~u : u | 0x80000000;
}

float unorder_float(uint u)
{
    return asfloat((u & 0x80000000) != 0 ? u & 0x7fffffff : ~u);
}

// What a pixel shows that receives, from lo to hi: its receivers from front to back, within the
// near plane and the reach (shadow_fit.c, shown).
bool shown(float front, float back, out float lo, out float hi)
{
    lo = max(front, depths.x);
    hi = min(back > front ? back : front, depths.y);
    return front > 0.0 && lo <= hi;
}

// shadow_fit_keep: the range the frame cuts its slices from, from the one it measured and the one
// the frame before kept; where nothing receives, the frame before's
float2 keep_range(float nearest, float farthest, float2 previous)
{
    precise float kn = previous.x, kf = previous.y;
    if (!(nearest <= farthest && nearest > 0.0))
        return previous;
    if (!(kf >= farthest && kf <= farthest * FAR_KEEP && kf <= depths.y)) {
        kf = depths.y <= farthest * FAR_KEEP ? depths.y : farthest * FAR_AHEAD;
        kf = kf > farthest ? kf : farthest;
    }
    if (!(nearest >= kn && nearest <= kn * NEAR_ABOVE))
        kn = nearest;
    return float2(kn, kf);
}

// shadow_fit_split: the far depth of each cascade's slice, split[0] the nearest; geometric
void split_range(float nearest, float farthest, out float split[CASCADES + 1])
{
    precise float power[CASCADES + 1];
    precise float last = farthest + 1.0;
    int c;
    if (!(nearest <= farthest && nearest > 0.0)) {
        for (c = 0; c <= CASCADES; c++)
            split[c] = 0.0;
        return;
    }
    power[4] = last / nearest;
    power[2] = sqrt(power[4]);
    power[1] = sqrt(power[2]);
    power[3] = power[1] * power[2];
    split[0] = nearest;
    for (c = 1; c < CASCADES; c++) {
        precise float value = nearest * power[c];
        split[c] = value;
    }
    split[CASCADES] = last;
}

// the frame's split, from the state the range pass left and the range the frame before kept
void frame_split(uint nearestBits, uint farthestBits, float2 previous, out float split[CASCADES + 1])
{
    float2 kept = keep_range(asfloat(nearestBits), asfloat(farthestBits), previous);
    split_range(kept.x, kept.y, split);
}

// the slice of a depth: the first whose far depth lies beyond it, CASCADES for none
uint slice_of(float d, float split[CASCADES + 1])
{
    uint c = 0;
    while (c < CASCADES && !(d < split[c + 1]))
        c++;
    return c;
}

// where the pixel (i, j) at depth d lies across the light, a zero made +0 (shadow_fit.c,
// pixel_across)
float pixel_across(float4 k, uint i, uint j, float d)
{
    precise float t = ((float)i - screen.x) * k.x + ((float)j - screen.y) * k.y + k.z;
    precise float x = k.w + d * t;
    return x == 0.0 ? 0.0 : x;
}
