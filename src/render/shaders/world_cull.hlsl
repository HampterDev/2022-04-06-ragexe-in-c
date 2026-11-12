// The view's cull of the static world's draw records (render/static_world.c, sw_record_in_view), to
// the bit (exact.hlsli), run by run (render/world_layout.h, WlRun): a group of 64 threads takes one
// run's records in draw order, keeps those whose bounds, an animated record's moved by its slot's
// place in the world (world_transform.hlsl), are not wholly outside one of the view's planes, and
// gives each kept record its place in the run's part of the frame's index buffer, one after the
// other in the records' order, and a culled one none; then writes the run's one indexed indirect
// draw command of them (SDL's indirect draws take no count the GPU writes, so a run is one command,
// with no indices where it keeps none). world_compact.hlsl then copies the kept records' indices
// to their places, a group to each 64 of them, where this group would copy a long run's one chunk
// after the other.
#include "exact.hlsli"
#include "world.hlsli"

#define SW_ANIMATED 0x04  // render/static_world.h
#define GROUP 64
#define CULLED 0xffffffff

// A record in draw order (render/world_layout.h, WlCullRecord).
struct CullRecord
{
    uint4 centre;   // xyz: the centre's bits, w: slot | flags << 16
    uint4 half;     // xyz: the half size's bits, w: the first index
    uint4 draw;     // x: the index count, y: the base vertex
};

// A run (render/world_layout.h, WlRun): its records, where its indices go, and how many at most.
struct Run
{
    uint first;
    uint count;
    uint start;
    uint indices;
};

StructuredBuffer<CullRecord> cullRecords : register(t0, space0);
StructuredBuffer<Matrix43> placed : register(t1, space0);
StructuredBuffer<Run> runs : register(t2, space0);
RWStructuredBuffer<uint> places : register(u0, space1);    // by record: where its kept indices go, or CULLED
RWStructuredBuffer<uint> commands : register(u1, space1);  // SDL_GPUIndexedIndirectDrawCommand, 5 a run

cbuffer Frame : register(b0, space2)
{
    float4 planes[6];  // render/static_world.h, SwFrustum
    uint4 counts;      // x: the runs, y: the dispatch's groups across (render/world_layout.h, wl_groups)
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

// static_world.c, outside_frustum
bool outside(float3 c, float3 h)
{
    [unroll] for (uint i = 0; i < 6; i++) {
        float4 n = planes[i];
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

// the indices the view keeps of a record: all of them, or none
uint kept(CullRecord r)
{
    float3 centre = asfloat(r.centre.xyz), half = asfloat(r.half.xyz);
    uint slot = r.centre.w & 0xffff, flags = r.centre.w >> 16;
    if ((flags & SW_ANIMATED) != 0) {
        Matrix43 m = placed[slot];
        centre = place_point(m, centre);
        half = moved_half(m, half);
    }
    return outside(centre, half) ? 0 : r.draw.x;
}

groupshared uint ends[GROUP];  // the indices records 0 to i of the chunk keep

[numthreads(GROUP, 1, 1)]
void cs_main(uint3 group : SV_GroupID, uint3 thread : SV_GroupThreadID)
{
    uint lane = thread.x;
    uint index = group.y * counts.y + group.x;
    if (index >= counts.x)
        return;
    Run run = runs[index];
    uint total = 0;
    for (uint base = 0; base < run.count; base += GROUP) {
        uint n = 0;
        if (base + lane < run.count)
            n = kept(cullRecords[run.first + base + lane]);
        ends[lane] = n;
        GroupMemoryBarrierWithGroupSync();
        // the chunk's running sums (Hillis and Steele)
        for (uint stride = 1; stride < GROUP; stride <<= 1) {
            uint add = lane >= stride ? ends[lane - stride] : 0;
            GroupMemoryBarrierWithGroupSync();
            ends[lane] += add;
            GroupMemoryBarrierWithGroupSync();
        }
        if (base + lane < run.count)
            places[run.first + base + lane] = n ? run.start + total + ends[lane] - n : CULLED;
        total += ends[GROUP - 1];
        GroupMemoryBarrierWithGroupSync();
    }
    if (lane == 0) {
        uint at = index * 5;
        commands[at] = total;
        commands[at + 1] = 1;
        commands[at + 2] = run.start;
        commands[at + 3] = 0;
        commands[at + 4] = 0;
    }
}
