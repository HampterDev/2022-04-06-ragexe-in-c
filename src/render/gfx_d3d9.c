#include "render/display_devices.h"
#include "render/render_api.h"
extern const IID IID_IDirect3DHALDevice;
#include "diag/perf.h"
#include "render/gfx_batch.h"
#include "diag/diag.h"

static void d9_raw_draw(int prim, int vertexKind, const void *verts, int nverts, const WORD *indices, int nindices);
#define COBJMACROS
#include <d3d9.h>
#include <stdlib.h>
#include <string.h>

STATIC_ASSERT(d9_matrix, sizeof(D3DMATRIX) == sizeof(struct GfxMatrix));
STATIC_ASSERT(d9_rect, sizeof(D3DRECT) == sizeof(RECT));
STATIC_ASSERT(d9_rs_zen, GFX_RS_ZENABLE == D3DRS_ZENABLE);
STATIC_ASSERT(d9_rs_fogv, GFX_RS_FOGVERTEXMODE == D3DRS_FOGVERTEXMODE);
STATIC_ASSERT(d9_rs_amb, GFX_RS_AMBIENT == D3DRS_AMBIENT);
STATIC_ASSERT(d9_rs_rfog, GFX_RS_RANGEFOGENABLE == D3DRS_RANGEFOGENABLE);
STATIC_ASSERT(d9_tss_cop, GFX_TSS_COLOROP == D3DTSS_COLOROP);
STATIC_ASSERT(d9_tss_tci, GFX_TSS_TEXCOORDINDEX == D3DTSS_TEXCOORDINDEX);
STATIC_ASSERT(d9_top, GFX_TOP_MODULATEALPHA_ADDCOLOR == D3DTOP_MODULATEALPHA_ADDCOLOR);
STATIC_ASSERT(d9_ta, GFX_TA_ALPHAREPLICATE == D3DTA_ALPHAREPLICATE);
STATIC_ASSERT(d9_addr, GFX_ADDRESS_BORDER == D3DTADDRESS_BORDER);
STATIC_ASSERT(d9_blend, GFX_BLEND_INVSRCALPHA == D3DBLEND_INVSRCALPHA);
STATIC_ASSERT(d9_cmp, GFX_CMP_GREATEREQUAL == D3DCMP_GREATEREQUAL);
STATIC_ASSERT(d9_cull, GFX_CULL_CW == D3DCULL_CW);
STATIC_ASSERT(d9_pt, GFX_PT_TRIANGLEFAN == D3DPT_TRIANGLEFAN);
STATIC_ASSERT(d9_clear, GFX_CLEAR_ZBUFFER == D3DCLEAR_ZBUFFER);
STATIC_ASSERT(d9_lost, GFX_E_SURFACELOST != (int)D3DERR_DEVICELOST);

#define FVF_TEX1 (D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEX1)
#define FVF_TEX2 (D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEX2)

enum { K_SCREEN, K_TARGET, K_SAVE, K_SYSMEM, K_TEXTURE, K_RTTEX, K_ZBUFFER,
       K_MSRT };

struct gfx_surface {
    int kind, refs;
    int width, height;
    int fmt;
    D3DFORMAT d3dfmt;
    IDirect3DTexture9 *tex;
    IDirect3DSurface9 *surf;
    int levels;
    int dirty;
    DWORD key;
    struct gfx_surface *prev, *next;
    unsigned version;
};

static unsigned content_tick;

static int device_gone;
static int reset_failures;
#define RESET_TRIES_BEFORE_RECREATE 8
#ifndef D3DERR_DEVICEREMOVED
#define D3DERR_DEVICEREMOVED MAKE_D3DHRESULT(2160)
#endif
#ifndef D3DERR_DEVICEHUNG
#define D3DERR_DEVICEHUNG MAKE_D3DHRESULT(2164)
#endif

struct gfx_vbuffer {
    IDirect3DVertexBuffer9 *vb;
    DWORD fvf, stride;
};

#define N_RS 256
#define N_TSS 33
#define N_SAMP 14

static struct {
    IDirect3D9 *d3d;
    IDirect3DDevice9 *dev;
    D3DPRESENT_PARAMETERS pp;
    UINT adapter;
    DWORD behaviour;
    D3DFORMAT bbFormat, zFormat;
    int bbBpp;
    DWORD rMask, gMask, bMask, aMask;
    int inScene, mipmaps, dialogMode;
    struct gfx_surface *handles;
    struct gfx_surface *target;
    struct gfx_surface *mainTarget;
    struct gfx_surface *depth;
    struct gfx_surface *msRT;
    struct gfx_surface *bbCopy;
    D3DMULTISAMPLE_TYPE msType;
    DWORD msQuality;
    int msDirty;
    int taa;
    DWORD taaOn;
    int taaWanted;
    int taaFlat;
    int taaApplied;
    int msaaApplied;
    DWORD rs[N_RS]; BYTE rsSet[N_RS];
    DWORD tss[8][N_TSS]; BYTE tssSet[8][N_TSS];
    DWORD samp[8][N_SAMP]; BYTE sampSet[8][N_SAMP];
    D3DMATRIX xf[4]; BYTE xfSet[4];
    D3DLIGHT9 light; int lightSet;
    struct gfx_surface *stageTex[8];
    struct { GUID guid; UINT ordinal; } adapters[16];
    int nAdapters;
} d9;

static D3DFORMAT client_format(int fmt)
{
    switch (fmt) {
    case GFX_FMT_1555: return D3DFMT_A1R5G5B5;
    case GFX_FMT_4444: return D3DFMT_A4R4G4B4;
    case GFX_FMT_565:  return D3DFMT_R5G6B5;
    case GFX_FMT_8888: return D3DFMT_A8R8G8B8;
    case GFX_FMT_BUMP: return D3DFMT_V8U8;
    default:           return D3DFMT_UNKNOWN;
    }
}

static int format_bpp(D3DFORMAT f)
{
    switch (f) {
    case D3DFMT_A8R8G8B8: case D3DFMT_X8R8G8B8: case D3DFMT_A8B8G8R8: case D3DFMT_X8B8G8R8:
    case D3DFMT_A2R10G10B10: case D3DFMT_A2B10G10R10:
        return 32;
    case D3DFMT_R8G8B8:
        return 24;
    case D3DFMT_R5G6B5: case D3DFMT_X1R5G5B5: case D3DFMT_A1R5G5B5: case D3DFMT_A4R4G4B4:
    case D3DFMT_X4R4G4B4: case D3DFMT_V8U8: case D3DFMT_D16:
        return 16;
    default:
        return 32;
    }
}

static DWORD unpack(D3DFORMAT f, DWORD p)
{
    DWORD r, g, b, a;
    switch (f) {
    case D3DFMT_A8R8G8B8: return p;
    case D3DFMT_X8R8G8B8: return p | 0xff000000;
    case D3DFMT_R5G6B5:
        r = (p >> 11) & 0x1f; g = (p >> 5) & 0x3f; b = p & 0x1f;
        return 0xff000000 | ((r << 3 | r >> 2) << 16) | ((g << 2 | g >> 4) << 8) | (b << 3 | b >> 2);
    case D3DFMT_X1R5G5B5: case D3DFMT_A1R5G5B5:
        a = (f == D3DFMT_A1R5G5B5) ? ((p & 0x8000) ? 0xff : 0) : 0xff;
        r = (p >> 10) & 0x1f; g = (p >> 5) & 0x1f; b = p & 0x1f;
        return (a << 24) | ((r << 3 | r >> 2) << 16) | ((g << 3 | g >> 2) << 8) | (b << 3 | b >> 2);
    case D3DFMT_A4R4G4B4:
        a = (p >> 12) & 0xf; r = (p >> 8) & 0xf; g = (p >> 4) & 0xf; b = p & 0xf;
        return (a * 17 << 24) | (r * 17 << 16) | (g * 17 << 8) | (b * 17);
    default:
        return p;
    }
}

static DWORD pack(D3DFORMAT f, DWORD argb)
{
    DWORD a = argb >> 24, r = (argb >> 16) & 0xff, g = (argb >> 8) & 0xff, b = argb & 0xff;
    switch (f) {
    case D3DFMT_A8R8G8B8: return argb;
    case D3DFMT_X8R8G8B8: return argb & 0xffffff;
    case D3DFMT_R5G6B5:   return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
    case D3DFMT_X1R5G5B5: return ((r >> 3) << 10) | ((g >> 3) << 5) | (b >> 3);
    case D3DFMT_A1R5G5B5: return ((a >> 7) << 15) | ((r >> 3) << 10) | ((g >> 3) << 5) | (b >> 3);
    case D3DFMT_A4R4G4B4: return ((a >> 4) << 12) | ((r >> 4) << 8) | ((g >> 4) << 4) | (b >> 4);
    default:              return argb;
    }
}

static DWORD read_pixel(const BYTE *p, int bpp)
{
    return bpp == 32 ? *(const DWORD *)p : bpp == 16 ? *(const WORD *)p : (DWORD)p[0] | (DWORD)p[1] << 8 | (DWORD)p[2] << 16;
}

static void write_pixel(BYTE *p, int bpp, DWORD v)
{
    if (bpp == 32)
        *(DWORD *)p = v;
    else if (bpp == 16)
        *(WORD *)p = (WORD)v;
    else {
        p[0] = (BYTE)v;
        p[1] = (BYTE)(v >> 8);
        p[2] = (BYTE)(v >> 16);
    }
}

static struct gfx_surface *new_handle(int kind, int w, int h, D3DFORMAT f)
{
    struct gfx_surface *s = (struct gfx_surface *)calloc(1, sizeof(*s));
    if (!s)
        return NULL;
    s->kind = kind;
    s->refs = 1;
    s->width = w;
    s->height = h;
    s->fmt = -1;
    s->d3dfmt = f;
    s->levels = 1;
    s->version = ++content_tick;
    s->next = d9.handles;
    if (d9.handles)
        d9.handles->prev = s;
    d9.handles = s;
    return s;
}

static void drop_objects(struct gfx_surface *s)
{
    s->version = ++content_tick;
    if (s->surf) {
        IDirect3DSurface9_Release(s->surf);
        s->surf = NULL;
    }
    if (s->tex) {
        IDirect3DTexture9_Release(s->tex);
        s->tex = NULL;
    }
}

static void free_handle(struct gfx_surface *s)
{
    int i;
    for (i = 0; i < 8; i++)
        if (d9.stageTex[i] == s) {
            batch_flush();
            if (d9.dev)
                IDirect3DDevice9_SetTexture(d9.dev, i, NULL);
            d9.stageTex[i] = NULL;
        }
    drop_objects(s);
    if (d9.mainTarget == s)
        d9.mainTarget = NULL;
    if (d9.depth == s)
        d9.depth = NULL;
    if (d9.target == s)
        d9.target = d9.mainTarget;
    if (s->prev)
        s->prev->next = s->next;
    else
        d9.handles = s->next;
    if (s->next)
        s->next->prev = s->prev;
    free(s);
}

void gfx_addref(gfx_surface *s)
{
    if (s)
        s->refs++;
}

void gfx_release(gfx_surface *s)
{
    if (s && --s->refs <= 0)
        free_handle(s);
}

static void apply_rs(int id, DWORD v)
{
    batch_flush();
    if (d9.dev)
        IDirect3DDevice9_SetRenderState(d9.dev, (D3DRENDERSTATETYPE)id, v);
}

static void apply_tss(int stage, int id, DWORD v)
{
    batch_flush();
    if (d9.dev)
        IDirect3DDevice9_SetTextureStageState(d9.dev, stage, (D3DTEXTURESTAGESTATETYPE)id, v);
}

static void apply_samp(int stage, int id, DWORD v)
{
    batch_flush();
    if (d9.dev)
        IDirect3DDevice9_SetSamplerState(d9.dev, stage, (D3DSAMPLERSTATETYPE)id, v);
}

static void update_transparency_aa(void);

static void forget_state(void)
{
    memset(d9.rsSet, 0, sizeof(d9.rsSet));
    memset(d9.tssSet, 0, sizeof(d9.tssSet));
    memset(d9.sampSet, 0, sizeof(d9.sampSet));
    memset(d9.xfSet, 0, sizeof(d9.xfSet));
    memset(d9.stageTex, 0, sizeof(d9.stageTex));
    d9.lightSet = 0;
    d9.dialogMode = 0;
    d9.taaWanted = 0;
    d9.taaFlat = 0;
    d9.taaApplied = -1;
    d9.msaaApplied = -1;
}

static void set_rs(int id, DWORD v)
{
    if ((unsigned)id < N_RS) {
        if (d9.rsSet[id] && d9.rs[id] == v) {
            g_perf.statesSkipped++;
            return;
        }
        d9.rs[id] = v;
        d9.rsSet[id] = 1;
    }
    g_perf.statesSent++;
    apply_rs(id, v);
    if (id == D3DRS_ALPHABLENDENABLE) {
        d9.taaWanted = !v;
        update_transparency_aa();
    }
}

static void set_tss(int stage, int id, DWORD v)
{
    if ((unsigned)stage < 8 && (unsigned)id < N_TSS) {
        if (d9.tssSet[stage][id] && d9.tss[stage][id] == v) {
            g_perf.statesSkipped++;
            return;
        }
        d9.tss[stage][id] = v;
        d9.tssSet[stage][id] = 1;
    }
    g_perf.statesSent++;
    apply_tss(stage, id, v);
}

static void set_samp(int stage, int id, DWORD v)
{
    if ((unsigned)stage < 8 && (unsigned)id < N_SAMP) {
        if (d9.sampSet[stage][id] && d9.samp[stage][id] == v) {
            g_perf.statesSkipped++;
            return;
        }
        d9.samp[stage][id] = v;
        d9.sampSet[stage][id] = 1;
    }
    g_perf.statesSent++;
    apply_samp(stage, id, v);
}

static IDirect3DBaseTexture9 *texture_of(struct gfx_surface *s);

static void replay_state(void)
{
    int i, j;
    for (i = 0; i < N_RS; i++)
        if (d9.rsSet[i])
            apply_rs(i, d9.rs[i]);
    for (i = 0; i < 8; i++) {
        for (j = 0; j < N_TSS; j++)
            if (d9.tssSet[i][j])
                apply_tss(i, j, d9.tss[i][j]);
        for (j = 0; j < N_SAMP; j++)
            if (d9.sampSet[i][j])
                apply_samp(i, j, d9.samp[i][j]);
    }
    for (i = 0; i < 4; i++)
        if (d9.xfSet[i])
            IDirect3DDevice9_SetTransform(d9.dev, i == 1 ? D3DTS_WORLD : (D3DTRANSFORMSTATETYPE)i, &d9.xf[i]);
    if (d9.lightSet) {
        IDirect3DDevice9_SetLight(d9.dev, 0, &d9.light);
        IDirect3DDevice9_LightEnable(d9.dev, 0, TRUE);
    }
    for (i = 0; i < 8; i++)
        if (d9.stageTex[i])
            IDirect3DDevice9_SetTexture(d9.dev, i, texture_of(d9.stageTex[i]));
    if (d9.dialogMode)
        IDirect3DDevice9_SetDialogBoxMode(d9.dev, TRUE);
    d9.taaWanted = d9.rsSet[D3DRS_ALPHABLENDENABLE] && !d9.rs[D3DRS_ALPHABLENDENABLE];
    d9.taaApplied = -1;
    d9.msaaApplied = -1;
    update_transparency_aa();
}

static int cpu_blit(struct gfx_surface *dst, const RECT *dr, struct gfx_surface *src, const RECT *sr, int keyed, DWORD key)
{
    g_perf.cpuBlits++;
    D3DLOCKED_RECT dl, sl;
    RECT d, s;
    int sbpp = format_bpp(src->d3dfmt), dbpp = format_bpp(dst->d3dfmt);
    int same = src->d3dfmt == dst->d3dfmt;
    int x, y, dw, dh, sw, sh;
    if (sr) s = *sr; else { s.left = 0; s.top = 0; s.right = src->width; s.bottom = src->height; }
    if (dr) d = *dr; else { d.left = 0; d.top = 0; d.right = dst->width; d.bottom = dst->height; }
    dw = d.right - d.left;
    dh = d.bottom - d.top;
    sw = s.right - s.left;
    sh = s.bottom - s.top;
    if (dw <= 0 || dh <= 0 || sw <= 0 || sh <= 0)
        return 0;
    if (FAILED(IDirect3DSurface9_LockRect(src->surf, &sl, &s, D3DLOCK_READONLY)))
        return -1;
    if (FAILED(IDirect3DSurface9_LockRect(dst->surf, &dl, &d, 0))) {
        IDirect3DSurface9_UnlockRect(src->surf);
        return -1;
    }
    for (y = 0; y < dh; y++) {
        const BYTE *srow = (const BYTE *)sl.pBits + (dh == sh ? y : y * sh / dh) * sl.Pitch;
        BYTE *drow = (BYTE *)dl.pBits + y * dl.Pitch;
        for (x = 0; x < dw; x++) {
            int sx = dw == sw ? x : x * sw / dw;
            DWORD p = read_pixel(srow + sx * (sbpp >> 3), sbpp);
            if (keyed && p == key)
                continue;
            if (!same)
                p = pack(dst->d3dfmt, unpack(src->d3dfmt, p));
            write_pixel(drow + x * (dbpp >> 3), dbpp, p);
        }
    }
    IDirect3DSurface9_UnlockRect(dst->surf);
    IDirect3DSurface9_UnlockRect(src->surf);
    return 0;
}

static int ensure_cache(struct gfx_surface *s)
{
    D3DLOCKED_RECT dl, sl;
    int bpp = format_bpp(s->d3dfmt), x, y;
    if (s->tex && !s->dirty)
        return 0;
    if (!s->tex) {
        if (FAILED(IDirect3DDevice9_CreateTexture(d9.dev, s->width, s->height, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED,
                                                  &s->tex, NULL)))
            return -1;
    }
    if (FAILED(IDirect3DSurface9_LockRect(s->surf, &sl, NULL, D3DLOCK_READONLY)))
        return -1;
    if (FAILED(IDirect3DTexture9_LockRect(s->tex, 0, &dl, NULL, 0))) {
        IDirect3DSurface9_UnlockRect(s->surf);
        return -1;
    }
    for (y = 0; y < s->height; y++) {
        const BYTE *srow = (const BYTE *)sl.pBits + y * sl.Pitch;
        DWORD *drow = (DWORD *)((BYTE *)dl.pBits + y * dl.Pitch);
        for (x = 0; x < s->width; x++) {
            DWORD p = read_pixel(srow + x * (bpp >> 3), bpp);
            drow[x] = p == s->key ? (unpack(s->d3dfmt, p) & 0xffffff) : (unpack(s->d3dfmt, p) | 0xff000000);
        }
    }
    IDirect3DTexture9_UnlockRect(s->tex, 0);
    IDirect3DSurface9_UnlockRect(s->surf);
    s->dirty = 0;
    return 0;
}

static IDirect3DBaseTexture9 *texture_of(struct gfx_surface *s)
{
    if (!s)
        return NULL;
    if (s->kind == K_SYSMEM)
        ensure_cache(s);
    return (IDirect3DBaseTexture9 *)s->tex;
}

static IDirect3DSurface9 *target_copy(struct gfx_surface *rt)
{
    IDirect3DSurface9 *sys = NULL;
    if (FAILED(IDirect3DDevice9_CreateOffscreenPlainSurface(d9.dev, rt->width, rt->height, rt->d3dfmt,
                                                            D3DPOOL_SYSTEMMEM, &sys, NULL)))
        return NULL;
    if (FAILED(IDirect3DDevice9_GetRenderTargetData(d9.dev, rt->surf, sys))) {
        IDirect3DSurface9_Release(sys);
        return NULL;
    }
    return sys;
}

static int draw_surface_quad(struct gfx_surface *dst, const RECT *dr, struct gfx_surface *src, const RECT *sr, int keyed);

static int is_main_target(const struct gfx_surface *s)
{
    return s && (s->kind == K_SCREEN || s->kind == K_TARGET);
}

static int msaa_on(void)
{
    return d9.msRT && d9.msRT->surf;
}

static HRESULT bind_target(struct gfx_surface *s)
{
    HRESULT hr;
    if (!d9.dev || !s || !s->surf)
        return D3DERR_INVALIDCALL;
    if (is_main_target(s) && msaa_on()) {
        hr = IDirect3DDevice9_SetRenderTarget(d9.dev, 0, d9.msRT->surf);
        if (SUCCEEDED(hr))
            IDirect3DDevice9_SetDepthStencilSurface(d9.dev, d9.depth && d9.depth->surf ? d9.depth->surf : NULL);
        return hr;
    }
    hr = IDirect3DDevice9_SetRenderTarget(d9.dev, 0, s->surf);
    if (FAILED(hr))
        return hr;
    if (s->kind == K_RTTEX || msaa_on())
        IDirect3DDevice9_SetDepthStencilSurface(d9.dev, NULL);
    else if (d9.depth && d9.depth->surf)
        IDirect3DDevice9_SetDepthStencilSurface(d9.dev, d9.depth->surf);
    return hr;
}

static void scene_write(void)
{
    if (msaa_on())
        d9.msDirty = 1;
}

static void resolve_ms(void)
{
    if (d9.msDirty && msaa_on() && d9.mainTarget && d9.mainTarget->surf) {
        batch_flush();
        IDirect3DDevice9_StretchRect(d9.dev, d9.msRT->surf, NULL, d9.mainTarget->surf, NULL, D3DTEXF_NONE);
    }
    d9.msDirty = 0;
}

static void unresolve_ms(void)
{
    if (!msaa_on() || !d9.mainTarget || !d9.mainTarget->surf || !d9.bbCopy || !d9.bbCopy->surf)
        return;
    if (SUCCEEDED(IDirect3DDevice9_StretchRect(d9.dev, d9.mainTarget->surf, NULL, d9.bbCopy->surf, NULL, D3DTEXF_NONE)))
        draw_surface_quad(d9.mainTarget, NULL, d9.bbCopy, NULL, 0);
    d9.msDirty = 0;
}

static void pick_multisample(void)
{
    char env[8];
    int want = 4, n;
    DWORD q;
    if (GetEnvironmentVariableA("RAGEXE_MSAA", env, sizeof(env)) && env[0])
        want = atoi(env);
    d9.msType = D3DMULTISAMPLE_NONE;
    d9.msQuality = 0;
    if (want > 16)
        want = 16;
    for (n = want; n >= 2; n--) {
        if (SUCCEEDED(IDirect3D9_CheckDeviceMultiSampleType(d9.d3d, d9.adapter, D3DDEVTYPE_HAL, d9.bbFormat, d9.pp.Windowed,
                                                            (D3DMULTISAMPLE_TYPE)n, &q))
            && SUCCEEDED(IDirect3D9_CheckDeviceMultiSampleType(d9.d3d, d9.adapter, D3DDEVTYPE_HAL, d9.zFormat, d9.pp.Windowed,
                                                               (D3DMULTISAMPLE_TYPE)n, NULL))) {
            d9.msType = (D3DMULTISAMPLE_TYPE)n;
            break;
        }
    }
}

static void pick_transparency_aa(void)
{
    static const DWORD ssaa = MAKEFOURCC('S', 'S', 'A', 'A'), atoc = MAKEFOURCC('A', 'T', 'O', 'C'),
                       a2m1 = MAKEFOURCC('A', '2', 'M', '1');
    D3DDISPLAYMODE mode;
    char env[8];
    int wantSsaa = 1, wantAtoc = 1;
    d9.taa = 0;
    d9.taaOn = 0;
    if (d9.msType == D3DMULTISAMPLE_NONE)
        return;
    if (GetEnvironmentVariableA("RAGEXE_MSAA_ALPHA", env, sizeof(env)) && env[0]) {
        if (env[0] == '0' && !env[1])
            return;
        wantSsaa = lstrcmpiA(env, "ssaa") == 0;
        wantAtoc = lstrcmpiA(env, "atoc") == 0;
    }
    if (FAILED(IDirect3D9_GetAdapterDisplayMode(d9.d3d, d9.adapter, &mode)))
        mode.Format = D3DFMT_X8R8G8B8;
#define TAA_SUPPORTED(fourcc) SUCCEEDED(IDirect3D9_CheckDeviceFormat(d9.d3d, d9.adapter, D3DDEVTYPE_HAL, mode.Format, 0, \
                                                                     D3DRTYPE_SURFACE, (D3DFORMAT)(fourcc)))
    if (wantSsaa && TAA_SUPPORTED(ssaa)) {
        d9.taa = 1;
        d9.taaOn = ssaa;
    } else if (wantAtoc && TAA_SUPPORTED(atoc)) {
        d9.taa = 1;
        d9.taaOn = atoc;
    } else if (wantAtoc && TAA_SUPPORTED(a2m1)) {
        d9.taa = 2;
        d9.taaOn = a2m1;
    }
#undef TAA_SUPPORTED
}

static void update_transparency_aa(void)
{
    int on = d9.taaWanted && !d9.taaFlat, ms = !d9.taaFlat;
    if (!d9.dev || !msaa_on())
        return;
    if (d9.msaaApplied != ms) {
        d9.msaaApplied = ms;
        IDirect3DDevice9_SetRenderState(d9.dev, D3DRS_MULTISAMPLEANTIALIAS, ms);
    }
    if (!d9.taa || d9.taaApplied == on)
        return;
    d9.taaApplied = on;
    if (d9.taa == 1)
        IDirect3DDevice9_SetRenderState(d9.dev, D3DRS_ADAPTIVETESS_Y, on ? d9.taaOn : (DWORD)D3DFMT_UNKNOWN);
    else
        IDirect3DDevice9_SetRenderState(d9.dev, D3DRS_POINTSIZE, on ? d9.taaOn : MAKEFOURCC('A', '2', 'M', '0'));
}

static int flat_draw(int vertexKind, const void *verts, int nverts)
{
    int stride = vertexKind == GFX_VTX_TEX2 ? 40 : 32, i;
    const BYTE *v = (const BYTE *)verts;
    DWORD rhw;
    if (nverts < 1)
        return 1;
    rhw = *(const DWORD *)(v + 12);
    for (i = 1; i < nverts; i++)
        if (*(const DWORD *)(v + i * stride + 12) != rhw)
            return 0;
    return 1;
}

#define FLAT_KIND 0x100

static int create_scene_target(int w, int h)
{
    IDirect3DSurface9 *rt = NULL;
    IDirect3DTexture9 *tex = NULL;
    if (d9.msType == D3DMULTISAMPLE_NONE)
        return -1;
    if (FAILED(IDirect3DDevice9_CreateRenderTarget(d9.dev, w, h, d9.bbFormat, d9.msType, d9.msQuality, FALSE, &rt, NULL)))
        return -1;
    if (FAILED(IDirect3DDevice9_CreateTexture(d9.dev, w, h, 1, D3DUSAGE_RENDERTARGET, d9.bbFormat, D3DPOOL_DEFAULT, &tex, NULL))) {
        IDirect3DSurface9_Release(rt);
        return -1;
    }
    if (!d9.msRT)
        d9.msRT = new_handle(K_MSRT, w, h, d9.bbFormat);
    if (!d9.bbCopy) {
        d9.bbCopy = new_handle(K_RTTEX, w, h, d9.bbFormat);
        d9.bbCopy->fmt = GFX_FMT_8888;
    }
    d9.msRT->width = d9.bbCopy->width = w;
    d9.msRT->height = d9.bbCopy->height = h;
    d9.msRT->d3dfmt = d9.bbCopy->d3dfmt = d9.bbFormat;
    d9.msRT->surf = rt;
    d9.bbCopy->tex = tex;
    IDirect3DTexture9_GetSurfaceLevel(tex, 0, &d9.bbCopy->surf);
    d9.msDirty = 0;
    return 0;
}

static int draw_surface_quad(struct gfx_surface *dst, const RECT *dr, struct gfx_surface *src, const RECT *sr, int keyed)
{
    batch_flush();
    g_perf.blitQuads++;
    struct { float x, y, z, rhw; DWORD diffuse, specular; float u, v; } v[4];
    RECT d, s;
    float tw, th;
    int i, scene = d9.inScene;
    struct gfx_surface *prevTarget = d9.target;
    static const int rs_ids[] = { D3DRS_ALPHATESTENABLE, D3DRS_ALPHABLENDENABLE, D3DRS_ZENABLE, D3DRS_ZWRITEENABLE,
                                  D3DRS_FOGENABLE, D3DRS_CULLMODE, D3DRS_ALPHAREF, D3DRS_ALPHAFUNC, D3DRS_LIGHTING };
    static const DWORD rs_default[] = { FALSE, FALSE, TRUE, TRUE, FALSE, D3DCULL_CCW, 0, D3DCMP_ALWAYS, TRUE };
    static const int tss_ids[] = { D3DTSS_COLOROP, D3DTSS_COLORARG1, D3DTSS_ALPHAOP, D3DTSS_ALPHAARG1, D3DTSS_TEXCOORDINDEX };
    static const DWORD tss0_default[] = { D3DTOP_MODULATE, D3DTA_TEXTURE, D3DTOP_SELECTARG1, D3DTA_TEXTURE, 0 };
    static const DWORD tss1_default[] = { D3DTOP_DISABLE, D3DTA_TEXTURE, D3DTOP_DISABLE, D3DTA_TEXTURE, 1 };
    static const int samp_ids[] = { D3DSAMP_ADDRESSU, D3DSAMP_ADDRESSV, D3DSAMP_MAGFILTER, D3DSAMP_MINFILTER, D3DSAMP_MIPFILTER };
    static const DWORD samp_default[] = { D3DTADDRESS_WRAP, D3DTADDRESS_WRAP, D3DTEXF_POINT, D3DTEXF_POINT, D3DTEXF_NONE };
    if (!d9.dev || !dst->surf)
        return -1;
    if (src->kind == K_SYSMEM) {
        if (ensure_cache(src) < 0)
            return -1;
    } else if (!src->tex) {
        return -1;
    }
    if (sr) s = *sr; else { s.left = 0; s.top = 0; s.right = src->width; s.bottom = src->height; }
    if (dr) d = *dr; else { d.left = 0; d.top = 0; d.right = dst->width; d.bottom = dst->height; }
    tw = (float)src->width;
    th = (float)src->height;
    for (i = 0; i < 4; i++) {
        v[i].z = 0.0f;
        v[i].rhw = 1.0f;
        v[i].diffuse = 0xffffffff;
        v[i].specular = 0;
    }
    v[0].x = (float)d.left - 0.5f;  v[0].y = (float)d.top - 0.5f;    v[0].u = s.left / tw;  v[0].v = s.top / th;
    v[1].x = (float)d.right - 0.5f; v[1].y = (float)d.top - 0.5f;    v[1].u = s.right / tw; v[1].v = s.top / th;
    v[2].x = (float)d.left - 0.5f;  v[2].y = (float)d.bottom - 0.5f; v[2].u = s.left / tw;  v[2].v = s.bottom / th;
    v[3].x = (float)d.right - 0.5f; v[3].y = (float)d.bottom - 0.5f; v[3].u = s.right / tw; v[3].v = s.bottom / th;

    if (dst != d9.target)
        bind_target(dst);
    if (is_main_target(dst))
        scene_write();
    d9.taaFlat = 1;
    update_transparency_aa();
    apply_rs(D3DRS_ALPHATESTENABLE, keyed);
    apply_rs(D3DRS_ALPHABLENDENABLE, FALSE);
    apply_rs(D3DRS_ZENABLE, FALSE);
    apply_rs(D3DRS_ZWRITEENABLE, FALSE);
    apply_rs(D3DRS_FOGENABLE, FALSE);
    apply_rs(D3DRS_CULLMODE, D3DCULL_NONE);
    apply_rs(D3DRS_ALPHAREF, 1);
    apply_rs(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL);
    apply_rs(D3DRS_LIGHTING, FALSE);
    apply_tss(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    apply_tss(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    apply_tss(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
    apply_tss(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    apply_tss(0, D3DTSS_TEXCOORDINDEX, 0);
    apply_tss(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
    apply_tss(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
    apply_samp(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    apply_samp(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
    apply_samp(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
    apply_samp(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
    apply_samp(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
    IDirect3DDevice9_SetTexture(d9.dev, 0, (IDirect3DBaseTexture9 *)src->tex);
    IDirect3DDevice9_SetTexture(d9.dev, 1, NULL);
    IDirect3DDevice9_SetFVF(d9.dev, FVF_TEX1);
    if (!scene)
        IDirect3DDevice9_BeginScene(d9.dev);
    IDirect3DDevice9_DrawPrimitiveUP(d9.dev, D3DPT_TRIANGLESTRIP, 2, v, sizeof(v[0]));
    if (!scene)
        IDirect3DDevice9_EndScene(d9.dev);

    for (i = 0; i < (int)(sizeof(rs_ids) / sizeof(rs_ids[0])); i++)
        apply_rs(rs_ids[i], d9.rsSet[rs_ids[i]] ? d9.rs[rs_ids[i]] : rs_default[i]);
    for (i = 0; i < (int)(sizeof(tss_ids) / sizeof(tss_ids[0])); i++) {
        apply_tss(0, tss_ids[i], d9.tssSet[0][tss_ids[i]] ? d9.tss[0][tss_ids[i]] : tss0_default[i]);
        apply_tss(1, tss_ids[i], d9.tssSet[1][tss_ids[i]] ? d9.tss[1][tss_ids[i]] : tss1_default[i]);
    }
    for (i = 0; i < (int)(sizeof(samp_ids) / sizeof(samp_ids[0])); i++)
        apply_samp(0, samp_ids[i], d9.sampSet[0][samp_ids[i]] ? d9.samp[0][samp_ids[i]] : samp_default[i]);
    IDirect3DDevice9_SetTexture(d9.dev, 0, texture_of(d9.stageTex[0]));
    IDirect3DDevice9_SetTexture(d9.dev, 1, texture_of(d9.stageTex[1]));
    if (dst != prevTarget && prevTarget && prevTarget->surf)
        bind_target(prevTarget);
    return 0;
}

#define ADAPTER_TAG 0x43524548u

static void adapter_guid(UINT ordinal, GUID *g)
{
    memset(g, 0, sizeof(*g));
    g->Data1 = ADAPTER_TAG;
    g->Data2 = (WORD)ordinal;
    g->Data3 = 0x3944;
    memcpy(g->Data4, "adapter", 7);
}

static UINT adapter_ordinal(const GUID *guid)
{
    int i;
    if (!guid)
        return D3DADAPTER_DEFAULT;
    if (guid->Data1 == ADAPTER_TAG && guid->Data3 == 0x3944 && d9.d3d
        && guid->Data2 < IDirect3D9_GetAdapterCount(d9.d3d))
        return guid->Data2;
    for (i = 0; i < d9.nAdapters; i++)
        if (memcmp(&d9.adapters[i].guid, guid, sizeof(GUID)) == 0)
            return d9.adapters[i].ordinal;
    return D3DADAPTER_DEFAULT;
}

static int ensure_d3d(void)
{
    if (!d9.d3d)
        d9.d3d = Direct3DCreate9(D3D_SDK_VERSION);
    return d9.d3d != NULL;
}

int gfx_open(struct C3dDevice *d, const GUID *adapter, int flags)
{
    D3DDISPLAYMODE mode;
    if (!ensure_d3d())
        return GFX_E_DDRAW_CREATE;
    d->dd = d9.d3d;
    d9.adapter = adapter_ordinal(adapter);
    if (SUCCEEDED(IDirect3D9_GetAdapterDisplayMode(d9.d3d, d9.adapter, &mode)))
        d->desktopBpp = format_bpp(mode.Format);
    d9.behaviour = D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_MULTITHREADED;
    if (flags & 0x10)
        d9.behaviour |= D3DCREATE_FPU_PRESERVE;
    return 0;
}

static void format_masks(D3DFORMAT f, DWORD *r, DWORD *g, DWORD *b, DWORD *a)
{
    switch (f) {
    case D3DFMT_R5G6B5:   *r = 0xf800; *g = 0x7e0; *b = 0x1f; *a = 0; break;
    case D3DFMT_X1R5G5B5: *r = 0x7c00; *g = 0x3e0; *b = 0x1f; *a = 0; break;
    case D3DFMT_A1R5G5B5: *r = 0x7c00; *g = 0x3e0; *b = 0x1f; *a = 0x8000; break;
    case D3DFMT_A8R8G8B8: *r = 0xff0000; *g = 0xff00; *b = 0xff; *a = 0xff000000; break;
    default:              *r = 0xff0000; *g = 0xff00; *b = 0xff; *a = 0; break;
    }
}

static void fill_surface_desc(struct SurfaceDesc *sd, int w, int h, D3DFORMAT f, int refresh)
{
    memset(sd, 0, sizeof(*sd));
    sd->size = sizeof(*sd);
    sd->flags = 0x1 | 0x2 | 0x4 | 0x8 | 0x1000 | 0x40000;
    sd->height = h;
    sd->width = w;
    sd->pitch = w * (format_bpp(f) >> 3);
    sd->refreshRate = refresh;
    sd->pfSize = 0x20;
    sd->pfFlags = 0x40;
    sd->bitCount = format_bpp(f);
    format_masks(f, &sd->rMask, &sd->gMask, &sd->bMask, &sd->aMask);
}

int gfx_create_screen(struct C3dDevice *d, const struct SurfaceDesc *mode, int flags)
{
    IDirect3DSurface9 *bb = NULL, *save = NULL;
    D3DSURFACE_DESC desc;
    HRESULT hr;
    int backBuffer = flags & 2;
    if (!d9.d3d)
        return GFX_E_DDRAW_CREATE;
    memset(&d9.pp, 0, sizeof(d9.pp));
    d9.pp.BackBufferWidth = d->width;
    d9.pp.BackBufferHeight = d->height;
    d9.pp.BackBufferCount = 1;
    d9.pp.SwapEffect = D3DSWAPEFFECT_COPY;
    d9.pp.hDeviceWindow = d->hwnd;
    d9.pp.Windowed = !d->fullscreen;
    d9.pp.Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
    d9.pp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
    {
        char env[8];
        if (GetEnvironmentVariableA("RAGEXE_VSYNC", env, sizeof(env)) && env[0] == '0' && !env[1])
            d9.pp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
    }
    if (d->fullscreen) {
        d9.pp.BackBufferFormat = D3DFMT_X8R8G8B8;
        d9.pp.FullScreen_RefreshRateInHz = mode->refreshRate;
    } else {
        d9.pp.BackBufferFormat = D3DFMT_UNKNOWN;
    }
    hr = IDirect3D9_CreateDevice(d9.d3d, d9.adapter, D3DDEVTYPE_HAL, d->hwnd, d9.behaviour, &d9.pp, &d9.dev);
    if (FAILED(hr) && (d9.behaviour & D3DCREATE_HARDWARE_VERTEXPROCESSING)) {
        d9.behaviour = (d9.behaviour & ~D3DCREATE_HARDWARE_VERTEXPROCESSING) | D3DCREATE_SOFTWARE_VERTEXPROCESSING;
        hr = IDirect3D9_CreateDevice(d9.d3d, d9.adapter, D3DDEVTYPE_HAL, d->hwnd, d9.behaviour, &d9.pp, &d9.dev);
    }
    if (FAILED(hr))
        return d->fullscreen ? GFX_E_DISPLAY_MODE : GFX_E_PRIMARY;
    d->dev = d9.dev;
    forget_state();
    batch_init(d9_raw_draw);
    if (FAILED(IDirect3DDevice9_GetBackBuffer(d9.dev, 0, 0, D3DBACKBUFFER_TYPE_MONO, &bb)))
        return GFX_E_PRIMARY;
    IDirect3DSurface9_GetDesc(bb, &desc);
    d9.bbFormat = desc.Format;
    d9.bbBpp = format_bpp(desc.Format);
    format_masks(desc.Format, &d9.rMask, &d9.gMask, &d9.bMask, &d9.aMask);

    d->primary = new_handle(K_SCREEN, d->width, d->height, desc.Format);
    d->primary->surf = bb;
    if (backBuffer) {
        d->back = new_handle(K_TARGET, d->width, d->height, desc.Format);
        d->back->surf = bb;
        IDirect3DSurface9_AddRef(bb);
    }
    d->target = backBuffer ? d->back : d->primary;
    d->target->refs++;
    d9.mainTarget = d->target;
    d9.target = d->target;

    fill_surface_desc(&d->primaryDesc, d->width, d->height, desc.Format, 0);
    d->bpp = d9.bbBpp;
    d->rMask = d9.rMask;
    d->gMask = d9.gMask;
    d->bMask = d9.bMask;
    d->aMask = d9.aMask;

    if (FAILED(IDirect3DDevice9_CreateRenderTarget(d9.dev, d->width, d->height, desc.Format, D3DMULTISAMPLE_NONE, 0, FALSE,
                                                            &save, NULL)))
        return E_OUTOFMEMORY;
    d->save = new_handle(K_SAVE, d->width, d->height, desc.Format);
    d->save->surf = save;
    return 0;
}

static D3DFORMAT pick_zformat(int stencil)
{
    static const D3DFORMAT withStencil[] = { D3DFMT_D24S8, D3DFMT_D24X4S4, D3DFMT_D15S1, D3DFMT_D16 };
    static const D3DFORMAT plain[] = { D3DFMT_D16, D3DFMT_D24X8, D3DFMT_D32, D3DFMT_D24S8 };
    const D3DFORMAT *list = stencil ? withStencil : plain;
    D3DDISPLAYMODE mode;
    int i;
    if (FAILED(IDirect3D9_GetAdapterDisplayMode(d9.d3d, d9.adapter, &mode)))
        mode.Format = D3DFMT_X8R8G8B8;
    for (i = 0; i < 4; i++) {
        if (SUCCEEDED(IDirect3D9_CheckDeviceFormat(d9.d3d, d9.adapter, D3DDEVTYPE_HAL, mode.Format, D3DUSAGE_DEPTHSTENCIL,
                                                   D3DRTYPE_SURFACE, list[i]))
            && SUCCEEDED(IDirect3D9_CheckDepthStencilMatch(d9.d3d, d9.adapter, D3DDEVTYPE_HAL, mode.Format,
                                                           d9.bbFormat, list[i])))
            return list[i];
    }
    return D3DFMT_UNKNOWN;
}

int gfx_create_3d(struct C3dDevice *d, const GUID *device, int flags)
{
    D3DVIEWPORT9 vp;
    (void)device;
    if (!d9.dev)
        return GFX_E_DEVICE_CREATE;
    d->d3d = d9.d3d;
    d->zbufferCaps = 0;
    vp.X = 0;
    vp.Y = 0;
    vp.Width = d->width;
    vp.Height = d->height;
    vp.MinZ = 0.0f;
    vp.MaxZ = 1.0f;
    if (FAILED(IDirect3DDevice9_SetViewport(d9.dev, &vp)))
        return GFX_E_VIEWPORT;
    d9.zFormat = pick_zformat(flags & 8);
    memset(d->zformat, 0, sizeof(d->zformat));
    *(DWORD *)d->zformat = 0x20;
    *(DWORD *)(d->zformat + 4) = d9.zFormat;
    return d9.zFormat != D3DFMT_UNKNOWN ? 0 : GFX_E_ZBUFFER;
}

int gfx_create_zbuffer(struct C3dDevice *d)
{
    IDirect3DSurface9 *z = NULL;
    pick_multisample();
    if (d9.msType != D3DMULTISAMPLE_NONE && create_scene_target(d->width, d->height) < 0)
        d9.msType = D3DMULTISAMPLE_NONE;
    pick_transparency_aa();
    if (FAILED(IDirect3DDevice9_CreateDepthStencilSurface(d9.dev, d->width, d->height, d9.zFormat, d9.msType, d9.msQuality,
                                                          TRUE, &z, NULL)))
        return GFX_E_ZBUFFER;
    d->zbuffer = new_handle(K_ZBUFFER, d->width, d->height, d9.zFormat);
    d->zbuffer->surf = z;
    d9.depth = d->zbuffer;
    if (FAILED(d9.target ? bind_target(d9.target) : IDirect3DDevice9_SetDepthStencilSurface(d9.dev, z)))
        return GFX_E_ZBUFFER;
    return 0;
}

int gfx_get_caps(struct C3dDevice *d, struct gfx_caps *out)
{
    D3DCAPS9 caps;
    HRESULT hr;
    (void)d;
    if (!d9.d3d)
        return -1;
    hr = IDirect3D9_GetDeviceCaps(d9.d3d, d9.adapter, D3DDEVTYPE_HAL, &caps);
    if (hr == D3D_OK) {
        out->zbufferless = 0;
        out->vertexFog = (caps.RasterCaps & D3DPRASTERCAPS_FOGVERTEX) != 0;
        out->mipNearest = (caps.TextureFilterCaps & D3DPTFILTERCAPS_MIPFPOINT) != 0;
        out->bumpEnv = (caps.TextureOpCaps & D3DTEXOPCAPS_BUMPENVMAP) != 0;
        out->bumpEnvLuminance = (caps.TextureOpCaps & D3DTEXOPCAPS_BUMPENVMAPLUMINANCE) != 0;
        out->waterBump = (caps.MaxTextureBlendStages >= 2 && (caps.TextureOpCaps & D3DTEXOPCAPS_MODULATEALPHA_ADDCOLOR)) ? 1 : 0;
        out->minTexW = 1;
        out->minTexH = 1;
        out->maxTexW = caps.MaxTextureWidth;
        out->maxTexH = caps.MaxTextureHeight;
        out->texAspect = caps.MaxTextureAspectRatio;
        out->guardLeft = caps.GuardBandLeft;
        out->guardTop = caps.GuardBandTop;
        out->guardRight = caps.GuardBandRight;
        out->guardBottom = caps.GuardBandBottom;
        out->bltStretch = 1;
    }
    return hr;
}

void gfx_default_states(struct C3dDevice *d)
{
    static const struct { int state; DWORD value; } render_states[] = {
        { GFX_RS_CULLMODE, GFX_CULL_NONE },
        { GFX_RS_ZFUNC, GFX_CMP_LESSEQUAL },
        { GFX_RS_TEXTUREPERSPECTIVE, TRUE },
        { GFX_RS_ALPHAFUNC, GFX_CMP_GREATEREQUAL },
        { GFX_RS_ALPHAREF, 0xf0 },
        { GFX_RS_COLORKEYENABLE, TRUE },
        { GFX_RS_ZENABLE, TRUE },
        { GFX_RS_ZWRITEENABLE, TRUE },
        { GFX_RS_SRCBLEND, GFX_BLEND_SRCALPHA },
        { GFX_RS_DESTBLEND, GFX_BLEND_INVSRCALPHA },
    };
    static const struct { int stage, state; DWORD value; } stage_states[] = {
        { 0, GFX_TSS_TEXCOORDINDEX, 0 },
        { 0, GFX_TSS_COLOROP, GFX_TOP_MODULATE },
        { 0, GFX_TSS_COLORARG1, GFX_TA_TEXTURE },
        { 0, GFX_TSS_COLORARG2, GFX_TA_DIFFUSE },
        { 0, GFX_TSS_ALPHAOP, GFX_TOP_MODULATE },
        { 0, GFX_TSS_ALPHAARG1, GFX_TA_TEXTURE },
        { 0, GFX_TSS_ALPHAARG2, GFX_TA_DIFFUSE },
        { 1, GFX_TSS_TEXCOORDINDEX, 1 },
        { 1, GFX_TSS_COLORARG1, GFX_TA_TEXTURE },
        { 1, GFX_TSS_COLOROP, GFX_TOP_MODULATEALPHA_ADDCOLOR },
        { 1, GFX_TSS_COLORARG2, GFX_TA_CURRENT },
        { 1, GFX_TSS_ALPHAOP, GFX_TOP_DISABLE },
        { 2, GFX_TSS_COLOROP, GFX_TOP_DISABLE },
        { 2, GFX_TSS_ALPHAOP, GFX_TOP_DISABLE },
        { 0, GFX_TSS_MINFILTER, GFX_FILTER_LINEAR },
        { 0, GFX_TSS_MAGFILTER, GFX_FILTER_LINEAR },
        { 1, GFX_TSS_MINFILTER, GFX_FILTER_LINEAR },
        { 1, GFX_TSS_MAGFILTER, GFX_FILTER_LINEAR },
        { 0, GFX_TSS_ADDRESS, GFX_ADDRESS_CLAMP },
        { 1, GFX_TSS_ADDRESS, GFX_ADDRESS_BORDER },
        { 1, GFX_TSS_BORDERCOLOR, 0xff000000 },
    };
    int i;
    (void)d;
    for (i = 0; i < (int)(sizeof(render_states) / sizeof(render_states[0])); i++)
        gfx_set_render_state(render_states[i].state, render_states[i].value);
    for (i = 0; i < (int)(sizeof(stage_states) / sizeof(stage_states[0])); i++)
        gfx_set_stage_state(stage_states[i].stage, stage_states[i].state, stage_states[i].value);
}

int gfx_close(struct C3dDevice *d)
{
    batch_drop();
    struct gfx_surface *s, *next;
    if (d->target)  { gfx_release(d->target);  d->target = NULL; }
    if (d->back)    { gfx_release(d->back);    d->back = NULL; }
    if (d->primary) { gfx_release(d->primary); d->primary = NULL; }
    if (d->zbuffer) { gfx_release(d->zbuffer); d->zbuffer = NULL; }
    if (d->save)    { gfx_release(d->save);    d->save = NULL; }
    if (d9.msRT)    { gfx_release(d9.msRT);    d9.msRT = NULL; }
    if (d9.bbCopy)  { gfx_release(d9.bbCopy);  d9.bbCopy = NULL; }
    d9.msDirty = 0;
    d9.mainTarget = d9.target = d9.depth = NULL;
    for (s = d9.handles; s; s = next) {
        next = s->next;
        drop_objects(s);
    }
    if (d9.dev) {
        IDirect3DDevice9_Release(d9.dev);
        d9.dev = NULL;
    }
    if (d9.d3d) {
        IDirect3D9_Release(d9.d3d);
        d9.d3d = NULL;
    }
    d->dev = NULL;
    d->d3d = NULL;
    d->dd = NULL;
    forget_state();
    return 0;
}

void gfx_flip_to_gdi(struct C3dDevice *d, int unlockFirst)
{
    batch_flush();
    (void)unlockFirst;
    if (d9.dev && d->fullscreen && !d9.dialogMode) {
        if (SUCCEEDED(IDirect3DDevice9_SetDialogBoxMode(d9.dev, TRUE)))
            d9.dialogMode = 1;
    }
}

int gfx_present(struct C3dDevice *d, int vsync, int whole)
{
    HRESULT hr;
    batch_flush();
    (void)d;
    (void)whole;
    (void)vsync;
    if (!d9.dev)
        return -1;
    resolve_ms();
    hr = IDirect3DDevice9_Present(d9.dev, NULL, NULL, NULL, NULL);
    if (hr == D3DERR_DEVICELOST)
        return GFX_E_SURFACELOST;
    if (hr == D3DERR_DEVICEHUNG || hr == D3DERR_DEVICEREMOVED) {
        device_gone = 1;
        return GFX_E_SURFACELOST;
    }
    return FAILED(hr) ? (int)hr : 0;
}

static int recreate_after_reset(void)
{
    struct gfx_surface *s;
    IDirect3DSurface9 *bb = NULL;
    D3DSURFACE_DESC desc;
    d9.inScene = 0;
    if (FAILED(IDirect3DDevice9_GetBackBuffer(d9.dev, 0, 0, D3DBACKBUFFER_TYPE_MONO, &bb)))
        return GFX_E_SURFACELOST;
    if (SUCCEEDED(IDirect3DSurface9_GetDesc(bb, &desc)))
        d9.bbFormat = desc.Format;
    if (d9.msRT) {
        pick_multisample();
        if (d9.msType != D3DMULTISAMPLE_NONE && create_scene_target(d9.msRT->width, d9.msRT->height) < 0)
            d9.msType = D3DMULTISAMPLE_NONE;
        pick_transparency_aa();
    }
    for (s = d9.handles; s; s = s->next) {
        if (s == d9.msRT || s == d9.bbCopy)
            continue;
        switch (s->kind) {
        case K_SCREEN:
        case K_TARGET:
            s->surf = bb;
            IDirect3DSurface9_AddRef(bb);
            break;
        case K_SAVE:
            IDirect3DDevice9_CreateRenderTarget(d9.dev, s->width, s->height, s->d3dfmt, D3DMULTISAMPLE_NONE, 0, FALSE, &s->surf, NULL);
            break;
        case K_RTTEX:
            if (SUCCEEDED(IDirect3DDevice9_CreateTexture(d9.dev, s->width, s->height, 1, D3DUSAGE_RENDERTARGET, s->d3dfmt,
                                                         D3DPOOL_DEFAULT, &s->tex, NULL)))
                IDirect3DTexture9_GetSurfaceLevel(s->tex, 0, &s->surf);
            break;
        case K_ZBUFFER:
            IDirect3DDevice9_CreateDepthStencilSurface(d9.dev, s->width, s->height, s->d3dfmt, d9.msType, d9.msQuality,
                                                       TRUE, &s->surf, NULL);
            break;
        default:
            break;
        }
    }
    IDirect3DSurface9_Release(bb);
    d9.target = d9.mainTarget;
    if (d9.mainTarget && d9.mainTarget->surf)
        bind_target(d9.mainTarget);
    replay_state();
    return 0;
}

static int reset_device(void)
{
    batch_flush();
    struct gfx_surface *s;
    IDirect3DDevice9_SetDepthStencilSurface(d9.dev, NULL);
    if (d9.mainTarget && d9.mainTarget->surf)
        IDirect3DDevice9_SetRenderTarget(d9.dev, 0, d9.mainTarget->surf);
    d9.msDirty = 0;
    for (s = d9.handles; s; s = s->next) {
        if (s->kind == K_SCREEN || s->kind == K_TARGET || s->kind == K_SAVE || s->kind == K_RTTEX || s->kind == K_ZBUFFER
            || s->kind == K_MSRT)
            drop_objects(s);
        else if (s->kind == K_SYSMEM)
            s->dirty = 1;
    }
    {
        HRESULT hr = IDirect3DDevice9_Reset(d9.dev, &d9.pp);
        if (FAILED(hr)) {
            if (hr == D3DERR_DEVICEHUNG || hr == D3DERR_DEVICEREMOVED)
                device_gone = 1;
            return GFX_E_SURFACELOST;
        }
    }
    return recreate_after_reset();
}

static void migrate_texture(IDirect3DDevice9 *dev, struct gfx_surface *s);
static void migrate_sysmem(IDirect3DDevice9 *dev, struct gfx_surface *s);

static int recreate_device(struct C3dDevice *d)
{
    IDirect3DDevice9 *dev = NULL, *old = d9.dev;
    D3DPRESENT_PARAMETERS pp = d9.pp;
    struct gfx_surface *s;
    HRESULT hr;
    batch_drop();
    hr = IDirect3D9_CreateDevice(d9.d3d, d9.adapter, D3DDEVTYPE_HAL, d->hwnd, d9.behaviour, &pp, &dev);
    if (FAILED(hr) && !pp.Windowed) {
        pp.Windowed = TRUE;
        pp.BackBufferFormat = D3DFMT_UNKNOWN;
        pp.FullScreen_RefreshRateInHz = 0;
        hr = IDirect3D9_CreateDevice(d9.d3d, d9.adapter, D3DDEVTYPE_HAL, d->hwnd, d9.behaviour, &pp, &dev);
    }
    if (FAILED(hr))
        return GFX_E_SURFACELOST;
    for (s = d9.handles; s; s = s->next) {
        switch (s->kind) {
        case K_TEXTURE: migrate_texture(dev, s); break;
        case K_SYSMEM:  migrate_sysmem(dev, s); break;
        default:        drop_objects(s); break;
        }
    }
    IDirect3DDevice9_Release(old);
    d9.dev = dev;
    d->dev = dev;
    d9.pp = pp;
    d9.dialogMode = 0;
    return recreate_after_reset();
}

int gfx_restore_all(struct C3dDevice *d)
{
    batch_flush();
    HRESULT hr;
    int rc;
    if (!d9.dev)
        return -1;
    hr = IDirect3DDevice9_TestCooperativeLevel(d9.dev);
    if (hr == D3D_OK) {
        reset_failures = 0;
        device_gone = 0;
        return 0;
    }
    if (hr == D3DERR_DEVICEHUNG || hr == D3DERR_DEVICEREMOVED || device_gone
        || reset_failures >= RESET_TRIES_BEFORE_RECREATE) {
        rc = recreate_device(d);
        if (rc == 0) {
            reset_failures = 0;
            device_gone = 0;
        }
        return rc;
    }
    if (hr == D3DERR_DEVICENOTRESET) {
        rc = reset_device();
        if (rc == 0)
            reset_failures = 0;
        else
            reset_failures++;
        return rc;
    }
    return GFX_E_SURFACELOST;
}

void gfx_evict_managed_textures(struct C3dDevice *d)
{
    (void)d;
    if (d9.dev)
        IDirect3DDevice9_EvictManagedResources(d9.dev);
}

static void size_screen_handles(int w, int h)
{
    struct gfx_surface *s;
    d9.pp.BackBufferWidth = w;
    d9.pp.BackBufferHeight = h;
    for (s = d9.handles; s; s = s->next)
        if (s->kind == K_SCREEN || s->kind == K_TARGET || s->kind == K_SAVE || s->kind == K_ZBUFFER
            || s->kind == K_MSRT || s == d9.bbCopy) {
            s->width = w;
            s->height = h;
        }
}

static int change_mode(int w, int h, int fullscreen)
{
    D3DPRESENT_PARAMETERS old = d9.pp;
    int hr;
    size_screen_handles(w, h);
    d9.pp.Windowed = !fullscreen;
    d9.pp.BackBufferFormat = fullscreen ? D3DFMT_X8R8G8B8 : D3DFMT_UNKNOWN;
    d9.pp.FullScreen_RefreshRateInHz = 0;
    d9.dialogMode = 0;
    hr = reset_device();
    if (hr != 0) {
        d9.pp = old;
        size_screen_handles(old.BackBufferWidth, old.BackBufferHeight);
        reset_device();
    }
    return hr;
}

static int same_gpu(UINT a, UINT b)
{
    D3DADAPTER_IDENTIFIER9 ia, ib;
    if (FAILED(IDirect3D9_GetAdapterIdentifier(d9.d3d, a, 0, &ia)) || FAILED(IDirect3D9_GetAdapterIdentifier(d9.d3d, b, 0, &ib)))
        return 0;
    return ia.VendorId == ib.VendorId && ia.DeviceId == ib.DeviceId;
}

static int copy_surface(IDirect3DSurface9 *dst, IDirect3DSurface9 *src)
{
    D3DLOCKED_RECT dl, sl;
    D3DSURFACE_DESC desc;
    UINT y, bytes;
    if (FAILED(IDirect3DSurface9_GetDesc(src, &desc)))
        return -1;
    if (FAILED(IDirect3DSurface9_LockRect(src, &sl, NULL, D3DLOCK_READONLY)))
        return -1;
    if (FAILED(IDirect3DSurface9_LockRect(dst, &dl, NULL, 0))) {
        IDirect3DSurface9_UnlockRect(src);
        return -1;
    }
    bytes = (UINT)(sl.Pitch < dl.Pitch ? sl.Pitch : dl.Pitch);
    for (y = 0; y < desc.Height; y++)
        memcpy((BYTE *)dl.pBits + y * dl.Pitch, (const BYTE *)sl.pBits + y * sl.Pitch, bytes);
    IDirect3DSurface9_UnlockRect(dst);
    IDirect3DSurface9_UnlockRect(src);
    return 0;
}

static void migrate_texture(IDirect3DDevice9 *dev, struct gfx_surface *s)
{
    IDirect3DTexture9 *tex = NULL;
    IDirect3DSurface9 *from, *to;
    D3DSURFACE_DESC desc;
    DWORD levels, i;
    unsigned version = s->version;
    int copied = 1;
    if (!s->tex)
        return;
    levels = IDirect3DTexture9_GetLevelCount(s->tex);
    if (FAILED(IDirect3DTexture9_GetLevelDesc(s->tex, 0, &desc))
        || FAILED(IDirect3DDevice9_CreateTexture(dev, desc.Width, desc.Height, levels, desc.Usage, desc.Format,
                                                 D3DPOOL_MANAGED, &tex, NULL))) {
        drop_objects(s);
        return;
    }
    for (i = 0; i < levels; i++) {
        if (SUCCEEDED(IDirect3DTexture9_GetSurfaceLevel(s->tex, i, &from))) {
            if (SUCCEEDED(IDirect3DTexture9_GetSurfaceLevel(tex, i, &to))) {
                if (copy_surface(to, from) < 0)
                    copied = 0;
                IDirect3DSurface9_Release(to);
            } else {
                copied = 0;
            }
            IDirect3DSurface9_Release(from);
        } else {
            copied = 0;
        }
    }
    drop_objects(s);
    s->tex = tex;
    IDirect3DTexture9_GetSurfaceLevel(tex, 0, &s->surf);
    if (copied)
        s->version = version;
}

static void migrate_sysmem(IDirect3DDevice9 *dev, struct gfx_surface *s)
{
    IDirect3DSurface9 *surf = NULL;
    unsigned version = s->version;
    if (!s->surf)
        return;
    if (FAILED(IDirect3DDevice9_CreateOffscreenPlainSurface(dev, s->width, s->height, s->d3dfmt, D3DPOOL_SYSTEMMEM, &surf, NULL))) {
        drop_objects(s);
        return;
    }
    if (copy_surface(surf, s->surf) == 0)
        s->version = version;
    drop_objects(s);
    s->surf = surf;
    s->dirty = 1;
    if (s->version != version)
        s->version = ++content_tick;
}

static int switch_adapter(struct C3dDevice *d, UINT ordinal, int w, int h, int fullscreen)
{
    IDirect3DDevice9 *dev = NULL, *old = d9.dev;
    D3DPRESENT_PARAMETERS pp, oldpp = d9.pp;
    D3DDISPLAYMODE mode;
    struct gfx_surface *s;
    HRESULT hr;
    int rc;
    if (!same_gpu(d9.adapter, ordinal))
        return -1;
    batch_drop();
    if (!d9.pp.Windowed) {
        d9.pp.Windowed = TRUE;
        d9.pp.BackBufferFormat = D3DFMT_UNKNOWN;
        d9.pp.FullScreen_RefreshRateInHz = 0;
        if (reset_device() != 0) {
            d9.pp = oldpp;
            reset_device();
            return GFX_E_SURFACELOST;
        }
    }
    pp = d9.pp;
    pp.BackBufferWidth = w;
    pp.BackBufferHeight = h;
    pp.Windowed = TRUE;
    pp.BackBufferFormat = D3DFMT_UNKNOWN;
    pp.FullScreen_RefreshRateInHz = 0;
    hr = IDirect3D9_CreateDevice(d9.d3d, ordinal, D3DDEVTYPE_HAL, d->hwnd, d9.behaviour, &pp, &dev);
    if (FAILED(hr) && (d9.behaviour & D3DCREATE_HARDWARE_VERTEXPROCESSING)) {
        DWORD b = (d9.behaviour & ~D3DCREATE_HARDWARE_VERTEXPROCESSING) | D3DCREATE_SOFTWARE_VERTEXPROCESSING;
        hr = IDirect3D9_CreateDevice(d9.d3d, ordinal, D3DDEVTYPE_HAL, d->hwnd, b, &pp, &dev);
        if (SUCCEEDED(hr))
            d9.behaviour = b;
    }
    if (FAILED(hr)) {
        if (!oldpp.Windowed) {
            d9.pp = oldpp;
            reset_device();
        }
        return (int)hr;
    }
    for (s = d9.handles; s; s = s->next) {
        switch (s->kind) {
        case K_TEXTURE: migrate_texture(dev, s); break;
        case K_SYSMEM:  migrate_sysmem(dev, s); break;
        default:        drop_objects(s); break;
        }
    }
    IDirect3DDevice9_Release(old);
    d9.dev = dev;
    d->dev = dev;
    d9.adapter = ordinal;
    d9.pp = pp;
    d9.dialogMode = 0;
    size_screen_handles(w, h);
    if (SUCCEEDED(IDirect3D9_GetAdapterDisplayMode(d9.d3d, ordinal, &mode)))
        d->desktopBpp = format_bpp(mode.Format);
    rc = recreate_after_reset();
    if (rc != 0)
        return rc;
    if (fullscreen) {
        d9.pp.Windowed = FALSE;
        d9.pp.BackBufferFormat = D3DFMT_X8R8G8B8;
        d9.pp.FullScreen_RefreshRateInHz = 0;
        if (reset_device() != 0) {
            d9.pp.Windowed = TRUE;
            d9.pp.BackBufferFormat = D3DFMT_UNKNOWN;
            reset_device();
            return GFX_E_SURFACELOST;
        }
    }
    return 0;
}

int gfx_set_mode(struct C3dDevice *d, const GUID *adapter, int w, int h, int fullscreen)
{
    IDirect3DSurface9 *bb = NULL;
    D3DSURFACE_DESC desc;
    UINT ordinal;
    int hr;
    if (!d9.dev || w <= 0 || h <= 0)
        return -1;
    ordinal = adapter ? adapter_ordinal(adapter) : d9.adapter;
    {
        char env[8];
        int force = GetEnvironmentVariableA("RAGEXE_SWITCH_DEVICE", env, sizeof(env)) && env[0] == '1' && !env[1];
        hr = (ordinal != d9.adapter || force) ? switch_adapter(d, ordinal, w, h, fullscreen) : change_mode(w, h, fullscreen);
    }
    if (hr != 0)
        return hr;
    if (SUCCEEDED(IDirect3DDevice9_GetBackBuffer(d9.dev, 0, 0, D3DBACKBUFFER_TYPE_MONO, &bb))) {
        IDirect3DSurface9_GetDesc(bb, &desc);
        IDirect3DSurface9_Release(bb);
        d9.bbFormat = desc.Format;
        d9.bbBpp = format_bpp(desc.Format);
        format_masks(desc.Format, &d9.rMask, &d9.gMask, &d9.bMask, &d9.aMask);
        d->bpp = d9.bbBpp;
        d->rMask = d9.rMask;
        d->gMask = d9.gMask;
        d->bMask = d9.bMask;
        d->aMask = d9.aMask;
    }
    fill_surface_desc(&d->primaryDesc, w, h, d9.bbFormat, 0);
    return 0;
}

HMONITOR gfx_adapter_monitor(const GUID *adapter)
{
    if (!ensure_d3d())
        return NULL;
    return IDirect3D9_GetAdapterMonitor(d9.d3d, adapter ? adapter_ordinal(adapter) : d9.adapter);
}

void gfx_set_mipmaps(int on)
{
    d9.mipmaps = on;
}

void gfx_enum_adapter(const GUID *adapter, const char *desc, HMONITOR monitor)
{
    static const D3DFORMAT modeFormats[] = { D3DFMT_X8R8G8B8, D3DFMT_R5G6B5 };
    struct DisplayModeList modes = {0};
    D3DCAPS9 caps;
    UINT ordinal = D3DADAPTER_DEFAULT, n, i, f;
    if (!ensure_d3d())
        return;
    if (adapter) {
        n = IDirect3D9_GetAdapterCount(d9.d3d);
        for (i = 0; i < n; i++)
            if (IDirect3D9_GetAdapterMonitor(d9.d3d, i) == monitor)
                ordinal = i;
        if (d9.nAdapters < (int)(sizeof(d9.adapters) / sizeof(d9.adapters[0]))) {
            d9.adapters[d9.nAdapters].guid = *adapter;
            d9.adapters[d9.nAdapters].ordinal = ordinal;
            d9.nAdapters++;
        }
    }
    if (FAILED(IDirect3D9_GetDeviceCaps(d9.d3d, ordinal, D3DDEVTYPE_HAL, &caps)))
        return;
    for (f = 0; f < 2; f++) {
        struct { UINT w, h; } seen[256];
        int nseen = 0, k, dup;
        D3DDISPLAYMODE desk;
        if (FAILED(IDirect3D9_GetAdapterDisplayMode(d9.d3d, ordinal, &desk)))
            desk.Width = desk.Height = 0;
        n = IDirect3D9_GetAdapterModeCount(d9.d3d, ordinal, modeFormats[f]);
        for (i = 0; i < n; i++) {
            D3DDISPLAYMODE m;
            if (FAILED(IDirect3D9_EnumAdapterModes(d9.d3d, ordinal, modeFormats[f], i, &m)))
                continue;
            if (desk.Width && (m.Width > desk.Width || m.Height > desk.Height))
                continue;
            for (dup = 0, k = 0; k < nseen; k++)
                if (seen[k].w == m.Width && seen[k].h == m.Height) {
                    dup = 1;
                    break;
                }
            if (dup)
                continue;
            if (nseen < (int)(sizeof(seen) / sizeof(seen[0]))) {
                seen[nseen].w = m.Width;
                seen[nseen].h = m.Height;
                nseen++;
            }
            if (!display_modes_append(&modes, m.Width, m.Height,
                                      modeFormats[f] == D3DFMT_R5G6B5 ? 16 : 32))
                goto modes_ready;
        }
    }
modes_ready:
    display_modes_sort(&modes);
    display_devices_add(desc, adapter, desc, &IID_IDirect3DHALDevice, 1,
                        DISPLAY_DEPTH_16 | DISPLAY_DEPTH_32, &modes);
    display_modes_destroy(&modes);
}

gfx_surface *gfx_create_surface(int w, int h)
{
    g_perf.surfCreated++;
    D3DFORMAT f = d9.bbFormat != D3DFMT_UNKNOWN ? d9.bbFormat : D3DFMT_X8R8G8B8;
    IDirect3DSurface9 *surf = NULL;
    struct gfx_surface *s;
    if (!d9.dev)
        return NULL;
    if (FAILED(IDirect3DDevice9_CreateOffscreenPlainSurface(d9.dev, w, h, f, D3DPOOL_SYSTEMMEM, &surf, NULL)))
        return NULL;
    s = new_handle(K_SYSMEM, w, h, f);
    s->surf = surf;
    s->key = pack(f, 0x00ff00ff);
    s->dirty = 1;
    return s;
}

int gfx_texture_format_ok(int fmt)
{
    D3DDISPLAYMODE mode;
    D3DFORMAT f = client_format(fmt);
    if (!d9.d3d || f == D3DFMT_UNKNOWN)
        return 0;
    if (FAILED(IDirect3D9_GetAdapterDisplayMode(d9.d3d, d9.adapter, &mode)))
        mode.Format = D3DFMT_X8R8G8B8;
    return SUCCEEDED(IDirect3D9_CheckDeviceFormat(d9.d3d, d9.adapter, D3DDEVTYPE_HAL, mode.Format, 0, D3DRTYPE_TEXTURE, f));
}

static int mip_levels(int w, int h)
{
    int levels = 1, most = 1, n;
    if (w > 2) {
        for (;;) {
            if ((1 << levels) >= h)
                break;
            levels++;
            n = 1 << levels;
            if (n >= w)
                break;
        }
    }
    for (n = w > h ? w : h; n > 1; n >>= 1)
        most++;
    return levels < most ? levels : most;
}

gfx_surface *gfx_create_texture(int w, int h, int fmt)
{
    g_perf.texCreated++;
    D3DFORMAT f = client_format(fmt);
    IDirect3DTexture9 *tex = NULL;
    struct gfx_surface *s;
    int levels;
    if (!d9.dev || f == D3DFMT_UNKNOWN)
        return NULL;
    levels = (d9.mipmaps && fmt != GFX_FMT_BUMP) ? mip_levels(w, h) : 1;
    if (FAILED(IDirect3DDevice9_CreateTexture(d9.dev, w, h, levels, 0, f, D3DPOOL_MANAGED, &tex, NULL)))
        return NULL;
    s = new_handle(K_TEXTURE, w, h, f);
    s->fmt = fmt;
    s->tex = tex;
    s->levels = levels;
    IDirect3DTexture9_GetSurfaceLevel(tex, 0, &s->surf);
    return s;
}

gfx_surface *gfx_create_render_target(int w, int h)
{
    IDirect3DTexture9 *tex = NULL;
    struct gfx_surface *s;
    if (!d9.dev)
        return NULL;
    if (FAILED(IDirect3DDevice9_CreateTexture(d9.dev, w, h, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT,
                                              &tex, NULL)))
        return NULL;
    s = new_handle(K_RTTEX, w, h, D3DFMT_A8R8G8B8);
    s->fmt = GFX_FMT_8888;
    s->tex = tex;
    IDirect3DTexture9_GetSurfaceLevel(tex, 0, &s->surf);
    return s;
}

int gfx_get_size(gfx_surface *s, int *w, int *h)
{
    *w = s->width;
    *h = s->height;
    return 0;
}

int gfx_lock(gfx_surface *s, const RECT *r, int flags, struct gfx_lock *out)
{
    batch_flush();
    D3DLOCKED_RECT lr;
    HRESULT hr;
    if (flags & GFX_LOCK_UNSIZED)
        return D3DERR_INVALIDCALL;
    if (!s->surf)
        return D3DERR_INVALIDCALL;
    g_perf.locks++;
    if (s->kind == K_SCREEN || s->kind == K_TARGET) {
        g_perf.targetLocks++;
        resolve_ms();
    } else {
        perf_begin(PERF_TEXLOCK);
    }
    hr = IDirect3DSurface9_LockRect(s->surf, &lr, r, 0);
    if (s->kind != K_SCREEN && s->kind != K_TARGET)
        perf_end(PERF_TEXLOCK);
    if (FAILED(hr))
        return hr;
    out->bits = lr.pBits;
    out->pitch = lr.Pitch;
    out->width = s->width;
    out->height = s->height;
    out->bpp = format_bpp(s->d3dfmt);
    return 0;
}

int gfx_unlock(gfx_surface *s)
{
    HRESULT result = D3D_OK;
    s->version = ++content_tick;
    if (s->surf)
        result = IDirect3DSurface9_UnlockRect(s->surf);
    if (s->kind == K_SYSMEM)
        s->dirty = 1;
    if (is_main_target(s))
        unresolve_ms();
    return result;
}

static int is_default_pool(const struct gfx_surface *s)
{
    return s->kind == K_SCREEN || s->kind == K_TARGET || s->kind == K_SAVE || s->kind == K_RTTEX;
}

int gfx_blit(gfx_surface *dst, const RECT *dr, gfx_surface *src, const RECT *sr, int flags)
{
    batch_flush();
    (void)flags;
    g_perf.blits++;
    dst->version = ++content_tick;
    if (!d9.dev || !dst->surf || !src->surf)
        return -1;
    if (dst->surf == src->surf)
        return 0;
    if (is_main_target(src))
        resolve_ms();
    if (is_default_pool(dst) && is_default_pool(src)) {
        HRESULT hr = IDirect3DDevice9_StretchRect(d9.dev, src->surf, sr, dst->surf, dr, D3DTEXF_POINT);
        if (is_main_target(dst))
            unresolve_ms();
        return hr;
    }
    if (is_default_pool(dst) && src->kind == K_SYSMEM)
        return draw_surface_quad(dst, dr, src, sr, 0);
    if (is_default_pool(src)) {
        IDirect3DSurface9 *sys = target_copy(src);
        struct gfx_surface tmp;
        int r;
        if (!sys)
            return -1;
        memset(&tmp, 0, sizeof(tmp));
        tmp.kind = K_SYSMEM;
        tmp.width = src->width;
        tmp.height = src->height;
        tmp.d3dfmt = src->d3dfmt;
        tmp.surf = sys;
        r = cpu_blit(dst, dr, &tmp, sr, 0, 0);
        IDirect3DSurface9_Release(sys);
        if (dst->kind == K_SYSMEM)
            dst->dirty = 1;
        return r;
    }
    if (dst->kind == K_SYSMEM)
        dst->dirty = 1;
    return cpu_blit(dst, dr, src, sr, 0, 0);
}

int gfx_refresh_mips(gfx_surface *tex, const RECT *r)
{
    batch_flush();
    RECT rc, top;
    D3DLOCKED_RECT lr;
    BYTE *stage;
    HRESULT result;
    int level, bytes, pw, ph, x, y;
    if (!tex || !tex->tex || tex->levels <= 1 || tex->d3dfmt == D3DFMT_V8U8)
        return D3D_OK;
    bytes = format_bpp(tex->d3dfmt) >> 3;
    if (r) top = *r; else { top.left = 0; top.top = 0; top.right = tex->width; top.bottom = tex->height; }
    pw = top.right - top.left;
    ph = top.bottom - top.top;
    if (pw <= 0 || ph <= 0)
        return D3DERR_INVALIDCALL;
    result = IDirect3DTexture9_LockRect(tex->tex, 0, &lr, &top, D3DLOCK_READONLY);
    if (FAILED(result))
        return result;
    stage = (BYTE *)malloc((size_t)pw * ph * bytes);
    if (stage) {
        for (y = 0; y < ph; y++)
            memcpy(stage + (size_t)y * pw * bytes, (const BYTE *)lr.pBits + y * lr.Pitch, (size_t)pw * bytes);
    }
    result = IDirect3DTexture9_UnlockRect(tex->tex, 0);
    if (!stage || FAILED(result)) {
        free(stage);
        return FAILED(result) ? result : E_OUTOFMEMORY;
    }
    rc = top;
    for (level = 1; level < tex->levels; level++) {
        int w, h;
        rc.left >>= 1;
        rc.right >>= 1;
        rc.top >>= 1;
        rc.bottom >>= 1;
        if (rc.left == rc.right || rc.top == rc.bottom)
            continue;
        w = rc.right - rc.left;
        h = rc.bottom - rc.top;
        result = IDirect3DTexture9_LockRect(tex->tex, level, &lr, &rc, 0);
        if (FAILED(result))
            break;
        for (y = 0; y < h; y++) {
            BYTE *d = (BYTE *)lr.pBits + y * lr.Pitch;
            const BYTE *srow = stage + (size_t)((2 * y + 1) * ph / (2 * h)) * pw * bytes;
            for (x = 0; x < w; x++)
                memcpy(d + x * bytes, srow + (size_t)((2 * x + 1) * pw / (2 * w)) * bytes, bytes);
        }
        result = IDirect3DTexture9_UnlockRect(tex->tex, level);
        if (FAILED(result))
            break;
    }
    free(stage);
    return result;
}

int gfx_is_lost(gfx_surface *s)
{
    (void)s;
    if (!d9.dev)
        return 0;
    return IDirect3DDevice9_TestCooperativeLevel(d9.dev) == D3D_OK ? 0 : GFX_E_SURFACELOST;
}

void gfx_restore(gfx_surface *s)
{
    s->version = ++content_tick;
}

int gfx_get_dc(gfx_surface *s, HDC *out)
{
    batch_flush();
    if (!s->surf)
        return -1;
    g_perf.dcGets++;
    if (is_main_target(s))
        resolve_ms();
    return IDirect3DSurface9_GetDC(s->surf, out);
}

void gfx_release_dc(gfx_surface *s, HDC dc)
{
    s->version = ++content_tick;
    if (s->surf)
        IDirect3DSurface9_ReleaseDC(s->surf, dc);
    if (s->kind == K_SYSMEM)
        s->dirty = 1;
    if (is_main_target(s))
        unresolve_ms();
}

int gfx_begin_scene(void)
{
    HRESULT hr;
    if (!d9.dev)
        return -1;
    hr = IDirect3DDevice9_BeginScene(d9.dev);
    if (SUCCEEDED(hr))
        d9.inScene = 1;
    return hr;
}

void gfx_end_scene(void)
{
    batch_flush();
    if (d9.dev)
        IDirect3DDevice9_EndScene(d9.dev);
    d9.inScene = 0;
}

int gfx_clear(int flags, DWORD colour, float z, const RECT *r)
{
    batch_flush();
    if (!d9.dev)
        return -1;
    if (is_main_target(d9.target))
        scene_write();
    return IDirect3DDevice9_Clear(d9.dev, r ? 1 : 0, (const D3DRECT *)r, flags, colour, z, 0);
}

gfx_surface *gfx_get_render_target(void)
{
    if (!d9.target)
        return NULL;
    d9.target->refs++;
    return d9.target;
}

int gfx_set_render_target(gfx_surface *s)
{
    batch_flush();
    HRESULT hr;
    if (!d9.dev || !s || !s->surf)
        return -1;
    hr = bind_target(s);
    if (FAILED(hr))
        return hr;
    d9.target = s;
    return 0;
}

void gfx_set_render_state(int state, DWORD value)
{
    if (state == GFX_RS_TEXTUREPERSPECTIVE || state == GFX_RS_COLORKEYENABLE)
        return;
    set_rs(state, value);
}

int gfx_get_render_state(int state, DWORD *out)
{
    if (!d9.dev)
        return -1;
    if (state == GFX_RS_TEXTUREPERSPECTIVE || state == GFX_RS_COLORKEYENABLE) {
        *out = (unsigned)state < N_RS && d9.rsSet[state] ? d9.rs[state] : TRUE;
        return 0;
    }
    return IDirect3DDevice9_GetRenderState(d9.dev, (D3DRENDERSTATETYPE)state, out);
}

void gfx_set_stage_state(int stage, int state, DWORD value)
{
    switch (state) {
    case GFX_TSS_ADDRESS:
        set_samp(stage, D3DSAMP_ADDRESSU, value);
        set_samp(stage, D3DSAMP_ADDRESSV, value);
        break;
    case GFX_TSS_BORDERCOLOR:
        set_samp(stage, D3DSAMP_BORDERCOLOR, value);
        break;
    case GFX_TSS_MAGFILTER:
        set_samp(stage, D3DSAMP_MAGFILTER, value == GFX_FILTER_LINEAR ? D3DTEXF_LINEAR : D3DTEXF_POINT);
        break;
    case GFX_TSS_MINFILTER:
        set_samp(stage, D3DSAMP_MINFILTER, value == GFX_FILTER_LINEAR ? D3DTEXF_LINEAR : D3DTEXF_POINT);
        break;
    case GFX_TSS_MIPFILTER:
        set_samp(stage, D3DSAMP_MIPFILTER, value == GFX_MIPFILTER_LINEAR ? D3DTEXF_LINEAR
                                          : value == GFX_MIPFILTER_NONE ? D3DTEXF_NONE : D3DTEXF_POINT);
        break;
    default:
        set_tss(stage, state, value);
        break;
    }
}

void gfx_set_texture(int stage, gfx_surface *s)
{
    if ((unsigned)stage < 8 && d9.stageTex[stage] == s)
        return;
    batch_flush();
    g_perf.texSets++;
    if ((unsigned)stage < 8)
        d9.stageTex[stage] = s;
    if (d9.dev)
        IDirect3DDevice9_SetTexture(d9.dev, stage, texture_of(s));
}

void gfx_set_transform(int which, const struct GfxMatrix *m)
{
    batch_flush();
    if ((unsigned)which < 4) {
        memcpy(&d9.xf[which], m, sizeof(D3DMATRIX));
        d9.xfSet[which] = 1;
    }
    if (d9.dev)
        IDirect3DDevice9_SetTransform(d9.dev, which == GFX_TS_WORLD ? D3DTS_WORLD : (D3DTRANSFORMSTATETYPE)which,
                                      (const D3DMATRIX *)m);
}

void gfx_set_light(const float *dir, const float *diffuse, const float *ambient)
{
    batch_flush();
    D3DLIGHT9 *l = &d9.light;
    memset(l, 0, sizeof(*l));
    l->Type = D3DLIGHT_DIRECTIONAL;
    l->Diffuse.r = diffuse[0];
    l->Diffuse.g = diffuse[1];
    l->Diffuse.b = diffuse[2];
    l->Diffuse.a = 255.0f;
    l->Ambient.r = ambient[0];
    l->Ambient.g = ambient[1];
    l->Ambient.b = ambient[2];
    l->Ambient.a = 255.0f;
    l->Direction.x = -dir[0];
    l->Direction.y = -dir[1];
    l->Direction.z = -dir[2];
    d9.lightSet = 1;
    if (d9.dev) {
        IDirect3DDevice9_SetLight(d9.dev, 0, l);
        IDirect3DDevice9_LightEnable(d9.dev, 0, TRUE);
    }
}

static UINT prim_count(int prim, int n)
{
    switch (prim) {
    case GFX_PT_POINTLIST:     return n;
    case GFX_PT_LINELIST:      return n / 2;
    case GFX_PT_LINESTRIP:     return n > 1 ? n - 1 : 0;
    case GFX_PT_TRIANGLELIST:  return n / 3;
    case GFX_PT_TRIANGLESTRIP:
    case GFX_PT_TRIANGLEFAN:   return n > 2 ? n - 2 : 0;
    default:                   return 0;
    }
}

void gfx_draw(int prim, int vertexKind, const void *verts, int nverts, const WORD *indices, int nindices)
{
    g_perf.drawCalls++;
    if (flat_draw(vertexKind, verts, nverts))
        vertexKind |= FLAT_KIND;
    if (batch_add(prim, vertexKind, verts, nverts, indices, nindices))
        return;
    batch_flush();
    d9_raw_draw(prim, vertexKind, verts, nverts, indices, nindices);
}

static void d9_raw_draw(int prim, int vertexKind, const void *verts, int nverts, const WORD *indices, int nindices)
{
    int flat = (vertexKind & FLAT_KIND) != 0;
    DWORD fvf, stride;
    vertexKind &= ~FLAT_KIND;
    fvf = vertexKind == GFX_VTX_TEX2 ? FVF_TEX2 : FVF_TEX1;
    stride = vertexKind == GFX_VTX_TEX2 ? 40 : 32;
    if (!d9.dev)
        return;
    g_perf.draws++;
    g_perf.prims += perf_prim_count(prim, nindices ? nindices : nverts);
    if (is_main_target(d9.target))
        scene_write();
    d9.taaFlat = flat;
    update_transparency_aa();
    IDirect3DDevice9_SetFVF(d9.dev, fvf);
    if (nindices)
        IDirect3DDevice9_DrawIndexedPrimitiveUP(d9.dev, (D3DPRIMITIVETYPE)prim, 0, nverts, prim_count(prim, nindices),
                                                indices, D3DFMT_INDEX16, verts, stride);
    else
        IDirect3DDevice9_DrawPrimitiveUP(d9.dev, (D3DPRIMITIVETYPE)prim, prim_count(prim, nverts), verts, stride);
}

void gfx_draw_vbuffer(int prim, gfx_vbuffer *vb, DWORD start, DWORD count, const WORD *indices, DWORD nindices)
{
    batch_flush();
    if (!d9.dev)
        return;
    if (is_main_target(d9.target))
        scene_write();
    d9.taaFlat = 0;
    update_transparency_aa();
    IDirect3DDevice9_SetStreamSource(d9.dev, 0, vb->vb, 0, vb->stride);
    IDirect3DDevice9_SetFVF(d9.dev, vb->fvf);
    if (indices) {
        IDirect3DIndexBuffer9 *ib = NULL;
        void *p = NULL;
        if (FAILED(IDirect3DDevice9_CreateIndexBuffer(d9.dev, nindices * 2, D3DUSAGE_WRITEONLY, D3DFMT_INDEX16,
                                                      D3DPOOL_MANAGED, &ib, NULL)))
            return;
        if (SUCCEEDED(IDirect3DIndexBuffer9_Lock(ib, 0, 0, &p, 0))) {
            memcpy(p, indices, nindices * 2);
            IDirect3DIndexBuffer9_Unlock(ib);
            IDirect3DDevice9_SetIndices(d9.dev, ib);
            IDirect3DDevice9_DrawIndexedPrimitive(d9.dev, (D3DPRIMITIVETYPE)prim, start, 0, count, 0, prim_count(prim, nindices));
            IDirect3DDevice9_SetIndices(d9.dev, NULL);
        }
        IDirect3DIndexBuffer9_Release(ib);
    } else {
        IDirect3DDevice9_DrawPrimitive(d9.dev, (D3DPRIMITIVETYPE)prim, start, prim_count(prim, count));
    }
}

int gfx_ddraw_enumerate(GfxAdapterCallback callback, void *ctx, DWORD flags)
{
    UINT n, i;
    (void)flags;
    if (!ensure_d3d())
        return E_FAIL;
    n = IDirect3D9_GetAdapterCount(d9.d3d);
    for (i = 0; i < n; i++) {
        D3DADAPTER_IDENTIFIER9 id;
        char desc[MAX_DEVICE_IDENTIFIER_STRING + 24];
        GUID g;
        if (FAILED(IDirect3D9_GetAdapterIdentifier(d9.d3d, i, 0, &id)))
            continue;
        adapter_guid(i, &g);
        if (i == 0)
            lstrcpynA(desc, id.Description, sizeof(desc));
        else
            wsprintfA(desc, "%s (display %u)", id.Description, i + 1);
        if (!callback(i == 0 ? NULL : &g, desc, id.DeviceName, ctx, IDirect3D9_GetAdapterMonitor(d9.d3d, i)))
            break;
    }
    return 0;
}

unsigned gfx_content_version(gfx_surface *s)
{
    return s ? s->version : 0;
}

struct gfx_caps g_gfxCaps;
