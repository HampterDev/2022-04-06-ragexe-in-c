#ifndef GFX_H
#define GFX_H

#define WIN32_LEAN_AND_MEAN
#include "platform/win_types.h"

typedef struct gfx_surface gfx_surface;
typedef struct gfx_vbuffer gfx_vbuffer;
struct C3dDevice;

struct SurfaceDesc {
    DWORD size;
    DWORD flags;
    DWORD height;
    DWORD width;
    LONG  pitch;
    DWORD backBufferCount;
    DWORD refreshRate;
    DWORD alphaBitDepth;
    void *surface;
    DWORD colourKeys[8];
    DWORD pfSize;
    DWORD pfFlags;
    DWORD fourCC;
    DWORD bitCount;
    DWORD rMask;
    DWORD gMask;
    DWORD bMask;
    DWORD aMask;
    DWORD caps[4];
    DWORD textureStage;
};

struct gfx_lock {
    void *bits;
    LONG pitch;
    DWORD width, height;
    DWORD bpp;
};

struct GfxMatrix {
    float m[4][4];
};

struct gfx_caps {
    int zbufferless;
    int mipNearest;
    int vertexFog;
    int bumpEnv;
    int bumpEnvLuminance;
    int waterBump;
    DWORD minTexW, minTexH;
    DWORD maxTexW, maxTexH;
    DWORD texAspect;
    float guardLeft, guardTop, guardRight, guardBottom;
    BYTE bltStretch;
};
extern struct gfx_caps g_gfxCaps;

#define GFX_FMT_1555 0
#define GFX_FMT_4444 1
#define GFX_FMT_565  2
#define GFX_FMT_8888 4
#define GFX_FMT_BUMP 5

#define GFX_VTX_TEX1 1
#define GFX_VTX_TEX2 2

#define GFX_PT_POINTLIST     1
#define GFX_PT_LINELIST      2
#define GFX_PT_LINESTRIP     3
#define GFX_PT_TRIANGLELIST  4
#define GFX_PT_TRIANGLESTRIP 5
#define GFX_PT_TRIANGLEFAN   6

#define GFX_TS_WORLD      1
#define GFX_TS_VIEW       2
#define GFX_TS_PROJECTION 3

#define GFX_CLEAR_TARGET  1
#define GFX_CLEAR_ZBUFFER 2

#define GFX_LOCK_WAIT      1
#define GFX_LOCK_WRITEONLY 2
#define GFX_LOCK_RETRY     4
#define GFX_LOCK_UNSIZED   8

#define GFX_BLT_WAIT  1
#define GFX_BLT_ASYNC 2

#define GFX_RS_TEXTUREPERSPECTIVE 4
#define GFX_RS_ZENABLE            7
#define GFX_RS_ZWRITEENABLE       14
#define GFX_RS_ALPHATESTENABLE    15
#define GFX_RS_SRCBLEND           19
#define GFX_RS_DESTBLEND          20
#define GFX_RS_CULLMODE           22
#define GFX_RS_ZFUNC              23
#define GFX_RS_ALPHAREF           24
#define GFX_RS_ALPHAFUNC          25
#define GFX_RS_ALPHABLENDENABLE   27
#define GFX_RS_FOGENABLE          28
#define GFX_RS_FOGCOLOR           34
#define GFX_RS_FOGTABLEMODE       35
#define GFX_RS_FOGSTART           36
#define GFX_RS_FOGEND             37
#define GFX_RS_FOGDENSITY         38
#define GFX_RS_COLORKEYENABLE     41
#define GFX_RS_RANGEFOGENABLE     48
#define GFX_RS_AMBIENT            139
#define GFX_RS_FOGVERTEXMODE      140

#define GFX_TSS_COLOROP       1
#define GFX_TSS_COLORARG1     2
#define GFX_TSS_COLORARG2     3
#define GFX_TSS_ALPHAOP       4
#define GFX_TSS_ALPHAARG1     5
#define GFX_TSS_ALPHAARG2     6
#define GFX_TSS_TEXCOORDINDEX 11
#define GFX_TSS_ADDRESS       12
#define GFX_TSS_BORDERCOLOR   15
#define GFX_TSS_MAGFILTER     16
#define GFX_TSS_MINFILTER     17
#define GFX_TSS_MIPFILTER     18

#define GFX_BLEND_ONE         2
#define GFX_BLEND_SRCALPHA    5
#define GFX_BLEND_INVSRCALPHA 6
#define GFX_CULL_NONE         1
#define GFX_CULL_CW           2
#define GFX_CMP_LESS          2
#define GFX_CMP_LESSEQUAL     4
#define GFX_CMP_GREATEREQUAL  7
#define GFX_FOG_NONE          0
#define GFX_TOP_DISABLE       1
#define GFX_TOP_SELECTARG1    2
#define GFX_TOP_SELECTARG2    3
#define GFX_TOP_MODULATE      4
#define GFX_TOP_MODULATE2X    5
#define GFX_TOP_ADD           7
#define GFX_TOP_ADDSIGNED     8
#define GFX_TOP_BLENDDIFFUSEALPHA      12
#define GFX_TOP_BLENDTEXTUREALPHA      13
#define GFX_TOP_MODULATEALPHA_ADDCOLOR 18
#define GFX_TA_DIFFUSE        0
#define GFX_TA_CURRENT        1
#define GFX_TA_TEXTURE        2
#define GFX_TA_COMPLEMENT     0x10
#define GFX_TA_ALPHAREPLICATE 0x20
#define GFX_ADDRESS_WRAP      1
#define GFX_ADDRESS_CLAMP     3
#define GFX_ADDRESS_BORDER    4
#define GFX_FILTER_LINEAR     2
#define GFX_MIPFILTER_NONE    1
#define GFX_MIPFILTER_LINEAR  3

#define GFX_E_INIT              0x82000000
#define GFX_E_DDRAW_CREATE      0x82000001
#define GFX_E_DEVICE_CREATE     0x82000004
#define GFX_E_ZBUFFER           0x82000005
#define GFX_E_VIEWPORT          0x82000006
#define GFX_E_PRIMARY           0x82000007
#define GFX_E_DISPLAY_MODE      0x82000009
#define GFX_E_NO_PRIMARY        0x8200000e
#define GFX_E_SURFACELOST        ((int)0x887601c2)

int gfx_open(struct C3dDevice *d, const GUID *adapter, int flags);
int gfx_create_screen(struct C3dDevice *d, const struct SurfaceDesc *mode, int flags);
int gfx_create_3d(struct C3dDevice *d, const GUID *device, int flags);
int gfx_create_zbuffer(struct C3dDevice *d);
int gfx_get_caps(struct C3dDevice *d, struct gfx_caps *out);
void gfx_default_states(struct C3dDevice *d);
int gfx_close(struct C3dDevice *d);
void gfx_flip_to_gdi(struct C3dDevice *d, int unlockFirst);
int gfx_present(struct C3dDevice *d, int vsync, int whole);
int gfx_restore_all(struct C3dDevice *d);
void gfx_evict_managed_textures(struct C3dDevice *d);
void gfx_set_mipmaps(int on);
void gfx_enum_adapter(const GUID *adapter, const char *desc, HMONITOR monitor);

gfx_surface *gfx_create_surface(int w, int h);
gfx_surface *gfx_create_texture(int w, int h, int fmt);
int gfx_texture_format_ok(int fmt);
gfx_surface *gfx_create_render_target(int w, int h);
void gfx_addref(gfx_surface *s);
void gfx_release(gfx_surface *s);
int gfx_get_size(gfx_surface *s, int *w, int *h);
int gfx_lock(gfx_surface *s, const RECT *r, int flags, struct gfx_lock *out);
int gfx_unlock(gfx_surface *s);
int gfx_blit(gfx_surface *dst, const RECT *dstRect, gfx_surface *src, const RECT *srcRect, int flags);
int gfx_refresh_mips(gfx_surface *tex, const RECT *r);
int gfx_is_lost(gfx_surface *s);
void gfx_restore(gfx_surface *s);
int gfx_get_dc(gfx_surface *s, HDC *out);
void gfx_release_dc(gfx_surface *s, HDC dc);

int gfx_begin_scene(void);
void gfx_end_scene(void);
int gfx_clear(int flags, DWORD colour, float z, const RECT *r);
gfx_surface *gfx_get_render_target(void);
int gfx_set_render_target(gfx_surface *s);
void gfx_set_render_state(int state, DWORD value);
int gfx_get_render_state(int state, DWORD *out);
void gfx_set_stage_state(int stage, int state, DWORD value);
void gfx_set_texture(int stage, gfx_surface *s);
void gfx_set_transform(int which, const struct GfxMatrix *m);
void gfx_set_light(const float *dir, const float *diffuse, const float *ambient);
void gfx_draw(int prim, int vertexKind, const void *verts, int nverts, const WORD *indices, int nindices);

void gfx_draw_vbuffer(int prim, gfx_vbuffer *vb, DWORD start, DWORD count, const WORD *indices, DWORD nindices);

int gfx_ddraw_enumerate(BOOL (WINAPI *callback)(GUID *, LPSTR, LPSTR, LPVOID, HMONITOR), void *ctx, DWORD flags);

unsigned gfx_content_version(gfx_surface *s);

int gfx_set_mode(struct C3dDevice *d, const GUID *adapter, int w, int h, int fullscreen);
HMONITOR gfx_adapter_monitor(const GUID *adapter);

int stale_guid(void);

#endif
