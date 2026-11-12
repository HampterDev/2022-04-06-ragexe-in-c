#ifndef TESTS_FIXTURES_QUAD_REFERENCE_H
#define TESTS_FIXTURES_QUAD_REFERENCE_H

/* The client's CPU code that the quads' instances replaced (render/shaders/quad.hlsl), as it was
   before them (a005c851), the reference tests/test_quad_instances.c compares the GPU with. */
#include "render/quads.h"

struct SqQuad;
struct SpriteRegion;
struct Particle;
struct SpriteParticle;

/* A vertex as the device took it: pre-transformed, with the colour's bytes in the D3DCOLOR's order
   in memory and both stages' coordinates (render/gfx_gpu.c's 36-byte quad vertex). */
struct ReferenceVertex {
    float x, y, z, rhw;
    unsigned char bytes[4];
    float u0, v0, u1, v1;
};

/* render_object_submit_spr_quad's four vertices (actor/sprite_quad.c): from the quad, the
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

/* a vertex of one texture's coordinates as the device widened it, both stages reading them */
void reference_widen(const struct TexDrawVertex *in, struct ReferenceVertex *out);

/* render/gfx_gpu.c's snap_flat: in a multisampled pass the n vertices of a flat draw (triangles
   of `indices`, count of them, or of the vertices in order without) moved onto pixel boundaries */
void reference_snap_flat(struct ReferenceVertex *verts, int n, const unsigned short *indices, unsigned count);

#endif
