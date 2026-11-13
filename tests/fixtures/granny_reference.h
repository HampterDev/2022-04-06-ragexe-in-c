#ifndef TESTS_FIXTURES_GRANNY_REFERENCE_H
#define TESTS_FIXTURES_GRANNY_REFERENCE_H

/* The vertices Granny's actors had on the CPU before the device skinned them
   (tests/test_skin_instances.c compares the skinning shader with them): the shim's deformer
   (resources/granny_gltf.c, _GrannyDeformVertices), a mesh's vertices and matrix for the frame
   (resources/granny_mesh.c, render_mesh) and the three loops of actor/granny.c that projected,
   placed in the world (world_corner) and lit every corner of a mesh's triangles, as they were. */
#include "render/quads.h"
#include "render/skin_layout.h"
#include "render/vec3.h"

struct Renderer;

/* a vertex of a mesh for the frame, as the deformer left it (GrannyMeshVertex) */
struct GrannyReferenceVertex {
    Vec3 pos;
    float nx, ny, nz;
    float u, v;
};

/* A mesh's `count` vertices for the frame (render_mesh): a rigid mesh's as they lie, with its
   matrix the rows of its bone's composite; a skinned one's through the deformer, its bones the
   skeleton's own, with the identity. */
void granny_reference_mesh(const struct SkinSource *src, int count, int rigidBone, const float *composite,
                           struct GrannyReferenceVertex *out, float *matrix);

/* The light of a frame: the lit model's (sun the sun's position, intensity, light colour), the
   corpse's and the guild flag's, from the view as each loop took it. */
enum { GRANNY_REFERENCE_LIT, GRANNY_REFERENCE_CORPSE, GRANNY_REFERENCE_EMBLEM };
struct GrannyReferenceLight {
    Vec3 sun;
    int intensity, r, g, b;
    Vec3 viewDir;
};

/* The corners of a mesh's `tris` triangles (indices) of the vertices v, as the loop of `kind`
   made them for the model matrix r (granny.c's model_matrix, or the lit model's own), the mesh's
   matrix and the view (renderer_project_vertex's x, y, z and rhw, the colour and texture
   coordinates), and each corner's world point (world_corner). The light vector it lit with goes
   into light_out. */
void granny_reference_corners(struct Renderer *renderer, int kind, const struct GrannyReferenceVertex *v,
                              const unsigned short *indices, int tris, const float *mesh, const float *r,
                              const float *view, const struct GrannyReferenceLight *light, unsigned char alpha,
                              struct TexDrawVertex *out, float (*world)[3], float *light_out);

#endif
