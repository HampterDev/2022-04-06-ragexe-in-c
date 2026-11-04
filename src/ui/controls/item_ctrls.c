#include "ui/controls/item_ctrls.h"
#include "ui/controls/buttons.h"
#include "world/tooltip.h"
#include "ui/character/selection_buttons.h"
#include "core/text_array.h"
#include "render/rp_pool.h"
#include "skills/skill_info.h"
#include "core/object_type_tables.h"
#include "world/mode_manager.h"
#include "ui/window_mgr.h"
#include "ui/game.h"
#include "ui/ui_items.h"
#include "ui/controls/controls.h"
#include "ui/ui_std.h"
#include <stdlib.h>
#include <string.h>
#include "game/drag_info.h"
#include "login/char_info.h"
#include "core/msg_str.h"
#include "core/skin_path.h"
#include "core/text_util.h"
#include "diag/perf.h"
#include "session/session_items.h"
#include "session/session_jobs.h"
#include "session/session_names.h"
#include "ui/draw_key.h"

#define ITEM_WND_ID 0xc

#define MSG_ITEM_TOOLTIP 0xb7
#define MSG_ITEM_TOOLTIP_OPTIONS 0x9f7

struct DragInfo;

#define SLOT_MSG_SET_STYLE 0x1a7
#define SLOT_ID_DBLCLK 0xb0
#define SLOT_VAL_R_BTN_DOWN 0xda
#define MODE_MSG_CHAR_INFO 8
#define MSG_DEL_DATE 0xc20
#define MSG_DEL_TIME 0xd15
#define MSG_MAKE_CHARACTER 0xaed
#define SLOT_DIR UI_SKIN "select_character_ver3\\"

const struct UIButtonOps ui_slot_bmp_button_ops = {
    .base = {
        .destroy = ui_slot_bmp_button_destroy,
        .resize = ui_window_resize_base,
        .is_frame = 0,
        .refreshes_children = 0,
        .move = ui_window_move_base,
        .is_focusable = ui_window_is_focusable_base,
        .get_backdrop_box = ui_window_get_backdrop_box_base,
        .is_click_through = ui_window_is_click_through_base,
        .is_hit_testable = ui_window_is_hit_testable_base,
        .drag_and_drop = ui_window_drag_and_drop_base,
        .store_info = ui_window_store_info_base,
        .save_pos = ui_window_save_pos_base,
        .move_delta = ui_window_move_delta_base,
        .set_show = ui_window_set_show_base,
        .on_create = ui_slot_bmp_button_on_create,
        .on_destroy = ui_window_on_destroy_base,
        .on_process = ui_slot_bmp_button_on_process,
        .on_draw = ui_slot_bmp_button_on_draw,
        .on_focus = ui_window_on_focus_base,
        .on_focus_lost = ui_window_on_focus_lost_base,
        .on_l_btn_down = ui_slot_bmp_button_on_l_btn_down,
        .on_l_btn_dbl_clk = ui_slot_bmp_button_on_l_btn_dbl_clk,
        .on_mouse_move = ui_button_on_mouse_move,
        .on_mouse_hover = ui_button_on_mouse_hover,
        .track_mouse = ui_button_track_mouse,
        .on_mouse_shape = ui_button_on_mouse_shape,
        .on_l_btn_up = ui_button_on_l_btn_up,
        .on_r_btn_down = ui_slot_bmp_button_on_r_btn_down,
        .on_r_btn_up = ui_window_on_r_btn_up_base,
        .on_wheel = ui_window_on_wheel_base,
        .snap = ui_window_snap_base,
        .send_msg = ui_slot_bmp_button_send_msg,
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
    },
    .set_show = ui_button_set_show,
};

void ui_slot_bmp_button_destruct(struct UISlotBmpButton *self)
{
    self->bmp.button.base.ops = &ui_slot_bmp_button_ops.base;
    ui_bitmap_button_destruct(&self->bmp);
}

void ui_slot_bmp_button_destroy(struct UIWindow *window)
{
    struct UISlotBmpButton *self = (struct UISlotBmpButton *)window;
    ui_slot_bmp_button_destruct(self);
    mem_free(self);
}

void ui_slot_bmp_button_on_create(struct UIWindow *window, int width, int height)
{
    struct UISlotBmpButton *self = (struct UISlotBmpButton *)window;
    struct String dir;
    char path[0x104];
    struct UIWindow *icon;

    string_init(&dir);
    string_assign(&dir, UI_SKIN "renewalparty\\", 0x1c);
    sprintf(path, "%sicon_jobs_%d.bmp", string_cstr(&dir), 0);
    icon = mem_alloc(sizeof(struct UIStaticBitmap));
    if (icon)
        icon = (struct UIWindow *)ui_static_bitmap_construct((struct UIStaticBitmap *)icon);
    self->job_icon = icon;
    ui_static_bitmap_set_bitmap((struct UIStaticBitmap *)icon, path);
    ui_window_create(self->job_icon, self->job_icon->width, self->job_icon->height);
    ui_window_move_base(self->job_icon, 0x76, 0xe);
    ui_window_add_child(&self->bmp.button.base, self->job_icon);
    string_free(&dir);
}

void ui_slot_bmp_button_set_slot(struct UISlotBmpButton *self, int slot)
{
    const struct CharInfo *info;
    struct String dir;
    char path[0x104];

    self->slot = slot;
    if (slot < 0) {
        ui_window_set_show_base(self->job_icon, 0);
        return;
    }
    info = (const struct CharInfo *)mode_send_message(client_current_mode(), MODE_MSG_CHAR_INFO, slot, 0, 0, 0);
    if (!info) {
        ui_window_set_show_base(self->job_icon, 0);
        return;
    }
    string_init(&dir);
    string_assign(&dir, UI_SKIN "renewalparty\\", 0x1c);
    sprintf(path, "%sicon_jobs_%d.bmp", string_cstr(&dir), info->job);
    ui_window_set_show_base(self->job_icon, 1);
    ui_static_bitmap_set_bitmap((struct UIStaticBitmap *)self->job_icon, path);
    string_free(&dir);
}

void ui_slot_bmp_button_on_process(struct UIWindow *window)
{
    struct UISlotBmpButton *self = (struct UISlotBmpButton *)window;
    self->anim_tick++;
    if (self->anim_tick / self->anim_divisor > 7)
        self->anim_tick = 0;
}

static unsigned long long slot_bmp_button_key(struct UISlotBmpButton *self)
{
    const struct CharInfo *info;
    unsigned long long key = DRAW_KEY_START;

    key = draw_key_int(key, self->bmp.button.base.state);
    key = draw_key_int(key, self->bmp.button.disabled);
    key = draw_key_int(key, self->style);
    key = draw_key_int(key, self->selected);
    if (!self->bmp.button.disabled && self->style == 1 && self->selected == 1)
        key = draw_key_int(key, self->anim_tick / self->anim_divisor);
    key = draw_key_int(key, self->occupied);
    key = draw_key_int(key, g_session1.charListCounts[0]);
    key = draw_key_int(key, self->slot);
    if (self->slot < 0)
        return key;
    info = (const struct CharInfo *)mode_send_message(client_current_mode(), MODE_MSG_CHAR_INFO, self->slot, 0, 0, 0);
    if (!info)
        return key;
    key = draw_key_int(key, info->rename_count);
    key = draw_key_int(key, info->slot_change_count);
    key = draw_key_int(key, info->delete_date);
    if (info->delete_date) {
        struct Time when;

        time_construct(&when, info->delete_date);
        key = draw_key_int(key, time_is_past(&when));
        time_destruct(&when);
    }
    return key;
}

int ui_slot_bmp_button_changed(struct UISlotBmpButton *self)
{
    return slot_bmp_button_key(self) != self->drawn_key;
}

void ui_slot_bmp_button_on_draw(struct UIWindow *window)
{
    struct UISlotBmpButton *self = (struct UISlotBmpButton *)window;
    struct UIWindow *wnd = &self->bmp.button.base;
    const struct CharInfo *info;

    self->drawn_key = slot_bmp_button_key(self);
    ui_window_clear_canvas(wnd, UI_TRANSPARENT);
    ui_bitmap_button_on_draw((struct UIWindow *)(&self->bmp));

    if (!self->bmp.button.disabled) {
        const char *path = NULL;
        char name[0x3a] = SLOT_DIR "img_slot_select7.bmp";

        if (self->style == 0 && self->selected == 1) {
            path = ui_skin_path(SLOT_DIR "img_slot2_select.bmp");
        } else if (self->style == 1 && self->selected == 1) {
            snprintf(name, sizeof name, SLOT_DIR "img_slot_select%d.bmp", self->anim_tick / self->anim_divisor);
            path = ui_skin_path(name);
        }
        if (path) {
            struct BitmapRes *bitmap = (struct BitmapRes *)res_mgr_get(g_resMgr(), path);

            if (bitmap)
                ui_window_draw_bitmap(wnd, 0, 0, bitmap, 0);
        }
    }

    if (!self->occupied && !g_session1.charListCounts[0]) {
        struct TextArray lines = {0};
        unsigned i;
        int y = 0x22;

        ui_window_draw_bitmap(wnd, wnd->width / 2 - 0x2e, 0x1a,
                            ui_skin_bitmap(SLOT_DIR "balloon.bmp"), 0);
        text_split_lines(msg_str(MSG_MAKE_CHARACTER), &lines, 0x14);
        for (i = 0; i < lines.size; i++, y += 0xd)
            ui_window_text_out_centred(wnd, 0x21, y, text_array_at(&lines, i).data,
                                    0x5c, 0, 0xb, 0xffffff, 0);
        text_array_clear(&lines);
    }

    if (self->slot < 0)
        return;
    info = (const struct CharInfo *)mode_send_message(client_current_mode(), MODE_MSG_CHAR_INFO, self->slot, 0, 0, 0);
    if (!info)
        return;
    if (info->rename_count > 0 || info->slot_change_count > 0)
        ui_window_draw_bitmap(wnd, 0x13, 0x19,
                            (struct BitmapRes *)res_mgr_get(g_resMgr(), UI_SKIN "Add_Ons.bmp"), 0);
    if (info->delete_date) {
        struct Time when;
        struct TimeRecord f;
        struct String date, time;
        COLORREF colour;
        int y;

        time_construct(&when, info->delete_date);
        colour = time_is_past(&when) ? 0xff0000 : 0xff;
        time_get_as_record(&when, &f);
        string_init(&date);
        string_format(&date, msg_str(MSG_DEL_DATE), f.month, f.day);
        string_init(&time);
        string_format(&time, msg_str(MSG_DEL_TIME), f.hour, f.minute, f.second);
        y = 0x28;
        ui_window_text_out(wnd,
                         0x4f - ui_window_text_width(wnd, string_cstr(&date), 0, 0, 0xb, 0, 0) / 2, y,
                         string_cstr(&date), 0, 0, 0xb, colour, 0, 0);
        ui_window_text_out(wnd,
                         0x4f - ui_window_text_width(wnd, string_cstr(&time), 0, 0, 0xb, 0, 0) / 2, 0x34,
                         string_cstr(&time), 0, 0, 0xb, colour, 0, 0);
        string_free(&time);
        string_free(&date);
        time_destruct(&when);
    }
}

void ui_slot_bmp_button_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UISlotBmpButton *self = (struct UISlotBmpButton *)window;
    struct UIWindow *wnd = &self->bmp.button.base;

    ui_button_on_l_btn_down((struct UIWindow *)(&self->bmp.button), x, y);
    if (wnd->parent) {
        ui_window_send_msg(wnd->parent, wnd, UI_MSG_SET_STATE, 0, 0, 0, 0);
        ui_window_send_msg(wnd->parent, wnd, UI_MSG_COMMAND, wnd->id, 0, 0, 0);
    }
}

void ui_slot_bmp_button_on_l_btn_dbl_clk(struct UIWindow *window, int x, int y)
{
    struct UISlotBmpButton *self = (struct UISlotBmpButton *)window;
    struct UIWindow *wnd = &self->bmp.button.base;

    ui_button_on_l_btn_dbl_clk((struct UIWindow *)(&self->bmp.button), x, y);
    if (wnd->parent)
        ui_window_send_msg(wnd->parent, wnd, UI_MSG_COMMAND, SLOT_ID_DBLCLK, 0, 0, 0);
}

void ui_slot_bmp_button_on_r_btn_down(struct UIWindow *window, int x, int y)
{
    struct UISlotBmpButton *self = (struct UISlotBmpButton *)window;
    struct UIWindow *wnd = &self->bmp.button.base;

    if (wnd->parent)
        ui_window_send_msg(wnd->parent, wnd, UI_MSG_COMMAND, wnd->id, SLOT_VAL_R_BTN_DOWN, 0, 0);
}

intptr_t ui_slot_bmp_button_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, intptr_t val1,
                                       intptr_t val2, intptr_t val3, intptr_t val4)
{
    struct UISlotBmpButton *self = (struct UISlotBmpButton *)window;
    if (msg == UI_MSG_SET_STATE) {
        self->selected = val1 != 0;
        return 0;
    }
    if (msg != SLOT_MSG_SET_STYLE)
        return ui_window_send_msg_base(&self->bmp.button.base, sender, msg, val1, val2, val3, val4);
    self->style = val1;
    if (val1 == 0) {
        ui_bitmap_button_set_bitmap(&self->bmp, SLOT_DIR "img_slot2_normal.bmp", 0, 0);
        ui_bitmap_button_set_bitmap(&self->bmp, SLOT_DIR "img_slot2_over.bmp", 1, 0);
        ui_bitmap_button_set_bitmap(&self->bmp, SLOT_DIR "img_slot2_press.bmp", 2, 0);
    } else if (val1 == 1) {
        ui_bitmap_button_set_bitmap(&self->bmp, SLOT_DIR "img_slot_normal.bmp", 0, 0);
        ui_bitmap_button_set_bitmap(&self->bmp, SLOT_DIR "img_slot_over.bmp", 1, 0);
        ui_bitmap_button_set_bitmap(&self->bmp, SLOT_DIR "img_slot_press.bmp", 2, 0);
    }
    return 0;
}

static struct UISlotBmpButton *slot_bmp_button_construct(struct UISlotBmpButton *self, int occupied)
{
    ui_bitmap_button_construct(&self->bmp);
    self->bmp.button.base.ops = &ui_slot_bmp_button_ops.base;
    self->occupied = occupied;
    self->selected = 0;
    self->style = 0;
    self->slot = -1;
    self->job_icon = NULL;
    self->anim_tick = 0;
    self->anim_divisor = 5;
    self->drawn_key = 0;
    return self;
}

struct UISlotBmpButton *ui_slot_bmp_button_construct(struct UISlotBmpButton *self, int occupied)
{
    return slot_bmp_button_construct(self, occupied);
}
