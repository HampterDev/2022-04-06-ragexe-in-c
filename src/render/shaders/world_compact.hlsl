// The kept records' indices into their places in the frame's index buffer, which world_cull.hlsl
// gave each kept record in each view's section: a group of 64 threads to each chunk of a record's
// indices, 64 of them (render/world_layout.h, WlChunk), in each section the dispatch takes, which
// copies them, the record's base vertex added, where the record's kept indices go there, and
// leaves at once where the view culled the record. The chunks are fixed with the records, so that a
// thread finds its index without a search, and a long run's or a long record's copy is spread over
// as many groups as it has chunks.

#define CULLED 0xffffffff
#define RECORD_WORDS 12  // render/world_layout.h, WlCullRecord

StructuredBuffer<uint> cullData : register(t0, space0);  // render/world_layout.h, WlCullLayout
StructuredBuffer<uint> draws : register(t1, space0);     // WlDrawLayout: the commands, then the places
StructuredBuffer<uint> indices : register(t2, space0);
RWStructuredBuffer<uint> drawn : register(u0, space1);

cbuffer Frame : register(b0, space2)
{
    uint4 counts;  // the chunks, the dispatch's groups across (wl_groups), the sections, the first
    uint4 layout;  // where the chunks lie in cullData, where the places lie in draws, the records
};

[numthreads(64, 1, 1)]
void cs_main(uint3 group : SV_GroupID, uint3 thread : SV_GroupThreadID)
{
    uint g = group.y * counts.y + group.x;
    if (g >= counts.x * counts.z)
        return;
    uint section = counts.w + g / counts.x, c = layout.x + (g % counts.x) * 2;
    uint record = cullData[c], offset = cullData[c + 1];
    uint to = draws[layout.y + section * layout.z + record];
    if (to == CULLED)
        return;
    uint base = record * RECORD_WORDS, within = offset + thread.x;
    if (within < cullData[base + 8])
        drawn[to + within] = indices[cullData[base + 7] + within] + cullData[base + 9];
}
