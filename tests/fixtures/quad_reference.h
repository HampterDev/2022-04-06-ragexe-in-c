#ifndef TESTS_FIXTURES_QUAD_REFERENCE_H
#define TESTS_FIXTURES_QUAD_REFERENCE_H

/* The client's CPU code that the quads' instances replaced (render/shaders/quad.hlsl), as it was
   before them (a005c851; the effect primitives' triangles and the ground's cell tiles as at
   5ea7b76b), the reference tests/test_quad_instances.c compares the GPU with. */
#include "render/quads.h"

#include "platform/win_types.h"

struct SqQuad;
struct SpriteRegion;
struct Particle;
struct SpriteParticle;
struct C3dGround15;
struct Texture;

/* A vertex as the device took it: pre-transformed, with the colour's bytes in the D3DCOLOR's order
   in memory and both stages' coordinates (render/gfx_gpu.c's 36-byte quad vertex). */
struct ReferenceVertex {
    float x, y, z, rhw;
    unsigned char bytes[4];
    float u0, v0, u1, v1;
};

/* render_object_submit_spr_quad's four vertices (actors/sprite_quad.c): from the quad, the
   texture's region (with its 0xd77b palette's narrower right edge where `palette` is that) and the
   flipped screen's x and y */
void reference_sprite_vertices(const struct SqQuad *quad, const struct SpriteRegion *region, int palette, float x,
                               float y, struct TexDrawVertex *out);

/* particle_render's four vertices (effects/particle.c): the particle's corners, painted, through
   its matrix (turned to face the camera unless facing_fixed) and the view, projected by
   renderer_project_vertex, its depth bias added to their rhw, z of that rhw or 0; 0 where it drew
   nothing */
int reference_particle_vertices(struct Particle *self, unsigned char facing_fixed, float v_top, float v_bottom,
                                float u_left, float u_right, unsigned char with_depth, struct TexDrawVertex *out);

/* sprite_particle_render's quads (effects/sprite_particle.c), four vertices each, at most `most`,
   of a particle whose motion stays (SPRITE_STILL): how many it drew */
int reference_sprite_particle_vertices(struct SpriteParticle *self, struct TexDrawVertex (*out)[4], int most);

/* One of an effect primitive's triangles as effects/rag/effect_prim_draw.c's set_vertex and
   quad_depth made it before the device projected its corners (5ea7b76b): the points p projected
   by renderer_project_vertex through `view`, in colour argb with u and v, then moved by `mode`
   (PRIM_QUAD_DEPTH_BIAS, PRIM_QUAD_IN_FRONT, PRIM_QUAD_UPRIGHT about the point `centre`, lifted by
   the renderer's height and `scale`, `second` the quad's second triangle; others as projected) */
void reference_prim_triangle(const float (*p)[3], const float *u, const float *v, unsigned argb, const float *view,
                             unsigned char mode, const float *centre, float scale, int second,
                             struct TexDrawVertex *out);

/* The ground's cell tiles as world/ground_draw.c made their four vertices before the device
   projected their corners (5ea7b76b): attr_quad's (c3d_ground15_draw_attr_quad and _raised, a
   quarter of the cell's quad through renderer_project_vertex, z less `bias`) and
   c3d_ground15_draw_attr_marker's; 0 where they drew nothing */
int reference_attr_quad(struct C3dGround15 *self, const float *view, int ax, int ay, DWORD colour,
                        struct Texture *texture, const float *uv, float bias, struct TexDrawVertex *out);
int reference_attr_marker(struct C3dGround15 *self, const float *view, int ax, int ay, DWORD colour,
                          struct TexDrawVertex *out);

/* a vertex of one texture's coordinates as the device widened it, both stages reading them */
void reference_widen(const struct TexDrawVertex *in, struct ReferenceVertex *out);

/* render/gfx_gpu.c's snap_flat: in a multisampled pass the n vertices of a flat draw (triangles
   of `indices`, count of them, or of the vertices in order without) moved onto pixel boundaries */
void reference_snap_flat(struct ReferenceVertex *verts, int n, const unsigned short *indices, unsigned count);

#endif
