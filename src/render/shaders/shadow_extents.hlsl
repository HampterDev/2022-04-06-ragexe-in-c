// Where each slice's receivers lie across the light (render/shadow_fit.c, shadow_fit_extents):
// each group of 8 by 8 pixels (within the 128 invocations every Vulkan device takes) reduces its
// own, then merges them into the fit's state. Every thread cuts the same slices, from the range
// shadow_range found and the range the frame before kept (shadow_fit_keep); a pixel's receivers
// from front to back reach each slice from their first point in it to their last.
#include "shadow_fit.hlsli"

// the depth pass's two targets, read-only storage textures (SDL binds them as sampled images)
Texture2D<float> front : register(t0, space0);
Texture2D<float> back : register(t1, space0);
StructuredBuffer<float4> history : register(t2, space0);
RWStructuredBuffer<uint> state : register(u0, space1);

groupshared uint extent[4 * CASCADES];

void extend(uint c, uint2 pixel, float d)
{
    uint x = order_float(pixel_across(pixelX, pixel.x, pixel.y, d));
    uint y = order_float(pixel_across(pixelY, pixel.x, pixel.y, d));
    InterlockedMin(extent[4 * c], x);
    InterlockedMin(extent[4 * c + 1], y);
    InterlockedMax(extent[4 * c + 2], x);
    InterlockedMax(extent[4 * c + 3], y);
}

[numthreads(8, 8, 1)]
void cs_main(uint3 id : SV_DispatchThreadID, uint index : SV_GroupIndex)
{
    float split[CASCADES + 1];
    frame_split(state[STATE_NEAREST], state[STATE_FARTHEST], history[1].xy, split);
    if (index < 4 * CASCADES)
        extent[index] = (index & 2) != 0 ? ORDERED_MINUS_INFINITY : ORDERED_INFINITY;
    GroupMemoryBarrierWithGroupSync();
    if (id.x < size.x && id.y < size.y) {
        float lo, hi;
        if (shown(front[id.xy], back[id.xy], lo, hi)) {
            uint first = slice_of(lo, split), last = min(slice_of(hi, split), CASCADES - 1);
            for (uint c = first; c <= last; c++) {
                extend(c, id.xy, c == first ? lo : split[c]);
                extend(c, id.xy, c == last ? hi : split[c + 1]);
            }
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
