#ifndef RENDER_TEXTURE_QUAD_H
#define RENDER_TEXTURE_QUAD_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

struct Renderer;
struct Texture;

struct TextureUV { float l, t, r, b; };
extern const struct TextureUV g_fullUV;

void screen_texture_rect(struct Renderer *renderer, const RECT *rect, int angle,
                         float rhw, DWORD colour, struct Texture *texture,
                         struct TextureUV uv);

#endif
