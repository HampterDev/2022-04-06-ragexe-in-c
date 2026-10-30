#include "ui/items/items.h"

#define BOX_EDGE 14
#define BOX_SKIN_PREFIX UI_SKIN "sysbox_"

#include "resources/bitmap.h"

static int edge_steps(int length)
{
    return (length - 2 * BOX_EDGE) / BOX_EDGE + ((length - 2 * BOX_EDGE) % BOX_EDGE != 0);
}

static void draw(struct UIWindow *self, int x, int y, struct BitmapRes *bitmap)
{
    ui_window_draw_bitmap(self, x, y, bitmap, 0);
}

void ui_window_draw_sys_box(struct UIWindow *self, int x, int y, int width, int height)
{
    struct BitmapRes *bitmap;
    int cols, rows, i;

    if (!width)
        width = self->width;
    if (!height)
        height = self->height;
    cols = edge_steps(width);
    for (i = 0; i < cols; i++)
        draw(self, x + BOX_EDGE + i * BOX_EDGE, y, ui_skin_bitmap(UI_SKIN "sysbox_mu.bmp"));
    draw(self, x, y, ui_skin_bitmap(UI_SKIN "sysbox_lu.bmp"));
    bitmap = ui_skin_bitmap(UI_SKIN "sysbox_ru.bmp");
    draw(self, x - bitmap->width + width, y, bitmap);
    rows = edge_steps(height);
    for (i = 0; i < rows; i++)
        draw(self, x, y + BOX_EDGE + i * BOX_EDGE, ui_skin_bitmap(UI_SKIN "sysbox_lm.bmp"));
    for (i = 0; i < rows; i++) {
        bitmap = ui_skin_bitmap(UI_SKIN "sysbox_rm.bmp");
        draw(self, x - bitmap->width + width, y + BOX_EDGE + i * BOX_EDGE, bitmap);
    }
    for (i = 0; i < cols; i++) {
        bitmap = ui_skin_bitmap(UI_SKIN "sysbox_md.bmp");
        draw(self, x + BOX_EDGE + i * BOX_EDGE, y - bitmap->height + height, bitmap);
    }
    bitmap = ui_skin_bitmap(UI_SKIN "sysbox_ld.bmp");
    draw(self, x, y - bitmap->height + height, bitmap);
    bitmap = ui_skin_bitmap(UI_SKIN "sysbox_rd.bmp");
    draw(self, x - bitmap->width + width, y - bitmap->height + height, bitmap);
    ui_window_fill_rect(self, x + BOX_EDGE, y + BOX_EDGE, width - 2 * BOX_EDGE, height - 2 * BOX_EDGE,
                      ui_skin_colour(2, 2));
}

static struct BitmapRes *box_bitmap(const struct String *prefix, const char *part)
{
    struct String path;
    struct BitmapRes *bitmap;

    string_copy(&path, prefix);
    string_append_cstr(&path, part);
    bitmap = ui_skin_bitmap(string_cstr(&path));
    string_free(&path);
    return bitmap;
}

static void box_prefix(struct String *out, const char *prefix)
{
    string_init(out);
    string_assign(out, BOX_SKIN_PREFIX, sizeof BOX_SKIN_PREFIX - 1);
    if (prefix)
        string_assign_cstr(out, prefix);
}

void ui_window_draw_box_bitmaps(struct UIWindow *self, const char *prefix, int x, int y,
                                        int width, int height, BYTE tile_bg)
{
    struct String base;
    struct BitmapRes *bitmap;
    int cols, rows, row, col;

    box_prefix(&base, prefix);
    if (width <= 0)
        width = self->width;
    if (height <= 0)
        height = self->height;
    if (tile_bg == 1) {
        cols = edge_steps(width);
        rows = edge_steps(height);
        for (row = 0; row < rows; row++)
            for (col = 0; col < cols; col++)
                draw(self, x + BOX_EDGE + col * BOX_EDGE, y + BOX_EDGE + row * BOX_EDGE, box_bitmap(&base, "bg.bmp"));
    } else {
        ui_window_fill_rect(self, x + BOX_EDGE, y + BOX_EDGE, width - 2 * BOX_EDGE, height - 2 * BOX_EDGE,
                          ui_skin_colour(2, 2));
    }
    cols = edge_steps(width);
    for (col = 0; col < cols; col++)
        draw(self, x + BOX_EDGE + col * BOX_EDGE, y, box_bitmap(&base, "mu.bmp"));
    draw(self, x, y, box_bitmap(&base, "lu.bmp"));
    bitmap = box_bitmap(&base, "ru.bmp");
    draw(self, x - bitmap->width + width, y, bitmap);
    rows = edge_steps(height);
    for (row = 0; row < rows; row++)
        draw(self, x, y + BOX_EDGE + row * BOX_EDGE, box_bitmap(&base, "lm.bmp"));
    for (row = 0; row < rows; row++) {
        bitmap = box_bitmap(&base, "rm.bmp");
        draw(self, x - bitmap->width + width, y + BOX_EDGE + row * BOX_EDGE, bitmap);
    }
    for (col = 0; col < cols; col++) {
        bitmap = box_bitmap(&base, "md.bmp");
        draw(self, x + BOX_EDGE + col * BOX_EDGE, y - bitmap->height + height, bitmap);
    }
    bitmap = box_bitmap(&base, "ld.bmp");
    draw(self, x, y - bitmap->height + height, bitmap);
    bitmap = box_bitmap(&base, "rd.bmp");
    draw(self, x - bitmap->width + width, y - bitmap->height + height, bitmap);
    string_free(&base);
}
