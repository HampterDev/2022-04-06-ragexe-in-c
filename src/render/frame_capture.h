#ifndef RENDER_FRAME_CAPTURE_H
#define RENDER_FRAME_CAPTURE_H

#include "render/renderer.h"
struct BitmapRes;
struct Surface;

void pixel16_to_bgr(WORD pixel, BYTE *out);
int renderer_lock_target(struct Renderer *renderer);
int renderer_save_jpeg_region(struct Renderer *renderer, const char *filename, const RECT *rect);
char renderer_screenshot(struct Renderer *renderer, struct String filename);

#endif
