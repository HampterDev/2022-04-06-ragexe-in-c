// The nearest and the farthest view depth that receives a shadow (render/shadow_fit.c,
// shadow_fit_range): each group of 8 by 8 pixels (within the 128 invocations every Vulkan device
// takes) reduces its own, then merges them into the fit's state. Depths are positive, so their
// bits order as they do.
#include "shadow_fit.hlsli"

// the depth pass's two targets, read-only storage textures (SDL binds them as sampled images)
Texture2D<float> front : register(t0, space0);
Texture2D<float> back : register(t1, space0);
RWStructuredBuffer<uint> state : register(u0, space1);

groupshared uint nearestBits;
groupshared uint farthestBits;

[numthreads(8, 8, 1)]
void cs_main(uint3 id : SV_DispatchThreadID, uint index : SV_GroupIndex)
{
    if (index == 0) {
        nearestBits = 0x7f800000;
        farthestBits = 0;
    }
    GroupMemoryBarrierWithGroupSync();
    if (id.x < size.x && id.y < size.y) {
        float lo, hi;
        if (shown(front[id.xy], back[id.xy], lo, hi)) {
            InterlockedMin(nearestBits, asuint(lo));
            InterlockedMax(farthestBits, asuint(hi));
        }
    }
    GroupMemoryBarrierWithGroupSync();
    if (index == 0) {
        InterlockedMin(state[STATE_NEAREST], nearestBits);
        InterlockedMax(state[STATE_FARTHEST], farthestBits);
    }
}
