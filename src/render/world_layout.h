#ifndef RENDER_WORLD_LAYOUT_H
#define RENDER_WORLD_LAYOUT_H

/* custom: the static world (render/static_world.h) as the device's shaders read it
   (src/render/shaders/world.hlsli, world_cull.hlsl, world_compact.hlsl, world_transform.hlsl): its
   records by id and in draw order, the runs the cull compacts and the chunks the compaction copies,
   its transform slots, and the uniforms and dispatches of the frame's compute passes, packed from
   the client's terms. Pure: render/gfx_gpu.c packs what it uploads with it, and
   tests/test_static_world_maps.c packs the same bytes for the passes it runs on the GPU. */

#include "render/static_world.h"

#include <stddef.h>
#include <stdint.h>

/* A record by its id, which every vertex of the record carries (world.hlsli, WorldRecord): the
   texture's layer and, above, the lightmap's; the transform slot and, above, the texture-coordinate
   slot; the pass, the flags, the texture's array and the lightmap's, a byte each. */
struct WlRecord {
    uint32_t data[4];
};

/* A record in draw order as the cull and the compaction read it (world_cull.hlsl and
   world_compact.hlsl, CullRecord): its bounds' centre and half size as bits, then its slot and flags
   (slot | flags << 16) and its first index; its index count and base vertex. */
struct WlCullRecord {
    uint32_t centre[4];
    uint32_t half[4];
    uint32_t draw[4];
};

/* A run of records one indirect call draws (sw_draw_runs) as the cull compacts it: its records in
   draw order, first to first + count, and where its kept indices go in the frame's compacted index
   buffer, `indices` of them at most from `start`. */
struct WlRun {
    uint32_t first, count, start, indices;
};

/* A transform slot (world_transform.hlsl, Slot): its matrix, then its parent (-1: the camera's
   view), 1 where it is the world itself, and its animated matrix in the frame's (-1: none). */
struct WlSlot {
    float local[12];
    int32_t link[4];
};

/* The cull's uniform (world_cull.hlsl, Frame): the frustum's planes, then the number of runs and
   the dispatch's groups across. */
struct WlCullFrame {
    float planes[6][4];
    uint32_t counts[4];
};

/* Up to WL_CHUNK indices of a record, from `offset` on, that one group of the compaction copies
   where the record is kept (world_compact.hlsl, Chunk, a thread an index): `record` its place in
   draw order. */
#define WL_CHUNK 64
struct WlChunk {
    uint32_t record, offset;
};

/* The compaction's uniform (world_compact.hlsl, Frame): the number of chunks and the dispatch's
   groups across. */
struct WlCompactFrame {
    uint32_t counts[4];
};

/* The transform pass's uniform (world_transform.hlsl, Frame): the camera's view, view space to clip
   space, view space to each shadow cascade's light space, then the number of slots and of
   cascades. */
#define WL_CASCADES 4
struct WlTransformFrame {
    float camera[12];
    float projection[16];
    float fromView[WL_CASCADES][16];
    uint32_t counts[4];
};

/* Records in draw order into byId[record.id] and cull[i]; byId must hold every id the records
   have, and its entries no record has are left as they were. */
void wl_records(const struct SwRecord *records, size_t count, struct WlRecord *byId, struct WlCullRecord *cull);
/* The chunks of records[0..count) (the records of the runs drawn indirectly, which come first in
   draw order), each record's indices WL_CHUNK at a time in order, into out where it is not NULL:
   answers how many, a record of n indices having (n + WL_CHUNK - 1) / WL_CHUNK. */
size_t wl_chunks(const struct SwRecord *records, size_t count, struct WlChunk *out);
/* The runs the device draws indirectly, those of the passes up to the ground (SW_PASS_OPAQUE_CLAMP,
   SW_PASS_OPAQUE_WRAP, SW_PASS_GROUND), which come first in draw order: out[0..n), each run's
   indices placed behind the run's before it. Answers n; *indices takes the indices they hold
   together (at most 2^32 - 1: the runs that would pass it are left out). */
size_t wl_runs(const struct SwDrawRun *runs, size_t count, struct WlRun *out, uint32_t *indices);
void wl_slot(const float *local, int parent, int animated, int world, struct WlSlot *out);

/* View space to clip space for a target w x h pixels: a view-space point lands where
   renderer_project_vertex (render/rp_project.c) puts it, with the same depth: proj and centre the
   projection's scale and centre in pixels, nearZ and depthScale its depth (render/gfx.h,
   GfxWorldView). */
void wl_projection(float out[16], const float proj[2], const float centre[2], float nearZ, float depthScale, int w,
                   int h);
/* The client's 4x3 view (render/gfx.h) as a row-vector 4x4 matrix, times `projection`, each element
   rounded as the Direct3D 9 client computed it. */
void wl_view_clip(float out[16], const float view[12], const float projection[16]);
/* The most groups a dispatch takes along one dimension: Vulkan's least maxComputeWorkGroupCount. */
#define WL_GROUPS_ACROSS 65535u
/* `groups` groups of a compute pass as a dispatch of out[0] across by out[1] down, which the
   shaders number group.y * out[0] + group.x: both 0 where groups is. */
void wl_groups(uint32_t groups, uint32_t out[2]);
/* The cull of `runs` runs, a group each: its uniform, and its dispatch's groups (wl_groups). */
void wl_cull_frame(const struct SwFrustum *frustum, uint32_t runs, struct WlCullFrame *out, uint32_t groups[2]);
/* The compaction of `chunks` chunks, a group each: its uniform, and its dispatch's groups
   (wl_groups). */
void wl_compact_frame(uint32_t chunks, struct WlCompactFrame *out, uint32_t groups[2]);
void wl_transform_frame(const float camera[12], const float projection[16], const float (*fromView)[16],
                        uint32_t cascades, uint32_t slots, struct WlTransformFrame *out);

#endif
