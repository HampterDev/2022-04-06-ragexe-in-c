#include "render/cdc.h"
#include "render/textdc.h"
#include "render/texture.h"
#include "render/texture_cache.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include "render/renderer.h"
#include "render/device.h"
#include "contents/map_info.h"
#include "contents/client_res.h"
#include "platform/language.h"

#define UI "유저인터페이스\\"

#define RENDERER_WIDTH(r)  ((r)->width)
#define RENDERER_HEIGHT(r) ((r)->height)

#define BANNER_HOLD_MS  5000
#define BANNER_END_MS   0x1487
#define FACE_DEFAULT    0x1d

static void draw_line(struct TextDC *dc, struct MapNameDisplay *self, int face,
                      int size, int y, const char *text)
{
    SIZE extent;

    text_dc_select_font(dc, face, size, 1, 0);
    text_dc_text_extent(dc, text, (int)strlen(text), &extent);
    text_dc_text_out(dc, (int)((float)((int)self->band_width - extent.cx) * 0.5f), y,
                 text, (int)strlen(text));
}

struct MapNameDisplay *map_name_display_construct(struct MapNameDisplay *self,
                                                             const char *line1, const char *line2,
                                                             const char *ornament)
{
    char *path;
    size_t length = strlen(ornament);
    static const char prefix[] = UI "display_mapname\\";
    struct Texture *tex;

    memset(self, 0, sizeof *self);
    if (length > SIZE_MAX - sizeof prefix - 4)
        return NULL;
    path = malloc(sizeof prefix + length + 4);
    if (!path)
        return NULL;
    memcpy(path, prefix, sizeof prefix - 1);
    memcpy(path + sizeof prefix - 1, ornament, length);
    memcpy(path + sizeof prefix - 1 + length, ".png", 5);
    tex = texture_cache_get(&g_textureCache, path, 0, 0);
    free(path);
    self->ornament = tex;
    if (tex && tex->reqHeight > 0) {
        struct TextDC dc;
        int band_width, face;
        unsigned short band_height;
        float scale;

        band_width = (unsigned short)RENDERER_WIDTH(g_renderer);
        self->band_width = (unsigned short)band_width;
        band_height = (unsigned short)(int)((float)RENDERER_HEIGHT(g_renderer) * 0.2f);
        self->band_height = band_height;
        scale = (float)band_height / (float)tex->reqHeight;

        self->band = dc_bitmap_create(band_width, band_height);
        if (!self->band) goto done;
        dc_bitmap_fill(self->band, NULL, 0);
        self->band->singleTexture = 1;
        text_dc_construct(&dc, self->band);
        text_dc_set_text_color(&dc, 0xffffff);
        face = g_serviceType != 0 ? 0 : FACE_DEFAULT;
        draw_line(&dc, self, face, (int)(scale * 16.0f), (int)(scale * 72.0f), line1);
        draw_line(&dc, self, face, (int)(scale * 44.0f), (int)(scale * 95.0f), line2);
        dc_bitmap_alpha_from_brightness(self->band, 0xff);
        self->start_time = timeGetTime();
        text_dc_destruct(&dc);
    }
done:
    return self;
}

void map_name_display_destruct(struct MapNameDisplay *self)
{
    if (self->band) {
        dc_bitmap_destroy(self->band);
        self->band = 0;
    }
}

static int banner_alpha(const struct MapNameDisplay *self)
{
    unsigned elapsed = timeGetTime() - self->start_time;

    if (elapsed <= BANNER_HOLD_MS)
        return 0xff;
    return BANNER_END_MS - (int)elapsed;
}

static int ornament_width(const struct MapNameDisplay *self)
{
    const struct Texture *tex = self->ornament;

    return (int)((float)self->band_height / (float)tex->reqHeight * (float)tex->reqWidth);
}

static int banner_top(void)
{
    return (int)((float)RENDERER_HEIGHT(g_renderer) * 0.12f);
}

char map_name_display_render(struct MapNameDisplay *self)
{
    unsigned color;
    int alpha, w;

    if (!self->ornament || !self->band)
        return 0;
    alpha = banner_alpha(self);
    if (alpha <= 0)
        return 0;
    color = ((unsigned)alpha << 24) | 0xffffff;
    w = ornament_width(self);
    texture_draw(self->ornament, (int)((float)((int)self->band_width - w) * 0.5f), banner_top(),
                 w, self->band_height, color);
    dc_bitmap_draw(self->band, 0, banner_top(), self->band_width,
                                         self->band_height, color);
    return 1;
}
