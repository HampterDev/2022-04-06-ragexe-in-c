// The nearest and the farthest view depth that receives a shadow (render/shadow_fit.c,
// shadow_fit_range): each group of 16 by 16 pixels reduces its own, then merges them into the
// fit's state. Depths are positive, so their bits order as they do.
#include "shadow_fit.hlsli"

StructuredBuffer<float> depths : register(t0, space0);
RWStructuredBuffer<uint> state : register(u0, space1);

groupshared uint nearestBits;
groupshared uint farthestBits;

[numthreads(16, 16, 1)]
void cs_main(uint3 id : SV_DispatchThreadID, uint index : SV_GroupIndex)
{
    if (index == 0) {
        nearestBits = 0x7f800000;
        farthestBits = 0;
    }
    GroupMemoryBarrierWithGroupSync();
    if (id.x < size.x && id.y < size.y) {
        float d = depths[id.y * size.x + id.x];
        if (receives(d)) {
            InterlockedMin(nearestBits, asuint(d));
            InterlockedMax(farthestBits, asuint(d));
        }
    }
    GroupMemoryBarrierWithGroupSync();
    if (index == 0) {
        InterlockedMin(state[STATE_NEAREST], nearestBits);
        InterlockedMax(state[STATE_FARTHEST], farthestBits);
    }
}
