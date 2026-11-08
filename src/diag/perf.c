#include "render/renderer.h"
#include "render/render_api.h"
#include "diag/perf.h"
#include "ui/window_types.h"
#include <string.h>

struct perf_counters g_perf;

#define OV_TEX_W 1024
#define OV_TEX_H 128
#define OV_LINES 7
#define OV_LINE_CHARS 200
#define OV_MARGIN 8

static const char *const phase_name[PERF_N] = {
    "frame", "scene", "destroy", "endscene", "overlay", "targetlock", "present", "text", "texlock",
    "l_ground", "l_opaque_mesh", "l_water", "l_multitexture", "l_blended_nodepth", "l_alpha_nodepth", "l_alpha_depth", "l_additive_fog", "l_alphatest",
    "l_alpha_writedepth", "l_alpha_cutout", "l_alpha_mesh", "l_additive", "l_blended_wrap", "l_additive_overlay", "l_overlay_ordered", "l_blended_overlay",
};

static struct {
    int inited, enabled, shown, lastScroll;
    LARGE_INTEGER freq;
    LARGE_INTEGER t0[PERF_N];
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
    HDC dc;
    HBITMAP bmp;
    HGDIOBJ oldBmp;
    HFONT font;
    DWORD *bits;
    int charW, lineH;
    gfx_surface *tex;
    int usedW, usedH, dirty;
    char lines[OV_LINES][OV_LINE_CHARS];
    int nlines;
} P;

static LONGLONG now(void)
{
    LARGE_INTEGER t;
    QueryPerformanceCounter(&t);
    return t.QuadPart;
}

static void init(void);

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
    QueryPerformanceCounter(&P.t0[phase]);
}

void perf_end(int phase)
{
    if (!overlay_enabled())
        return;
    P.acc[phase] += now() - P.t0[phase].QuadPart;
}

static int ms10_of(LONGLONG ticks)
{
    return (int)((double)ticks * 10000.0 / (double)P.freq.QuadPart);
}

static void init(void)
{
    char buf[MAX_PATH];
    P.inited = 1;
    QueryPerformanceFrequency(&P.freq);
    if (GetEnvironmentVariableA("RAGEXE_OVERLAY", buf, sizeof(buf)) && buf[0] && buf[0] != '0')
        P.enabled = P.shown = 1;
    P.lastScroll = GetKeyState(VK_SCROLL) & 1;
    P.start = now();
}

static const char *tenths(char *buf, int v10)
{
    if (v10 < 0)
        v10 = 0;
    wsprintfA(buf, "%d.%d", v10 / 10, v10 % 10);
    return buf;
}

static void build_lines(void)
{
    char b1[16], b2[16], b3[16], b4[16], b5[16];
    const struct perf_counters *c = &P.snap.per;
    unsigned f = P.snap.frames ? P.snap.frames : 1;
    int order[PERF_N], nl = 0, i, j, k;
    char *s;

    wsprintfA(P.lines[0], "Direct3D 9  FPS %s  frame %s ms (max %s)  frames %u",
              tenths(b1, P.snap.fps10), tenths(b2, P.snap.ms10[PERF_FRAME]),
              tenths(b3, P.snap.frameMax10), P.snap.frames);
    wsprintfA(P.lines[1], "scene %s  destroy %s  endscene %s  targetlock %s",
              tenths(b1, P.snap.ms10[PERF_SCENE]), tenths(b2, P.snap.ms10[PERF_DESTROY]),
              tenths(b3, P.snap.ms10[PERF_ENDSCENE]), tenths(b4, P.snap.ms10[PERF_TARGETLOCK]));
    wsprintfA(P.lines[2], "present %s (the blank wait)  logic %s (text %s, surface locks %s)  overlay %s",
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
    lstrcpyA(s, "lists:");
    for (i = 0; i < 5 && i < nl; i++) {
        wsprintfA(s + lstrlenA(s), "  %s %s", phase_name[order[i]] + 2, tenths(b1, P.snap.ms10[order[i]]));
    }
    wsprintfA(P.lines[4], "draws %u (%u calls)  prims %u  states %u sent / %u skipped  tex sets %u  locks %u (target %u)",
              c->draws / f, c->drawCalls / f, c->prims / f, c->statesSent / f, c->statesSkipped / f, c->texSets / f,
              c->locks / f, c->targetLocks / f);
    wsprintfA(P.lines[5], "textures +%u  surfaces +%u  blits %u (quad %u, cpu %u)  dc %u  text extents %u/frame, %u%% hits  res gets %u/frame, %u%% keyed",
              c->texCreated, c->surfCreated, c->blits / f, c->blitQuads / f, c->cpuBlits / f, c->dcGets / f,
              (c->textMeasured + c->textHits) / f,
              c->textMeasured + c->textHits ? c->textHits * 100 / (c->textMeasured + c->textHits) : 0,
              c->resGets / f, c->resGets ? c->resHits * 100 / c->resGets : 0);
    wsprintfA(P.lines[6], "texture builds %u/frame, %u skipped as unchanged  windows: %u invalidated, %u redrawn /frame  actor lookups %u/frame, %u%% indexed",
              c->builds / f, c->buildsSkipped / f, c->invalidates / f, c->ondraws / f,
              c->actorLookups / f, c->actorLookups ? c->actorIndexHits * 100 / c->actorLookups : 0);
    P.nlines = 7;
    P.dirty = 1;
}

static void roll(LONGLONG t)
{
    LONGLONG span = t - P.intervalStart;
    unsigned f = P.frames ? P.frames : 1;
    int i, logic;
    for (i = 0; i < PERF_N; i++)
        P.snap.ms10[i] = (int)((double)P.acc[i] * 10000.0 / (double)P.freq.QuadPart / (double)f);
    P.snap.frameMax10 = ms10_of(P.frameMax);
    P.snap.fps10 = span > 0 ? (int)((double)P.frames * 10.0 * (double)P.freq.QuadPart / (double)span) : 0;
    P.snap.frames = P.frames;
    P.snap.per = g_perf;
    logic = P.snap.ms10[PERF_FRAME] - P.snap.ms10[PERF_SCENE] - P.snap.ms10[PERF_DESTROY] - P.snap.ms10[PERF_ENDSCENE]
          - P.snap.ms10[PERF_OVERLAY] - P.snap.ms10[PERF_TARGETLOCK] - P.snap.ms10[PERF_PRESENT];
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
    else if (t - P.intervalStart >= (P.freq.QuadPart >> 1))
        roll(t);
}

static int make_dib(void)
{
    BITMAPINFO bi;
    SIZE sz;
    P.dc = CreateCompatibleDC(NULL);
    if (!P.dc)
        return 0;
    memset(&bi, 0, sizeof(bi));
    bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
    bi.bmiHeader.biWidth = OV_TEX_W;
    bi.bmiHeader.biHeight = -OV_TEX_H;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;
    P.bmp = CreateDIBSection(P.dc, &bi, DIB_RGB_COLORS, (void **)&P.bits, NULL, 0);
    if (!P.bmp || !P.bits) {
        DeleteDC(P.dc);
        P.dc = NULL;
        return 0;
    }
    P.oldBmp = SelectObject(P.dc, P.bmp);
    P.font = CreateFontA(-12, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                         NONANTIALIASED_QUALITY, FIXED_PITCH | FF_MODERN, "Lucida Console");
    if (P.font)
        SelectObject(P.dc, P.font);
    SetBkMode(P.dc, TRANSPARENT);
    SetTextColor(P.dc, RGB(255, 255, 255));
    P.charW = 7;
    P.lineH = 13;
    if (GetTextExtentPoint32W(P.dc, L"W", 1, &sz)) {
        P.charW = sz.cx;
        P.lineH = sz.cy + 1;
    }
    return 1;
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
        int len = lstrlenA(P.lines[i]);
        TextOutA(P.dc, 4, 3 + i * P.lineH, P.lines[i], len);
        w = len * P.charW + 8;
        if (w > P.usedW)
            P.usedW = w;
    }
    if (P.usedW > OV_TEX_W)
        P.usedW = OV_TEX_W;
    SelectObject(P.dc, P.font);
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
    if (gfx_lock(P.tex, NULL, GFX_LOCK_WAIT, &lk) < 0) {
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

struct ov_vertex {
    float x, y, z, rhw;
    DWORD diffuse, specular;
    float u, v;
};

static void draw(struct Renderer *r)
{
    static const int rs_ids[6] = { GFX_RS_ZENABLE, GFX_RS_ALPHABLENDENABLE, GFX_RS_ALPHATESTENABLE,
                                   GFX_RS_FOGENABLE, GFX_RS_SRCBLEND, GFX_RS_DESTBLEND };
    static const DWORD rs_vals[6] = { FALSE, TRUE, FALSE, FALSE, GFX_BLEND_SRCALPHA, GFX_BLEND_INVSRCALPHA };
    DWORD saved[6];
    struct ov_vertex v[4];
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
        v[i].specular = 0;
        v[i].u = (i & 1) ? u1 : 0.0f;
        v[i].v = (i & 2) ? v1 : 0.0f;
    }
    for (i = 0; i < 6; i++) {
        saved[i] = rs_vals[i];
        gfx_get_render_state(rs_ids[i], &saved[i]);
        gfx_set_render_state(rs_ids[i], rs_vals[i]);
    }
    gfx_set_stage_state(0, GFX_TSS_COLOROP, GFX_TOP_SELECTARG1);
    gfx_set_stage_state(0, GFX_TSS_COLORARG1, GFX_TA_TEXTURE);
    gfx_set_stage_state(0, GFX_TSS_ALPHAOP, GFX_TOP_SELECTARG1);
    gfx_set_stage_state(0, GFX_TSS_ALPHAARG1, GFX_TA_TEXTURE);
    gfx_set_stage_state(1, GFX_TSS_COLOROP, GFX_TOP_DISABLE);
    gfx_set_stage_state(1, GFX_TSS_ALPHAOP, GFX_TOP_DISABLE);
    gfx_set_texture(0, P.tex);
    gfx_set_texture(1, NULL);
    gfx_draw(GFX_PT_TRIANGLESTRIP, GFX_VTX_TEX1, v, 4, NULL, 0);
    gfx_set_texture(0, NULL);
    gfx_set_texture(1, NULL);
    r->boundTextures[0] = NULL;
    r->boundTextures[1] = NULL;
    renderer_set_blend_stages(r, 0);
    for (i = 0; i < 6; i++)
        gfx_set_render_state(rs_ids[i], saved[i]);
}

static void draw_overlay(struct Renderer *r)
{
    if (!P.dc && !make_dib())
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
    visible = GetKeyState(VK_SCROLL) & 1;
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
