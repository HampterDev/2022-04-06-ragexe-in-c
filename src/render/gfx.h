#ifndef GFX_H
#define GFX_H

/* The graphics device (render/gpu_device.c): surfaces, textures and render targets, the modes the
   client draws in, its quads and triangles, Granny's skinned meshes, the wobble, presenting and
   reading back. The layers that record into its frames go through render/gpu_layer.h; the static
   world is one (render/gfx_world.h). */
#define WIN32_LEAN_AND_MEAN
#include "platform/win_types.h"
#include "platform/window_handle.h"
#include "render/gfx_format.h"
#include "render/draw_modes.h"
#include "render/quads.h"
#include <stddef.h>
#include <stdint.h>

typedef struct gfx_surface gfx_surface;
struct SkinVertex;
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


#define GFX_E_DDRAW_CREATE      0x82000001
#define GFX_E_ZBUFFER           0x82000005
#define GFX_E_PRIMARY           0x82000007
#define GFX_E_NO_PRIMARY        0x8200000e
#define GFX_E_INVALIDARG         ((int)0x80070057)

/* The device for a window: 0, or GFX_E_DDRAW_CREATE without one. The layers are added before
   (gpu_add_layer). */
int gfx_open(PlatformWindow window);
/* custom: the device, made with its screen, draws into its window at w by h and never changes the
   display's mode; fullscreen is the window's (render/display_options.h). The screen is the one
   surface the frame is drawn into and the window shows, a render target of the caller's, which
   gfx_release lets go; NULL where the device or the screen cannot be made. The screen's samples
   and depth come after it (gfx_create_zbuffer), and then every pipeline: 0, or GFX_E_ZBUFFER. */
gfx_surface *gfx_create_screen(int w, int h);
int gfx_create_zbuffer(void);
int gfx_get_caps(struct gfx_caps *out);
/* What was recorded goes to the GPU, and the device goes with everything it made; the surfaces
   stay, without their GPU textures, for a device made again. */
void gfx_close(void);
/* custom: ends the frame: what was recorded goes to the device's render thread, which records it
   into the GPU's command buffer and submits it while the next frame is recorded, and the window
   shows the frame before, the last one the render thread finished; the device claims the window at
   the first present while it is shown. A window never shown (the scene harness's) has no
   swapchain, and its frames are drawn and read (gfx_read_frame), not shown. */
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
void gfx_release(gfx_surface *s);
/* custom: holds the surface until gfx_release_holds: its first hold since then takes one
   reference, which gfx_release_holds lets go, and later holds take none. The renderer holds the
   surfaces and Granny models (gfx_hold_skin) its queued draws name, which take no reference each
   (render/rp_pool.c). gfx_release_holds lets go the models' holds too. */
void gfx_hold(gfx_surface *s);
void gfx_release_holds(void);
int gfx_get_size(gfx_surface *s, int *w, int *h);
/* custom: the CPU's pixels of a texture or a system surface, a rectangle of them or all; a render
   target has none (-1), its pixels stay on the GPU. */
int gfx_lock(gfx_surface *s, const RECT *r, struct gfx_lock *out);
int gfx_unlock(gfx_surface *s);
/* custom: a copy from a render target goes into a render target, on the GPU; into a texture or a
   system surface it is refused (-1). */
int gfx_blit(gfx_surface *dst, const RECT *dstRect, gfx_surface *src, const RECT *srcRect);
int gfx_refresh_mips(gfx_surface *tex, const RECT *r);
/* the mipmap levels of a texture (1 for any other surface) */
int gfx_get_levels(gfx_surface *s);

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
   on the CPU into a vertex's specular alpha: 1 - t in 1/255 steps rounded down,
   t = ((1 / rhw - origin) / span - start) / (end - start) clamped to 0..1, without the division
   where end equals start (a vertex whose rhw is not positive as at the smallest normal one), which
   the vertex shader computes within a step of the CPU's (render/shaders/quad.hlsli, vertex_fog);
   Granny's meshes (gfx_draw_skin) take the same steps by the rhw the GPU computes of their view
   depth, the water's by the rhw of its corners (render/shaders/water.hlsl).
   For the static world's meshes f is (b - vz) / (b - a) clamped to 0..1 for the view depth vz, a
   and b the view depths start * span + origin and end * span + origin (origin and origin + span
   where end equals start), as the Direct3D 9 client's mesh shader computed it. The device starts
   without fog. */
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
   the device's vertex shader makes as the client's C code made them (within the tolerance of
   plans/gpu-renderer.md), in the current mode, textures and target. Consecutive instances drawn
   with the same mode, effect, fog, textures (or layers of one array, gfx_create_layer_texture) and
   target, all flat or none, become one draw, in the order given, as Direct3D 9's draws were until a
   state changed. Direct3D 9 drew flat draws
   (every vertex the same rhw) without multisampling; in a multisampled pass their corners move
   onto the pixel boundaries that keep the pixels Direct3D 9 covered.
   gfx_draw_quad draws one the client made (a sprite, a projected quad, a triangle of a set of
   projected corners, the set put among the frame's quads by the first triangle that names it);
   gfx_draw_vertices one
   of its primitives of n pre-transformed vertices: a strip of 4 (a quad) or a list of triangles,
   drawn as they are, flat when every one has the same rhw. */
void gfx_draw_quad(const struct GfxQuad *quad);
void gfx_draw_vertices(const struct TexDrawVertex *v, int n, int strip);

/* custom: Granny's actors (actors/granny.c), skinned, lit and projected by the device's vertex shader
   (src/render/shaders/skin.hlsl) as the CPU did each vertex before (render/skin_layout.h). A model
   (gfx_create_skin) holds its meshes' vertices and indices, one mesh behind the other; they go up
   in the frame's upload when a draw first takes the model and stay on the device, and a copy stays
   on the CPU, as a texture's pixels do, for a device made again. Each frame an actor gives its pose
   (gfx_skin_pose: skin_pose_rows' rows, among the frame's quads until the frame is presented or
   read; it answers their first row, -1 where there is no memory for them, and a draw in pose -1
   draws nothing), and each of its meshes is drawn in it (gfx_draw_skin): `count` indices from
   `first` of the model, at the pixels and depths the pose projects them to (the original's vertex
   fog by their rhw), in the current mode, which must draw meshes into the screen (render/draw_modes.h,
   DRAW_MESHES; other modes and targets draw nothing), with stage 0's texture and both faces of
   every triangle. Draws of one model, mode, fog and textures one after the other are one draw where
   skin_batch_join joins them: an actor's meshes that run on in the model's indices, or one range in
   several poses, instanced. A model goes when its last reference is released and no recorded draw
   takes it; gfx_hold_skin holds it until gfx_release_holds, as gfx_hold holds a surface. The actors
   take no part in the static world's shadows (render/gfx_world.h): they neither cast into the maps
   nor receive from them. */
typedef struct gfx_skin gfx_skin;
gfx_skin *gfx_create_skin(const struct SkinVertex *vertices, uint32_t vertexCount, const uint32_t *indices,
                          uint32_t indexCount);
void gfx_hold_skin(gfx_skin *skin);
void gfx_skin_release(gfx_skin *skin);
int gfx_skin_pose(const float (*rows)[4], size_t count);
void gfx_draw_skin(gfx_skin *skin, uint32_t first, uint32_t count, int pose);

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
   reads the frame just ended (screenshots, the scene harness's captures), which the window shows
   only at the next present. 0, or -1 for a rectangle that is empty or reaches outside the screen,
   a pitch narrower than its rows, or a download that failed (or a frame that could not go to the
   GPU). */
int gfx_read_frame(const RECT *r, void *out, size_t pitch);

/* custom: for the tests, a mipmap level of a texture as the device holds it (its own texture, or its
   layer of an array of layered textures), downloaded after everything recorded before it:
   max(1, width >> level) by max(1, height >> level) texels of B8G8R8A8, row by row. 0, or -1 where
   the device holds no such level or a download failed. */
int gfx_read_texture_level(gfx_surface *texture, int level, void *out);

unsigned gfx_content_version(gfx_surface *s);

/* custom: the screen, its multisampled picture and its depth at w by h, for a window resized to
   that; -1, with the old ones kept, when the device has no memory for them. A render target of the
   old size keeps it (c3d_device_save_frame makes its own again). */
int gfx_resize(int w, int h);

#endif
