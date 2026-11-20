#ifndef RENDER_QUADS_H
#define RENDER_QUADS_H

/* The client's quads and triangles as instances (phase 6 of plans/gpu-renderer.md), as pure logic:
   what an instance holds (struct GfxQuad, which render/gfx.h draws), and the record of each in the
   frame's quads, the words the quad shader reads (render/shaders/instances.hlsli), made from an
   instance or from one of the client's primitives of vertices. The frame's quads hold the records
   and, each on rows of four words of its own, the matrices and vertices they name. The shader
   makes from a record the vertices the client's C code made before, within the tolerance of
   plans/gpu-renderer.md; tests/test_quad_instances.c compares them. Uses nothing else of the
   client. */
#include <stdint.h>

/* A vertex as the client makes one, pre-transformed: x and y in Direct3D 9's pixel coordinates,
   pixel centres on whole numbers, z the depth from 0 to 1, rhw 1 / w; the colour a D3DCOLOR. */
struct TexDrawVertex {
    float x, y, z, rhw;
    uint32_t diffuse;
    float u, v;
};

/* An instance: a quad, its corners 0 top left, 1 top right, 2 bottom left and 3 bottom right drawn
   as the triangles (0, 1, 2) and (2, 1, 3).
   - GFX_QUAD_SPRITE: render_object_submit_spr_quad's (actor/sprite_quad.c). The corners are
     (x0, top), (x1, top), (x0, bottom) and (x1, bottom), each moved by half a pixel; a turned one
     turns them by cosine and sine about the quad's centre moved by offsetX and offsetY. Every
     corner has rhwTop as its rhw, the top row zTop as its z and the bottom row zBottom, which the
     client computes (renderer_project_depth of rhwTop and rhwBottom): the layers of a figure lie at
     depths that may tie, and the depth test decides between them by the z's bits. u0 and v0 go
     with the top left corner, u1 and v1 with the bottom right. It lies flat on the screen.
   - GFX_QUAD_WORLD: points through the client's 4x3 view matrix as renderer_project_vertex
     (render/rp_project.c) projects them with the renderer's proj[0], proj[1], centreX and
     centreY (points already in view space, viewSpace, are projected alone); the rhw is the
     projection's (GFX_RHW_PROJECTED), that plus rhwValue (GFX_RHW_ADD) or rhwValue
     (GFX_RHW_VALUE); z is renderer_project_depth of that rhw (GFX_DEPTH_PROJECTED), depthValue
     (GFX_DEPTH_VALUE), that depth plus depthValue (GFX_DEPTH_ADD), or z[i] as the client computed
     it (GFX_DEPTH_CORNERS: what lies where a figure stands ties with its depth, and the depth test
     decides by the bits). Each corner has its texture
     coordinates, and with stage2 the second stage's of its own; flat when every corner's rhw is the
     same, bit for bit, as the client found it.
   - GFX_QUAD_CORNERS: a triangle of three of a set's corners (struct GfxCornerSet), corner[0] to
     corner[2] its corners 0 to 2; flat when their rhw are the same, bit for bit, as the client found
     them. In an upright set, corner 2 stands right of the set's point where `right`, left
     otherwise. */
#define GFX_QUAD_SPRITE 1
#define GFX_QUAD_WORLD 4
#define GFX_QUAD_CORNERS 5

#define GFX_RHW_PROJECTED 0
#define GFX_RHW_ADD 1
#define GFX_RHW_VALUE 2
#define GFX_DEPTH_PROJECTED 0
#define GFX_DEPTH_VALUE 1
#define GFX_DEPTH_ADD 2
#define GFX_DEPTH_CORNERS 3

/* Up to twelve points in the world with their texture coordinates, in one colour, which the
   triangles that name them share (the effect primitives' quads and triangles, so that they share
   the set's header and the corners they have in common): projected through
   the client's 4x3 view matrix as GFX_QUAD_WORLD's points are, with its rhw and depth rules. An
   upright set (the effect primitives' quads that stand up on the screen) takes x and y on the
   screen otherwise: a triangle's corner's x is the set's `upright` point's projected x plus or
   minus half the distance between the triangle's corners 0 and 1 projected (plus for corner 0,
   minus for corner 1, either for corner 2: GfxQuad's `right`), its y the projection's plus `lift`.
   The device puts a set among the frame's quads when the first triangle that names it is drawn
   and keeps the row it gave it (row, in the device's frame `frame`) for the others. */
#define GFX_CORNERS_MAX 12

struct GfxCorners {
    int count;
    float point[GFX_CORNERS_MAX][3];
    float u[GFX_CORNERS_MAX], v[GFX_CORNERS_MAX];
    float view[12];
    float proj[2], centre[2];
    int rhw, depth;
    float rhwValue, depthValue;
    uint32_t colour;
    int upright;
    float uprightPoint[3], lift;
};

struct GfxCornerSet {
    struct GfxCorners corners;
    unsigned frame;
    int row;
};

struct GfxQuad {
    int kind;
    union {
        struct {
            float x0, x1, top, bottom;
            float rhwBottom, rhwTop;
            float zBottom, zTop;
            float offsetX, offsetY;
            int turned;
            float cosine, sine;
            float u0, v0, u1, v1;
            uint32_t colour;
        } sprite;
        struct {
            float point[4][3];
            float view[12];
            float proj[2], centre[2];
            int viewSpace, rhw, depth;
            float rhwValue, depthValue;
            float z[4];
            uint32_t colour;
            float u[4], v[4];
            int stage2, flat;
            float u1[4], v1[4];
        } world;
        struct {
            struct GfxCornerSet *set;
            unsigned char corner[3];
            int right, flat;
        } corners;
    };
};

/* What the device puts into a record's header: the layer of the atlas array stage 0 reads, the
   draw's record, whether the corners move onto pixel boundaries (a flat instance in a multisampled
   pass), and the first row of what the record names in the frame's quads: for GFX_QUAD_WORLD the
   matrix it goes through (16 floats: the 4x3 view, or nothing for points in view space, then
   proj[0], proj[1], centreX and centreY), for GFX_QUAD_CORNERS its set (quad_corner_words). */
struct QuadHeader {
    uint32_t layer, draw, snap, matrix;
};

/* The most words (32 bits each) a record takes; records are a whole number of 16-byte rows. */
#define QUAD_WORDS_MAX 40

/* An instance's record into words; answers how many it took. */
int quad_words(const struct GfxQuad *quad, const struct QuadHeader *header, uint32_t *words);

/* A set of corners into words (a whole number of rows), its matrix at row `matrix` among the
   frame's quads (as GFX_QUAD_WORLD's, never in view space): a header (the rules' flags, the colour,
   the matrix's row, the word of the first corner), the rules' values where its rules take any
   (rhwValue, depthValue, lift), the upright point where it is upright, then each corner's x, y, z,
   u and v one after the other. Answers how many words it took, 0 for a count of none or more than
   GFX_CORNERS_MAX. */
#define QUAD_CORNER_WORDS_MAX (12 + 5 * GFX_CORNERS_MAX)
int quad_corner_words(const struct GfxCorners *corners, uint32_t matrix, uint32_t *words);

/* A primitive of the client's n vertices, a strip (a quad of 4) or a list (a triangle of each 3):
   how many instances it makes, 0 for a shape the client does not make. */
int quad_instance_count(int n, int strip);

/* Whether every vertex of a primitive has the same rhw, bit for bit: a pre-transformed one then
   lies flat on the screen; quad_rhw_flat the same of n rhw. */
int quad_vertices_flat(const struct TexDrawVertex *v, int n);
int quad_rhw_flat(const float *rhw, int n);

/* A primitive's instances drawn from its vertices, which the device puts into the frame's quads
   (7 words each: x, y, z, rhw, the colour, u, v) from word `first` on: the record of instance
   `index`. Answers how many words it took, 0 where the primitive makes no such instance. */
int quad_vertex_words(int n, int strip, uint32_t first, int index, const struct QuadHeader *header, uint32_t *words);

/* A strip of four pre-transformed vertices that make an axis-aligned rectangle with one rhw and
   colour, the same z along each row and texture coordinates by column and row, as a record of
   its own that holds them all: how many words it took, 0 for a strip that does not. */
int quad_rect_words(const struct TexDrawVertex *v, const struct QuadHeader *header, uint32_t *words);

#endif
