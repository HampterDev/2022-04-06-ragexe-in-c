#ifndef RENDER_SKIN_LAYOUT_H
#define RENDER_SKIN_LAYOUT_H

/* custom: Granny's actors (actor/granny.c) as the skinning shader reads them
   (src/render/shaders/skin.hlsli), as pure packing: a model's vertices and indices, which go up to
   the device once (render/gfx.h, gfx_create_skin); an actor's pose for a frame, lit as its kind
   is, rows of four words among the frame's quads (gfx_skin_pose); which draws of its meshes one
   draw of the device takes
   (skin_batch_join); and a vertex where Granny's deformer put it, on the CPU, for the one effect
   that starts at a vertex. render/gfx_gpu.c and the tests pack alike. Uses nothing else of the
   client. */

#include <stddef.h>
#include <stdint.h>

/* The most bones a model's skeleton has on the device: as many as the animations are sampled for
   (resources/granny_gltf.c, _GrannySampleModelAnimations), and as a vertex's byte can name. */
#define SKIN_BONES_MAX 256

/* A vertex as the converted file holds it (resources/granny_sdk.h, _GrannyCopyMeshSkinVertices):
   its position, normal and texture coordinates, and up to four bones of the skeleton with their
   weights; the skinned position is the sum of weight times the point through each bone's matrix. */
struct SkinSource {
    float position[3], normal[3], uv[2], weights[4];
    int bones[4];
};

/* A vertex as the device keeps it, 56 bytes (skin.hlsl, SkinIn): the source's, with bones that name
   none of the skeleton's taken as bone 0, as the file's reader takes them. SKIN_RIGID: a vertex of a
   rigid mesh, which Granny moved whole with its one bone and lit by its normal as it lies; its
   bones are that bone alone, with the weight 1. */
#define SKIN_RIGID 1u

struct SkinVertex {
    float position[3], normal[3], uv[2], weights[4];
    uint8_t bones[4];
    uint32_t flags;
};

/* `in` as the device keeps it, for a skeleton of `bones` bones (1 to SKIN_BONES_MAX), of a rigid
   mesh where rigidBone is its bone (a bone outside the skeleton taken as bone 0), of a skinned one
   where it is negative. */
void skin_pack_vertex(const struct SkinSource *in, int rigidBone, int bones, struct SkinVertex *out);

/* A mesh's `count` 16-bit indices as the device's 32-bit ones, its vertices placed from `base` on
   in the model's: 0, with out undefined, where one names no vertex of the mesh's `vertices` or
   lands past 2^32 - 1. */
int skin_pack_indices(const uint16_t *in, uint32_t count, uint32_t vertices, uint32_t base, uint32_t *out);

/* An actor's pose for a frame: its bones' matrices, the matrix of its meshes' space in the view,
   the projection and its light, which every vertex of its meshes takes. */
struct SkinPose {
    /* boneCount matrices of 16 floats, Granny's composites (the bone's bind pose undone and its
       pose of the frame done): row vectors, a point's x is p.x * m[0] + p.y * m[4] + p.z * m[8] +
       m[12], its y and z likewise from m[1] and m[2] */
    const float *bones;
    uint32_t boneCount;
    /* the meshes' space into view space, a 4x3 as render/math3d.h's: x = p.x * m[0] + p.y * m[3] +
       p.z * m[6] + m[9], y and z (the depth) likewise from m[1] and m[2]; and the projection a
       view-space point lands on the screen through, as render/rp_project.c's
       renderer_project_vertex projects it: at the pixel (x * proj[0] / z + centre[0], y * proj[1] /
       z + centre[1]) with the depth (z - nearZ) * depthScale / z */
    float view[12];
    float proj[2], centre[2], nearZ, depthScale;
    /* a vertex's colour: b = (unsigned)(min(max(n . light, 0), 1) * scale + offset) for its normal
       n, each of blue, green and red (b + that byte of `add`) mod 256, `add` a D3DCOLOR whose
       alpha is not read, and the alpha `alpha` */
    float light[3];
    float scale, offset;
    uint32_t add;
    uint8_t alpha;
    /* rigid meshes drawn as they lie, without their bone, as the guild flag's were */
    int rigidStill;
};

/* The kinds of Granny actor (actor/granny.c), each lit its own way */
enum { SKIN_ACTOR_LIT, SKIN_ACTOR_CORPSE, SKIN_ACTOR_GUILD_FLAG };

/* The pose's scale, offset, add and rigidStill as actor/granny.c's loop of each kind lit its
   vertices: the lit model b = n . l times its light's `intensity` plus 255 - intensity, each of
   blue, green and red that byte of its light's colour (red, green, blue) plus 0x38 plus b, modulo
   256; the corpse and the guild flag b = n . l times 100 plus 155, adding nothing, the guild
   flag's rigid meshes as they lie. */
void skin_pose_kind(struct SkinPose *pose, int kind, int intensity, int red, int green, int blue);

/* The rows a pose takes (SKIN_POSE_HEADER, then three for each bone, one bone at least) for
   `boneCount` bones; 0 for more than SKIN_BONES_MAX. */
#define SKIN_POSE_HEADER 7
size_t skin_pose_size(uint32_t boneCount);

/* The pose packed as the shader reads it (skin.hlsli), into skin_pose_size(pose->boneCount) rows:
   row 0 the number of bones, the flags (bit 0 rigidStill) with the alpha above them (bits 8 to
   15), `add` and the offset's bits; rows 1 to 3 the view matrix's columns (x = dot of (p, 1) and
   row 1); row 4 the light and the scale; row 5 proj and centre, row 6 nearZ and depthScale; then
   each bone's matrix as three columns likewise. A pose without bones gets one of zeros, where
   Granny's deformer found an empty matrix. Answers the rows written, 0 for more than SKIN_BONES_MAX
   bones. */
size_t skin_pose_rows(const struct SkinPose *pose, float (*rows)[4]);

/* A vertex where Granny's deformer left it in its mesh's space for the pose's bones (boneCount of
   them, each 16 floats as SkinPose.bones): a rigid one where it lies, a skinned one the sum of its
   weighted bones' points, a weight of 0 skipped, a bone the pose does not have taken as its last,
   as the shader takes it (none: the origin). */
void skin_vertex_position(const struct SkinVertex *v, const float *bones, uint32_t boneCount, float out[3]);

/* The draws of one device draw (render/gfx_gpu.c), in the order given: a range of a model's
   indices drawn for `instances` poses, the poses one after the other, each drawing the whole range
   (the GPU draws an instanced draw instance by instance), `pose` the last. */
struct SkinBatch {
    uint32_t first, count;
    uint32_t instances;
    int pose;
};

/* What a draw of `count` indices from `first` in `pose`, of the batch's model with what the batch
   draws with, does to it: SKIN_JOIN_EXTEND, the batch is one pose's and the draw goes on where its
   range ends (an actor's next mesh); SKIN_JOIN_INSTANCE, the draw is the batch's range in another
   pose, or in the same again (the same mesh of another actor); SKIN_JOIN_NEW, it begins a batch of
   its own. Each keeps the order the draws came in. */
enum { SKIN_JOIN_NEW, SKIN_JOIN_EXTEND, SKIN_JOIN_INSTANCE };
int skin_batch_join(const struct SkinBatch *batch, uint32_t first, uint32_t count, int pose);

#endif
