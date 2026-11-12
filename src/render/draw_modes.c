#include "render/draw_modes.h"

#define SA DRAW_BLEND_SRCALPHA
#define ISA DRAW_BLEND_INVSRCALPHA
#define ONE DRAW_BLEND_ONE
#define OFF DRAW_BLEND_OFF
#define BYDRAW DRAW_BLEND_FROM_DRAW
#define CLAMP DRAW_ADDRESS_CLAMP
#define WRAP DRAW_ADDRESS_WRAP
#define BORDER DRAW_ADDRESS_BORDER
#define SCREEN DRAW_INTO_SCREEN
#define ANY (DRAW_INTO_SCREEN | DRAW_INTO_TEXTURE)
#define MESHES DRAW_MESHES

/* The Direct3D 9 client's states for each (render/crenderer.c): every list filters stage 0 with
   mipmaps and stage 1 reads with a border unless it says otherwise; blending, the alpha test and
   fog as renderer_draw_lists set them before the list. The interface's windows were composited
   with the states the frame's last list left, without depth and fog; the frame-time overlay with
   its own. */
static const struct DrawModeDesc modes[DRAW_MODE_COUNT] = {
    /*                        combine                       src     dst     depth              alpha test            ref   fog colour address        filter               lightmap into */
    [DRAW_OPAQUE_CLAMP]     = { DRAW_COMBINE_MODULATE,      OFF,    OFF,    DRAW_DEPTH_WRITE,  DRAW_ALPHA_AT_LEAST, 0xcf, 1, 1, { CLAMP, BORDER }, DRAW_FILTER_MIPMAPS, 0, SCREEN | MESHES },
    [DRAW_OPAQUE_WRAP]      = { DRAW_COMBINE_MODULATE,      OFF,    OFF,    DRAW_DEPTH_WRITE,  DRAW_ALPHA_AT_LEAST, 0xcf, 1, 1, { WRAP, BORDER },  DRAW_FILTER_MIPMAPS, 0, SCREEN | MESHES },
    [DRAW_GROUND]           = { DRAW_COMBINE_MODULATE,      OFF,    OFF,    DRAW_DEPTH_WRITE,  DRAW_ALPHA_AT_LEAST, 0xcf, 1, 1, { CLAMP, BORDER }, DRAW_FILTER_MIPMAPS, 0, MESHES },
    [DRAW_GROUND_LIGHTMAP]  = { DRAW_COMBINE_MODULATE,      OFF,    OFF,    DRAW_DEPTH_WRITE,  DRAW_ALPHA_AT_LEAST, 0xcf, 1, 1, { CLAMP, BORDER }, DRAW_FILTER_MIPMAPS, 1, MESHES },
    [DRAW_MULTITEXTURE]     = { DRAW_COMBINE_TWO_TEXTURES,  SA,     ISA,    DRAW_DEPTH_WRITE,  DRAW_ALPHA_AT_LEAST, 1,    1, 1, { WRAP, CLAMP },   DRAW_FILTER_MIPMAPS, 0, SCREEN },
    [DRAW_EFFECT_DEPTH]     = { DRAW_COMBINE_FROM_DRAW,     BYDRAW, BYDRAW, DRAW_DEPTH_TEST,   DRAW_ALPHA_AT_LEAST, 1,    0, 1, { CLAMP, BORDER }, DRAW_FILTER_MIPMAPS, 0, SCREEN },
    [DRAW_ALPHA_NO_DEPTH]   = { DRAW_COMBINE_MODULATE,      SA,     ISA,    DRAW_DEPTH_OFF,    DRAW_ALPHA_AT_LEAST, 1,    1, 1, { CLAMP, BORDER }, DRAW_FILTER_MIPMAPS, 0, SCREEN },
    [DRAW_ALPHA_DEPTH]      = { DRAW_COMBINE_MODULATE,      SA,     ISA,    DRAW_DEPTH_WRITE,  DRAW_ALPHA_AT_LEAST, 1,    1, 1, { CLAMP, BORDER }, DRAW_FILTER_MIPMAPS, 0, SCREEN | MESHES },
    [DRAW_ADDITIVE_FOG]     = { DRAW_COMBINE_MODULATE,      ONE,    ONE,    DRAW_DEPTH_WRITE,  DRAW_ALPHA_AT_LEAST, 1,    1, 1, { CLAMP, BORDER }, DRAW_FILTER_MIPMAPS, 0, SCREEN },
    [DRAW_ALPHA_CLAMP]      = { DRAW_COMBINE_MODULATE,      SA,     ISA,    DRAW_DEPTH_TEST,   DRAW_ALPHA_AT_LEAST, 1,    1, 1, { CLAMP, BORDER }, DRAW_FILTER_MIPMAPS, 0, SCREEN | MESHES },
    [DRAW_ALPHA_WRAP]       = { DRAW_COMBINE_MODULATE,      SA,     ISA,    DRAW_DEPTH_TEST,   DRAW_ALPHA_AT_LEAST, 1,    1, 1, { WRAP, BORDER },  DRAW_FILTER_MIPMAPS, 0, SCREEN | MESHES },
    [DRAW_SOLID_CLAMP]      = { DRAW_COMBINE_MODULATE,      SA,     ISA,    DRAW_DEPTH_WRITE,  DRAW_ALPHA_AT_LEAST, 0xf0, 1, 1, { CLAMP, BORDER }, DRAW_FILTER_MIPMAPS, 0, SCREEN | MESHES },
    [DRAW_SOLID_WRAP]       = { DRAW_COMBINE_MODULATE,      SA,     ISA,    DRAW_DEPTH_WRITE,  DRAW_ALPHA_AT_LEAST, 0xf0, 1, 1, { WRAP, BORDER },  DRAW_FILTER_MIPMAPS, 0, SCREEN | MESHES },
    [DRAW_FRINGE_CLAMP]     = { DRAW_COMBINE_MODULATE,      SA,     ISA,    DRAW_DEPTH_TEST,   DRAW_ALPHA_BELOW,    0xf0, 1, 1, { CLAMP, BORDER }, DRAW_FILTER_MIPMAPS, 0, SCREEN | MESHES },
    [DRAW_FRINGE_WRAP]      = { DRAW_COMBINE_MODULATE,      SA,     ISA,    DRAW_DEPTH_TEST,   DRAW_ALPHA_BELOW,    0xf0, 1, 1, { WRAP, BORDER },  DRAW_FILTER_MIPMAPS, 0, SCREEN | MESHES },
    [DRAW_ALPHA_CUTOUT]     = { DRAW_COMBINE_MODULATE,      SA,     ISA,    DRAW_DEPTH_TEST,   DRAW_ALPHA_AT_LEAST, 0xcf, 1, 1, { CLAMP, BORDER }, DRAW_FILTER_MIPMAPS, 0, SCREEN },
    [DRAW_ADDITIVE]         = { DRAW_COMBINE_MODULATE,      ONE,    ONE,    DRAW_DEPTH_TEST,   DRAW_ALPHA_AT_LEAST, 1,    0, 1, { CLAMP, BORDER }, DRAW_FILTER_MIPMAPS, 0, SCREEN },
    [DRAW_EFFECT_WRAP]      = { DRAW_COMBINE_FROM_DRAW,     BYDRAW, BYDRAW, DRAW_DEPTH_TEST,   DRAW_ALPHA_AT_LEAST, 1,    0, 1, { WRAP, BORDER },  DRAW_FILTER_MIPMAPS, 0, SCREEN },
    [DRAW_ADDITIVE_OVERLAY] = { DRAW_COMBINE_MODULATE,      ONE,    ONE,    DRAW_DEPTH_OFF,    DRAW_ALPHA_AT_LEAST, 1,    0, 1, { CLAMP, BORDER }, DRAW_FILTER_MIPMAPS, 0, SCREEN },
    [DRAW_OVERLAY]          = { DRAW_COMBINE_MODULATE,      SA,     ISA,    DRAW_DEPTH_OFF,    DRAW_ALPHA_AT_LEAST, 1,    0, 1, { CLAMP, BORDER }, DRAW_FILTER_MIPMAPS, 0, SCREEN },
    [DRAW_EFFECT_OVERLAY]   = { DRAW_COMBINE_FROM_DRAW,     BYDRAW, BYDRAW, DRAW_DEPTH_OFF,    DRAW_ALPHA_AT_LEAST, 1,    0, 1, { CLAMP, BORDER }, DRAW_FILTER_MIPMAPS, 0, SCREEN },
    [DRAW_COMPOSITE]        = { DRAW_COMBINE_MODULATE,      SA,     ISA,    DRAW_DEPTH_OFF,    DRAW_ALPHA_AT_LEAST, 1,    0, 1, { CLAMP, BORDER }, DRAW_FILTER_LINEAR,  0, DRAW_INTO_TEXTURE },
    [DRAW_PANEL]            = { DRAW_COMBINE_TEXTURE,       SA,     ISA,    DRAW_DEPTH_OFF,    DRAW_ALPHA_OFF,      0,    0, 1, { CLAMP, BORDER }, DRAW_FILTER_LINEAR,  0, SCREEN },
    [DRAW_BLIT]             = { DRAW_COMBINE_TEXTURE,       OFF,    OFF,    DRAW_DEPTH_OFF,    DRAW_ALPHA_OFF,      0,    0, 1, { CLAMP, BORDER }, DRAW_FILTER_POINT,   0, ANY },
    [DRAW_CLEAR]            = { DRAW_COMBINE_DIFFUSE,       OFF,    OFF,    DRAW_DEPTH_ALWAYS, DRAW_ALPHA_OFF,      0,    0, 1, { CLAMP, BORDER }, DRAW_FILTER_POINT,   0, SCREEN },
    [DRAW_CLEAR_COLOUR]     = { DRAW_COMBINE_DIFFUSE,       OFF,    OFF,    DRAW_DEPTH_OFF,    DRAW_ALPHA_OFF,      0,    0, 1, { CLAMP, BORDER }, DRAW_FILTER_POINT,   0, ANY },
    [DRAW_CLEAR_DEPTH]      = { DRAW_COMBINE_DIFFUSE,       OFF,    OFF,    DRAW_DEPTH_ALWAYS, DRAW_ALPHA_OFF,      0,    0, 0, { CLAMP, BORDER }, DRAW_FILTER_POINT,   0, SCREEN },
};

const struct DrawModeDesc *draw_mode(int mode)
{
    return mode >= 0 && mode < DRAW_MODE_COUNT ? &modes[mode] : 0;
}

int draw_effect_combine(int preset, int current)
{
    if (preset == 2)
        return DRAW_COMBINE_TEXTURE_ALPHA;
    return preset >= 0 && preset <= 15 ? preset : current;
}

static int factor(int blend)
{
    switch (blend) {
    case DRAW_BLEND_ZERO:
    case DRAW_BLEND_INVBLENDFACTOR:
        return DRAW_FACTOR_ZERO;
    case DRAW_BLEND_SRCCOLOR:
        return DRAW_FACTOR_SRC_COLOUR;
    case DRAW_BLEND_INVSRCCOLOR:
        return DRAW_FACTOR_INV_SRC_COLOUR;
    case DRAW_BLEND_SRCALPHA:
        return DRAW_FACTOR_SRC_ALPHA;
    case DRAW_BLEND_INVSRCALPHA:
        return DRAW_FACTOR_INV_SRC_ALPHA;
    case DRAW_BLEND_DESTALPHA:
        return DRAW_FACTOR_ONE;
    case DRAW_BLEND_INVDESTALPHA:
        return DRAW_FACTOR_ZERO;
    case DRAW_BLEND_DESTCOLOR:
        return DRAW_FACTOR_DST_COLOUR;
    case DRAW_BLEND_INVDESTCOLOR:
        return DRAW_FACTOR_INV_DST_COLOUR;
    case DRAW_BLEND_SRCALPHASAT:
        /* min(source alpha, 1 - destination alpha), 0 where that alpha is 1 */
        return DRAW_FACTOR_ZERO;
    /* only meaningful as a source factor, which sets both; as a destination factor the source's
       half of it */
    case DRAW_BLEND_BOTHSRCALPHA:
        return DRAW_FACTOR_INV_SRC_ALPHA;
    case DRAW_BLEND_BOTHINVSRCALPHA:
        return DRAW_FACTOR_SRC_ALPHA;
    default:
        return DRAW_FACTOR_ONE;
    }
}

void draw_blend_factors(int src, int dst, int *srcFactor, int *dstFactor)
{
    if (src == DRAW_BLEND_BOTHSRCALPHA) {
        *srcFactor = DRAW_FACTOR_SRC_ALPHA;
        *dstFactor = DRAW_FACTOR_INV_SRC_ALPHA;
    } else if (src == DRAW_BLEND_BOTHINVSRCALPHA) {
        *srcFactor = DRAW_FACTOR_INV_SRC_ALPHA;
        *dstFactor = DRAW_FACTOR_SRC_ALPHA;
    } else {
        *srcFactor = factor(src);
        *dstFactor = factor(dst);
    }
}
