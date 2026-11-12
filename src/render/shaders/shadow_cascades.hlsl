// Each cascade's square, matrices and lookups (render/shadow_fit.c, shadow_fit_cascades), one
// thread a cascade, from the fit's state; written as render/shadow_fit.h's ShadowFitCascades.
#include "shadow_fit.hlsli"

struct Cascades
{
    float4x4 world[CASCADES];     // world to light, row vectors
    float4x4 fromView[CASCADES];  // view to light
    float4 farDepth;
    float4 offset;
    float4 blur;
    float4 slant;
    float4 square[CASCADES];      // across the light: the least x and y, then the greatest
    uint4 count;                  // x: the cascades, CASCADES or 0
};

StructuredBuffer<uint> state : register(t0, space0);
RWStructuredBuffer<Cascades> result : register(u0, space1);

bool slice_used(uint c)
{
    uint at = STATE_EXTENTS + 4 * c;
    return unorder_float(state[at]) <= unorder_float(state[at + 2])
           && unorder_float(state[at + 1]) <= unorder_float(state[at + 3]);
}

[numthreads(CASCADES, 1, 1)]
void cs_main(uint3 id : SV_DispatchThreadID)
{
    uint c = id.x, at = STATE_EXTENTS + 4 * c, i, j, k;
    float split[CASCADES + 1];
    precise float lo[2], hi[2], square[2], radius = 8.0, texel, n[2], w[4][4], wide;
    float inv[12];
    bool used = slice_used(c);

    split_range(asfloat(state[STATE_NEAREST]), asfloat(state[STATE_FARTHEST]), split);
    for (i = 0; i < 2; i++) {
        lo[i] = unorder_float(state[at + i]);
        hi[i] = unorder_float(state[at + 2 + i]);
        square[i] = 0.0;
    }
    // a margin for the soft edge and the offset, then up to a multiple of 8 units
    if (used) {
        for (i = 0; i < 2; i++)
            square[i] = (lo[i] + hi[i]) * 0.5;
        wide = hi[0] - lo[0] > hi[1] - lo[1] ? hi[0] - lo[0] : hi[1] - lo[1];
        radius = wide * 0.5;
        radius = ceil((radius * 1.05 + 4.0 + SOFTNESS) / 8.0) * 8.0;
    }
    // the centre on whole texels: n texels, which are radius / 1024 wide
    texel = 2.0 * radius / MAP_SIZE;
    for (i = 0; i < 2; i++) {
        n[i] = floor(square[i] / texel + 0.5);
        square[i] = n[i] * texel;
    }
    result[0].square[c] = used ? float4(square[0] - radius, square[1] - radius, square[0] + radius, square[1] + radius)
                               : float4(1e30, 1e30, -1e30, -1e30);
    for (i = 0; i < 3; i++) {
        w[i][0] = axisX[i] / radius;
        w[i][1] = axisY[i] / radius;
        w[i][2] = alongLight[i];
        w[i][3] = 0.0;
    }
    w[3][0] = -n[0] * (1.0 / 1024);
    w[3][1] = -n[1] * (1.0 / 1024);
    w[3][2] = alongLight.w;
    w[3][3] = 1.0;
    for (i = 0; i < 4; i++)
        result[0].world[c][i] = float4(w[i][0], w[i][1], w[i][2], w[i][3]);
    // view to world (the inverse view, a 4x4 with the last column 0 0 0 1), then to light
    for (i = 0; i < 12; i++)
        inv[i] = inverse[i / 4][i % 4];
    for (i = 0; i < 4; i++) {
        precise float row[4];
        for (j = 0; j < 4; j++) {
            precise float sum = i == 3 ? w[3][j] : 0.0;
            for (k = 0; k < 3; k++)
                sum += inv[i * 3 + k] * w[k][j];
            row[j] = sum;
        }
        result[0].fromView[c][i] = float4(row[0], row[1], row[2], row[3]);
    }
    result[0].farDepth[c] = split[c + 1];
    result[0].offset[c] = OFFSET * texel;
    result[0].slant[c] = (SOFTNESS + texel) / lightRange.x;
    result[0].blur[c] = SOFTNESS / (2.0 * radius);
    if (c == 0)
        result[0].count = uint4(slice_used(0) || slice_used(1) || slice_used(2) || slice_used(3) ? CASCADES : 0, 0, 0, 0);
}
