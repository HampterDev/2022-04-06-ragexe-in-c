// The frame's shadow cascades as the fit leaves them on the GPU (shadow_cascades.hlsl; struct
// ShadowFitGpuCascades in render/shadow_fit_gpu.h, the same layout: 16-byte vectors and row-major
// matrices), which
// the receivers (world.hlsl), the casters (world_caster.hlsl) and the casters' cull
// (world_cull.hlsl) read. The CPU never sees it.

#define CASCADES 4
#define MAP_SIZE 2048  // render/gfx.h, GFX_SHADOW_SIZE

struct Cascades
{
    float4x4 world[CASCADES];     // world to each cascade's light space, row vectors: x and y from -1
                                  // to 1 across its map, z from 0 to 1 along the light
    float4x4 fromView[CASCADES];  // the same from view space
    float4 farDepth;              // each cascade's far view depth, all 0 where nothing receives
    float4 offset;                // the lookup moved off a surface towards the light, view units
    float4 blur;                  // the soft edge's radius, texture units of the cascade's map
    float4 slant;                 // the light depth a texel spans per unit of a slant's tangent
    float4 square[CASCADES];      // across the light: the least x and y, then the greatest
    float4 kept;                  // each square's half size, for the next frame
    float4 previous;              // the frame before's, which the squares were kept from
    float4 range;                 // the range the slices were cut from (nearest, farthest), then the
                                  // frame before's, which it was kept from
    float4 planes[CASCADES * 6];  // each cascade's box in the world, inside where dot(n, p) + w >= 0
    float4 lightView;             // towards the light, view space; w: the bias, light depth
    float4 ambient;               // the ambient light; w: the darkness of a shadow (at least 1 / 255)
    uint4 count;                  // x: the cascades, CASCADES or 0 where nothing receives
};

// A point's texture coordinates in a cascade's map from its light space: the map's texel centres
// where the casters drew them (world_caster.hlsl), y down.
float2 map_uv(float2 light)
{
    return light * float2(0.5, -0.5) + (0.5 + 0.5 / MAP_SIZE);
}
