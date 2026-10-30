#include "ui/controls/edit.h"
#include "ui/draw.h"
#include "render/textdc.h"
#include <string.h>

#define EDIT_FONT_HEIGHT 12

void ui_edit_ctrl_custom_draw_string(struct UIEditCtrlCustom *self, int x, int y, const char *text)
{
    struct UIEditCtrl *edit = &self->edit;
    struct TextDC dc;
    int length = (int)strlen(text);

    if (edit->double_colour == -1) {
        ui_window_text_out(&edit->base, x, y, text, 0, 0, EDIT_FONT_HEIGHT, edit->text_colour, self->font_flag,
                         0);
        return;
    }
    text_dc_construct(&dc, edit->base.canvas);
    text_dc_select_font(&dc, 0, EDIT_FONT_HEIGHT, self->font_flag, 0);
    text_dc_set_text_color(&dc, edit->text_colour);
    text_dc_text_out(&dc, x, y, text, length);
    text_dc_set_text_color(&dc, (COLORREF)edit->double_colour);
    text_dc_text_out(&dc, x - 1, y, text, length);
    text_dc_destruct(&dc);
}
