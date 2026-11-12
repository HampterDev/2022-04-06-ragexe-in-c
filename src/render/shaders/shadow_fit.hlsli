// The frame's shadow cascades fitted to the receivers' view depths, in three compute passes
// (shadow_range, shadow_extents, shadow_cascades), each the step of render/shadow_fit.c of the
// same name, which tests/test_shadow_fit_gpu.c checks them against. Every float the fit computes
// is precise, so that no multiply and add are fused, as in the C.
//
// The fit's state, a buffer of uints the CPU resets each frame: the nearest and the farthest
// depth (their bits; both positive), then per cascade the least and the greatest of its pixels
// across the light, x and y each, as ordered (order_float) so that uint atomics take them.

#define CASCADES 4
#define MAP_SIZE 2048
#define SPLIT 0.7
#define SOFTNESS 0.5
#define OFFSET 1.5

#define STATE_NEAREST 0
#define STATE_FARTHEST 1
#define STATE_EXTENTS 2   // cascade c: STATE_EXTENTS + 4c + 0, 1 the least x, y; + 2, 3 the greatest
#define STATE_SIZE (STATE_EXTENTS + 4 * CASCADES)

// The frame's constants (render/shadow_fit.h, ShadowFitFrame).
cbuffer Frame : register(b0, space2)
{
    float4 pixelX;     // ShadowFitFrame.pixel[0] and [1]
    float4 pixelY;
    float4 screen;     // the centre's x and y, the near plane, the reach
    uint4 size;        // the depth buffer's width and height
    float4 axisX;      // the light's axes across it, ShadowFitFrame.axis[0] and [1]
    float4 axisY;
    float4 alongLight; // the light's axis over its depth range, -lightNear over it
    float4 lightRange; // x: ShadowFitFrame.lightRange
    float4 inverse[3]; // ShadowFitFrame.inverse, 12 floats
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

bool receives(float d)
{
    return d >= screen.z && d <= screen.w;
}

// shadow_fit_split: the far depth of each cascade's slice, split[0] the nearest
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
    power[0] = 1.0;
    power[4] = last / nearest;
    power[2] = sqrt(power[4]);
    power[1] = sqrt(power[2]);
    power[3] = power[1] * power[2];
    for (c = 0; c <= CASCADES; c++) {
        precise float t = (float)c / CASCADES;
        precise float even = nearest + (last - nearest) * t;
        precise float value = (float)SPLIT * nearest * power[c] + (1.0 - (float)SPLIT) * even;
        split[c] = value;
    }
    split[0] = nearest;
    split[CASCADES] = last;
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
