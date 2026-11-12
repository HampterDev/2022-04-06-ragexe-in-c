// Where each slice's pixels lie across the light (render/shadow_fit.c, shadow_fit_extents): each
// group of 16 by 16 pixels reduces its own, then merges them into the fit's state. Every thread
// cuts the range shadow_range found into the same slices.
#include "shadow_fit.hlsli"

StructuredBuffer<float> depths : register(t0, space0);
RWStructuredBuffer<uint> state : register(u0, space1);

groupshared uint extent[4 * CASCADES];

[numthreads(16, 16, 1)]
void cs_main(uint3 id : SV_DispatchThreadID, uint index : SV_GroupIndex)
{
    float split[CASCADES + 1];
    split_range(asfloat(state[STATE_NEAREST]), asfloat(state[STATE_FARTHEST]), split);
    if (index < 4 * CASCADES)
        extent[index] = (index & 2) != 0 ? ORDERED_MINUS_INFINITY : ORDERED_INFINITY;
    GroupMemoryBarrierWithGroupSync();
    if (id.x < size.x && id.y < size.y) {
        float d = depths[id.y * size.x + id.x];
        uint c = slice_of(d, split);
        if (receives(d) && c < CASCADES) {
            uint x = order_float(pixel_across(pixelX, id.x, id.y, d));
            uint y = order_float(pixel_across(pixelY, id.x, id.y, d));
            InterlockedMin(extent[4 * c], x);
            InterlockedMin(extent[4 * c + 1], y);
            InterlockedMax(extent[4 * c + 2], x);
            InterlockedMax(extent[4 * c + 3], y);
        }
    }
    GroupMemoryBarrierWithGroupSync();
    if (index < 4 * CASCADES) {
        if ((index & 2) != 0)
            InterlockedMax(state[STATE_EXTENTS + index], extent[index]);
        else
            InterlockedMin(state[STATE_EXTENTS + index], extent[index]);
    }
}
