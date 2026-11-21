#ifndef RENDER_RENDERER_H
#define RENDER_RENDERER_H

#include "render/projection.h"

#define WIN32_LEAN_AND_MEAN
#include "platform/win_types.h"
#include "base.h"
#include "render/draw_queue.h"
#include "render/gfx_world.h"

struct Texture;
struct SpriteAtlasCache;
struct RenderStorage;

struct Renderer {
    float proj[2];
    float scaleX, scaleY;
    int halfW, halfH;
    int width, height;
    int centreX, centreY;
    int frame;
    int fpsCount;
    DWORD fpsTick;
    int fps;
    int fogEnable;
    gfx_surface *boundTextures[2];
    float clipLeft, clipRight;
    float clipTop, clipBottom;
    DWORD clearColor;
    BYTE twoPassAlpha;
    struct RenderQueue queues[RENDER_PASS_COUNT];
    struct SpriteAtlasCache *sprite_atlas;
    struct RenderStorage *storage;
    /* custom: the direction the map's light travels, its ambient light and what remains of the
       light in a shadow (renderer_set_light), and whether the frame being drawn has shadows
       (shadowSet: renderer_shadow_frame and renderer_shadow_bounds set them up): what the device
       fits the cascades with, and where a caster can shade the view */
    float lightDir[3], lightAmbient[3], shadowDarkness;
    int lightSet;
    struct GfxShadow shadow;
    struct ShadowFitReach shadowReach;
    int shadowSet;
    /* custom: the camera's view of the frame, which the water is drawn with (renderer_set_camera),
       and what the static world's ground and opaque models are drawn with this frame
       (renderer_set_world; render/gfx.h, gfx_world_draw), while worldSet */
    float cameraView[12];
    int cameraSet;
    struct GfxWorldDraw worldModels, worldGround;
    int worldSet;
    /* custom: the cells of water a run of the alpha lists' water draws (render/crenderer.c) */
    uint32_t *waterCells;
    size_t waterCellCapacity;
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
void renderer_set_light(struct Renderer *self, const float *dir, const float *ambient, float shadowOpacity);
/* custom: shadows for this frame's ground and models (render/gfx.h), which the device fits on the
   GPU to what the view, with the view matrix `view` (which the world's draws reach view space
   with), shows of them up to four times the camera's `distance` deep: the frame's constants for
   that (render/shadow_fit.h) and where a caster can shade the view. With lightmaps off, where the
   map or the device has no shadows, or where nothing can be fitted, shadowSet is 0
   (render/rp_project.c). */
void renderer_shadow_frame(struct Renderer *self, const float *view, float distance);
/* custom: the box everything that receives and casts this frame's shadows lies in, which bounds
   the shadow maps along the light (render/shadow_fit.h, shadow_fit_bounds); a box that is empty or
   not finite leaves the frame without shadows. */
void renderer_shadow_bounds(struct Renderer *self, const struct ShadowFitBox *bounds);
/* custom: the camera's view matrix for the frame being queued, which the water is drawn with */
void renderer_set_camera(struct Renderer *self, const float *view);
/* custom: the static world in this frame (render/gfx.h): its ground and opaque models are drawn
   where the original drew their lists, with these */
void renderer_set_world(struct Renderer *self, const struct GfxWorldDraw *models, const struct GfxWorldDraw *ground);
/* custom: where the frame has shadows, the box whatever can shade what its view shows lies in, as
   planes (render/static_world.h, SwFrustum: the static world's records are tested against it as
   against the view's): across the light the shadows' reach (render/shadow_fit.h,
   shadow_fit_reach), along it before the reach's farthest point. 0 without shadows. */
int renderer_shadow_reach(const struct Renderer *self, struct SwFrustum *out);
void renderer_draw_battle_field_icon(struct Renderer *self);

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
