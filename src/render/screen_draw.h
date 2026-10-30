#ifndef RENDER_SCREEN_DRAW_H
#define RENDER_SCREEN_DRAW_H

#include "render/render_api.h"

void screen_quad_points(float ax, float ay, float bx, float by, float cx, float cy, float dx, float dy,
                                struct Texture *tex, DWORD colour);
void screen_thick_line(float x0, float y0, float x1, float y1, int width, struct Texture *tex,
                               DWORD colour);
void draw_box_screen(int x, int y, int w, int h, DWORD colour);
void screen_fill_rect(int x, int y, int w, int h, DWORD colour);
void screen_box_textured(int x, int y, int w, int h, DWORD colour, struct Texture *tex);

void renderer_queue_texture_quad(float x, float y, float w, float h, DWORD colour, struct Texture *tex);

#endif
