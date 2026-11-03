#ifndef RENDER_RENDERER_H
#define RENDER_RENDERER_H

#include "render/projection.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "base.h"
#include "render/draw_queue.h"

struct Texture;
struct SpriteAtlasCache;
struct RenderStorage;

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
void renderer_set_light(struct Renderer *self, const float *dir,
                       const float *diffuse, const float *ambient);
void renderer_set_blend_stages(struct Renderer *self, int mode);
void renderer_draw_battle_field_icon(struct Renderer *self);
SIZE *renderer_get_text_extent(struct Renderer *self, SIZE *out, const char *text, int length,
                int a, int b, int c);
int renderer_text_out(struct Renderer *self, int x, int y,
                      const char *text, COLORREF colour);

extern struct Renderer *g_renderer;

extern float g_spriteDepthScale;
extern float g_spriteRowDepthStep;
extern float g_spriteScaleDivisor;

#endif
