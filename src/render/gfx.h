#ifndef GFX_H
#define GFX_H

#define WIN32_LEAN_AND_MEAN
#include "platform/win_types.h"
#include "platform/window_handle.h"

typedef struct gfx_surface gfx_surface;
typedef struct gfx_mesh gfx_mesh;
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
    int trilinear;
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
#define GFX_CULL_CCW          3
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
#define GFX_E_FAIL               ((int)0x80004005)
#define GFX_E_INVALIDARG         ((int)0x80070057)

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
void gfx_enum_adapter(const GUID *adapter, const char *desc, PlatformDisplay monitor);

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

/* custom: static geometry in world coordinates that the device projects itself, where everything
   else arrives projected by the CPU (renderer_project_vertex, render/rp_project.c). A mesh keeps
   its vertices and indices; the device copies them to its own buffers on the first draw and again
   after it is recreated. Indices are triangle lists into the mesh's own vertices. Meshes are made,
   drawn and destroyed on the thread that draws. A device draws meshes with shaders only:
   gfx_mesh_create answers NULL where it has none, and the geometry then goes the CPU's way.
   A vertex has two colours, a draw picks one (GfxMeshDraw). */
struct GfxMeshVertex {
    float x, y, z;
    float nx, ny, nz;
    DWORD diffuse, colour2;
    float u0, v0, u1, v1;
};

/* The projection a mesh is drawn with, the CPU's own (render/rp_project.c): view is the client's
   4x3 view matrix, view-space x = view[0] * x + view[3] * y + view[6] * z + view[9], y likewise
   with entries 1, 4, 7, 10 and the depth vz with 2, 5, 8, 11. A point lands at the pixel
   (x * proj[0] / vz + centre[0], y * proj[1] / vz + centre[1]) with the depth
   (vz - nearZ) * depthScale / vz, as a pre-transformed vertex with rhw 1 / vz would. Its fog factor
   is (fogEnd - vz) / (fogEnd - fogStart), clamped to 0..1, as rp_vertex_fog_alpha puts it in the
   specular alpha of a pre-transformed vertex. */
struct GfxWorldView {
    float view[12];
    float proj[2];
    float centre[2];
    float nearZ, depthScale;
    float fogStart, fogEnd;
};

/* One draw of a mesh: `count` indices from `first`, a multiple of three each. kind picks what the
   stages would do with the bound textures and what a vertex's colours hold:
   - GFX_MESH_GROUND: texture times colour, times the lightmap's alpha, plus its colour, as the
     ground's two-texture quads have it; the colour is the vertex's diffuse.
   - GFX_MESH_MODEL: texture times colour, both colour and alpha. The diffuse's red, green and blue
     are the model's own lit colour (0 to 255 a channel) and its alpha the share of the light it
     was lit with (0, 128 or 255, 0 for a model lit without its light); with lightGround set,
     colour2 is the ground's light under the vertex, added at most to 255 a channel. The alpha
     is colour[3] alone.
   The colour's red, green and blue are then multiplied by colour (0..1) and rounded down, as the
   CPU scales them by the weather colour, its alpha by colour[3]. The first texture's coordinates
   go through uv: (u * uv[0] + v * uv[2] + uv[4], u * uv[1] + v * uv[3] + uv[5]). The render
   states (blending, the alpha test, depth, culling, fog on or off) and the texture addressing are
   the current ones, as they would be for gfx_draw. */
#define GFX_MESH_MODEL 1
#define GFX_MESH_GROUND 2

struct GfxMeshDraw {
    int kind, first, count, lightGround;
    float colour[4];
    float uv[6];
};

gfx_mesh *gfx_mesh_create(const struct GfxMeshVertex *verts, int nverts, const WORD *indices, int nindices);
void gfx_mesh_destroy(gfx_mesh *mesh);
void gfx_set_world_view(const struct GfxWorldView *view);
/* Draws as gfx_draw would draw the same triangles pre-transformed, in the world view set last. */
void gfx_draw_mesh(gfx_mesh *mesh, const struct GfxMeshDraw *draw);

/* custom: cascaded shadow maps for meshes, each GFX_SHADOW_SIZE texels square. Between
   gfx_shadow_begin and gfx_shadow_end, gfx_draw_mesh draws the meshes' depth from the light into
   the cascade gfx_shadow_cascade chose instead of colour, testing the texture bound to stage 0,
   where there is one, for alpha at the draw's texture coordinates: texels the opaque lists' alpha
   test (0xcf) drops cast nothing.
   fromView[c] takes the view space of the draws' world views to cascade c's light space, row
   vectors: x and y from -1 to 1 across its map, z from 0 to 1 along the light. While
   gfx_shadow_receive is on, a pixel that lies more than bias behind the nearest caster is drawn
   in shadow: a model's with its own lit colour times darkness, but no darker than ambient times
   its share of the light (what a face turned from the light has), before the ground's light is
   added; the ground's as described below. A pixel is looked up in the first cascade whose
   farDepth (a view depth) lies beyond it,
   from a point moved off the surface towards the light by offset[c] (view units) times the sine of
   the angle between the surface and the light (lightView: towards the light, view space; the
   surface's normal comes from the vertex's), as the share of 16 points spread over a disc of
   radius blur[c] (texture units of the cascade's map) that are lit, so that shadow edges fade over
   a fixed width. Over that disc a surface at a slant to the light moves along it, so the bias grows
   by slant (light depth) times the tangent of that slant, up to 8 times. On a GFX_MESH_GROUND draw
   the shadow maps take the place of the lightmap's alpha, the shadows the map's editor baked into
   it: down to darkness that alpha counts as lit, and the shadow maps darken to darkness; a texel
   darker still keeps its own. gfx_shadow_begin answers 0 where the device cannot draw shadows;
   gfx_shadow_receive(0) turns receiving off, and the draws then look as without shadows. */
#define GFX_SHADOW_SIZE 2048
#define GFX_SHADOW_CASCADES 4

struct GfxShadow {
    int cascades;
    float fromView[GFX_SHADOW_CASCADES][16];
    float farDepth[GFX_SHADOW_CASCADES];
    float offset[GFX_SHADOW_CASCADES];
    float blur[GFX_SHADOW_CASCADES];
    float lightView[3];
    float bias, slant, darkness;
    float ambient[3];
};

int gfx_shadow_begin(const struct GfxShadow *shadow);
int gfx_shadow_cascade(int cascade);
void gfx_shadow_end(void);
void gfx_shadow_receive(int receiver);

typedef BOOL (*GfxAdapterCallback)(GUID *adapter, char *desc, char *name, void *ctx, PlatformDisplay monitor);
int gfx_ddraw_enumerate(GfxAdapterCallback callback, void *ctx, DWORD flags);

unsigned gfx_content_version(gfx_surface *s);

int gfx_set_mode(struct C3dDevice *d, const GUID *adapter, int w, int h, int fullscreen);
int gfx_can_set_mode_live(const GUID *adapter);
PlatformDisplay gfx_adapter_monitor(const GUID *adapter);

int stale_guid(void);

#endif
