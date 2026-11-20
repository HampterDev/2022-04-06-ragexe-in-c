#ifndef RENDER_WORLD_LAYOUT_H
#define RENDER_WORLD_LAYOUT_H

/* custom: the static world (render/static_world.h) as the device's shaders read it
   (src/render/shaders/world.hlsli, world_cull.hlsl, world_compact.hlsl, world_transform.hlsl): its
   records by id and in draw order, the runs the cull compacts and the chunks the compaction copies,
   its transform slots, and the uniforms and dispatches of the frame's compute passes, packed from
   the client's terms. Pure: render/gfx_gpu.c packs what it uploads with it, and
   tests/test_static_world_maps.c packs the same bytes for the passes it runs on the GPU.

   The cull compacts the records for five views a frame: the camera's (the main pass and the depth
   pass draw its runs) and each of the WL_CASCADES shadow cascades' (the shadow pass draws the
   casters' runs of each). Each view has a section of the frame's compacted index buffer, as large
   as all the runs' indices together, and a place for each record in it. */

#include "render/static_world.h"

#include <stddef.h>
#include <stdint.h>

#define WL_CASCADES 4
/* the views a frame compacts the records for: the camera's, then the cascades' */
#define WL_SECTIONS (1 + WL_CASCADES)
/* the indices a section holds at most: all sections' indices, 4 bytes each, fit in a buffer of up
   to 2^32 - 1 bytes */
#define WL_SECTION_INDICES (0xffffffffu / (4u * WL_SECTIONS))

/* A record by its id, which every vertex of the record carries (world.hlsli, WorldRecord): the
   texture's layer and, above, the lightmap's; the transform slot and, above, the texture-coordinate
   slot; the pass, the flags, the texture's array and the lightmap's, a byte each. */
struct WlRecord {
    uint32_t data[4];
};

/* A record in draw order as the cull and the compaction read it (world_cull.hlsl and
   world_compact.hlsl, RECORD_WORDS words each): its bounds' centre and half size as bits, then its
   slot and flags (slot | flags << 16) and its first index; its index count and base vertex. */
struct WlCullRecord {
    uint32_t centre[4];
    uint32_t half[4];
    uint32_t draw[4];
};

/* A run of records one indirect call draws, as the cull compacts it: its records, first to
   first + count in draw order (the view's runs, sw_draw_runs) or in the casters' order (the
   casters' runs, wl_caster_runs), and where its kept indices go in a view's section of the
   compacted index buffer, `indices` of them at most from `start`. */
struct WlRun {
    uint32_t first, count, start, indices;
};

/* A transform slot (world_transform.hlsl, Slot): its matrix, then its parent (-1: the camera's
   view), 1 where it is the world itself, and its animated matrix in the frame's (-1: none). */
struct WlSlot {
    float local[12];
    int32_t link[4];
};

/* Up to WL_CHUNK indices of a record, from `offset` on, that one group of the compaction copies
   where the record is kept (world_compact.hlsl, a thread an index): `record` its place in draw
   order. */
#define WL_CHUNK 64
struct WlChunk {
    uint32_t record, offset;
};

/* Where the cull's data lies in the words the cull and the compaction read (world_cull.hlsl and
   world_compact.hlsl, `cullData`): the records in draw order from word 0, then the view's runs,
   the casters' runs, the casters' order (each of their records' place in draw order) and the
   chunks, each from the word given; `words` in all. */
struct WlCullLayout {
    uint32_t runs, casterRuns, order, chunks, words;
};

/* Where the cull's output lies in the words of the frame's draws (`draws`, which the passes also
   draw indirectly from): the indirect commands, 5 words each, from word 0, the view's runs first and
   then each cascade's casters' runs; then from word `places` each section's place of each record in
   the compacted index buffer, culled ones 0xffffffff; `words` in all. */
struct WlDrawLayout {
    uint32_t commands, places, words;
};

/* The cull's uniform (world_cull.hlsl, Frame): the view's frustum (the cascades' boxes come from
   the shadow fit on the GPU, src/render/shaders/cascades.hlsli); then the runs of one view, the
   dispatch's groups across, the views and whether they are the cascades; then where the runs and
   the order lie in the cull's data (WL_NO_ORDER: draw order), the first command and where the
   places lie; then the first view's section and the indices of a section, and the records. */
#define WL_NO_ORDER 0xffffffffu
struct WlCullFrame {
    float planes[6][4];
    uint32_t counts[4];
    uint32_t layout[4];
    uint32_t sections[4];
};

/* The compaction's uniform (world_compact.hlsl, Frame): the chunks, the dispatch's groups across,
   the sections and the first; where the chunks and the places lie, and the records. */
struct WlCompactFrame {
    uint32_t counts[4];
    uint32_t layout[4];
};

/* The transform pass's uniform (world_transform.hlsl, Frame): the camera's view, view space to clip
   space, then the number of slots. */
struct WlTransformFrame {
    float camera[12];
    float projection[16];
    uint32_t counts[4];
};

/* Records in draw order into byId[record.id] and cull[i]; byId must hold every id the records
   have, and its entries no record has are left as they were. */
void wl_records(const struct SwRecord *records, size_t count, struct WlRecord *byId, struct WlCullRecord *cull);
/* The chunks of records[0..count), each record's indices WL_CHUNK at a time in order, into out
   where it is not NULL: answers how many, a record of n indices having (n + WL_CHUNK - 1) / WL_CHUNK. */
size_t wl_chunks(const struct SwRecord *records, size_t count, struct WlChunk *out);
/* The view's runs the device compacts, out[0..n), each run's indices placed behind the run's before
   it. Answers n; *indices takes the indices they hold together, at most WL_SECTION_INDICES: the
   runs that would pass it, and those after them, are left out. */
size_t wl_runs(const struct SwDrawRun *runs, size_t count, struct WlRun *out, uint32_t *indices);
/* The casters' runs of the records in draw order, out[0..n): a run for each texture array the
   records have (SW_NO_ARRAY too), in the order of the arrays, its records in draw order, since a
   caster's pipeline knows nothing else of a record (world_caster.hlsl). order[0..held) takes each
   run's records' places in draw order, one run after the other, each run's indices placed behind
   the run's before it. Answers n; *held takes the records placed and *indices the indices they hold
   together, at most WL_SECTION_INDICES: the records that would pass it are left out. */
size_t wl_caster_runs(const struct SwRecord *records, size_t count, uint32_t *order, size_t *held, struct WlRun *out,
                      uint32_t *indices);
void wl_slot(const float *local, int parent, int animated, int world, struct WlSlot *out);
void wl_cull_layout(size_t records, size_t runs, size_t casterRuns, size_t orderCount, size_t chunks,
                    struct WlCullLayout *out);
void wl_draw_layout(size_t records, size_t runs, size_t casterRuns, struct WlDrawLayout *out);
/* the indirect command of the view's run `run`, or of the casters' run `run` in cascade
   `cascade` (0 to WL_CASCADES - 1) */
uint32_t wl_view_command(uint32_t run);
uint32_t wl_caster_command(size_t runs, size_t casterRuns, int cascade, uint32_t run);

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
/* The view's cull of its `runs` runs, a group each (frustum given), or the cascades' cull of the
   casters' `runs` runs, a group each for every cascade (frustum NULL): its uniform, and its
   dispatch's groups (wl_groups). sectionIndices is the indices of a section (the larger of the
   view's runs' and the casters' together). */
void wl_cull_frame(const struct SwFrustum *frustum, uint32_t runs, const struct WlCullLayout *cull,
                   const struct WlDrawLayout *draws, size_t viewRuns, uint32_t records, uint32_t sectionIndices,
                   struct WlCullFrame *out, uint32_t groups[2]);
/* The compaction of `chunks` chunks for `sections` sections from `first`, a group each: its
   uniform, and its dispatch's groups (wl_groups). */
void wl_compact_frame(uint32_t chunks, uint32_t first, uint32_t sections, const struct WlCullLayout *cull,
                      const struct WlDrawLayout *draws, uint32_t records, struct WlCompactFrame *out,
                      uint32_t groups[2]);
void wl_transform_frame(const float camera[12], const float projection[16], uint32_t slots,
                        struct WlTransformFrame *out);

#endif
