#include "ui/controls/controls.h"
#include "ui/draw.h"
#include "ui/skin.h"

#define TITLE_X 0x12
#define TITLE_FONT_HEIGHT 0xc

int ui_window_draw_title_bar(struct UIWindow *self, int sys_icon, const char *title,
                                     int width)
{
    struct BitmapRes *bmp;
    int left = 0, right = 0, height = 0;

    if (!width)
        width = self->width;
    bmp = ui_skin_bitmap(UI_SKIN "basic_interface\\titlebar_left.bmp");
    if (bmp) {
        ui_window_draw_bitmap(self, 0, 0, bmp, 0);
        left = bmp->width;
    }
    bmp = ui_skin_bitmap(UI_SKIN "basic_interface\\titlebar_right.bmp");
    if (bmp) {
        right = bmp->width;
        ui_window_draw_bitmap(self, width - right, 0, bmp, 0);
    }
    bmp = ui_skin_bitmap(UI_SKIN "basic_interface\\titlebar_mid.bmp");
    if (bmp) {
        int span = width - right - left;
        int tiles = span / bmp->width + (span % bmp->width != 0);

        height = bmp->height;
        for (; tiles > 0; tiles--) {
            ui_window_draw_bitmap(self, left, 0, bmp, 0);
            left += bmp->width;
        }
    }
    ui_window_text_out(self, TITLE_X + 1, height - 0xd, title, 0, 0, TITLE_FONT_HEIGHT, 0xffffff, 0, 0);
    ui_window_text_out(self, TITLE_X, height - 0xe, title, 0, 0, TITLE_FONT_HEIGHT, 0, 0, 0);
    if ((BYTE)sys_icon)
        ui_window_draw_bitmap(self, 3, 3, ui_skin_bitmap(UI_SKIN "basic_interface\\sys_base_off.bmp"), 1);
    return height;
}
