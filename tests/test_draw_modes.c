/* render/draw_modes.c: the table of modes against the render states the Direct3D 9 client set for
   each of its lists, replayed from its renderer_draw_lists (render/crenderer.c before the modes,
   02fc9e5e), the effect presets' combinations and Direct3D 9's blend factors as a GPU's. */
#include "render/draw_modes.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

/* ---- the table ---- */

static void table(void)
{
    int mode;
    assert(!draw_mode(-1) && !draw_mode(DRAW_MODE_COUNT));
    for (mode = 0; mode < DRAW_MODE_COUNT; mode++) {
        const struct DrawModeDesc *m = draw_mode(mode);
        int effect;
        assert(m);
        effect = m->combine == DRAW_COMBINE_FROM_DRAW;
        assert(effect || (m->combine < DRAW_COMBINE_COUNT && m->combine != 2));
        /* blending off, the draw's, or a pair of factors */
        assert((m->src == DRAW_BLEND_OFF) == (m->dst == DRAW_BLEND_OFF));
        assert((m->src == DRAW_BLEND_FROM_DRAW) == (m->dst == DRAW_BLEND_FROM_DRAW));
        assert(m->src == DRAW_BLEND_OFF || m->src == DRAW_BLEND_FROM_DRAW
               || (m->src >= DRAW_BLEND_ZERO && m->src <= DRAW_BLEND_INVBLENDFACTOR));
        assert(m->dst == DRAW_BLEND_OFF || m->dst == DRAW_BLEND_FROM_DRAW
               || (m->dst >= DRAW_BLEND_ZERO && m->dst <= DRAW_BLEND_INVBLENDFACTOR));
        /* the effect modes take both from the draw, draw into the screen alone (whose factors the
           device has pipelines for) and test depth or not at all */
        assert(effect == (m->src == DRAW_BLEND_FROM_DRAW));
        assert(!effect || (m->into == DRAW_INTO_SCREEN && (m->depth == DRAW_DEPTH_TEST || m->depth == DRAW_DEPTH_OFF)));
        assert(m->depth <= DRAW_DEPTH_ALWAYS && m->alphaTest <= DRAW_ALPHA_BELOW);
        assert(m->alphaTest != DRAW_ALPHA_OFF || m->alphaRef == 0);
        assert(m->address[0] <= DRAW_ADDRESS_BORDER && m->address[1] <= DRAW_ADDRESS_BORDER);
        assert(m->filter <= DRAW_FILTER_POINT && m->fog <= 1 && m->colour <= 1 && m->lightmap <= 1);
        assert(m->into && !(m->into & ~(DRAW_INTO_SCREEN | DRAW_INTO_TEXTURE | DRAW_MESHES)));
        /* meshes draw into the screen, and only the ground's modes have its lightmap */
        assert(!m->lightmap || (m->into == DRAW_MESHES && mode == DRAW_GROUND_LIGHTMAP));
    }
}

/* ---- the effect presets ---- */

static void presets(void)
{
    int preset, current;
    for (current = 0; current <= 15; current++) {
        if (current == 2)
            continue;
        for (preset = -3; preset < 40; preset++) {
            int c = draw_effect_combine(preset, current);
            if (preset == 2)
                assert(c == DRAW_COMBINE_TEXTURE_ALPHA);
            else if (preset >= 0 && preset <= 15)
                assert(c == preset);
            else
                assert(c == current);
        }
    }
    assert(draw_effect_combine(0x7fffffff, 9) == 9 && draw_effect_combine(-0x7fffffff - 1, 4) == 4);
}

/* ---- blend factors ---- */

static void blend_factors(void)
{
    static const int screen[16] = { DRAW_FACTOR_ONE, DRAW_FACTOR_ZERO, DRAW_FACTOR_ONE, DRAW_FACTOR_SRC_COLOUR,
                                    DRAW_FACTOR_INV_SRC_COLOUR, DRAW_FACTOR_SRC_ALPHA, DRAW_FACTOR_INV_SRC_ALPHA,
                                    DRAW_FACTOR_ONE, DRAW_FACTOR_ZERO, DRAW_FACTOR_DST_COLOUR,
                                    DRAW_FACTOR_INV_DST_COLOUR, DRAW_FACTOR_ZERO, -1, -1, DRAW_FACTOR_ONE,
                                    DRAW_FACTOR_ZERO };
    static const int whole[] = {0x105, 0x7fffffff, -0x7fffffff - 1, 0x100, 0x10000 + 5};
    int src, dst, s, d;
    size_t i;
    for (src = -2; src < 300; src++)
        for (dst = -2; dst < 300; dst++) {
            draw_blend_factors(src, dst, &s, &d);
            assert(s >= 0 && s < DRAW_FACTOR_COUNT && d >= 0 && d < DRAW_FACTOR_COUNT);
            if (src == DRAW_BLEND_BOTHSRCALPHA)
                assert(s == DRAW_FACTOR_SRC_ALPHA && d == DRAW_FACTOR_INV_SRC_ALPHA);
            else if (src == DRAW_BLEND_BOTHINVSRCALPHA)
                assert(s == DRAW_FACTOR_INV_SRC_ALPHA && d == DRAW_FACTOR_SRC_ALPHA);
            else if (src >= 0 && src < 16)
                assert(s == screen[src]);
            else
                assert(s == DRAW_FACTOR_ONE);
            if (dst >= 0 && dst < 16 && src != DRAW_BLEND_BOTHSRCALPHA && src != DRAW_BLEND_BOTHINVSRCALPHA)
                assert(screen[dst] < 0 ? d == (dst == DRAW_BLEND_BOTHSRCALPHA ? DRAW_FACTOR_INV_SRC_ALPHA
                                                                              : DRAW_FACTOR_SRC_ALPHA)
                                       : d == screen[dst]);
        }
    /* a number is taken whole: one whose low byte is a factor is no factor */
    for (i = 0; i < sizeof whole / sizeof whole[0]; i++) {
        draw_blend_factors(whole[i], whole[i], &s, &d);
        assert(s == DRAW_FACTOR_ONE && d == DRAW_FACTOR_ONE);
    }
}

/* ---- the Direct3D 9 client's states, replayed ---- */

/* The states that make a mode, as the Direct3D 9 client kept them: depth test (Z) and write (ZW),
   alpha test (AT) by function and reference, blending (AB) by factors, fog, the stages'
   combination (STAGES: a DrawCombine) and addressing, stage 0's mip filter. */
enum { Z, ZW, AT, FUNC, REF, AB, SRC, DST, FOG, STAGES, ADDR0, ADDR1, MIP0, NSTATE };
enum { GE = 7, LESS = 2 };   /* D3DCMP_GREATEREQUAL, D3DCMP_LESS */
enum { MIP_NONE = 1, MIP_LINEAR = 3 };

#define SET(s, v) { 0, s, v }
#define DRAW(mode) { 1, mode, 0 }
#define EFFECT(mode) { 2, mode, 0 }
#define SAVE(s) { 3, s, 0 }
#define RESTORE(s) { 4, s, 0 }
#define TWO_PASS { 5, 0, 0 }
#define ONE_PASS { 6, 0, 0 }
#define BOTH { 7, 0, 0 }
#define END { 8, 0, 0 }

struct step {
    int op, a, b;
};

/* renderer_draw_lists, the interface's composites (render/render_clients.c) and the frame-time
   overlay (diag/perf.c), from a frame after the last; the states the Direct3D 9 client's device
   started with were set by gfx_default_states. */
static const struct step frame[] = {
    SET(MIP0, MIP_LINEAR), SET(STAGES, DRAW_COMBINE_MODULATE), SET(FOG, 1), SET(REF, 0xcf), SET(AT, 1),
    SET(AB, 0), SET(Z, 1), SET(ZW, 1),
    SET(ADDR0, DRAW_ADDRESS_CLAMP), DRAW(DRAW_OPAQUE_CLAMP), SET(ADDR0, DRAW_ADDRESS_WRAP),
    DRAW(DRAW_OPAQUE_WRAP), SET(ADDR0, DRAW_ADDRESS_CLAMP),
    DRAW(DRAW_GROUND), DRAW(DRAW_GROUND_LIGHTMAP),
    SET(REF, 1), SET(AB, 1),
    SET(STAGES, DRAW_COMBINE_TWO_TEXTURES), SET(ADDR0, DRAW_ADDRESS_WRAP), SET(ADDR1, DRAW_ADDRESS_CLAMP),
    DRAW(DRAW_MULTITEXTURE),
    SET(STAGES, DRAW_COMBINE_MODULATE), SET(ADDR0, DRAW_ADDRESS_CLAMP), SET(ADDR1, DRAW_ADDRESS_BORDER),
    SAVE(FOG), SAVE(ZW), SET(FOG, 0), SET(ZW, 0), SET(SRC, DRAW_BLEND_ONE), SET(DST, DRAW_BLEND_ONE),
    EFFECT(DRAW_EFFECT_DEPTH),
    RESTORE(FOG), RESTORE(ZW), SET(SRC, DRAW_BLEND_SRCALPHA), SET(DST, DRAW_BLEND_INVSRCALPHA), SET(Z, 0),
    DRAW(DRAW_ALPHA_NO_DEPTH), SET(Z, 1), DRAW(DRAW_ALPHA_DEPTH),
    SET(SRC, DRAW_BLEND_ONE), SET(DST, DRAW_BLEND_ONE), DRAW(DRAW_ADDITIVE_FOG),
    SET(ZW, 0), SET(SRC, DRAW_BLEND_SRCALPHA), SET(DST, DRAW_BLEND_INVSRCALPHA),
    TWO_PASS,
    SAVE(ZW), SAVE(AT), SAVE(REF), SAVE(FUNC), SET(AT, 1), SET(ZW, 1), SET(REF, 0xf0), SET(FUNC, GE),
    SET(ADDR0, DRAW_ADDRESS_CLAMP), DRAW(DRAW_SOLID_CLAMP), SET(ADDR0, DRAW_ADDRESS_WRAP), DRAW(DRAW_SOLID_WRAP),
    SET(ADDR0, DRAW_ADDRESS_CLAMP), SET(ZW, 0), SET(REF, 0xf0), SET(FUNC, LESS),
    SET(ADDR0, DRAW_ADDRESS_CLAMP), DRAW(DRAW_FRINGE_CLAMP), SET(ADDR0, DRAW_ADDRESS_WRAP),
    DRAW(DRAW_FRINGE_WRAP), SET(ADDR0, DRAW_ADDRESS_CLAMP), SET(REF, 1), SET(FUNC, GE),
    DRAW(DRAW_ALPHA_CLAMP), RESTORE(ZW), RESTORE(AT), RESTORE(REF), RESTORE(FUNC),
    ONE_PASS,
    SET(ADDR0, DRAW_ADDRESS_CLAMP), DRAW(DRAW_ALPHA_CLAMP), SET(ADDR0, DRAW_ADDRESS_WRAP), DRAW(DRAW_ALPHA_WRAP),
    SET(ADDR0, DRAW_ADDRESS_CLAMP),
    BOTH,
    SAVE(ZW), SET(ZW, 1), DRAW(DRAW_ALPHA_DEPTH), RESTORE(ZW),
    SAVE(REF), SET(REF, 0xcf), DRAW(DRAW_ALPHA_CUTOUT), RESTORE(REF),
    SET(FOG, 0), SET(SRC, DRAW_BLEND_ONE), SET(DST, DRAW_BLEND_ONE), DRAW(DRAW_ADDITIVE),
    SET(ADDR0, DRAW_ADDRESS_WRAP), EFFECT(DRAW_EFFECT_WRAP), SET(ADDR0, DRAW_ADDRESS_CLAMP),
    SET(Z, 0), SET(SRC, DRAW_BLEND_ONE), SET(DST, DRAW_BLEND_ONE), DRAW(DRAW_ADDITIVE_OVERLAY),
    SET(SRC, DRAW_BLEND_SRCALPHA), SET(DST, DRAW_BLEND_INVSRCALPHA), DRAW(DRAW_OVERLAY),
    EFFECT(DRAW_EFFECT_OVERLAY),
    SET(SRC, DRAW_BLEND_SRCALPHA), SET(DST, DRAW_BLEND_INVSRCALPHA), SET(MIP0, MIP_NONE),
    /* a window composited between frames, without depth and fog */
    SAVE(Z), SAVE(FOG), SET(Z, 0), SET(FOG, 0), DRAW(DRAW_COMPOSITE), RESTORE(Z), RESTORE(FOG),
    /* the frame-time overlay: its texture as it is, blended */
    SAVE(Z), SAVE(AB), SAVE(AT), SAVE(FOG), SAVE(SRC), SAVE(DST), SET(Z, 0), SET(AB, 1), SET(AT, 0),
    SET(FOG, 0), SET(SRC, DRAW_BLEND_SRCALPHA), SET(DST, DRAW_BLEND_INVSRCALPHA),
    SET(STAGES, DRAW_COMBINE_TEXTURE), DRAW(DRAW_PANEL), SET(STAGES, DRAW_COMBINE_MODULATE),
    RESTORE(Z), RESTORE(AB), RESTORE(AT), RESTORE(FOG), RESTORE(SRC), RESTORE(DST),
    END
};

/* Blits and clears were no draws under Direct3D 9 (StretchRect, UpdateSurface, Clear): their modes
   copy a texture's nearest texel or write the vertex colour and depth, over everything, into every
   target that has what they write. */
static void copies(void)
{
    static const int modes[] = {DRAW_BLIT, DRAW_CLEAR, DRAW_CLEAR_COLOUR, DRAW_CLEAR_DEPTH};
    size_t i;
    for (i = 0; i < sizeof modes / sizeof modes[0]; i++) {
        const struct DrawModeDesc *m = draw_mode(modes[i]);
        int depth = modes[i] == DRAW_CLEAR || modes[i] == DRAW_CLEAR_DEPTH;
        assert(m->src == DRAW_BLEND_OFF && m->alphaTest == DRAW_ALPHA_OFF && !m->fog && !m->lightmap);
        assert(m->filter == DRAW_FILTER_POINT);
        assert(m->combine == (modes[i] == DRAW_BLIT ? DRAW_COMBINE_TEXTURE : DRAW_COMBINE_DIFFUSE));
        assert(m->depth == (depth ? DRAW_DEPTH_ALWAYS : DRAW_DEPTH_OFF));
        assert(m->colour == (modes[i] != DRAW_CLEAR_DEPTH));
        /* depth is the screen's alone */
        assert(m->into == (depth ? DRAW_INTO_SCREEN : (DRAW_INTO_SCREEN | DRAW_INTO_TEXTURE)));
    }
}

/* The mode as those states make it; the effect modes' blending is the draw's, and the lists start
   with what the states held. */
static void check_mode(const unsigned *s, int mode, int effect)
{
    const struct DrawModeDesc *m = draw_mode(mode);
    int depth = !s[Z] ? DRAW_DEPTH_OFF : s[ZW] ? DRAW_DEPTH_WRITE : DRAW_DEPTH_TEST;
    int alpha = !s[AT] ? DRAW_ALPHA_OFF : s[FUNC] == GE ? DRAW_ALPHA_AT_LEAST : DRAW_ALPHA_BELOW;
    assert(m->depth == depth);
    assert(m->alphaTest == alpha && (alpha == DRAW_ALPHA_OFF || m->alphaRef == s[REF]));
    assert(m->fog == s[FOG]);
    assert(m->address[0] == s[ADDR0] && m->address[1] == s[ADDR1]);
    assert(m->filter == (s[MIP0] == MIP_LINEAR ? DRAW_FILTER_MIPMAPS : DRAW_FILTER_LINEAR));
    if (effect) {
        /* the list starts with the blending the states held and preset 0 */
        assert(m->combine == DRAW_COMBINE_FROM_DRAW && m->src == DRAW_BLEND_FROM_DRAW && s[AB]);
        assert(s[STAGES] == DRAW_COMBINE_MODULATE);
        return;
    }
    assert(s[AB] ? m->src == s[SRC] && m->dst == s[DST] : m->src == DRAW_BLEND_OFF);
    /* the ground's meshes do their stages themselves (render/gfx.h, GFX_MESH_GROUND) */
    assert(m->into == DRAW_MESHES || m->combine == s[STAGES]);
}

static void replay(int twoPass)
{
    unsigned s[NSTATE], saved[NSTATE];
    const struct step *p;
    int skipping = 0, drawn[DRAW_MODE_COUNT];
    memset(s, 0, sizeof s);
    memset(drawn, 0, sizeof drawn);
    /* the device's start (gfx_default_states) and the end of the frame before */
    s[FUNC] = GE;
    s[REF] = 0xf0;
    s[Z] = s[ZW] = 1;
    s[SRC] = DRAW_BLEND_SRCALPHA;
    s[DST] = DRAW_BLEND_INVSRCALPHA;
    s[STAGES] = DRAW_COMBINE_MODULATE;
    s[ADDR0] = DRAW_ADDRESS_CLAMP;
    s[ADDR1] = DRAW_ADDRESS_BORDER;
    s[MIP0] = MIP_NONE;
    for (p = frame; p->op != 8; p++) {
        if (p->op == 5 || p->op == 6 || p->op == 7) {
            skipping = (p->op == 5 && !twoPass) || (p->op == 6 && twoPass);
            continue;
        }
        if (skipping)
            continue;
        switch (p->op) {
        case 0: s[p->a] = (unsigned)p->b; break;
        case 1: check_mode(s, p->a, 0); drawn[p->a] = 1; break;
        case 2: check_mode(s, p->a, 1); drawn[p->a] = 1; break;
        case 3: saved[p->a] = s[p->a]; break;
        case 4: s[p->a] = saved[p->a]; break;
        }
    }
    /* every mode of the lists and of the interface's draws is drawn in */
    if (twoPass) {
        int mode;
        for (mode = 0; mode <= DRAW_PANEL; mode++)
            assert(drawn[mode] || mode == DRAW_ALPHA_WRAP);
    }
}

int main(void)
{
    table();
    presets();
    blend_factors();
    replay(0);
    replay(1);
    copies();
    puts("Draw modes: the table, the effect presets, the blend factors, every list's mode against the Direct3D 9 "
         "client's states, with and without two-pass alpha, the blits' and the clears' passed.");
    return 0;
}
