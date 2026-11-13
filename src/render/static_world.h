#ifndef RENDER_STATIC_WORLD_H
#define RENDER_STATIC_WORLD_H

/* custom: the static world as data the GPU keeps (plans/gpu-renderer.md, "The static world,
   GPU-driven"): a map's ground blocks and model nodes in one vertex and one index buffer, one draw
   record per (block or node, texture run) as the ground and the models split their draws, texture
   arrays by size, and the cull that picks the records a view or a shadow cascade draws, in the
   order the passes draw them. Pure rules: no I/O, no clock, no allocation; the caller owns every
   buffer. Matrices are the client's 4x3 row-vector matrices (render/gfx.h): a point x, y, z goes
   to x * m[0] + y * m[3] + z * m[6] + m[9], and likewise with entries 1, 4, 7, 10 and 2, 5, 8,
   11. */

#include <stddef.h>
#include <stdint.h>

/* no slot */
#define SW_NONE 0xffffu
/* no texture array: a draw without that texture */
#define SW_NO_ARRAY 0xffu
/* the vertices of one mesh, which its 16-bit indices reach */
#define SW_MESH_VERTICES 65536u
/* a vertex no record draws (sw_vertex_ids) */
#define SW_NO_RECORD 0xffffffffu

/* ---- Layout: the map's shared buffers ----
   Meshes are placed one behind the other and never move, so a mesh that arrives while the map
   plays (a model the loader thread streams in) is appended behind the ones in place. A mesh keeps
   its own 16-bit indices; a draw adds its baseVertex. */

struct SwLayout {
    uint32_t vertexCount, indexCount;
};

struct SwPlacement {
    uint32_t baseVertex, vertexCount, firstIndex, indexCount;
};

/* Places a mesh of `vertices` (1 to SW_MESH_VERTICES) and `indices` (a positive multiple of 3)
   behind the meshes placed so far. 0, with nothing placed, for other counts or where the buffers
   would pass 2^32 - 1 elements. */
int sw_layout_place(struct SwLayout *layout, uint32_t vertices, uint32_t indices, struct SwPlacement *out);
/* The capacity, in elements, a buffer of `capacity` grows to so that `needed` fit: at least
   `minimum`, doubled until it holds them, at most 2^32 - 1; `capacity` where they fit already.
   Copying the old contents to the start of the grown buffer keeps every placement. */
uint32_t sw_layout_grow(uint32_t capacity, uint32_t needed, uint32_t minimum);

/* ---- Draw records ---- */

/* The passes, in the order the frame draws them (render/crenderer.c): the models' opaque draws
   with clamped and with wrapped texture coordinates, the ground, then the models' alpha-blended
   draws, clamped, wrapped, and those that write depth. */
enum {
    SW_PASS_OPAQUE_CLAMP,
    SW_PASS_OPAQUE_WRAP,
    SW_PASS_GROUND,
    SW_PASS_ALPHA_CLAMP,
    SW_PASS_ALPHA_WRAP,
    SW_PASS_ALPHA_DEPTH,
    SW_PASS_COUNT
};

/* back faces are the clockwise ones without SW_CULL_CCW, the counter-clockwise ones with it, and
   none with SW_CULL_NONE (a two-sided face) */
#define SW_CULL_CCW 0x01
#define SW_CULL_NONE 0x02
/* the record's transform changes from frame to frame (an animated node): its bounds and key point
   are in the transform's space, not the world's */
#define SW_ANIMATED 0x04

/* One of today's draws. Its texture and its lightmap are layers of texture arrays
   (sw_texture_place; SW_NO_ARRAY: none), transform its slot in the transform buffer, uv its slot
   in the frame's buffer of texture-coordinate matrices (SW_NONE: the identity). sort is its place
   among the draws of its pass: the renderer sorts a pass by its texture's creation order, the
   ground by its texture's and its lightmap's. The key point is the first corner of its first
   triangle: a draw in an alpha pass is sorted by that corner's depth, farthest first. id is the
   record's own number, which stays with it however the records are sorted, and which its vertices
   carry (sw_vertex_ids). */
struct SwRecord {
    uint32_t firstIndex, indexCount, baseVertex;
    uint32_t sort[2];
    uint32_t id;
    uint16_t textureLayer, lightmapLayer, transform, uv;
    uint8_t textureArray, lightmapArray, pass, flags;
    float centre[3], half[3];
    float key[3];
};

/* A run of a mesh's indices, `count` (a positive multiple of 3) from `first`, as today's code
   draws it. */
struct SwRun {
    uint32_t first, count;
    uint32_t sort[2];
    uint16_t textureLayer, lightmapLayer, uv;
    uint8_t textureArray, lightmapArray, pass, flags;
};

/* A mesh as its runs read it: vertex i's x, y and z at positions[i * stride], its 16-bit indices,
   the transform slot of every run, and where the transform never changes, its matrix (NULL for the
   identity), which puts the bounds and key points in world space; with `animated` they stay in the
   mesh's own space and the cull moves them by the transform each frame. */
struct SwMesh {
    const float *positions;
    size_t stride;
    uint32_t vertexCount;
    const uint16_t *indices;
    uint32_t indexCount;
    uint16_t transform;
    int animated;
    const float *placed;
};

/* Writes the records of a mesh's `count` runs to out[0..count): each its run's index range moved
   to the mesh's place, its bounds the box around the corners of its triangles, its id firstId plus
   its run's number; a run with a corner that is not finite is bounded by nothing, and every view
   and cascade keeps it. 0, with nothing written, when the mesh does not match its place, a run lies
   outside the mesh's indices, an index outside its vertices, or a pass or a flag is unknown. */
int sw_records_build(const struct SwMesh *mesh, const struct SwPlacement *place, const struct SwRun *runs,
                     size_t count, uint32_t firstId, struct SwRecord *out);
/* The record each of the mesh's vertices belongs to, out[0..mesh->vertexCount): firstId plus the
   number of the run whose triangles use it, SW_NO_RECORD where none does, so that a draw can find
   its record from its vertices. 0 where two runs share a vertex or a run lies outside the mesh, with
   out undefined. */
int sw_vertex_ids(const struct SwMesh *mesh, const struct SwRun *runs, size_t count, uint32_t firstId, uint32_t *out);
/* Sorts records into the order the passes draw them: by pass, then sort[0] and sort[1] (the order
   the renderer draws textures in), then by cull mode, and then where they lie in the index buffer.
   Records with one texture come in no order of the renderer's; the cull mode keeps those of one
   pipeline together. The order is total (every field counts, the id last), so that merging two
   sorted lists by sw_record_before gives what sorting them together gives. */
void sw_records_sort(struct SwRecord *records, size_t count);
/* whether a comes before b in that order */
int sw_record_before(const struct SwRecord *a, const struct SwRecord *b);

/* The cull mode a record's triangles are drawn with: SW_CULL_NONE, SW_CULL_CCW or 0 (clockwise). */
#define SW_CULL_MASK (SW_CULL_CCW | SW_CULL_NONE)

/* A run of sorted records one indirect call draws: the same pass, texture array, lightmap array
   and cull mode, records first to first + count, whose index counts add up to `indices`. */
struct SwDrawRun {
    uint32_t first, count, indices;
    uint8_t pass, textureArray, lightmapArray, cull;
};

/* Splits sorted records into runs, out[0..n): answers n, at most `count`. */
size_t sw_draw_runs(const struct SwRecord *records, size_t count, struct SwDrawRun *out);

/* ---- The cull ---- */

/* How the view reaches the screen (render/gfx.h, GfxWorldView): a view-space point lands at the
   pixel (x * proj[0] / z + centre[0], y * proj[1] / z + centre[1]) of a screen size[0] by size[1]
   pixels, and is drawn from depth nearZ to farZ. */
struct SwProjection {
    float proj[2], centre[2], size[2];
    float nearZ, farZ;
};

/* The view's six planes: the screen's four edges and the near and far depths, those of the
   projection itself, so that a box outside one of them has no pixel on screen whatever the
   screen's size. A world point p lies on the inner side of plane n where n . p + n[3] >= 0. */
struct SwFrustum {
    float plane[6][4];
};

void sw_frustum(struct SwFrustum *out, const struct SwProjection *projection, const float *view);

/* A shadow cascade's box, world coordinates: along the light's axes 0 and 1 across its map from lo
   to hi, along axis 2 from depth[0] to depth[1] (render/shadow_fit.h: the frame's axes, a cascade's
   square, and the light's depth from lightNear to lightNear + lightRange). */
struct SwCascade {
    float axis[3][3];
    float lo[2], hi[2];
    float depth[2];
};

/* A record's bounds in the world, centre and half its size: a static record's own, an animated
   record's moved by its transform (transforms[slot], 4x3), the box around them. 0 for an animated
   record without transforms. A record bounded by nothing has a half size of 1e30. */
int sw_record_bounds(const struct SwRecord *record, const float (*transforms)[12], float *centre, float *half);
/* Whether the view draws a record: whether the frustum leaves its bounds not wholly outside one of
   its planes, an animated record's moved by its transform (transforms[slot], 4x3; a static record's
   transform is not read, and transforms may be NULL where no record is animated, which an animated
   record then fails). The device's cull (render/shaders/world_cull.hlsl) decides as this does, to
   the bit. */
int sw_record_in_view(const struct SwRecord *record, const float (*transforms)[12], const struct SwFrustum *frustum);
/* A cascade's box as six planes, as sw_frustum gives the view's: inside where n . p + n[3] >= 0;
   across the light at lo and hi along axes 0 and 1, then along it at depth[0] and depth[1], each
   plane the axis and minus the lower bound, then minus the axis and the upper bound, as the GPU's
   fit writes them (src/render/shaders/shadow_cascades.hlsl). */
void sw_cascade_planes(const struct SwCascade *cascade, struct SwFrustum *out);
/* The records that cast into a cascade, out[0..n) in record order: those whose bounds are not
   wholly outside one of its box's planes, an animated record's moved by its transform, as
   sw_record_in_view tests the view's. The device culls each cascade's casters by this rule
   (render/shaders/world_cull.hlsl), to the bit. */
size_t sw_cull_cascade(const struct SwRecord *records, size_t count, const float (*transforms)[12],
                       const struct SwCascade *cascade, uint32_t *out);

/* ---- Texture arrays ---- */

/* Texture arrays, one size and one number of mipmap levels each, at most maxLayers layers each, at
   most SW_NO_ARRAY arrays. Sizes are not merged: a texture in a larger layer would filter, wrap and
   mip differently, and a texture made while mipmaps were off has one level where one of its size
   made after has a chain. A texture keeps its array and layer when more arrive: it goes into the
   last array of its size and levels that has room, or opens a new one behind the others. */
struct SwTextureArray {
    uint16_t width, height;
    uint8_t levels;
    uint32_t layers;
};

struct SwTextureArrays {
    struct SwTextureArray *arrays;
    size_t count, capacity;
    uint32_t maxLayers;
};

/* Places a texture of the GPU size w by h with `levels` mipmap levels. Answers 2 where it opened a
   new array (the caller makes it, or takes it back with sw_texture_unplace), 1 where it took a
   layer of an array there was, 0, with nothing placed, when it needs a new array and `capacity` or
   SW_NO_ARRAY arrays are in use, or when a size, levels or maxLayers is 0 or maxLayers above
   65536. */
int sw_texture_place(struct SwTextureArrays *arrays, uint16_t w, uint16_t h, uint8_t levels, uint8_t *array,
                     uint16_t *layer);
/* Takes back the placement sw_texture_place made last, of `array` and `layer`, where the texture
   could not go there after all: its layer, and its array where the placement opened it. */
void sw_texture_unplace(struct SwTextureArrays *arrays, uint8_t array, uint16_t layer);

#endif
