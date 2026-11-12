#ifndef RENDER_DRAW_MODES_H
#define RENDER_DRAW_MODES_H

/* custom: the ways the client draws (render/gfx.h, gfx_set_mode). Each mode is one combination of
   what the two texture stages make of the textures and the vertex colour (enum DrawCombine),
   blending, the depth test and write, the alpha test, fog and how the stages sample; the device
   makes the pipelines of every mode once, when it knows the screen's samples and depth
   (render/gfx_gpu.c, gfx_create_zbuffer). The modes are the
   render states and texture stage states the Direct3D 9 client drew each of its lists with
   (render/crenderer.c, renderer_draw_lists), and those of the few things drawn outside them.
   Pure data and logic: it uses nothing else of the client. */

enum DrawMode {
    /* the lists, in the order renderer_draw_lists draws them */
    DRAW_OPAQUE_CLAMP,      /* opaque quads and models, texture coordinates clamped */
    DRAW_OPAQUE_WRAP,       /* the same, wrapped */
    DRAW_GROUND,            /* the ground's meshes without their lightmaps */
    DRAW_GROUND_LIGHTMAP,   /* the ground's meshes with their lightmaps */
    DRAW_MULTITEXTURE,      /* two-texture particles */
    DRAW_EFFECT_DEPTH,      /* effect quads behind the scene's depth, writing none */
    DRAW_ALPHA_NO_DEPTH,    /* blended quads without depth */
    DRAW_ALPHA_DEPTH,       /* blended quads and models that write depth */
    DRAW_ADDITIVE_FOG,      /* added quads that are fogged and write depth */
    DRAW_ALPHA_CLAMP,       /* blended quads and models behind the scene's depth, writing none */
    DRAW_ALPHA_WRAP,        /* the same, texture coordinates wrapped */
    DRAW_SOLID_CLAMP,       /* a map with two-pass alpha: the nearly opaque texels, writing depth */
    DRAW_SOLID_WRAP,
    DRAW_FRINGE_CLAMP,      /* and then the rest of them, writing none */
    DRAW_FRINGE_WRAP,
    DRAW_ALPHA_CUTOUT,      /* blended quads whose faint texels are dropped */
    DRAW_ADDITIVE,          /* added quads without fog */
    DRAW_EFFECT_WRAP,       /* effect quads behind the scene's depth, texture coordinates wrapped */
    DRAW_ADDITIVE_OVERLAY,  /* added quads over everything */
    DRAW_OVERLAY,           /* blended quads over everything: the interface */
    DRAW_EFFECT_OVERLAY,    /* effect quads over everything */
    /* the rest */
    DRAW_COMPOSITE,         /* interface quads into a window's texture (render/render_clients.c) */
    DRAW_PANEL,             /* the frame-time overlay (diag/perf.c): its texture as it is, blended */
    DRAW_BLIT,              /* a rectangle of a surface copied into a target (render/gfx_gpu.c) */
    DRAW_CLEAR,             /* a rectangle of a target cleared to the vertex colour and a depth */
    DRAW_CLEAR_COLOUR,      /* a rectangle of a target cleared to the vertex colour, its depth kept */
    DRAW_CLEAR_DEPTH,       /* a rectangle of a target's depth cleared, its colour kept */
    DRAW_MODE_COUNT
};

/* What the stages make of the first texture (T), the second (U) and the vertex colour (D), colour
   and then alpha, every step clamped to 0..1 as Direct3D 9 clamped each stage. A stage without a
   texture reads opaque white. 0 to 15 are the effect presets of the effect files (ez2str_draw.c,
   rag_str_anim.c), which the Direct3D 9 client set as texture stage states; their second texture
   is the one bound last to stage 1, read at the first texture's coordinates, and presets 5 to 15
   take their alpha from it. Presets 8 and 11 make the same, but were different states. The
   presets the client names are named; the others are their numbers, which the quad shader
   (render/shaders/quad.hlsl, combine) evaluates:
     3: T + D; Da                 8, 11: U + T * D; Ua           13: U * (T + D); Ua
     4: T over D by Ta; Da        9: U over T * D by Ua; Ua      14: 2 * U * T * D; Ua
     5: U * T * D; Ua             10: U over T * D by Da; Ua     15: U + T * D - 0.5; Ua
     6: (1 - U) * (1 - T) * D; Ua 12: (1 - U) + T * D; Ua
     7: Ua * T; Ua */
enum DrawCombine {
    DRAW_COMBINE_MODULATE,          /* 0: T * D; Ta * Da */
    DRAW_COMBINE_TEXTURE_ALPHA,     /* 1 (and 2): T * D; Ta */
    DRAW_COMBINE_TEXTURE = 16,      /* T; Ta */
    DRAW_COMBINE_DIFFUSE,           /* D; Da */
    DRAW_COMBINE_TWO_TEXTURES,      /* T * U; Ta * Ua */
    DRAW_COMBINE_COUNT,
    /* a mode whose draws say their combination (gfx_set_effect) */
    DRAW_COMBINE_FROM_DRAW = 0xff
};

/* Blend factors as the client and its effect files number them: Direct3D 9's D3DBLEND. */
#define DRAW_BLEND_ZERO            1
#define DRAW_BLEND_ONE             2
#define DRAW_BLEND_SRCCOLOR        3
#define DRAW_BLEND_INVSRCCOLOR     4
#define DRAW_BLEND_SRCALPHA        5
#define DRAW_BLEND_INVSRCALPHA     6
#define DRAW_BLEND_DESTALPHA       7
#define DRAW_BLEND_INVDESTALPHA    8
#define DRAW_BLEND_DESTCOLOR       9
#define DRAW_BLEND_INVDESTCOLOR    10
#define DRAW_BLEND_SRCALPHASAT     11
#define DRAW_BLEND_BOTHSRCALPHA    12
#define DRAW_BLEND_BOTHINVSRCALPHA 13
#define DRAW_BLEND_BLENDFACTOR     14
#define DRAW_BLEND_INVBLENDFACTOR  15
/* in a mode's blend: no blending; the draw's own factors (gfx_set_effect) */
#define DRAW_BLEND_OFF             0
#define DRAW_BLEND_FROM_DRAW       0xff

/* The factors a GPU blends with: those of a target without alpha of its own, as the screen was
   (X8R8G8B8 under Direct3D 9), into which the modes that blend by a destination's alpha draw. */
enum DrawFactor {
    DRAW_FACTOR_ZERO,
    DRAW_FACTOR_ONE,
    DRAW_FACTOR_SRC_COLOUR,
    DRAW_FACTOR_INV_SRC_COLOUR,
    DRAW_FACTOR_SRC_ALPHA,
    DRAW_FACTOR_INV_SRC_ALPHA,
    DRAW_FACTOR_DST_COLOUR,
    DRAW_FACTOR_INV_DST_COLOUR,
    DRAW_FACTOR_COUNT
};

enum DrawDepth {
    DRAW_DEPTH_OFF,     /* neither tested nor written */
    DRAW_DEPTH_TEST,    /* drawn where nearer or as near, not written */
    DRAW_DEPTH_WRITE,   /* tested and written */
    DRAW_DEPTH_ALWAYS   /* written everywhere, not tested */
};

enum DrawAlphaTest {
    DRAW_ALPHA_OFF,
    DRAW_ALPHA_AT_LEAST,  /* drawn where alpha (0 to 255) is at least the reference */
    DRAW_ALPHA_BELOW      /* drawn where it is below it */
};

/* How a stage reads its texture's coordinates outside 0 to 1; with DRAW_ADDRESS_BORDER, a stage with
   a texture reads opaque black there. */
enum DrawAddress {
    DRAW_ADDRESS_CLAMP,
    DRAW_ADDRESS_WRAP,
    DRAW_ADDRESS_BORDER
};

/* How stage 0 filters (stage 1 always filters linearly between texels and reads the top level):
   linearly between texels and between mipmap levels, linearly between texels from the top level,
   or the nearest texel of the top level. Linear minification is anisotropic. */
enum DrawFilter {
    DRAW_FILTER_MIPMAPS,
    DRAW_FILTER_LINEAR,
    DRAW_FILTER_POINT
};

/* What a mode is drawn into, and with: the screen, a render target's texture, and the static
   world's records (render/gfx.h, gfx_world_draw) as well as quads, into the screen. */
#define DRAW_INTO_SCREEN  1
#define DRAW_INTO_TEXTURE 2
#define DRAW_MESHES       4

struct DrawModeDesc {
    unsigned char combine;      /* enum DrawCombine, or DRAW_COMBINE_FROM_DRAW */
    unsigned char src, dst;     /* DRAW_BLEND_*: both DRAW_BLEND_OFF, or both DRAW_BLEND_FROM_DRAW */
    unsigned char depth;        /* enum DrawDepth */
    unsigned char alphaTest;    /* enum DrawAlphaTest */
    unsigned char alphaRef;
    unsigned char fog;          /* fogged while the frame's fog is on (render/gfx.h, gfx_set_fog) */
    unsigned char colour;       /* writes colour */
    unsigned char address[2];   /* enum DrawAddress of stage 0 and stage 1 */
    unsigned char filter;       /* enum DrawFilter of stage 0 */
    unsigned char lightmap;     /* the ground's lightmap stage (render/gfx.h, GFX_MESH_GROUND) */
    unsigned char into;         /* DRAW_INTO_* and DRAW_MESHES */
};

/* The mode's description, NULL for a number that is none. */
const struct DrawModeDesc *draw_mode(int mode);

/* The combination an effect preset draws with: 0 to 15 their own, 2 as 1; any other number keeps
   `current`, as the Direct3D 9 client's states stayed what the preset before had set. */
int draw_effect_combine(int preset, int current);

/* A blend's D3DBLEND factors as the GPU's, for a target without alpha of its own, whose alpha
   Direct3D 9 read as 1: DESTALPHA is ONE, INVDESTALPHA and SRCALPHASAT ZERO. A source factor of
   BOTHSRCALPHA or BOTHINVSRCALPHA sets both factors; BLENDFACTOR is ONE and INVBLENDFACTOR ZERO,
   Direct3D 9's blend factor being white; numbers that are no factor count as ONE. */
void draw_blend_factors(int src, int dst, int *srcFactor, int *dstFactor);

#endif
