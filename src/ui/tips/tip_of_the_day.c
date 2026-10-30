#include "ui/controls/listbox.h"
#include "render/rp_pool.h"
#include "core/object_type_tables.h"
#include <stdlib.h>
#include "ui/tips/tips.h"
#include "ui/ui_items.h"
#include "contents/tips.h"
#include "core/msg_str.h"
#include "diag/perf.h"

#define TIP_OF_THE_DAY_WND_ID 0x26

#define ID_BACK 0xee
#define ID_NEXT 0xd2

#define MSG_TOTD_TITLE 0xae
#define MSG_TOTD_HEADING 0xaf
#define MSG_TOTD_SHOW_AT_STARTUP 0xb0

#define TOTD_TEXT_LAYOUT 0x26
#define MSG_TOTD_SHOW_TIP UI_MSG_ADD_TEXT

struct UITipOfTheDayWnd {
    struct UIFrameWnd frame;
    struct UIWindow *text;
    int index;
    int show_at_startup;
};
OFFSET_ASSERT(UITipOfTheDayWnd, text, 0x94);
SIZE_ASSERT(UITipOfTheDayWnd, 0xa0);

static int totd_count(void)
{
    if (!g_tipOfTheDay.loaded)
        tip_of_the_day_read(&g_tipOfTheDay);
    return (int)VEC_COUNT(&g_tipOfTheDay.tips, struct Vec);
}

static void totd_add_tip(struct UITipOfTheDayWnd *self, const struct Vec *lines)
{
    unsigned i;

    ui_window_send_msg(self->text, NULL, UI_MSG_ADD_TEXT, (int)msg_str(MSG_TOTD_HEADING), TOTD_TEXT_LAYOUT, 0, 0);
    for (i = 0; i < VEC_COUNT(lines, struct String); i++)
        ui_window_send_msg(self->text, NULL, UI_MSG_ADD_TEXT,
                 (int)string_cstr(&VEC_AT(lines, struct String, i)), TOTD_TEXT_LAYOUT, 0, 0);
}

void ui_tip_of_the_day_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_tip_of_the_day_wnd_on_draw(struct UIWindow *self);
int ui_tip_of_the_day_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
void ui_tip_of_the_day_wnd_destroy(struct UIWindow *window);
const struct UIWindowOps ui_tip_of_the_day_wnd_ops = {
    .destroy = ui_tip_of_the_day_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_window_store_info_base,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_tip_of_the_day_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_tip_of_the_day_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_frame_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_frame_wnd_on_mouse_move,
    .on_mouse_hover = ui_frame_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_frame_wnd_on_l_btn_up,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_tip_of_the_day_wnd_send_msg,
    .invalidate = ui_window_invalidate_base,
    .render = ui_window_render_base,
    .has_custom_backdrop = 0,
    .draw_backdrop = ui_window_draw_backdrop_base,
    .get_records = ui_frame_wnd_get_records,
    .type = UI_TYPE_FRAME,
    .on_hover_enter = ui_window_on_hover_enter_base,
    .on_hover_leave = ui_window_on_hover_leave_base,
    .make_quad = ui_window_make_quad_base,
    .hit_test = ui_window_hit_test_base,
    .render_overlay = ui_window_render_overlay_base,
};

struct UITipOfTheDayWnd *ui_tip_of_the_day_wnd_construct(struct UITipOfTheDayWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_tip_of_the_day_wnd_ops;
    return self;
}

struct UIWindow *ui_tip_of_the_day_wnd_new(void)
{
    return (struct UIWindow *)ui_tip_of_the_day_wnd_construct(mem_alloc(sizeof(struct UITipOfTheDayWnd)));
}

void ui_tip_of_the_day_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UITipOfTheDayWnd *self = (struct UITipOfTheDayWnd *)window;
    static const char *const names[3][3] = {
        { "btn_back", "btn_back_a", "btn_back_b" },
        { "btn_next", "btn_next_a", "btn_next_b" },
        { "btn_close", "btn_close_a", "btn_close_b" },
    };
    static const int ids[3] = { ID_BACK, ID_NEXT, UI_ID_CLOSE };
    static const char *const tooltips[3] = { "", "", "/tip" };
    struct String dir;
    struct UIWindow *check;
    void *mem;
    int i, y;

    ui_skin_dir(&dir);
    y = self->frame.base.height - 0x18;
    for (i = 0; i < 3; i++) {
        struct UIBitmapButton *button;

        mem = mem_alloc(sizeof(struct UIBitmapButton));
        button = mem ? ui_bitmap_button_construct(mem) : NULL;
        ui_button_bitmap(button, &dir, names[i][0], 0);
        ui_button_bitmap(button, &dir, names[i][1], 1);
        ui_button_bitmap(button, &dir, names[i][2], 2);
        ui_window_create(&button->button.base, button->bitmap_width, button->bitmap_height);
        ui_window_move_base(&button->button.base, 0x90 + i * 0x2d, y);
        button->button.base.id = ids[i];
        ui_bitmap_button_set_tool_tip(button, tooltips[i]);
        ui_window_add_child(&self->frame.base, &button->button.base);
    }
    self->frame.default_id = ID_BACK;

    mem = mem_alloc(sizeof(struct UITextViewer));
    self->text = mem ? ui_text_viewer_construct(mem) : NULL;
    ui_list_box_create_window((struct UIListBox *)self->text, 10, 0x16, width - 0x14, height - 0x37, 0);
    UI_TEXT_VIEWER_BACKGROUND(self->text) = ui_skin_argb(2, 2);
    ui_window_invalidate(self->text);
    ui_window_add_child(&self->frame.base, self->text);

    self->show_at_startup = g_showTipsAtStartup;
    mem = mem_alloc(sizeof(struct UICheckBox));
    check = mem ? (struct UIWindow *)ui_check_box_construct(mem) : NULL;
    ui_window_create(check,
                     ui_window_text_width(&self->frame.base, msg_str(MSG_TOTD_SHOW_AT_STARTUP), 0, 0, 0xc, 0, 0) +
                         0xc,
                     0xc);
    ui_check_box_set_bitmaps(check, UI_SKIN "checkbox_1.bmp", UI_SKIN "checkbox_0.bmp",
                          msg_str(MSG_TOTD_SHOW_AT_STARTUP));
    ui_window_move_base(check, 10, height - 0x14);
    ui_check_box_set_check(check, self->show_at_startup);
    ui_window_add_child(&self->frame.base, check);

    if (!g_tipOfTheDay.loaded)
        tip_of_the_day_read(&g_tipOfTheDay);
    if (VEC_COUNT(&g_tipOfTheDay.tips, struct Vec)) {
        int r = rand();

        self->index = r % totd_count();
        totd_add_tip(self, tip_of_the_day_get(&g_tipOfTheDay, self->index));
    }
    string_free(&dir);
}

void ui_tip_of_the_day_wnd_on_draw(struct UIWindow *window)
{
    struct UITipOfTheDayWnd *self = (struct UITipOfTheDayWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    int body, n, k, y, cols, rows;

    ui_window_draw_title_bar(wnd, 1, msg_str(MSG_TOTD_TITLE), 0);
    if (wnd->height == 0x11)
        return;

    body = wnd->height - 0x26;
    n = body / 8 + (body % 8 != 0);
    for (k = 0, y = 0x11; k < n; k++, y += 8)
        ui_window_draw_bitmap(wnd, 0, y, ui_skin_bitmap(UI_SKIN "basic_interface\\itemwin_left.bmp"), 0);
    body = wnd->height - 0x26;
    n = body / 8 + (body % 8 != 0);
    for (k = 0, y = 0x11; k < n; k++, y += 8) {
        struct BitmapRes *right = ui_skin_bitmap(UI_SKIN "basic_interface\\itemwin_right.bmp");

        ui_window_draw_bitmap(wnd, wnd->width - right->width, y, right, 0);
    }

    ui_window_fill_rect(wnd, 0x14, 0x11, (wnd->width - 0x3c) % 32, wnd->height - 0x26, ui_skin_colour(2, 2));
    body = wnd->height - 0x26;
    ui_window_fill_rect(wnd, 0x14, body / 32 * 32 + 0x11, wnd->width - 0x18, body % 32, ui_skin_colour(2, 2));
    ui_window_draw_button_bar(wnd, 2, 0, 1);

    cols = (wnd->width - 0x28) / 32;
    rows = (wnd->height - 0x26) / 32;
    for (k = 0, y = 0x11; k < rows; k++, y += 0x20)
        ui_window_draw_item_win_mid(wnd, 0x28, y);
    ui_window_fill_rect(wnd, 0x48, 0x11, cols * 32 - 0x20, rows * 32, ui_skin_colour(2, 2));
}

int ui_tip_of_the_day_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1,
                                        int val2, int val3, int val4)
{
    struct UITipOfTheDayWnd *self = (struct UITipOfTheDayWnd *)window;

    if (msg == MSG_TOTD_SHOW_TIP) {
        ui_list_box_clear((struct UIListBox *)self->text);
        totd_add_tip(self, (const struct Vec *)val1);
        return 0;
    }
    if (msg != UI_MSG_COMMAND)
        return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);

    switch (val1) {
    case UI_ID_CLOSE:
        ui_window_mgr_post_quit1(&g_windowMgr, TIP_OF_THE_DAY_WND_ID);
        break;
    case UI_ID_CHECKBOX:
        self->show_at_startup = val2;
        g_showTipsAtStartup = val2;
        tips_save_show_at_startup(&self->show_at_startup);
        break;
    case ID_NEXT:
        if (!totd_count())
            break;
        self->index++;
        if (self->index >= totd_count())
            self->index = 0;
        ui_tip_of_the_day_wnd_send_msg(&self->frame.base, NULL, MSG_TOTD_SHOW_TIP, (int)tip_of_the_day_get(&g_tipOfTheDay, self->index),
                 0, 0, 0);
        break;
    case ID_BACK:
        if (!totd_count())
            break;
        if (--self->index < 0)
            self->index = totd_count() - 1;
        ui_tip_of_the_day_wnd_send_msg(&self->frame.base, NULL, MSG_TOTD_SHOW_TIP, (int)tip_of_the_day_get(&g_tipOfTheDay, self->index),
                 0, 0, 0);
        break;
    }
    return 0;
}

void ui_tip_of_the_day_wnd_destroy(struct UIWindow *window)
{
    struct UIFrameWnd *self = (struct UIFrameWnd *)window;
    self->base.ops = &ui_tip_of_the_day_wnd_ops;
    ui_frame_wnd_destruct(self);
    mem_free(self);
}
