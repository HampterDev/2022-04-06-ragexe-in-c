// Where each slice's receivers lie across the light (render/shadow_fit.c, shadow_fit_extents): each
// group a tile of the screen (shadow_fit.hlsli, TILE), each of its threads reducing its pixels in
// its own registers, the group then reducing its threads' in shared memory and merging the result
// into the fit's state. The group cuts the slices once, from the range shadow_range found and the
// range the frame before kept (shadow_fit_keep); a pixel's receivers from front to back reach each
// slice from their first point in it to their last. The least and the greatest of integers are the
// same in any order, so the state comes out as the C's.
#include "shadow_fit.hlsli"

// the depth pass's two targets, read-only storage textures (SDL binds them as sampled images)
Texture2D<float> front : register(t0, space0);
Texture2D<float> back : register(t1, space0);
StructuredBuffer<float4> history : register(t2, space0);
RWStructuredBuffer<uint> state : register(u0, space1);

#define VALUES (4 * CASCADES)        // a thread's: per cascade the least x and y, then the greatest
#define PARTS (THREADS / VALUES)     // the threads' values are reduced a part of the threads at a time,
                                     // as many threads in a part as there are values
#define STRIDE (VALUES + 1)          // a thread's values apart in `values`: an odd number of words, so
                                     // that the threads writing their values meet in no bank of 32;
                                     // with 16 values those reducing them meet in none either
#if THREADS % VALUES != 0
#error "a group's threads are a whole number of parts, one thread of each for every value"
#endif

groupshared float cut[CASCADES + 1];
groupshared uint values[THREADS * STRIDE];
groupshared uint parts[THREADS];

// a pixel's point at depth d into the least and the greatest of a cascade, x and y
void extend(inout uint4 e, uint2 pixel, float d)
{
    uint2 p = uint2(order_float(pixel_across(pixelX, pixel.x, pixel.y, d)),
                    order_float(pixel_across(pixelY, pixel.x, pixel.y, d)));
    e = uint4(min(e.xy, p), max(e.zw, p));
}

// value v of two threads merged: the least or, for v & 2, the greatest
uint merge(uint v, uint a, uint b)
{
    return (v & 2) != 0 ? max(a, b) : min(a, b);
}

[numthreads(TILE_THREADS, TILE_THREADS, 1)]
void cs_main(uint3 group : SV_GroupID, uint3 thread : SV_GroupThreadID, uint index : SV_GroupIndex)
{
    float split[CASCADES + 1];
    uint4 e[CASCADES];
    uint c, i, j, k;
    if (index == 0) {
        frame_split(state[STATE_NEAREST], state[STATE_FARTHEST], history[1].xy, split);
        for (c = 0; c <= CASCADES; c++)
            cut[c] = split[c];
    }
    [unroll] for (c = 0; c < CASCADES; c++)
        e[c] = uint4(ORDERED_INFINITY, ORDERED_INFINITY, ORDERED_MINUS_INFINITY, ORDERED_MINUS_INFINITY);
    GroupMemoryBarrierWithGroupSync();
    for (c = 0; c <= CASCADES; c++)
        split[c] = cut[c];
    for (j = 0; j < TILE; j += TILE_THREADS)
        for (i = 0; i < TILE; i += TILE_THREADS) {
            uint2 p = group.xy * TILE + thread.xy + uint2(i, j);
            float lo, hi;
            if (p.x < size.x && p.y < size.y) {
                if (shown(front[p], back[p], lo, hi)) {
                    uint first = slice_of(lo, split), last = min(slice_of(hi, split), CASCADES - 1);
                    [unroll] for (c = 0; c < CASCADES; c++)
                        if (c >= first && c <= last) {
                            extend(e[c], p, c == first ? lo : split[c]);
                            extend(e[c], p, c == last ? hi : split[c + 1]);
                        }
                }
            }
        }
    // the group's: each value over each part of the threads, then over the parts
    [unroll] for (c = 0; c < CASCADES; c++)
        [unroll] for (k = 0; k < 4; k++)
            values[index * STRIDE + 4 * c + k] = e[c][k];
    GroupMemoryBarrierWithGroupSync();
    uint v = index % VALUES, part = index / VALUES, r = values[part * (THREADS / PARTS) * STRIDE + v];
    for (k = 1; k < THREADS / PARTS; k++)
        r = merge(v, r, values[(part * (THREADS / PARTS) + k) * STRIDE + v]);
    parts[index] = r;
    GroupMemoryBarrierWithGroupSync();
    if (index < VALUES) {
        for (k = 1; k < PARTS; k++)
            r = merge(v, r, parts[k * VALUES + v]);
        if ((v & 2) != 0) {
            if (r != ORDERED_MINUS_INFINITY)
                InterlockedMax(state[STATE_EXTENTS + v], r);
        } else if (r != ORDERED_INFINITY) {
            InterlockedMin(state[STATE_EXTENTS + v], r);
        }
    }
}
