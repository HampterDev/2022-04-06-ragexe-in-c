#include "ui/window_access.h"
#include "render/rp_pool.h"
#include "core/object_type_tables.h"
#include "ui/items/items.h"
#include "game/context.h"
#include <stdlib.h>
#include "core/msg_str.h"
#include "core/skin_path.h"
#include "diag/perf.h"

#define CARD_ILLUST_MSG_TITLE 0x20f
#define UI_ILLUST_TRANSPARENT 0xffff00ff

struct UICardItemIllustWnd {
    struct UIFrameWnd frame;
    struct String bitmap;
    struct String item_id;
    BYTE identified;
};

struct UIWindow;
void ui_card_item_illust_wnd_destroy(struct UIWindow *window);
void ui_card_item_illust_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_card_item_illust_wnd_on_draw(struct UIWindow *self);
intptr_t ui_card_item_illust_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4);
const struct UIWindowOps ui_card_item_illust_wnd_ops = {
    .destroy = ui_card_item_illust_wnd_destroy,
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
    .on_create = ui_card_item_illust_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_card_item_illust_wnd_on_draw,
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
    .send_msg = ui_card_item_illust_wnd_send_msg,
    .invalidate = ui_window_invalidate_base,
    .render = ui_window_render_base,
    .has_custom_backdrop = 0,
    .draw_backdrop = ui_window_draw_backdrop_base,
    .type = UI_TYPE_FRAME,
    .on_hover_enter = ui_window_on_hover_enter_base,
    .on_hover_leave = ui_window_on_hover_leave_base,
    .make_quad = ui_window_make_quad_base,
    .hit_test = ui_window_hit_test_base,
    .render_overlay = ui_window_render_overlay_base,
};

struct UICardItemIllustWnd *ui_card_item_illust_wnd_construct(struct UICardItemIllustWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_card_item_illust_wnd_ops;
    string_init(&self->bitmap);
    string_init(&self->item_id);
    return self;
}

struct UIWindow *ui_card_item_illust_wnd_new(void)
{
    return (struct UIWindow *)ui_card_item_illust_wnd_construct(mem_alloc(sizeof(struct UICardItemIllustWnd)));
}

void ui_card_item_illust_wnd_destruct(struct UICardItemIllustWnd *self)
{
    self->frame.base.ops = &ui_card_item_illust_wnd_ops;
    string_free(&self->item_id);
    string_free(&self->bitmap);
    ui_frame_wnd_destruct(&self->frame);
}

void ui_card_item_illust_wnd_destroy(struct UIWindow *window)
{
    struct UICardItemIllustWnd *self = (struct UICardItemIllustWnd *)window;
    ui_card_item_illust_wnd_destruct(self);
    mem_free(self);
}

void ui_card_item_illust_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UICardItemIllustWnd *self = (struct UICardItemIllustWnd *)window;
    const char *text;

    self->identified = 0;
    text = msg_str(CARD_ILLUST_MSG_TITLE);
    string_assign_cstr(&self->item_id, text);
    ui_window_add_close_button(&self->frame.base, "", 0);
}

void ui_card_item_illust_wnd_on_draw(struct UIWindow *window)
{
    struct UICardItemIllustWnd *self = (struct UICardItemIllustWnd *)window;
    struct ItemInfo info;

    ui_window_draw_title_bar(&self->frame.base, 0, "", 0);
    if (self->bitmap.size != 0)
        ui_window_draw_bitmap(&self->frame.base, 0, 0x11,
                            res_mgr_get(g_resMgr(), string_cstr(&self->bitmap)), 0);
    item_info_construct(&info);
    string_assign(&info.id_text, string_cstr(&self->item_id), self->item_id.size);
    info.identified = self->identified;
    info.refine = 0;
    ui_window_draw_item_name(&self->frame.base, 10, 3, &info, 0, 0);
    item_info_destruct(&info);
}

intptr_t ui_card_item_illust_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                           intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4)
{
    struct UICardItemIllustWnd *self = (struct UICardItemIllustWnd *)window;
    const char *id_text, *name;

    switch (msg) {
    case UI_MSG_COMMAND:
        if (val1 == UI_ID_CLOSE)
            ui_window_mgr_post_quit1(&g_windowMgr, UI_WND_CARD_ILLUST);
        return 0;

    case UI_MSG_SHOW_CARD:
        id_text = (const char *)val1;
        self->identified = val2 != 0;
        name = card_affix_get(atoi(id_text))->illustName;
        if (!card_affix_has(atoi(id_text)) || string_equal_cstr(&self->item_id, id_text)) {
            ui_window_mgr_post_quit1(&g_windowMgr, UI_WND_CARD_ILLUST);
            return 0;
        }
        string_assign_cstr(&self->item_id, id_text);
        string_assign(&self->bitmap, UI_SKIN "cardBmp\\", sizeof(UI_SKIN "cardBmp\\") - 1);
        string_append_cstr(&self->bitmap, name);
        string_append_cstr(&self->bitmap, ".bmp");
        ui_window_invalidate_base(&self->frame.base);
        ui_window_mgr_raise_window(&g_windowMgr, &self->frame.base);
        return 0;
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
}

struct UIIllustWnd {
    struct UIFrameWnd frame;
    int titled;
    int closable;
    struct String bitmap;
};

void ui_illust_wnd_destroy(struct UIWindow *window);
BYTE ui_illust_wnd_is_hit_testable(struct UIWindow *self);
void ui_illust_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_illust_wnd_on_destroy(struct UIWindow *self);
void ui_illust_wnd_on_draw(struct UIWindow *self);
void ui_illust_wnd_on_mouse_move(struct UIWindow *self, int x, int y);
intptr_t ui_illust_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4);
const struct UIWindowOps ui_illust_wnd_ops = {
    .destroy = ui_illust_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_illust_wnd_is_hit_testable,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_window_store_info_base,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_illust_wnd_on_create,
    .on_destroy = ui_illust_wnd_on_destroy,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_illust_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_frame_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_illust_wnd_on_mouse_move,
    .on_mouse_hover = ui_frame_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_frame_wnd_on_l_btn_up,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_illust_wnd_send_msg,
    .invalidate = ui_window_invalidate_base,
    .render = ui_window_render_base,
    .has_custom_backdrop = 0,
    .draw_backdrop = ui_window_draw_backdrop_base,
    .type = UI_TYPE_FRAME,
    .on_hover_enter = ui_window_on_hover_enter_base,
    .on_hover_leave = ui_window_on_hover_leave_base,
    .make_quad = ui_window_make_quad_base,
    .hit_test = ui_window_hit_test_base,
    .render_overlay = ui_window_render_overlay_base,
};

struct UIIllustWnd *ui_illust_wnd_construct(struct UIIllustWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_illust_wnd_ops;
    string_init(&self->bitmap);
    self->titled = 0;
    self->closable = 0;
    return self;
}

struct UIWindow *ui_illust_wnd_new(void)
{
    return (struct UIWindow *)ui_illust_wnd_construct(mem_alloc(sizeof(struct UIIllustWnd)));
}

void ui_illust_wnd_destruct(struct UIIllustWnd *self)
{
    self->frame.base.ops = &ui_illust_wnd_ops;
    string_free(&self->bitmap);
    ui_frame_wnd_destruct(&self->frame);
}

void ui_illust_wnd_destroy(struct UIWindow *window)
{
    struct UIIllustWnd *self = (struct UIIllustWnd *)window;
    ui_illust_wnd_destruct(self);
    mem_free(self);
}

BYTE ui_illust_wnd_is_hit_testable(struct UIWindow *window)
{
    struct UIIllustWnd *self = (struct UIIllustWnd *)window;
    return self->titled || self->closable;
}

void ui_illust_wnd_on_create(struct UIWindow *self, int width, int height)
{
}

void ui_illust_wnd_on_destroy(struct UIWindow *window)
{
    struct UIIllustWnd *self = (struct UIIllustWnd *)window;
    void *res;

    if (self->bitmap.size == 0)
        return;
    res = res_mgr_get(g_resMgr(), ui_skin_path(string_cstr(&self->bitmap)));
    res_mgr_release(g_resMgr(), res);
}

void ui_illust_wnd_on_draw(struct UIWindow *window)
{
    struct UIIllustWnd *self = (struct UIIllustWnd *)window;
    void *res;
    int top = 0;

    ui_window_clear_canvas(&self->frame.base, UI_ILLUST_TRANSPARENT);
    res = res_mgr_get(g_resMgr(), ui_skin_path(string_cstr(&self->bitmap)));
    if (self->titled)
        top = ui_window_draw_title_bar(&self->frame.base, 0, "", 0);
    ui_window_draw_bitmap(&self->frame.base, 0, top, res, 1);
}

void ui_illust_wnd_on_mouse_move(struct UIWindow *window, int x, int y)
{
    struct UIIllustWnd *self = (struct UIIllustWnd *)window;
    if (self->closable)
        ui_frame_wnd_on_mouse_move((struct UIWindow *)(&self->frame), x, y);
}

#define ILLUST_MSG_SET_FRAME 0x22
#define ILLUST_MSG_SET_BITMAP 0x4e
#define MODE_MSG_SHOW_ILLUSTRATION 0x64

static void illust_set_name(struct UIIllustWnd *self, const char *folder, const char *name)
{
    string_assign_cstr(&self->bitmap, folder);
    string_append_cstr(&self->bitmap, name);
}
static struct BitmapRes *illust_bitmap(struct UIIllustWnd *self)
{
    return (struct BitmapRes *)res_mgr_get(g_resMgr(), ui_skin_path(string_cstr(&self->bitmap)));
}

intptr_t ui_illust_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2,
                                   intptr_t val3, intptr_t val4)
{
    struct UIIllustWnd *self = (struct UIIllustWnd *)window;
    struct UIWindow *wnd = &self->frame.base;

    if (msg == UI_MSG_COMMAND) {
        if (val1 == UI_ID_CLOSE) {
            game_send_message(MODE_MSG_SHOW_ILLUSTRATION, (intptr_t)"", -1, 1, 0);
            ui_window_mgr_release_capture(&g_windowMgr);
        }
        return 0;
    }
    if (msg == ILLUST_MSG_SET_FRAME) {
        if (val1 == -1) {
            self->titled = 0;
            self->closable = 0;
            ui_window_destroy_children(wnd);
        } else if (val1 == -2) {
            self->titled = 1;
        } else {
            struct UIWindow *close = ui_window_add_close_button(wnd, "", 0);

            ui_window_move_base(close, wnd->width - 0x10, 3);
            self->closable = 1;
            ui_window_invalidate_base(wnd);
        }
        return 0;
    }
    if (msg == ILLUST_MSG_SET_BITMAP) {
        const char *name = (const char *)val1;
        struct BitmapRes *bitmap = NULL;

        illust_set_name(self, UI_SKIN "Illust\\", name);
        if (res_mgr_is_available(g_resMgr(), string_cstr(&self->bitmap)))
            bitmap = illust_bitmap(self);
        if (!bitmap) {
            illust_set_name(self, UI_SKIN "cardbmp\\", name);
            bitmap = illust_bitmap(self);
        }
        if (bitmap && (bitmap->height != wnd->height || wnd->width != bitmap->width)) {
            if (self->titled) {
                struct BitmapRes *bar = ui_skin_bitmap(UI_SKIN "basic_interface\\titlebar_left.bmp");

                ui_window_resize_base(wnd, bitmap->width, bar->height + bitmap->height);
            } else {
                ui_window_resize_base(wnd, bitmap->width, bitmap->height);
            }
        }
        ui_window_invalidate_base(wnd);
        return 0;
    }
    if (self->closable)
        return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
    return ui_window_send_msg_base(wnd, sender, msg, val1, val2, val3, val4);
}
