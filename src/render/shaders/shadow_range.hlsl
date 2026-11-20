// The nearest and the farthest view depth that receives a shadow (render/shadow_fit.c,
// shadow_fit_range): each group a tile of the screen (shadow_fit.hlsli, TILE), each of its threads
// reducing its pixels in its own registers, the group then reducing its threads' and merging them
// into the fit's state. Depths are positive, so their bits order as they do.
#include "shadow_fit.hlsli"

// the depth pass's two targets, read-only storage textures (SDL binds them as sampled images)
Texture2D<float> front : register(t0, space0);
Texture2D<float> back : register(t1, space0);
RWStructuredBuffer<uint> state : register(u0, space1);

groupshared uint nearestBits;
groupshared uint farthestBits;

[numthreads(TILE_THREADS, TILE_THREADS, 1)]
void cs_main(uint3 group : SV_GroupID, uint3 thread : SV_GroupThreadID, uint index : SV_GroupIndex)
{
    uint nearest = 0x7f800000, farthest = 0, i, j;
    if (index == 0) {
        nearestBits = nearest;
        farthestBits = farthest;
    }
    for (j = 0; j < TILE; j += TILE_THREADS)
        for (i = 0; i < TILE; i += TILE_THREADS) {
            uint2 p = group.xy * TILE + thread.xy + uint2(i, j);
            float lo, hi;
            if (p.x < size.x && p.y < size.y) {
                if (shown(front[p], back[p], lo, hi)) {
                    nearest = min(nearest, asuint(lo));
                    farthest = max(farthest, asuint(hi));
                }
            }
        }
    GroupMemoryBarrierWithGroupSync();
    if (nearest <= farthest) {
        InterlockedMin(nearestBits, nearest);
        InterlockedMax(farthestBits, farthest);
    }
    GroupMemoryBarrierWithGroupSync();
    if (index == 0 && nearestBits <= farthestBits) {
        InterlockedMin(state[STATE_NEAREST], nearestBits);
        InterlockedMax(state[STATE_FARTHEST], farthestBits);
    }
}
