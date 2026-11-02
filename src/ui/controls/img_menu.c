#include "ui/controls/popups.h"
#include "render/rp_pool.h"
#include "core/object_type_tables.h"
#include "world/mode_manager.h"
#include "ui/window_access.h"
#include "ui/window_mgr.h"
#include "ui/game.h"
#include "ui/ui_items.h"
#include "ui/controls/controls.h"
#include "ui/ui_std.h"
#include <stdlib.h>
#include <string.h>
#include "diag/perf.h"

struct UIImgMenu {
    struct UIWindow base;
    int hot;
    int count;
    int cell_width;
    int cell_height;
    int horizontal;
    BYTE *lit;
    BYTE *disabled;
    struct String background;
    struct String name;
    int target;
};
const struct UIWindowOps ui_img_menu_ops = {
    .destroy = ui_img_menu_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 0,
    .refreshes_children = 0,
    .move = ui_img_menu_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_window_store_info_base,
    .save_pos = ui_window_save_pos_base,
    .move_delta = ui_window_move_delta_base,
    .set_show = ui_window_set_show_base,
    .on_create = ui_window_on_create_base,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_img_menu_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_window_on_l_btn_down_base,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_img_menu_on_mouse_move,
    .on_mouse_hover = ui_img_menu_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_img_menu_on_mouse_shape,
    .on_l_btn_up = ui_img_menu_on_l_btn_up,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_window_snap_base,
    .send_msg = ui_img_menu_send_msg,
    .invalidate = ui_window_invalidate_base,
    .render = ui_window_render_base,
    .has_custom_backdrop = 0,
    .draw_backdrop = ui_window_draw_backdrop_base,
    .type = UI_TYPE_WINDOW,
    .on_hover_enter = ui_window_on_hover_enter_base,
    .on_hover_leave = ui_window_on_hover_leave_base,
    .make_quad = ui_window_make_quad_base,
    .hit_test = ui_window_hit_test_base,
    .render_overlay = ui_window_render_overlay_base,
};

#define BORDER 1
#define FAR_BORDER 3
#define LIT_SHIFT 3
#define FACE_COLOUR 0x99a8ac
#define CLOSE_WINDOW 0x7a

enum {
    UI_MSG_IMG_MENU_SETUP = 0x22,
    UI_MSG_IMG_MENU_PICK = 0x30,
    UI_MSG_IMG_MENU_TARGET = 0x53,
    UI_MSG_IMG_MENU_ID = 0x56,
    UI_MSG_IMG_MENU_LIT = 0x59,
    UI_MSG_IMG_MENU_DISABLE = 0x5a,
};

int ui_img_menu_hit_test(struct UIImgMenu *self, int x, int y)
{
    int span = self->base.width - FAR_BORDER, cell, i;

    if (x < BORDER || x >= span || y < BORDER || y >= self->base.height - FAR_BORDER)
        return -1;
    if (self->horizontal) {
        cell = span / self->count;
        self->cell_width = cell;
        i = x / cell;
    } else {
        cell = (self->base.height - FAR_BORDER) / self->count;
        self->cell_height = cell;
        i = y / cell;
    }
    return i <= self->count - 1 ? i : self->count - 1;
}

void ui_img_menu_destroy(struct UIWindow *window)
{
    struct UIImgMenu *self = (struct UIImgMenu *)window;
    self->base.ops = &ui_img_menu_ops;
    string_free(&self->name);
    string_free(&self->background);
    ui_window_destruct(&self->base);
    mem_free(self);
}

void ui_img_menu_move(struct UIWindow *window, int x, int y)
{
    struct UIImgMenu *self = (struct UIImgMenu *)window;
    ui_window_move_base(&self->base, x, y);
}

void ui_img_menu_on_draw(struct UIWindow *window)
{
    struct UIImgMenu *self = (struct UIImgMenu *)window;
    char suffix[12];
    struct String path;
    struct BitmapRes *bmp;
    int shift;
    size_t n;

    ui_window_fill_rect(&self->base, 0, 0, self->base.width, self->base.height, FACE_COLOUR);
    ui_window_draw_bitmap(&self->base, BORDER, BORDER, ui_skin_bitmap(string_cstr(&self->background)), 0);
    if (self->hot <= -1)
        return;
    sprintf(suffix, "_%d.bmp", self->hot);
    string_init(&path);
    n = strlen(suffix);
    string_reserve(&path, n + self->name.size);
    string_append(&path, string_cstr(&self->name), self->name.size);
    string_append_cstr(&path, suffix);
    bmp = ui_skin_bitmap(string_cstr(&path));
    shift = self->lit[self->hot] ? LIT_SHIFT : 0;
    if (!self->disabled[self->hot]) {
        if (self->horizontal)
            ui_window_draw_bitmap(&self->base, self->cell_width * self->hot + BORDER + shift, BORDER, bmp, 0);
        else
            ui_window_draw_bitmap(&self->base, BORDER, self->cell_height * self->hot + BORDER + shift, bmp, 0);
    }
    string_free(&path);
}

void ui_img_menu_on_mouse_move(struct UIWindow *window, int x, int y)
{
    struct UIImgMenu *self = (struct UIImgMenu *)window;
    ui_window_on_mouse_hover(&self->base, x, y);
}

void ui_img_menu_on_mouse_hover(struct UIWindow *window, int x, int y)
{
    struct UIImgMenu *self = (struct UIImgMenu *)window;
    int old = self->hot;

    self->hot = ui_img_menu_hit_test(self, x, y);
    if (old != self->hot)
        ui_window_invalidate_base(&self->base);
}

void ui_img_menu_on_mouse_shape(struct UIWindow *self, int x, int y)
{
    mode_set_cursor_action(client_current_mode(), 2);
}

void ui_img_menu_on_l_btn_up(struct UIWindow *window, int x, int y)
{
    struct UIImgMenu *self = (struct UIImgMenu *)window;
    struct UIWindow *target;

    self->hot = ui_img_menu_hit_test(self, x, y);
    if (self->hot == -1 || self->disabled[self->hot])
        return;
    target = ui_window_find(self->target);
    if (target)
        ui_window_send_msg(target, NULL, UI_MSG_IMG_MENU_PICK, self->hot, self->base.id, 0, 0);
    ui_window_mgr_post_quit1(&g_windowMgr, CLOSE_WINDOW);
}

int ui_img_menu_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1, int val2,
                                 int val3, int val4)
{
    struct UIImgMenu *self = (struct UIImgMenu *)window;
    switch (msg) {
    case UI_MSG_IMG_MENU_SETUP: {
        struct String name, bg;
        int i;

        self->horizontal = val1;
        self->count = val2;
        string_init(&name);
        string_assign_cstr(&name, (const char *)val3);
        string_copy(&bg, &name);
        string_append_cstr(&bg, ".bmp");
        string_free(&self->background);
        string_move(&self->background, &bg);
        string_assign(&self->name, string_cstr(&name), name.size);
        self->lit = mem_alloc(self->count);
        self->disabled = mem_alloc(self->count);
        for (i = 0; i < self->count; i++) {
            self->lit[i] = 0;
            self->disabled[i] = 0;
        }
        ui_window_invalidate_base(&self->base);
        string_free(&name);
        return 0;
    }
    case UI_MSG_IMG_MENU_TARGET:
        self->target = val1;
        return 0;
    case UI_MSG_IMG_MENU_ID:
        self->base.id = val1;
        return 0;
    case UI_MSG_IMG_MENU_LIT:
        self->lit[val1 - 1] = val2 != 0;
        return 0;
    case UI_MSG_IMG_MENU_DISABLE:
        self->disabled[val1 - 1] = val2 != 0;
        return 0;
    default:
        return ui_window_send_msg_base(&self->base, sender, msg, val1, val2, val3, val4);
    }
}
