/* test_canvas_texels.c - the canvas's scaled texel draw of the original against the lift.
 *
 * The routine at 0x4d0fb0 (CDCBitmap's vtable slot 0x34, the one its sprite loop at 0x4cff70
 * calls) draws a texture's 4444 or 1555 texels, scaled, mirrored and tinted, into a canvas's 32-bit
 * pixels. It locks the texture's surface with a DDSURFACEDESC2 zeroed by memset and never sized,
 * with DDLOCK_WAIT. DirectDraw accepts that for a texture outside video memory, and the client's
 * textures are all managed (DDSCAPS_TEXTURE with DDSCAPS2_TEXTUREMANAGE, 0x4cdaa0 and 0x4cdfb0),
 * which DirectDraw keeps in system memory, so the lock succeeds and the routine draws.
 *
 * The test therefore gives the original real DirectDraw textures: managed, 16-bit, created through
 * the system's ddraw.dll, and first checks that the original's own unsized lock succeeds on them.
 * The lift (dc_bitmap_draw_texels_scaled, src/render/cdcbitmap.c) reads the same surfaces through
 * a gfx_lock that locks them sized. Both sides draw into canvases filled alike, sized with two
 * rows to spare, since both may write one row past the canvas, and the whole buffers are compared
 * over formats, tints, scales, mirroring, offsets, source rectangles and destination rectangles. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <ddraw.h>
#include "oracle_image.h"
#include "render/cdc.h"
#include "render/texture_types.h"

#define ORIG_DRAW_TEXELS_SCALED 0x4d0fb0u

#define TEX_W 64
#define TEX_H 72
#define CANVAS_W 64
#define CANVAS_H 48
#define CANVAS_PIXELS (CANVAS_W * (CANVAS_H + 2))
#define MAX_REPORTED 40

struct OrigCanvas {
    void *vtable;
    int width, height;
    void *hdc, *dib, *oldBitmap;
    DWORD *bits;
    BYTE rest[0x40];
};

struct OrigTexture {
    void *vtable;
    int width, height;
    IDirectDrawSurface7 *surface;
    int format;
    BYTE rest[0x40];
};

typedef void (__fastcall *draw_fn)(struct OrigCanvas *self, void *edx, int dx0, int dy0, int dw, int dh,
                                   struct OrigTexture *tex, int sw, int sh, float scaleX, float scaleY,
                                   DWORD colour, char mirror, int offX, int offY, int sx, int sy);

static unsigned mismatches, faults, reported, drawn, compared;
static unsigned fault_code, fault_va;

int gfx_lock(gfx_surface *s, const RECT *r, struct gfx_lock *out)
{
    IDirectDrawSurface7 *surface = (IDirectDrawSurface7 *)s;
    DDSURFACEDESC2 desc;
    if (r)
        return -1;
    memset(&desc, 0, sizeof desc);
    desc.dwSize = sizeof desc;
    if (FAILED(IDirectDrawSurface7_Lock(surface, NULL, &desc, DDLOCK_WAIT, NULL)))
        return -1;
    out->bits = desc.lpSurface;
    out->pitch = desc.lPitch;
    out->width = desc.dwWidth;
    out->height = desc.dwHeight;
    out->bpp = 16;
    return 0;
}

int gfx_unlock(gfx_surface *s)
{
    return IDirectDrawSurface7_Unlock((IDirectDrawSurface7 *)s, NULL);
}

static int note_fault(EXCEPTION_POINTERS *ep)
{
    fault_code = ep->ExceptionRecord->ExceptionCode;
    fault_va = (unsigned)((unsigned char *)ep->ExceptionRecord->ExceptionAddress - (unsigned char *)oracle_addr(0x400000u)) + 0x400000u;
    return EXCEPTION_EXECUTE_HANDLER;
}

static unsigned next_random(unsigned *state)
{
    *state = *state * 1103515245u + 12345u;
    return *state >> 8;
}

static IDirectDrawSurface7 *managed_texture(IDirectDraw7 *dd, int format)
{
    DDSURFACEDESC2 desc;
    IDirectDrawSurface7 *surface = NULL;
    unsigned state = 0x1234u + (unsigned)format, x, y;
    memset(&desc, 0, sizeof desc);
    desc.dwSize = sizeof desc;
    desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
    desc.dwWidth = TEX_W;
    desc.dwHeight = TEX_H;
    desc.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
    desc.ddsCaps.dwCaps2 = DDSCAPS2_TEXTUREMANAGE;
    desc.ddpfPixelFormat.dwSize = sizeof desc.ddpfPixelFormat;
    desc.ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
    desc.ddpfPixelFormat.dwRGBBitCount = 16;
    if (format == GFX_FMT_4444) {
        desc.ddpfPixelFormat.dwRGBAlphaBitMask = 0xf000;
        desc.ddpfPixelFormat.dwRBitMask = 0x0f00;
        desc.ddpfPixelFormat.dwGBitMask = 0x00f0;
        desc.ddpfPixelFormat.dwBBitMask = 0x000f;
    } else {
        desc.ddpfPixelFormat.dwRGBAlphaBitMask = 0x8000;
        desc.ddpfPixelFormat.dwRBitMask = 0x7c00;
        desc.ddpfPixelFormat.dwGBitMask = 0x03e0;
        desc.ddpfPixelFormat.dwBBitMask = 0x001f;
    }
    if (FAILED(IDirectDraw7_CreateSurface(dd, &desc, &surface, NULL)))
        return NULL;
    memset(&desc, 0, sizeof desc);
    desc.dwSize = sizeof desc;
    if (FAILED(IDirectDrawSurface7_Lock(surface, NULL, &desc, DDLOCK_WAIT, NULL))) {
        IDirectDrawSurface7_Release(surface);
        return NULL;
    }
    for (y = 0; y < TEX_H; y++) {
        WORD *row = (WORD *)((BYTE *)desc.lpSurface + y * desc.lPitch);
        for (x = 0; x < TEX_W; x++) {
            WORD texel = (WORD)next_random(&state);
            if (x % 7 == 3)
                texel &= format == GFX_FMT_4444 ? 0x0fff : 0x7fff;
            row[x] = texel;
        }
    }
    IDirectDrawSurface7_Unlock(surface, NULL);
    return surface;
}

static int original_lock_succeeds(IDirectDrawSurface7 *surface)
{
    DDSURFACEDESC2 desc;
    HRESULT hr;
    memset(&desc, 0, sizeof desc);
    hr = IDirectDrawSurface7_Lock(surface, NULL, &desc, DDLOCK_WAIT, NULL);
    if (FAILED(hr)) {
        printf("  the original's unsized lock failed: hr %08lx\n", (unsigned long)hr);
        return 0;
    }
    IDirectDrawSurface7_Unlock(surface, NULL);
    return desc.lpSurface != NULL && desc.lPitch >= TEX_W * 2;
}

static void fill_canvas(DWORD *pixels, unsigned seed)
{
    unsigned state = seed, i;
    for (i = 0; i < CANVAS_PIXELS; i++)
        pixels[i] = next_random(&state) * 0x101u + (next_random(&state) << 24);
}

struct Case {
    int format, dx0, dy0, dw, dh, sw, sh, offX, offY, sx, sy;
    float scaleX, scaleY;
    DWORD colour;
    char mirror;
};

static int call_original(draw_fn draw, struct OrigCanvas *canvas, struct OrigTexture *tex, const struct Case *c)
{
    __try {
        draw(canvas, NULL, c->dx0, c->dy0, c->dw, c->dh, tex, c->sw, c->sh, c->scaleX, c->scaleY, c->colour,
             c->mirror, c->offX, c->offY, c->sx, c->sy);
        return 1;
    }
    __except (note_fault(GetExceptionInformation())) { return 0; }
}

static void describe(const struct Case *c)
{
    printf("format %d dest %d,%d %dx%d source %d,%d %dx%d scale %g,%g colour %08lx mirror %d offset %d,%d",
           c->format, c->dx0, c->dy0, c->dw, c->dh, c->sx, c->sy, c->sw, c->sh, c->scaleX, c->scaleY,
           (unsigned long)c->colour, c->mirror, c->offX, c->offY);
}

static void run_case(draw_fn draw, IDirectDrawSurface7 *surface, const struct Case *c)
{
    static DWORD orig_pixels[CANVAS_PIXELS], lift_pixels[CANVAS_PIXELS], before[CANVAS_PIXELS];
    struct OrigCanvas ocanvas;
    struct OrigTexture otex;
    struct DcBitmap lcanvas;
    struct Texture ltex;
    unsigned i;

    fill_canvas(before, (unsigned)compared * 2654435761u);
    memcpy(orig_pixels, before, sizeof before);
    memcpy(lift_pixels, before, sizeof before);
    memset(&ocanvas, 0, sizeof ocanvas);
    ocanvas.width = CANVAS_W;
    ocanvas.height = CANVAS_H;
    ocanvas.bits = orig_pixels;
    memset(&otex, 0, sizeof otex);
    otex.width = TEX_W;
    otex.height = TEX_H;
    otex.surface = surface;
    otex.format = c->format;
    memset(&lcanvas, 0, sizeof lcanvas);
    lcanvas.width = CANVAS_W;
    lcanvas.height = CANVAS_H;
    lcanvas.bits = lift_pixels;
    memset(&ltex, 0, sizeof ltex);
    ltex.base.surface = (gfx_surface *)surface;
    ltex.base.width = TEX_W;
    ltex.base.height = TEX_H;
    ltex.format = c->format;

    compared++;
    if (!call_original(draw, &ocanvas, &otex, c)) {
        faults++;
        if (reported++ < MAX_REPORTED) {
            printf("  the original faulted (code %08x at 0x%x): ", fault_code, fault_va);
            describe(c);
            printf("\n");
        }
        return;
    }
    dc_bitmap_draw_texels_scaled(&lcanvas, c->dx0, c->dy0, c->dw, c->dh, &ltex, c->sw, c->sh, c->scaleX, c->scaleY,
                                 c->colour, c->mirror, c->offX, c->offY, c->sx, c->sy);
    if (memcmp(orig_pixels, before, sizeof before))
        drawn++;
    for (i = 0; i < CANVAS_PIXELS; i++)
        if (orig_pixels[i] != lift_pixels[i])
            break;
    if (i == CANVAS_PIXELS)
        return;
    mismatches++;
    if (reported++ < MAX_REPORTED) {
        printf("  ");
        describe(c);
        printf(": pixel %u,%u was %08lx, original %08lx, lift %08lx\n", i % CANVAS_W, i / CANVAS_W,
               (unsigned long)before[i], (unsigned long)orig_pixels[i], (unsigned long)lift_pixels[i]);
    }
}

static int visible_draw(draw_fn draw, IDirectDrawSurface7 *surface, int format)
{
    static DWORD pixels[CANVAS_PIXELS], before[CANVAS_PIXELS];
    struct OrigCanvas canvas;
    struct OrigTexture tex;
    struct Case c = { 0 };
    fill_canvas(before, 77u);
    memcpy(pixels, before, sizeof before);
    memset(&canvas, 0, sizeof canvas);
    canvas.width = CANVAS_W;
    canvas.height = CANVAS_H;
    canvas.bits = pixels;
    memset(&tex, 0, sizeof tex);
    tex.surface = surface;
    tex.format = format;
    c.format = format;
    c.dw = CANVAS_W;
    c.dh = CANVAS_H - 1;
    c.sw = 32;
    c.sh = 32;
    c.scaleX = c.scaleY = 1.0f;
    c.colour = 0xffffffffu;
    return call_original(draw, &canvas, &tex, &c) && memcmp(pixels, before, sizeof before) != 0;
}

int main(int argc, char **argv)
{
    static const int formats[] = { GFX_FMT_1555, GFX_FMT_4444, 2 };
    static const DWORD colours[] = { 0xffffffffu, 0x66ffffffu, 0x80ff8040u, 0x01000000u, 0x00ffffffu, 0x00000001u, 0u };
    static const float scales[][2] = { { 1.0f, 1.0f }, { 0.5f, 0.5f }, { 2.0f, 2.0f }, { 1.5f, 0.75f },
                                       { 0.37f, 2.6f }, { 3.0f, 1.0f }, { 0.0f, 1.0f }, { 1.0f, -1.0f } };
    static const int sources[][4] = { { 0, 0, 16, 12 }, { 3, 2, 40, 33 }, { 0, 0, 64, 64 }, { 5, 9, 1, 1 } };
    static const int dests[][4] = { { 0, 0, CANVAS_W, CANVAS_H }, { 4, 6, 40, 30 }, { 10, 10, 0, 0 },
                                    { 0, 0, CANVAS_W, CANVAS_H - 1 }, { 1, 0, CANVAS_W, 4 }, { -1, 0, 8, 8 },
                                    { 0, 0, -1, 8 } };
    static const int offsets[][2] = { { 0, 0 }, { -5, 3 }, { 20, -2 }, { 63, 47 } };
    const char *exe = argc > 1 ? argv[1] : getenv("ORACLE_EXE");
    IDirectDraw7 *dd = NULL;
    IDirectDrawSurface7 *surfaces[3];
    draw_fn draw;
    unsigned f, col, s, src, dst, o, m;
    HRESULT hr;

    if (!exe) {
        fprintf(stderr, "usage: test_canvas_texels <original exe>\n");
        return 2;
    }
    if (!oracle_load(exe))
        return 2;
    draw = (draw_fn)oracle_addr(ORIG_DRAW_TEXELS_SCALED);

    hr = DirectDrawCreateEx(NULL, (void **)&dd, &IID_IDirectDraw7, NULL);
    if (FAILED(hr)) {
        printf("canvas texels: DirectDraw could not be created (hr %08lx)\n", (unsigned long)hr);
        return 1;
    }
    hr = IDirectDraw7_SetCooperativeLevel(dd, NULL, DDSCL_NORMAL);
    if (FAILED(hr)) {
        printf("canvas texels: SetCooperativeLevel failed (hr %08lx)\n", (unsigned long)hr);
        return 1;
    }
    for (f = 0; f < 3; f++) {
        surfaces[f] = managed_texture(dd, formats[f] == GFX_FMT_4444 ? GFX_FMT_4444 : GFX_FMT_1555);
        if (!surfaces[f] || !original_lock_succeeds(surfaces[f])) {
            printf("canvas texels: no managed texture the original can lock (format %d)\n", formats[f]);
            return 1;
        }
    }
    for (f = 0; f < 2; f++)
        if (!visible_draw(draw, surfaces[f], formats[f])) {
            printf("canvas texels: the original drew nothing with format %d at full tint\n", formats[f]);
            return 1;
        }

    for (f = 0; f < 3; f++)
        for (col = 0; col < sizeof colours / sizeof colours[0]; col++)
            for (s = 0; s < sizeof scales / sizeof scales[0]; s++)
                for (src = 0; src < sizeof sources / sizeof sources[0]; src++)
                    for (dst = 0; dst < sizeof dests / sizeof dests[0]; dst++)
                        for (o = 0; o < sizeof offsets / sizeof offsets[0]; o++)
                            for (m = 0; m < 3; m++) {
                                struct Case c;
                                c.format = formats[f];
                                c.colour = colours[col];
                                c.scaleX = scales[s][0];
                                c.scaleY = scales[s][1];
                                c.sx = sources[src][0];
                                c.sy = sources[src][1];
                                c.sw = sources[src][2];
                                c.sh = sources[src][3];
                                c.dx0 = dests[dst][0];
                                c.dy0 = dests[dst][1];
                                c.dw = dests[dst][2];
                                c.dh = dests[dst][3];
                                c.offX = offsets[o][0];
                                c.offY = offsets[o][1];
                                c.mirror = (char)m;
                                run_case(draw, surfaces[f], &c);
                            }

    for (f = 0; f < 3; f++)
        IDirectDrawSurface7_Release(surfaces[f]);
    IDirectDraw7_Release(dd);
    printf("canvas texels: %u calls, %u drawn by the original, %u mismatches, %u faults\n", compared, drawn,
           mismatches, faults);
    return mismatches || faults || !drawn ? 1 : 0;
}
