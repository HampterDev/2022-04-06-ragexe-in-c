#include "render/renderer.h"
#include "render/render_api.h"
#include "diag/perf.h"
#include "ui/window_types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "platform/window.h"
#include "platform/clock.h"
#include "render/fonts.h"

/* the one GPU driver the device asks SDL for (render/gpu_device.c) */
#ifdef __APPLE__
#define PERF_RENDERER "Metal"
#else
#define PERF_RENDERER "Vulkan"
#endif

#define OV_TEX_W 1024
#define OV_TEX_H 128
#define OV_LINES 7
#define OV_LINE_CHARS 200
#define OV_MARGIN 8

static const char *const phase_name[PERF_N] = {
    "frame", "scene", "destroy", "endscene", "overlay", "present", "text", "texlock",
    "l_ground", "l_water", "l_multitexture", "l_blended_nodepth", "l_alpha_nodepth", "l_alpha_depth", "l_additive_fog", "l_alphatest",
    "l_alpha_writedepth", "l_alpha_cutout", "l_additive", "l_blended_wrap", "l_additive_overlay", "l_overlay_ordered", "l_blended_overlay",
};

static struct {
    int inited, enabled, shown, lastScroll;
    LONGLONG freq;
    LONGLONG t0[PERF_N];
    LONGLONG acc[PERF_N];
    LONGLONG frameMax;
    LONGLONG lastFrameEnd, intervalStart, start;
    unsigned frames;
    struct {
        int ms10[PERF_N];
        int frameMax10, fps10, logic10;
        unsigned frames;
        struct perf_counters per;
        int valid;
    } snap;
    FontHandle font;
    DWORD *bits;
    int charW, lineH;
    gfx_surface *tex;
    int usedW, usedH, dirty;
    char lines[OV_LINES][OV_LINE_CHARS];
    int nlines;
} P;

static LONGLONG now(void)
{
    return platform_perf_counter();
}

static void init(void);

/* The overlay is a developer tool and is off unless RAGEXE_OVERLAY is set to something
   other than 0. Off, it times and draws nothing (the counters in perf.h still count), and
   Scroll Lock, which players press for screenshots, does not show it. */
static int overlay_enabled(void)
{
    if (!P.inited)
        init();
    return P.enabled;
}

void perf_begin(int phase)
{
    if (!overlay_enabled())
        return;
    P.t0[phase] = now();
}

void perf_end(int phase)
{
    if (!overlay_enabled())
        return;
    P.acc[phase] += now() - P.t0[phase];
}

static int ms10_of(LONGLONG ticks)
{
    return (int)((double)ticks * 10000.0 / (double)P.freq);
}

static void init(void)
{
    const char *overlay = getenv("RAGEXE_OVERLAY");
    P.inited = 1;
    P.freq = platform_perf_frequency();
    if (overlay && overlay[0] && overlay[0] != '0')
        P.enabled = P.shown = 1;
    P.lastScroll = platform_key_state(VK_SCROLL) & 1;
    P.start = now();
}

static const char *tenths(char *buf, int v10)
{
    if (v10 < 0)
        v10 = 0;
    sprintf(buf, "%d.%d", v10 / 10, v10 % 10);
    return buf;
}

static void build_lines(void)
{
    char b1[16], b2[16], b3[16], b4[16], b5[16];
    const struct perf_counters *c = &P.snap.per;
    unsigned f = P.snap.frames ? P.snap.frames : 1;
    int order[PERF_N], nl = 0, i, j, k;
    char *s;

    snprintf(P.lines[0], sizeof P.lines[0], "%s  FPS %s  frame %s ms (max %s)  frames %u", PERF_RENDERER,
             tenths(b1, P.snap.fps10), tenths(b2, P.snap.ms10[PERF_FRAME]),
             tenths(b3, P.snap.frameMax10), P.snap.frames);
    snprintf(P.lines[1], sizeof P.lines[1], "scene %s  destroy %s  endscene %s",
             tenths(b1, P.snap.ms10[PERF_SCENE]), tenths(b2, P.snap.ms10[PERF_DESTROY]),
             tenths(b3, P.snap.ms10[PERF_ENDSCENE]));
    snprintf(P.lines[2], sizeof P.lines[2], "present %s (the blank wait)  logic %s (text %s, surface locks %s)  overlay %s",
             tenths(b1, P.snap.ms10[PERF_PRESENT]), tenths(b2, P.snap.logic10),
             tenths(b3, P.snap.ms10[PERF_TEXT]), tenths(b4, P.snap.ms10[PERF_TEXLOCK]), tenths(b5, P.snap.ms10[PERF_OVERLAY]));
    for (i = PERF_L_GROUND; i < PERF_N; i++)
        order[nl++] = i;
    for (i = 0; i < nl; i++)
        for (j = i + 1; j < nl; j++)
            if (P.snap.ms10[order[j]] > P.snap.ms10[order[i]]) {
                k = order[i]; order[i] = order[j]; order[j] = k;
            }
    s = P.lines[3];
    strcpy(s, "lists:");
    for (i = 0; i < 5 && i < nl; i++) {
        snprintf(s + strlen(s), sizeof P.lines[3] - strlen(s), "  %s %s", phase_name[order[i]] + 2, tenths(b1, P.snap.ms10[order[i]]));
    }
    snprintf(P.lines[4], sizeof P.lines[4],
             "draws %u (%u calls, %u indirect)  prims %u  modes %u sent / %u skipped  tex sets %u  locks %u",
             c->draws / f, c->drawCalls / f, c->indirectDraws / f, c->prims / f, c->modesSent / f,
             c->modesSkipped / f, c->texSets / f, c->locks / f);
    snprintf(P.lines[5], sizeof P.lines[5], "textures +%u  surfaces +%u  blits %u (quad %u, cpu %u)  text extents %u/frame, %u%% hits  res gets %u/frame, %u%% keyed",
             c->texCreated, c->surfCreated, c->blits / f, c->blitQuads / f, c->cpuBlits / f,
             (c->textMeasured + c->textHits) / f,
             c->textMeasured + c->textHits ? c->textHits * 100 / (c->textMeasured + c->textHits) : 0,
             c->resGets / f, c->resGets ? c->resHits * 100 / c->resGets : 0);
    snprintf(P.lines[6], sizeof P.lines[6], "texture builds %u/frame, %u skipped as unchanged  windows: %u invalidated, %u redrawn /frame  actor lookups %u/frame",
             c->builds / f, c->buildsSkipped / f, c->invalidates / f, c->ondraws / f,
             c->actorLookups / f);
    P.nlines = 7;
    P.dirty = 1;
}

static void roll(LONGLONG t)
{
    LONGLONG span = t - P.intervalStart;
    unsigned f = P.frames ? P.frames : 1;
    int i, logic;
    for (i = 0; i < PERF_N; i++)
        P.snap.ms10[i] = (int)((double)P.acc[i] * 10000.0 / (double)P.freq / (double)f);
    P.snap.frameMax10 = ms10_of(P.frameMax);
    P.snap.fps10 = span > 0 ? (int)((double)P.frames * 10.0 * (double)P.freq / (double)span) : 0;
    P.snap.frames = P.frames;
    P.snap.per = g_perf;
    logic = P.snap.ms10[PERF_FRAME] - P.snap.ms10[PERF_SCENE] - P.snap.ms10[PERF_DESTROY] - P.snap.ms10[PERF_ENDSCENE]
          - P.snap.ms10[PERF_OVERLAY] - P.snap.ms10[PERF_PRESENT];
    P.snap.logic10 = logic < 0 ? 0 : logic;
    P.snap.valid = 1;
    build_lines();
    memset(P.acc, 0, sizeof(P.acc));
    memset(&g_perf, 0, sizeof(g_perf));
    P.frames = 0;
    P.frameMax = 0;
    P.intervalStart = t;
}

void perf_frame_end(struct Renderer *r)
{
    LONGLONG t;
    (void)r;
    if (!overlay_enabled())
        return;
    t = now();
    if (P.lastFrameEnd) {
        LONGLONG d = t - P.lastFrameEnd;
        P.acc[PERF_FRAME] += d;
        if (d > P.frameMax)
            P.frameMax = d;
        P.frames++;
    }
    P.lastFrameEnd = t;
    if (!P.intervalStart)
        P.intervalStart = t;
    else if (t - P.intervalStart >= (P.freq >> 1))
        roll(t);
}

static int make_canvas(void)
{
    static const uint16_t widest = 'W';
    SIZE sz;
    P.font = font_cache_get_fixed(&g_fontCache, "Lucida Console", -12);
    if (!P.font)
        return 0;
    P.bits = malloc((size_t)OV_TEX_W * OV_TEX_H * sizeof *P.bits);
    if (!P.bits)
        return 0;
    font_text_extent(P.font, &widest, 1, 0, &sz);
    P.charW = sz.cx;
    P.lineH = sz.cy + 1;
    return 1;
}

static void overlay_text_out(int x, int y, const char *line, int len)
{
    struct FontTarget target = {(BYTE *)P.bits, OV_TEX_W, OV_TEX_H, OV_TEX_W * 4};
    uint16_t text[OV_LINE_CHARS];
    int i;
    for (i = 0; i < len && i < OV_LINE_CHARS; i++)
        text[i] = (unsigned char)line[i];
    font_text_out(P.font, &target, x, y, text, i, RGB(255, 255, 255), NULL);
}

static void rasterize(void)
{
    int i, x, y, w;
    P.usedW = 0;
    P.usedH = 3 + P.nlines * P.lineH + 3;
    if (P.usedH > OV_TEX_H)
        P.usedH = OV_TEX_H;
    memset(P.bits, 0, OV_TEX_W * OV_TEX_H * 4);
    for (i = 0; i < P.nlines; i++) {
        int len = (int)strlen(P.lines[i]);
        overlay_text_out(4, 3 + i * P.lineH, P.lines[i], len);
        w = len * P.charW + 8;
        if (w > P.usedW)
            P.usedW = w;
    }
    if (P.usedW > OV_TEX_W)
        P.usedW = OV_TEX_W;
    for (y = 0; y < P.usedH; y++) {
        DWORD *row = P.bits + y * OV_TEX_W;
        for (x = 0; x < P.usedW; x++)
            row[x] = (row[x] & 0xff) ? 0xffffffffu : 0xa0000000u;
    }
}

static int upload(void)
{
    struct gfx_lock lk;
    int y, x;
    if (!P.tex) {
        int fmt = gfx_texture_format_ok(GFX_FMT_8888) ? GFX_FMT_8888 : GFX_FMT_4444;
        P.tex = gfx_create_texture(OV_TEX_W, OV_TEX_H, fmt);
        if (!P.tex)
            return 0;
    }
    if (gfx_lock(P.tex, NULL, &lk) < 0) {
        gfx_release(P.tex);
        P.tex = NULL;
        return 0;
    }
    for (y = 0; y < P.usedH; y++) {
        const DWORD *src = P.bits + y * OV_TEX_W;
        BYTE *dst = (BYTE *)lk.bits + y * lk.pitch;
        if (lk.bpp == 32) {
            memcpy(dst, src, (size_t)P.usedW * 4);
        } else {
            WORD *d16 = (WORD *)dst;
            for (x = 0; x < P.usedW; x++) {
                DWORD p = src[x];
                d16[x] = (WORD)(((p >> 28) << 12) | (((p >> 20) & 0xf) << 8) | (((p >> 12) & 0xf) << 4) | ((p >> 4) & 0xf));
            }
        }
    }
    gfx_unlock(P.tex);
    return 1;
}

static void draw(struct Renderer *r)
{
    struct TexDrawVertex v[4];
    float x0 = (float)(g_3dDevice.width - P.usedW - OV_MARGIN) - 0.5f;
    float y0 = (float)(g_3dDevice.height - P.usedH - OV_MARGIN) - 0.5f;
    float x1 = x0 + (float)P.usedW, y1 = y0 + (float)P.usedH;
    float u1 = (float)P.usedW / OV_TEX_W, v1 = (float)P.usedH / OV_TEX_H;
    int i;
    for (i = 0; i < 4; i++) {
        v[i].x = (i & 1) ? x1 : x0;
        v[i].y = (i & 2) ? y1 : y0;
        v[i].z = 0.0f;
        v[i].rhw = 1.0f;
        v[i].diffuse = 0xffffffffu;
        v[i].u = (i & 1) ? u1 : 0.0f;
        v[i].v = (i & 2) ? v1 : 0.0f;
    }
    gfx_set_mode(DRAW_PANEL);
    gfx_set_texture(0, P.tex);
    gfx_set_texture(1, NULL);
    gfx_draw_vertices(v, 4, 1);
    gfx_set_texture(0, NULL);
    gfx_set_texture(1, NULL);
    r->boundTextures[0] = NULL;
    r->boundTextures[1] = NULL;
}

static void draw_overlay(struct Renderer *r)
{
    if (!P.bits && !make_canvas())
        return;
    if (P.dirty) {
        rasterize();
        if (!upload())
            return;
        P.dirty = 0;
    }
    if (P.tex)
        draw(r);
}

void perf_overlay_draw(struct Renderer *r)
{
    int visible;
    if (!overlay_enabled())
        return;
    /* With the overlay on, Scroll Lock hides and shows it. */
    visible = platform_key_state(VK_SCROLL) & 1;
    if (visible != P.lastScroll) {
        P.lastScroll = visible;
        P.shown = !P.shown;
    }
    if (!P.shown || !P.snap.valid)
        return;
    perf_begin(PERF_OVERLAY);
    draw_overlay(r);
    perf_end(PERF_OVERLAY);
}
