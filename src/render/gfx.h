#ifndef GFX_H
#define GFX_H

#define WIN32_LEAN_AND_MEAN
#include "platform/win_types.h"
#include "platform/window_handle.h"
#include "render/gfx_format.h"
#include "render/draw_modes.h"
#include "render/static_world.h"

typedef struct gfx_surface gfx_surface;
struct C3dDevice;

struct gfx_lock {
    void *bits;
    LONG pitch;
    DWORD width, height;
    DWORD bpp;
};

struct gfx_caps {
    int zbufferless;
    int trilinear;
    DWORD minTexW, minTexH;
    DWORD maxTexW, maxTexH;
    DWORD texAspect;
    float guardLeft, guardTop, guardRight, guardBottom;
    BYTE bltStretch;
};
extern struct gfx_caps g_gfxCaps;

#define GFX_VTX_TEX1 1
#define GFX_VTX_TEX2 2
/* GFX_VTX_TEX1: x, y, z, rhw, the diffuse colour (a D3DCOLOR) and one pair of texture coordinates,
   GFX_VTX_TEX1_SIZE bytes; GFX_VTX_TEX2 the same with a second pair. custom: GFX_VTX_WORLD is
   GFX_VTX_TEX1's layout with x, y and z in world coordinates, which the device takes through the
   camera's world view (gfx_set_world_camera) to the pixel and depth a pre-transformed vertex would
   have there; its rhw is the one the CPU's projection gave it, which only the fog reads. */
#define GFX_VTX_WORLD 3
#define GFX_VTX_TEX1_SIZE 28
#define GFX_VTX_TEX2_SIZE 36

#define GFX_PT_TRIANGLELIST  4
#define GFX_PT_TRIANGLESTRIP 5

#define GFX_CLEAR_TARGET  1
#define GFX_CLEAR_ZBUFFER 2

#define GFX_LOCK_WAIT      1
#define GFX_LOCK_WRITEONLY 2
#define GFX_LOCK_UNSIZED   8

#define GFX_BLT_WAIT  1
#define GFX_BLT_ASYNC 2

#define GFX_E_DDRAW_CREATE      0x82000001
#define GFX_E_DEVICE_CREATE     0x82000004
#define GFX_E_ZBUFFER           0x82000005
#define GFX_E_PRIMARY           0x82000007
#define GFX_E_NO_PRIMARY        0x8200000e
#define GFX_E_INVALIDARG         ((int)0x80070057)

int gfx_open(struct C3dDevice *d, int flags);
/* custom: the device draws into its window at d->width by d->height and never changes the
   display's mode; fullscreen is the window's (render/display_options.h). */
int gfx_create_screen(struct C3dDevice *d, int flags);
int gfx_create_3d(struct C3dDevice *d, int flags);
int gfx_create_zbuffer(struct C3dDevice *d);
int gfx_get_caps(struct C3dDevice *d, struct gfx_caps *out);
int gfx_close(struct C3dDevice *d);
int gfx_present(struct C3dDevice *d, int vsync);
void gfx_set_mipmaps(int on);

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
void gfx_draw(int prim, int vertexKind, const void *verts, int nverts, const WORD *indices, int nindices);

/* custom: the static world (plans/gpu-renderer.md, "The static world, GPU-driven";
   render/static_world.h): one map's ground and models, which the device keeps and projects itself,
   where everything else arrives projected by the CPU (renderer_project_vertex, render/rp_project.c).
   Its meshes lie one behind the other in two shared buffers (gfx_world_mesh), its textures in
   texture arrays of one size each (gfx_world_texture), its draw records (gfx_world_records) say
   what each run of a mesh draws with, and its transform slots (gfx_world_slots) where each model
   node stands. All of it stays on the device until gfx_world_clear, or until the device is made
   again, which gfx_world_generation tells; what is given goes up in the next frame's upload. Only
   the thread that draws calls these. A vertex has two colours, a draw picks one (GfxWorldDraw). */
struct GfxMeshVertex {
    float x, y, z;
    float nx, ny, nz;
    DWORD diffuse, colour2;
    float u0, v0, u1, v1;
};

/* The projection the world is drawn with, the CPU's own (render/rp_project.c): view is the
   client's 4x3 view matrix, view-space x = view[0] * x + view[3] * y + view[6] * z + view[9], y
   likewise with entries 1, 4, 7, 10 and the depth vz with 2, 5, 8, 11. A point lands at the pixel
   (x * proj[0] / vz + centre[0], y * proj[1] / vz + centre[1]) with the depth
   (vz - nearZ) * depthScale / vz, as a pre-transformed vertex with rhw 1 / vz would. */
struct GfxWorldView {
    float view[12];
    float proj[2];
    float centre[2];
    float nearZ, depthScale;
};

/* the layers of one texture array (Vulkan's least maxImageArrayLayers), and the most slots on the
   way from one to the camera's view */
#define GFX_WORLD_LAYERS 256
#define GFX_WORLD_CHAIN 64

/* A transform slot: its matrix (a model node's, or the model's own for the slot its top nodes hang
   from) and its parent's slot, -1 for a model's, whose parent is the camera's view; animated, an
   index into the frame's matrices that take local's place (GfxWorldFrame), or -1. Slot 0 is the
   world itself, whose view is the camera's as it is: the ground's records are drawn with it. The
   device computes each slot's view as the Direct3D 9 client's CPU computed a model node's, to the
   bit: its matrix times its parent's view, a model's matrix times the camera's view, each product
   rounded as render/math3d.c's mat_multiply rounds it; and the slot's place in the world likewise,
   up to its model's matrix, by which an animated record's bounds move (sw_record_in_view). */
struct GfxWorldSlot {
    float local[12];
    int parent, animated;
};

/* The device's world changes its generation whenever it is made again from nothing: by
   gfx_world_clear, with a new device, when textures are reloaded (render/tex_cache.c), and when
   uploads fail (after which, where they fail twice in a row, the device takes nothing more of the
   world until gfx_world_clear). The client then builds it again. */
int gfx_world_generation(void);
void gfx_world_clear(void);
/* A mesh's vertices, the record of each (sw_vertex_ids) and its indices, at the place sw_layout_place
   gave it. 0 without memory for it. */
int gfx_world_mesh(const struct SwPlacement *place, const struct GfxMeshVertex *verts, const uint32_t *ids,
                   const WORD *indices);
/* the mipmap levels of a texture (1 for any other surface) */
int gfx_get_levels(gfx_surface *s);
/* A texture array of textures like `like` (its size and mipmap levels), numbered `array`: the next
   one, as sw_texture_place opened it. 0 for another number or a surface that is no texture. */
int gfx_world_array(uint8_t array, gfx_surface *like);
/* A texture's pixels into layer `layer` of array `array`, as sw_texture_place placed it with
   GFX_WORLD_LAYERS layers an array: its pixels and mipmaps are what the texture's own would be. 0
   for a texture without pixels or of another size or levels than its array's. */
int gfx_world_texture(gfx_surface *texture, uint8_t array, uint16_t layer);
/* All the map's live records, in the order the passes draw them (sw_records_sort), each time they
   change; every id below `ids`. */
int gfx_world_records(const struct SwRecord *records, size_t count, uint32_t ids);
/* Slots first to first + count. */
int gfx_world_slots(const struct GfxWorldSlot *slots, size_t first, size_t count);

/* The map's water: the cells whose quad the frame may draw, placed once (gfx_world_water), each
   over a cell of the ground, whose corners the waves of its block raise and lower
   (world/ground_water.c). y grows downward: top is the greatest y of the corners of the cell's
   ground, its lowest corner, -infinity where none is a number; the quad shows only where that
   corner lies below the wave. */
struct GfxWaterCell {
    int x, y, block;
    float top;
    float corners[4][2];  /* x and z of the corners, as world/ground_draw.c orders a cell's quad */
};
/* A block's wave in the frame: its phase, pitch, height and level (struct GndWater). */
struct GfxWaterWave {
    float phase, pitch, height, level;
};
/* the cells, and the table of sines by whole degree from 0 to 359 the waves take
   (render/math3d.c, get_sin_deg) */
int gfx_world_water(const struct GfxWaterCell *cells, size_t count, const float *sines);

/* The frame (gfx_world_frame): the camera's view and the frustum of its projection (sw_frustum), the
   matrices of animated slots (GfxWorldSlot.animated), the texture-coordinate matrices by uv slot
   (SwRecord.uv), each (u, v) -> (u * uv[0] + v * uv[2] + uv[4], u * uv[1] + v * uv[3] + uv[5]), and
   the waves by block. */
struct GfxWorldFrame {
    struct GfxWorldView camera;
    struct SwFrustum frustum;
    const float (*animated)[12];
    size_t animatedCount;
    const float (*uv)[6];
    size_t uvCount;
    const struct GfxWaterWave *waves;
    size_t waveCount;
};
void gfx_world_frame(const struct GfxWorldFrame *frame);

/* What a draw of records does with their textures and their vertices' colours:
   - GFX_MESH_GROUND: in a mode with the lightmap (DRAW_GROUND_LIGHTMAP), texture times colour,
     times the lightmap's alpha, plus its colour, as the original's two-texture ground quads have it;
     otherwise texture times colour alone, as its ground quads without lightmaps. The colour is the
     vertex's diffuse.
   - GFX_MESH_MODEL: texture times colour, both colour and alpha. The diffuse's red, green and blue
     are the model's own lit colour (0 to 255 a channel) and its alpha the share of the light it
     was lit with (0, 128 or 255, 0 for a model lit without its light); with lightGround set,
     colour2 is the ground's light under the vertex, added at most to 255 a channel. The alpha
     is colour[3] alone.
   The colour's red, green and blue are then multiplied by colour (0..1) and rounded down, as the
   CPU scales them by the weather colour, its alpha by colour[3]. The texture's coordinates go
   through the record's texture-coordinate matrix. Blending, the alpha test, depth, fog and the
   texture addressing are the mode's, which must draw meshes (DRAW_MESHES); the record's flags give
   the cull (render/static_world.h, SW_CULL_*; Direct3D 9's, by the winding on the screen, y down). */
#define GFX_MESH_MODEL 1
#define GFX_MESH_GROUND 2

struct GfxWorldDraw {
    int kind, lightGround;
    float colour[4];
};

/* The records of pass `pass` (SW_PASS_OPAQUE_CLAMP, SW_PASS_OPAQUE_WRAP or SW_PASS_GROUND) that the
   frame's view keeps (sw_record_in_view), in their order, culled by the GPU: one indirect draw a
   run of records (sw_draw_runs), of the indices of the records it keeps. */
void gfx_world_draw(int pass, const struct GfxWorldDraw *draw);
/* One record the device has, drawn whatever the view. */
void gfx_world_draw_record(const struct SwRecord *record, const struct GfxWorldDraw *draw);
/* A run of the water's cells (gfx_world_water) in the frame's waves, as quads in world coordinates
   (GFX_VTX_WORLD) with the colour (a D3DCOLOR) and stage 0's texture, in the current mode. */
void gfx_world_draw_water(const uint32_t *cells, size_t count, DWORD colour);
/* custom: the world view GFX_VTX_WORLD vertices are drawn with (the camera's) */
void gfx_set_world_camera(const struct GfxWorldView *view);

/* custom: cascaded shadow maps for the static world, each GFX_SHADOW_SIZE texels square. Between
   gfx_shadow_begin and gfx_shadow_end, gfx_world_cast draws the world's depth from the light into
   the cascade gfx_shadow_cascade chose instead of colour, testing the record's texture, where it
   has one, for alpha at its texture coordinates: texels the opaque lists' alpha test (0xcf) drops
   cast nothing.
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
/* A caster: `count` indices from firstIndex, after baseVertex, the triangles of one record or of all
   the records of a ground block, which share textureArray (SW_NO_ARRAY: none) and whose ids lie
   below `ids`; a ground's records cast without their texture. */
void gfx_world_cast(uint32_t firstIndex, uint32_t count, uint32_t baseVertex, uint8_t textureArray, uint32_t ids);

unsigned gfx_content_version(gfx_surface *s);

/* custom: the back buffer and the targets of its size at w by h, for a window resized to that;
   -1, with the old ones kept, when the device has no memory for them. */
int gfx_resize(struct C3dDevice *d, int w, int h);

#endif
