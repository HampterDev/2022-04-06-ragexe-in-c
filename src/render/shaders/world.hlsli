// The static world's data on the device (render/gpu_world.c; render/static_world.h):
// the map's draw records by id, the transform slots of its model nodes, and what the frame's
// compute passes make of them. Only 16-byte vectors and row-major matrices, as records.hlsli.

// A draw record by its id (struct GpuWorldRecord), which every vertex of the record carries:
// x the texture's layer and, above, the lightmap's; y the transform slot and, above, the
// texture-coordinate matrix (WORLD_NO_UV: none); z the pass, the flags (render/static_world.h,
// SW_*) above, then the texture's array and the lightmap's (WORLD_NO_ARRAY: none).
struct WorldRecord
{
    uint4 data;
};

#define WORLD_NO_UV 0xffff
#define WORLD_NO_ARRAY 0xff
#define WORLD_PASS_GROUND 2  // render/static_world.h, SW_PASS_GROUND

uint record_slot(WorldRecord r) { return r.data.y & 0xffff; }
uint record_uv(WorldRecord r) { return r.data.y >> 16; }
uint record_pass(WorldRecord r) { return r.data.z & 0xff; }
bool record_textured(WorldRecord r) { return ((r.data.z >> 16) & 0xff) != WORLD_NO_ARRAY; }
bool record_lightmapped(WorldRecord r) { return (r.data.z >> 24) != WORLD_NO_ARRAY; }

// A transform slot's views for the frame, which world_transform.hlsl writes: its space to clip space
// and to view space.
struct WorldView
{
    float4x4 toClip;
    float4x4 toView;
};

// The texture-coordinate matrix of a uv slot, two per slot (render/gfx.h, GfxWorldFrame.uv):
// u' = dot(float3(u, v, 1), first.xyz), v' = dot(float3(u, v, 1), second.xyz)
float2 world_turn(float2 uv, float4 u, float4 v)
{
    return float2(dot(float3(uv, 1), u.xyz), dot(float3(uv, 1), v.xyz));
}
