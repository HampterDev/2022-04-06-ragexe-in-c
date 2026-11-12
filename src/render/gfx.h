#ifndef GFX_H
#define GFX_H

#define WIN32_LEAN_AND_MEAN
#include "platform/win_types.h"
#include "platform/window_handle.h"
#include "render/gfx_format.h"
#include "render/draw_modes.h"
#include "render/quads.h"
#include <stddef.h>

typedef struct gfx_surface gfx_surface;
typedef struct gfx_mesh gfx_mesh;
struct C3dDevice;
struct WobbleRow;

struct gfx_lock {
    void *bits;
    LONG pitch;
    DWORD width, height;
    DWORD bpp;
};

struct gfx_caps {
    int trilinear;
    DWORD minTexW, minTexH;
    DWORD maxTexW, maxTexH;
    DWORD texAspect;
    float guardLeft, guardTop, guardRight, guardBottom;
};
extern struct gfx_caps g_gfxCaps;

#define GFX_CLEAR_TARGET  1
#define GFX_CLEAR_ZBUFFER 2

/* gfx_lock's one flag: a lock the original asked for without the surface's size, which failed, so
   the lock fails as well */
#define GFX_LOCK_UNSIZED   8

/* custom: the cull mode of a mesh's draw (GfxMeshDraw): Direct3D 9's, which culls by the winding on
   the screen, y down: GFX_CULL_CW drops the clockwise triangles, GFX_CULL_CCW the others. */
#define GFX_CULL_NONE         1
#define GFX_CULL_CW           2
#define GFX_CULL_CCW          3

#define GFX_E_DDRAW_CREATE      0x82000001
#define GFX_E_ZBUFFER           0x82000005
#define GFX_E_PRIMARY           0x82000007
#define GFX_E_NO_PRIMARY        0x8200000e
#define GFX_E_INVALIDARG         ((int)0x80070057)

int gfx_open(struct C3dDevice *d);
/* custom: the device draws into its window at d->width by d->height and never changes the
   display's mode; fullscreen is the window's (render/display_options.h). The screen, d->target, is
   the one surface the frame is drawn into and the window shows. */
int gfx_create_screen(struct C3dDevice *d);
int gfx_create_zbuffer(struct C3dDevice *d);
int gfx_get_caps(struct gfx_caps *out);
int gfx_close(struct C3dDevice *d);
/* custom: the frame into the window, which the device claims at the first present while it is
   shown; a window never shown (the scene harness's) has no swapchain, and its frames are drawn and
   read (gfx_read_frame), not shown. */
int gfx_present(int vsync);
void gfx_set_mipmaps(int on);

gfx_surface *gfx_create_surface(int w, int h);
gfx_surface *gfx_create_texture(int w, int h, int fmt);
/* custom: a texture that the device places, when a draw first reads it, as a layer of an array of
   textures of its size, format and mipmaps, so that the quads of several such textures are drawn
   as one draw (the sprite atlas's pages, render/rp_atlas.c). Only quads read it. */
gfx_surface *gfx_create_layer_texture(int w, int h, int fmt);
int gfx_texture_format_ok(int fmt);
gfx_surface *gfx_create_render_target(int w, int h);
void gfx_addref(gfx_surface *s);
void gfx_release(gfx_surface *s);
int gfx_get_size(gfx_surface *s, int *w, int *h);
/* custom: the CPU's pixels of a texture or a system surface, a rectangle of them or all; a render
   target has none (-1), its pixels stay on the GPU. */
int gfx_lock(gfx_surface *s, const RECT *r, int flags, struct gfx_lock *out);
int gfx_unlock(gfx_surface *s);
/* custom: a copy from a render target goes into a render target, on the GPU; into a texture or a
   system surface it is refused (-1). */
int gfx_blit(gfx_surface *dst, const RECT *dstRect, gfx_surface *src, const RECT *srcRect);
int gfx_refresh_mips(gfx_surface *tex, const RECT *r);

int gfx_begin_scene(void);
void gfx_end_scene(void);
int gfx_clear(int flags, DWORD colour, float z, const RECT *r);
gfx_surface *gfx_get_render_target(void);
int gfx_set_render_target(gfx_surface *s);

/* custom: what draws draw with, in place of Direct3D 9's render and texture stage states: the mode
   (render/draw_modes.h, enum DrawMode), set last, which also says what targets it may draw into
   and whether meshes may draw with it; a draw with a mode that may not draw into the current target
   is not drawn. An effect mode (DRAW_EFFECT_*) blends by the D3DBLEND factors gfx_set_effect set
   last, taken whole (draw_blend_factors), and combines the stages by the enum DrawCombine it set
   (draw_effect_combine gives an effect preset's); a number that is no combination keeps the one
   before, as the Direct3D 9 client's stage states stayed what they were while its blend factors
   still changed. The device starts with DRAW_OVERLAY and blending by DRAW_BLEND_SRCALPHA and
   DRAW_BLEND_INVSRCALPHA with DRAW_COMBINE_MODULATE. */
void gfx_set_mode(int mode);
void gfx_set_effect(int srcBlend, int destBlend, int combine);

/* custom: the fog of the modes that fog, while on, computed for each vertex and interpolated across
   the triangle, as Direct3D 9 did with the vertex fog: the vertex keeps f of its colour and takes
   1 - f of `colour`'s (a D3DCOLOR). For the quads f is the original's vertex fog, which it computed
   on the CPU into a vertex's specular alpha, to the bit: 1 - t in 1/255 steps rounded down,
   t = ((1 / rhw - origin) / span - start) / (end - start) clamped to 0..1, without the division
   where end equals start, in single precision (render/vertex_fog.h; a vertex in world coordinates
   by the rhw it carries, one whose rhw is not positive as at the smallest normal one). For
   the meshes f is (b - vz) / (b - a) clamped to 0..1 for the view depth vz, a and b the view depths
   start * span + origin and end * span + origin (origin and origin + span where end equals start),
   as the Direct3D 9 client's mesh shader computed it. The device starts without fog. */
struct GfxFog {
    int on;
    DWORD colour;
    float start, end;
    float origin, span;
};
void gfx_set_fog(const struct GfxFog *fog);

/* The texture a stage reads, until another is set; NULL reads opaque white. */
void gfx_set_texture(int stage, gfx_surface *s);

/* custom: the client's quads and triangles, every one an instance (render/quads.h) whose vertices
   the device's vertex shader makes exactly as the client's C code made them, in the current mode,
   textures and target. Consecutive instances drawn with the same mode, effect, fog, textures (or
   layers of one array, gfx_create_layer_texture) and target, all flat or none, become one draw, in
   the order given, as Direct3D 9's draws were until a state changed. Direct3D 9 drew flat draws
   (every vertex the same rhw) without multisampling; in a multisampled pass their corners move
   onto the pixel boundaries that keep the pixels Direct3D 9 covered.
   gfx_draw_quad draws one the client made (a sprite, a projected quad); gfx_draw_vertices one
   of its primitives of n vertices: a strip of 4 (a quad) or a list of triangles. Pre-transformed
   vertices are drawn as they are, flat when every one has the same rhw; with world set, their x, y
   and z are world coordinates, which the device takes through the camera's world view
   (gfx_set_world_camera) to the pixel and depth a pre-transformed vertex would have there, as
   Direct3D 9 drew them, and their rhw is the CPU's projection's, which only the fog reads. */
void gfx_draw_quad(const struct GfxQuad *quad);
void gfx_draw_vertices(const struct TexDrawVertex *v, int n, int strip, int world);

/* custom: static geometry in world coordinates that the device projects itself, where the quads
   above arrive on the screen, as instances whose vertices the vertex shader makes as the CPU made
   them, or in the world for the camera's view (gfx_draw_vertices with world). A mesh keeps
   its vertices and indices; the device copies them to its own buffers on the first draw and again
   after it is recreated. Indices are triangle lists into the mesh's own vertices. Meshes are made,
   drawn and destroyed on the thread that draws. gfx_mesh_create answers NULL only for vertices or
   indices it cannot take, or without memory. A vertex has two colours, a draw picks
   one (GfxMeshDraw). */
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
   (vz - nearZ) * depthScale / vz, as a pre-transformed vertex with rhw 1 / vz would. */
struct GfxWorldView {
    float view[12];
    float proj[2];
    float centre[2];
    float nearZ, depthScale;
};

/* One draw of a mesh: `count` indices from `first`, a multiple of three each, culled by cull
   (GFX_CULL_*). kind picks what the stages do with the bound textures and what a vertex's colours
   hold:
   - GFX_MESH_GROUND: in a mode with the lightmap (DRAW_GROUND_LIGHTMAP), texture times colour,
     times the lightmap's alpha (the texture bound to stage 1), plus its colour, as the original's
     two-texture ground quads have it; otherwise texture times colour alone, as its ground quads
     without lightmaps. The colour is the vertex's diffuse.
   - GFX_MESH_MODEL: texture times colour, both colour and alpha. The diffuse's red, green and blue
     are the model's own lit colour (0 to 255 a channel) and its alpha the share of the light it
     was lit with (0, 128 or 255, 0 for a model lit without its light); with lightGround set,
     colour2 is the ground's light under the vertex, added at most to 255 a channel. The alpha
     is colour[3] alone.
   The colour's red, green and blue are then multiplied by colour (0..1) and rounded down, as the
   CPU scales them by the weather colour, its alpha by colour[3]. The first texture's coordinates
   go through uv: (u * uv[0] + v * uv[2] + uv[4], u * uv[1] + v * uv[3] + uv[5]). Blending, the
   alpha test, depth, fog and the texture addressing are the mode's, which must draw meshes
   (DRAW_MESHES). */
#define GFX_MESH_MODEL 1
#define GFX_MESH_GROUND 2

struct GfxMeshDraw {
    int kind, first, count, lightGround, cull;
    float colour[4];
    float uv[6];
};

gfx_mesh *gfx_mesh_create(const struct GfxMeshVertex *verts, int nverts, const WORD *indices, int nindices);
void gfx_mesh_destroy(gfx_mesh *mesh);
void gfx_set_world_view(const struct GfxWorldView *view);
/* Draws as the same triangles pre-transformed would be drawn, in the world view set last. */
void gfx_draw_mesh(gfx_mesh *mesh, const struct GfxMeshDraw *draw);
/* custom: the world view gfx_draw_vertices's world vertices are drawn with (the camera's) */
void gfx_set_world_camera(const struct GfxWorldView *view);

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
   a fixed width. Over that disc, and over the cascade's texels, a surface at a slant to the light
   moves along it, so the bias grows by slant[c] (light depth) times the tangent of that slant, up
   to 8 times. On a GFX_MESH_GROUND draw the shadow maps take the place of the lightmap's alpha,
   the shadows the map's editor baked into it: down to darkness that alpha counts as lit, and the
   shadow maps darken to darkness; a texel darker still keeps its own. Where a model's surface lies
   within the bias in front of the ground, or as far behind it as that slant reaches, the maps
   cannot tell whether it shades it, and the ground keeps the lightmap's alpha there (the shadow
   pass keeps the models' depths negative). gfx_shadow_begin answers 0 where the device cannot
   draw shadows; gfx_shadow_receive(0) turns receiving off, and the draws then look as without
   shadows. */
#define GFX_SHADOW_SIZE 2048
#define GFX_SHADOW_CASCADES 4

struct GfxShadow {
    int cascades;
    float fromView[GFX_SHADOW_CASCADES][16];
    float farDepth[GFX_SHADOW_CASCADES];
    float offset[GFX_SHADOW_CASCADES];
    float blur[GFX_SHADOW_CASCADES];
    float lightView[3];
    float slant[GFX_SHADOW_CASCADES];
    float bias, darkness;
    float ambient[3];
};

/* whether the device draws shadows at all: where it answers 0, gfx_shadow_begin does too */
int gfx_shadows_supported(void);
int gfx_shadow_begin(const struct GfxShadow *shadow);
int gfx_shadow_cascade(int cascade);
void gfx_shadow_end(void);
void gfx_shadow_receive(int receiver);

/* custom: the screen wobble (render/screen_wobble.h): the screen as drawn so far, multisampled or
   not, drawn again with row y showing row rows[y].source moved right by rows[y].shift pixels, the
   pixels left of that keeping row y's own, for the screen's first `count` rows; what is drawn after
   it is drawn over it as it is. The screen's picture is copied and read on the GPU. fill(context)
   fills rows[0] to rows[count - 1] once the device has all it needs to draw the wobble; where it
   cannot draw it (no screen, no memory), fill is not called, nothing is drawn and the answer is 0,
   so that the caller draws its random numbers only for a wobble drawn. */
int gfx_wobble(const struct WobbleRow *rows, int count, void (*fill)(void *context), void *context);

/* custom: the screen's picture as drawn so far, a rectangle of it or the whole without one, as
   B8G8R8A8 rows `pitch` bytes apart into out, alpha 255: downloaded from the GPU when asked, with
   what was recorded before it drawn first, the CPU waiting for both. Called after gfx_present it
   reads the frame just presented (screenshots, the scene harness's captures). 0, or -1 for a
   rectangle that is empty or reaches outside the screen, a pitch narrower than its rows, or a
   download that failed. */
int gfx_read_frame(const RECT *r, void *out, size_t pitch);

unsigned gfx_content_version(gfx_surface *s);

/* custom: the screen, its multisampled picture and its depth at w by h, for a window resized to
   that; -1, with the old ones kept, when the device has no memory for them. A render target of the
   old size keeps it (c3d_device_save_frame makes its own again). */
int gfx_resize(int w, int h);

#endif
