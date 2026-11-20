// Each cascade's square, matrices and lookups (render/shadow_fit.c, shadow_fit_keep and
// shadow_fit_cascades), one thread a cascade, from the fit's state, and the squares and the range
// the frame before kept (`history`, which the CPU clears with the map): written as cascades.hlsli's
// Cascades, with each cascade's box in the world for the casters' cull and the receivers'
// constants of the frame, and the history kept for the next frame.
#include "shadow_fit.hlsli"

StructuredBuffer<uint> state : register(t0, space0);
RWStructuredBuffer<float4> history : register(u0, space1);
RWStructuredBuffer<Cascades> result : register(u1, space1);

bool slice_used(uint c)
{
    uint at = STATE_EXTENTS + 4 * c;
    return unorder_float(state[at]) <= unorder_float(state[at + 2])
           && unorder_float(state[at + 1]) <= unorder_float(state[at + 3]);
}

// half a square that holds receivers spread over `wide` units, with a margin for the soft edge and
// the lookup's offset (shadow_fit.c, square_need)
float square_need(float wide)
{
    precise float need = wide * 0.5 * 1.05 + 4.0 + SOFTNESS;
    return need;
}

// the wider of the two extents across the light of the view from depth d0 to d1, its corners'
// (shadow_fit.c, view_wide)
float view_wide(float d0, float d1)
{
    precise float lo[2] = { asfloat(0x7f800000), asfloat(0x7f800000) };
    precise float hi[2] = { asfloat(0xff800000), asfloat(0xff800000) };
    uint k;
    for (k = 0; k < 8; k++) {
        uint i = (k & 1) != 0 ? (uint)screen.z - 1 : 0, j = (k & 2) != 0 ? (uint)screen.w - 1 : 0;
        float d = (k & 4) != 0 ? d1 : d0;
        float x = pixel_across(pixelX, i, j, d), y = pixel_across(pixelY, i, j, d);
        lo[0] = x < lo[0] ? x : lo[0];
        hi[0] = x > hi[0] ? x : hi[0];
        lo[1] = y < lo[1] ? y : lo[1];
        hi[1] = y > hi[1] ? y : hi[1];
    }
    precise float wide = hi[0] - lo[0] > hi[1] - lo[1] ? hi[0] - lo[0] : hi[1] - lo[1];
    return wide;
}

// the whole number of texels nearest x, ties to the greater, whichever way the division rounds:
// corrected by products that are exact while |x| < 2^16 units (shadow_fit.c, whole_texels)
float whole_texels(float x, float texel)
{
    precise float n = floor(x / texel + 0.5);
    precise float below = (n - 0.5) * texel, above = (n + 0.5) * texel;
    if (below > x)
        n -= 1.0;
    else if (above <= x)
        n += 1.0;
    return n;
}

[numthreads(CASCADES, 1, 1)]
void cs_main(uint3 id : SV_DispatchThreadID)
{
    uint c = id.x, at = STATE_EXTENTS + 4 * c, i, j, k;
    float split[CASCADES + 1];
    precise float lo[2], hi[2], square[2], radius = 8.0, texel, n[2], w[4][4], wide, need, base, most, keep;
    float inv[12], previous = history[0][c];
    float2 range = history[1].xy, kept = keep_range(asfloat(state[STATE_NEAREST]), asfloat(state[STATE_FARTHEST]), range);
    bool used = slice_used(c);
    // every thread has read the history before the first writes the next frame's into it
    AllMemoryBarrierWithGroupSync();

    split_range(kept.x, kept.y, split);
    for (i = 0; i < 2; i++) {
        lo[i] = unorder_float(state[at + i]);
        hi[i] = unorder_float(state[at + 2 + i]);
        square[i] = 0.0;
    }
    // kept while it holds the receivers and is at most a fifth larger than they need or no larger
    // than its slice of the view; otherwise, up to a multiple of 8 units and no larger than its
    // slice of the view, a fifth larger than they need where they outgrew it, 5 percent otherwise
    if (used) {
        for (i = 0; i < 2; i++)
            square[i] = (lo[i] + hi[i]) * 0.5;
        wide = hi[0] - lo[0] > hi[1] - lo[1] ? hi[0] - lo[0] : hi[1] - lo[1];
        need = square_need(wide);
        base = ceil(need / 8.0) * 8.0;
        most = ceil(square_need(view_wide(c != 0 ? split[c] : depths.x, split[c + 1])) / 8.0) * 8.0;
        keep = base * SQUARE_KEEP > most ? base * SQUARE_KEEP : most;
        if (previous >= base && previous <= keep) {
            radius = previous;
        } else {
            radius = ceil(need * (previous > 0.0 && previous < base ? SQUARE_KEEP : SQUARE_AHEAD) / 8.0) * 8.0;
            radius = radius < most ? radius : most;
            radius = radius > base ? radius : base;
        }
    }
    // the centre on whole texels: n texels, which are radius / 1024 wide
    texel = 2.0 * radius / MAP_SIZE;
    for (i = 0; i < 2; i++) {
        n[i] = whole_texels(square[i], texel);
        square[i] = n[i] * texel;
    }
    float4 box = used ? float4(square[0] - radius, square[1] - radius, square[0] + radius, square[1] + radius)
                      : float4(1e30, 1e30, -1e30, -1e30);
    result[0].square[c] = box;
    result[0].kept[c] = used ? radius : previous;
    result[0].previous[c] = previous;
    // the box in the world: within the square across the light, within the light's depth along it
    result[0].planes[c * 6] = float4(axisX.xyz, -box.x);
    result[0].planes[c * 6 + 1] = float4(-axisX.xyz, box.z);
    result[0].planes[c * 6 + 2] = float4(axisY.xyz, -box.y);
    result[0].planes[c * 6 + 3] = float4(-axisY.xyz, box.w);
    result[0].planes[c * 6 + 4] = float4(axisZ.xyz, -depths.z);
    result[0].planes[c * 6 + 5] = float4(-axisZ.xyz, depths.z + depths.w);
    for (i = 0; i < 3; i++) {
        w[i][0] = axisX[i] / radius;
        w[i][1] = axisY[i] / radius;
        w[i][2] = axisZ[i] / depths.w;
        w[i][3] = 0.0;
    }
    w[3][0] = -n[0] * (1.0 / 1024);
    w[3][1] = -n[1] * (1.0 / 1024);
    w[3][2] = -depths.z / depths.w;
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
    result[0].slant[c] = (SOFTNESS + texel) / depths.w;
    result[0].blur[c] = SOFTNESS / (2.0 * radius);
    history[0][c] = used ? radius : previous;
    if (c == 0) {
        result[0].range = float4(kept, range);
        result[0].lightView = lightView;
        result[0].ambient = ambient;
        result[0].count = uint4(slice_used(0) || slice_used(1) || slice_used(2) || slice_used(3) ? CASCADES : 0, 0, 0, 0);
        history[1] = float4(kept, 0.0, 0.0);
    }
}
