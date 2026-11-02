#ifndef RENDER_SPRITE_DRAW_H
#define RENDER_SPRITE_DRAW_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include "platform/win_types.h"

struct Renderer;
struct ActRes;
struct SprRes;

void screen_sprite_frame(struct Renderer *renderer, int x, int y,
                         struct ActRes *act, struct SprRes *spr,
                         int action, int motion, float scale, float angle,
                         DWORD colour, int flags);

#endif
