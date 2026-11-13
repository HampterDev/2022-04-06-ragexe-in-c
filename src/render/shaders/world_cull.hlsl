// The cull of the static world's draw records for the frame's views, run by run
// (render/world_layout.h): the camera's view by its frustum (render/static_world.c,
// sw_record_in_view, to the bit, exact.hlsli), the shadow cascades' by the boxes the shadow fit
// left on the GPU (cascades.hlsli, the rule of sw_cull_cascade). A group of 64 threads takes one
// run's records of one view in their order, keeps those whose bounds, an animated record's moved by
// its slot's place in the world (world_transform.hlsl), are not wholly outside one of the view's
// planes, and gives each kept record its place in the view's section of the frame's index buffer,
// one after the other in the run's order, and a culled one none; then writes the run's one indexed
// indirect draw command of them (SDL's indirect draws take no count the GPU writes, so a run is one
// command, with no indices where it keeps none). world_compact.hlsl then copies the kept records'
// indices to their places, a group to each 64 of them, where this group would copy a long run's one
// chunk after the other.
#include "exact.hlsli"
#include "world.hlsli"
#include "cascades.hlsli"

#define SW_ANIMATED 0x04  // render/static_world.h
#define GROUP 64
#define CULLED 0xffffffff
#define NO_ORDER 0xffffffff
#define RECORD_WORDS 12   // render/world_layout.h, WlCullRecord: the centre's bits, slot | flags << 16;
                          // the half size's bits, the first index; the index count, the base vertex

StructuredBuffer<uint> cullData : register(t0, space0);  // render/world_layout.h, WlCullLayout
StructuredBuffer<Matrix43> placed : register(t1, space0);
StructuredBuffer<Cascades> fit : register(t2, space0);
RWStructuredBuffer<uint> draws : register(u0, space1);   // WlDrawLayout: the commands, then the places

cbuffer Frame : register(b0, space2)
{
    float4 planes[6];  // the camera's view (render/static_world.h, SwFrustum)
    uint4 counts;      // the runs of a view, the dispatch's groups across (wl_groups), the views, and
                       // whether they are the cascades
    uint4 layout;      // where the runs lie in cullData, where the order does (NO_ORDER: draw order),
                       // the first command, where the places lie in draws
    uint4 sections;    // the first view's section, the indices a section holds, the records
};

// static_world.c, place_point and world_box
float3 place_point(Matrix43 m, float3 p)
{
    float3 moved;
    [unroll] for (uint k = 0; k < 3; k++) {
        precise float a = p.x * m43(m, k);
        precise float b = p.y * m43(m, 3 + k);
        precise float c = p.z * m43(m, 6 + k);
        precise float ab = a + b;
        precise float abc = ab + c;
        precise float r = abc + m43(m, 9 + k);
        moved[k] = r;
    }
    return moved;
}

float3 moved_half(Matrix43 m, float3 h)
{
    float3 moved;
    [unroll] for (uint k = 0; k < 3; k++) {
        precise float a = abs(m43(m, k)) * h.x;
        precise float b = abs(m43(m, 3 + k)) * h.y;
        precise float c = abs(m43(m, 6 + k)) * h.z;
        precise float ab = a + b;
        precise float r = ab + c;
        moved[k] = r;
    }
    return moved;
}

// the view's plane i: the camera's, or cascade v's box
float4 plane(uint v, uint i)
{
    return counts.w != 0 ? fit[0].planes[v * 6 + i] : planes[i];
}

// static_world.c, outside_frustum
bool outside(float3 c, float3 h, uint v)
{
    [unroll] for (uint i = 0; i < 6; i++) {
        float4 n = plane(v, i);
        precise float r0 = h.x * n.x;
        precise float r1 = n.y * h.y;
        precise float r2 = h.z * n.z;
        precise float ra = abs(r0) + abs(r1);
        precise float reach = ra + abs(r2);
        precise float d0 = c.x * n.x;
        precise float d1 = n.y * c.y;
        precise float d2 = c.z * n.z;
        precise float d01 = d0 + d1;
        precise float d012 = d01 + d2;
        precise float dist = d012 + n.w;
        precise float below = -reach;
        if (dist > reach)
            continue;
        if (!(dist > below))
            return true;
    }
    return false;
}

// the indices view v keeps of the record at `at` in draw order: all of them, or none
uint kept(uint at, uint v)
{
    uint base = at * RECORD_WORDS;
    float3 centre = asfloat(uint3(cullData[base], cullData[base + 1], cullData[base + 2]));
    float3 half = asfloat(uint3(cullData[base + 4], cullData[base + 5], cullData[base + 6]));
    uint slot = cullData[base + 3] & 0xffff, flags = cullData[base + 3] >> 16;
    if ((flags & SW_ANIMATED) != 0) {
        Matrix43 m = placed[slot];
        centre = place_point(m, centre);
        half = moved_half(m, half);
    }
    return outside(centre, half, v) ? 0 : cullData[base + 8];
}

groupshared uint ends[GROUP];  // the indices records 0 to i of the chunk keep

[numthreads(GROUP, 1, 1)]
void cs_main(uint3 group : SV_GroupID, uint3 thread : SV_GroupThreadID)
{
    uint lane = thread.x;
    uint index = group.y * counts.y + group.x;
    if (index >= counts.x * counts.z)
        return;
    uint v = index / counts.x, r = index % counts.x, at = layout.x + r * 4;
    uint first = cullData[at], count = cullData[at + 1];
    uint section = sections.x + v, start = section * sections.y + cullData[at + 2];
    uint places = layout.w + section * sections.z;
    uint total = 0;
    for (uint base = 0; base < count; base += GROUP) {
        uint n = 0, record = 0;
        if (base + lane < count) {
            record = layout.y == NO_ORDER ? first + base + lane : cullData[layout.y + first + base + lane];
            n = kept(record, v);
        }
        ends[lane] = n;
        GroupMemoryBarrierWithGroupSync();
        // the chunk's running sums (Hillis and Steele)
        for (uint stride = 1; stride < GROUP; stride <<= 1) {
            uint add = lane >= stride ? ends[lane - stride] : 0;
            GroupMemoryBarrierWithGroupSync();
            ends[lane] += add;
            GroupMemoryBarrierWithGroupSync();
        }
        if (base + lane < count)
            draws[places + record] = n ? start + total + ends[lane] - n : CULLED;
        total += ends[GROUP - 1];
        GroupMemoryBarrierWithGroupSync();
    }
    if (lane == 0) {
        uint command = (layout.z + v * counts.x + r) * 5;
        draws[command] = total;
        draws[command + 1] = 1;
        draws[command + 2] = start;
        draws[command + 3] = 0;
        draws[command + 4] = 0;
    }
}
