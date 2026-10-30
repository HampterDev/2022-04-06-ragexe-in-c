#include "ui/item_draw.h"
#include <stdio.h>
#include <string.h>
#include "ui/ui_items.h"
#include "ui/draw.h"
#include "ui/skin.h"
#include "ui/window_mgr.h"
#include "ui/game.h"
#include "ui/controls/controls.h"
#include "ui/ui_std.h"
#include <mmsystem.h>
#include <stdlib.h>
#include "ui/runtime.h"
#include "session/session_names.h"

#define COUNT_FONT_HEIGHT 0xb
#define COUNT_OUTLINE 0xffffff
#define COUNT_COLOUR 0
#define COUNT_PLAIN_DX 0x10
#define COUNT_PLAIN_DY 0x14
#define COUNT_DY 0xf
#define COUNT_SHORT_DX 0xe
#define COUNT_SHORT_BELOW 1000

static struct BitmapRes *item_bitmap(const char *id_text, BYTE identified)
{
    char name[0x80];

    session_get_item_bitmap_file_name(id_text, name, identified);
    return (struct BitmapRes *)res_mgr_get(g_resMgr(), name);
}

static void draw_count(struct UIWindow *self, int x, int y, int count, BYTE plain)
{
    char text[0x10];

    sprintf(text, "%d", count);
    if (plain) {
        ui_window_text_out(self, x + COUNT_PLAIN_DX, y + COUNT_PLAIN_DY, text, 0, 0, COUNT_FONT_HEIGHT,
                         COUNT_COLOUR, 0, 0);
        return;
    }
    if (count < COUNT_SHORT_BELOW)
        x += COUNT_SHORT_DX;
    y += COUNT_DY;
    ui_window_text_out(self, x + 1, y, text, 0, 0, COUNT_FONT_HEIGHT, COUNT_OUTLINE, 0, 0);
    ui_window_text_out(self, x - 1, y, text, 0, 0, COUNT_FONT_HEIGHT, COUNT_OUTLINE, 0, 0);
    ui_window_text_out(self, x, y + 1, text, 0, 0, COUNT_FONT_HEIGHT, COUNT_OUTLINE, 0, 0);
    ui_window_text_out(self, x, y - 1, text, 0, 0, COUNT_FONT_HEIGHT, COUNT_OUTLINE, 0, 0);
    ui_window_text_out(self, x, y, text, 0, 0, COUNT_FONT_HEIGHT, COUNT_COLOUR, 0, 0);
}

void ui_window_draw_item_icon(struct UIWindow *self, int x, int y,
                                      const struct ItemInfo *info, BYTE force_identified)
{
    struct BitmapRes *bitmap = item_bitmap(string_cstr(&info->id_text),
                                            info->identified || force_identified ? 1 : 0);

    if (bitmap)
        ui_window_draw_bitmap(self, x, y, bitmap, 1);
}

void ui_window_draw_item_icon_by_name(struct UIWindow *self, int x, int y,
                                            const char *id_text, BYTE identified)
{
    struct BitmapRes *bitmap = item_bitmap(id_text, identified);

    if (bitmap)
        ui_window_draw_bitmap(self, x, y, bitmap, 1);
}

void ui_window_draw_item_icon_count(struct UIWindow *self, int x, int y,
                                           const struct ItemInfo *info, BYTE force_identified, BYTE plain_count)
{
    struct BitmapRes *bitmap = item_bitmap(string_cstr(&info->id_text),
                                            info->identified || force_identified ? 1 : 0);

    if (bitmap)
        ui_window_draw_bitmap(self, x, y, bitmap, 1);
    if (info->count != -1 && info->count != 0)
        draw_count(self, x, y, info->count, plain_count);
}

void ui_window_draw_item_icon_count_grey(struct UIWindow *self, int tint, int x, int y,
                                               const struct ItemInfo *info, BYTE identified, BYTE plain_count)
{
    char name[0x80];
    struct BitmapRes *bitmap;

    memset(name, 0, sizeof name);
    session_get_item_bitmap_file_name(string_cstr(&info->id_text), name, identified);
    bitmap = (struct BitmapRes *)res_mgr_get(g_resMgr(), name);
    ui_window_draw_bitmap_tinted(self, tint, x, y, bitmap, 1, 0);
    if (info->count)
        draw_count(self, x, y, info->count, plain_count);
}

void ui_window_draw_item_win_mid(struct UIWindow *self, int x, int y)
{
    ui_window_draw_bitmap(self, x, y, ui_skin_bitmap(UI_SKIN "basic_interface\\itemwin_mid.bmp"), 0);
}

void ui_window_draw_new_item_mark(struct UIWindow *self, int x, int y)
{
    ui_window_draw_bitmap(self, x, y, ui_skin_bitmap(UI_SKIN "basic_interface\\new_item.bmp"), 0);
}
