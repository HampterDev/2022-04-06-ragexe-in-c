// The kept records' indices into their places in the frame's index buffer, which world_cull.hlsl
// gave each kept record: a group of 64 threads to each chunk of a record's indices, 64 of them
// (render/world_layout.h, WlChunk), which copies them, the record's base vertex added, where the
// record's kept indices go, and leaves at once where the record is culled. The chunks are fixed
// with the records, so that a thread finds its index without a search, and a long run's or a long
// record's copy is spread over as many groups as it has chunks.

#define CULLED 0xffffffff

// A record in draw order (render/world_layout.h, WlCullRecord).
struct CullRecord
{
    uint4 centre;
    uint4 half;     // w: the first index
    uint4 draw;     // x: the index count, y: the base vertex
};

struct Chunk
{
    uint record;
    uint offset;
};

StructuredBuffer<CullRecord> cullRecords : register(t0, space0);
StructuredBuffer<uint> places : register(t1, space0);
StructuredBuffer<uint> indices : register(t2, space0);
StructuredBuffer<Chunk> chunks : register(t3, space0);
RWStructuredBuffer<uint> drawn : register(u0, space1);

cbuffer Frame : register(b0, space2)
{
    uint4 counts;  // x: the chunks, y: the dispatch's groups across (render/world_layout.h, wl_groups)
};

[numthreads(64, 1, 1)]
void cs_main(uint3 group : SV_GroupID, uint3 thread : SV_GroupThreadID)
{
    uint c = group.y * counts.y + group.x;
    if (c >= counts.x)
        return;
    Chunk chunk = chunks[c];
    uint to = places[chunk.record];
    if (to == CULLED)
        return;
    CullRecord r = cullRecords[chunk.record];
    uint within = chunk.offset + thread.x;
    if (within < r.draw.x)
        drawn[to + within] = indices[r.half.w + within] + r.draw.y;
}
