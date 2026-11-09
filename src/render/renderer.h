#ifndef RENDER_RENDERER_H
#define RENDER_RENDERER_H

#include "render/projection.h"

#define WIN32_LEAN_AND_MEAN
#include "platform/win_types.h"
#include "base.h"
#include "render/draw_queue.h"

struct Texture;
struct SpriteAtlasCache;
struct RenderStorage;

/* custom: where a shadow caster can lie for the frame's shadow maps (renderer_shadow_frame): across
   the light, along axis[0] and axis[1], inside one cascade's square (lo to hi), and along it,
   axis[2], from depth[0] to depth[1]; world coordinates */
struct ShadowReach {
    float axis[3][3];
    float lo[GFX_SHADOW_CASCADES][2], hi[GFX_SHADOW_CASCADES][2];
    float depth[2];
};

struct Renderer {
    float proj[4];
    float aspect;
    float scaleX, scaleY;
    int halfW, halfH;
    int width, height;
    int centreX, centreY;
    int frame;
    DWORD bpp;
    int fpsCount;
    DWORD fpsTick;
    int fps;
    int fogEnable;
    int fogDirty;
    int fogVertexCaps;
    gfx_surface *boundTextures[2];
    float clipLeft, clipRight;
    float clipTop, clipBottom;
    DWORD clearColor;
    BYTE twoPassAlpha;
    void *targetBits;
    LONG targetPitch;
    struct RenderQueue queues[RENDER_PASS_COUNT];
    struct SpriteAtlasCache *sprite_atlas;
    struct RenderStorage *storage;
    /* custom: the direction the map's light travels, its ambient light and what remains of the
       light in a shadow (renderer_set_light), and the shadow map's view that renderer_shadow_frame
       set for the frame being drawn, valid while shadowPending */
    float lightDir[3], lightAmbient[3], shadowDarkness;
    int lightSet;
    struct GfxShadow shadow;
    struct ShadowReach shadowReach;
    int shadowPending, shadowDrawn;
    /* custom: the view of the world view the mesh draws set last this frame (render/gfx.h), and
       the camera's view of the frame, which world-space quads are drawn with (renderer_set_camera) */
    float meshView[12], cameraView[12];
    int meshViewSet, cameraSet;
};

void renderer_set_projection(struct Renderer *self, int width, int height);
void renderer_clear_frame(struct Renderer *self, int flag);
void renderer_clear_scene(struct Renderer *self);
void renderer_restore_frame(struct Renderer *self);

struct Renderer *renderer_construct(struct Renderer *self);
void renderer_destruct(struct Renderer *self);
int renderer_draw_scene(struct Renderer *self);
void renderer_flip(struct Renderer *self, char vsync);
void renderer_save_frame(struct Renderer *self);
void renderer_draw_frame_extras(struct Renderer *self);
void renderer_enable_fog(struct Renderer *self, int enable);
/* custom: shadowOpacity is the share of the light a shadow takes away (world_res.h), 0 to 1 */
void renderer_set_light(struct Renderer *self, const float *dir, const float *diffuse, const float *ambient,
                        float shadowOpacity);
void renderer_set_blend_stages(struct Renderer *self, int mode);
/* custom: shadows for this frame's meshes (render/gfx.h): cascades along the map's light
   fitted to what the view, with the view matrix `view` (which the meshes' draws reach view space
   with), sees around `focus` (world coordinates) from `distance` away (render/rp_project.c). */
void renderer_shadow_frame(struct Renderer *self, const float *view, const float *focus, float distance);
/* custom: the camera's view matrix for the frame being queued, which quads with world-space
   vertices (RPQuad.world) are drawn with */
void renderer_set_camera(struct Renderer *self, const float *view);
/* custom: whether a box can cast into this frame's shadow maps: centre and half its sizes along its
   three axes (unit vectors, NULL for the world's), world coordinates */
int renderer_shadow_reaches(const struct Renderer *self, const float *centre, const float *half, const float (*axes)[3]);
void renderer_draw_battle_field_icon(struct Renderer *self);
SIZE *renderer_get_text_extent(struct Renderer *self, SIZE *out, const char *text, int length,
                int a, int b, int c);
int renderer_text_out(struct Renderer *self, int x, int y,
                      const char *text, COLORREF colour);

extern struct Renderer *g_renderer;

extern float g_spriteDepthScale;
extern float g_spriteRowDepthStep;
/* custom: the rhw of a sprite row `shift` camera-space units behind the sprite's anchor at depth
   `depth`, where the original takes 1 / (depth + shift). That curve is replaced by its tangent at
   the anchor, so rhw (and with it z) is linear in the row: every layer of a figure then lies on one
   line and only the layer offsets order them, and a quad flat on the ground lies in the ground's
   plane. Within the original's zoom and tilt the two differ by about one percent at most. */
float sprite_row_rhw(float depth, float shift);
/* custom: the rhw of a row `rows` screen rows below the anchor of a sprite lying flat on the
   ground. `up` is the world's up axis in view space (entries 3..5 of a view matrix) and `pos` the
   anchor in view space. The ground plane's 1/z is linear in the screen row with the slope
   up_y / (proj[1] * up . pos), so the quad lies in the ground's plane at any zoom, tilt and screen
   position. The original took a fixed 0.16 / scaleY units per row times cot of the tilt, right only
   near the centre of the screen at the default distance of 300. */
float sprite_flat_row_rhw(const float *up, const float *pos, float rows);
/* custom: the rhw an upright sprite at `pos` (view space) needs on top of its rows' own so that
   `rows` screen rows below its anchor stay in front of the ground: going down, the ground's rhw
   rises by the slope above while the sprite's falls by step / depth^2. */
float sprite_ground_lift(const float *up, const float *pos, float step, float rows);
/* custom: how much a sprite lying on the ground at `pos` (view space) is foreshortened on screen
   compared with the original's view of it at a tilt of -45: the sine of the angle between the view
   ray and the ground, over sin 45. 1 at the original's default view, towards 0 as the view grows
   level with the ground. */
float sprite_ground_squash(const float *up, const float *pos);
/* custom: the up axis of the world view in view space, set where the view matrix is built */
extern float g_viewGroundUp[3];
extern float g_spriteScaleDivisor;

#endif
