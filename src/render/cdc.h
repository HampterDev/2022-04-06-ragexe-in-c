#ifndef RENDER_CDC_H
#define RENDER_CDC_H
#include "platform/win_types.h"
#include <stddef.h>

struct Texture;

#define CDC_COLOR_KEY 0xffff00ffu
#define CDC_TILE 0x100

struct DcBitmap {
    int width, height;
#ifdef _WIN32
    HDC hdc;
    HBITMAP dib;
    HGDIOBJ oldBitmap;
#endif
    DWORD *bits;
    BYTE changed;
    struct Texture **textures;
    size_t texture_count;
    BYTE singleTexture;
};

struct DcBitmap *dc_bitmap_create(int width, int height);
struct DcBitmap *dc_bitmap_construct(struct DcBitmap *self, int w, int h);
void dc_bitmap_destruct(struct DcBitmap *self);
void dc_bitmap_destroy(struct DcBitmap *self);
#ifdef _WIN32
char dc_bitmap_get_dc(struct DcBitmap *self, HDC *out);
#endif
void dc_bitmap_update(struct DcBitmap *self, int x, int y, int w, int h,
                                 const DWORD *src, char keyed);
void dc_bitmap_copy_scaled(struct DcBitmap *self, int x, int y, int sw, int sh, int dw, int dh,
                                      const DWORD *src);
void dc_bitmap_copy_bitmap(struct DcBitmap *self, unsigned dx, unsigned dy, unsigned w, unsigned h,
                                      struct DcBitmap *src);
void dc_bitmap_fill(struct DcBitmap *self, const RECT *r, DWORD colour);
void dc_bitmap_fill_triangle(struct DcBitmap *self, const POINT points[3], COLORREF colour);
void dc_bitmap_draw(struct DcBitmap *self, int x, int y, int w, int h, DWORD colour);
void dc_bitmap_draw_sprite(struct DcBitmap *self, int a, int b, int c, int d, void *spr, void *motion,
                                      int x, int y, float scale, DWORD *palette);
void dc_bitmap_draw_texels(struct DcBitmap *self, int x, int y, struct Texture *tex, int sx, int sy, int sw, int sh,
                           int mirror, int cw, int ch);
void dc_bitmap_draw_texels_scaled(struct DcBitmap *self, int dx0, int dy0, int dw, int dh, struct Texture *tex, int sw, int sh,
                           float scaleX, float scaleY, DWORD colour, char mirror, int offX, int offY, int sx, int sy);
void dc_bitmap_brighten(struct DcBitmap *self, int x0, int y0, int w, int h, int delta);

void dc_bitmap_alloc(struct DcBitmap *self, int w, int h);
void dc_bitmap_copy_rect(struct DcBitmap *self, int dx, int dy, int sx, int sy, int w, int h,
                                    struct DcBitmap *src);
void dc_bitmap_to_texture(struct DcBitmap *self, struct Texture **tex);
void dc_bitmap_alpha_from_brightness(struct DcBitmap *self, BYTE level);
void dc_bitmap_upload(struct DcBitmap *self);

void texture_draw_surface(struct Texture *self, int x, int y, int w, int h, DWORD colour);
void texture_draw_surface_stretch(struct Texture *self, int x, int y, int w, int h);
void texture_draw(struct Texture *self, int x, int y, int w, int h, DWORD colour);

#endif
