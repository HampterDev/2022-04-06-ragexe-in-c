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
#include <mmsystem.h>
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
#define ITEM_WND_MSG_SHOW 0x18
#define SKILL_WND_ID 0x2e
#define SKILL_WND_MSG_SHOW 0x3d

#define MSG_ITEM_TOOLTIP 0xb7
#define MSG_ITEM_TOOLTIP_OPTIONS 0x9f7

static void draw_outlined_number(struct UIWindow *wnd, int x, int y, const char *text)
{
    ui_window_text_out(wnd, x + 1, y, text, 0, 0, 0xb, 0xffffff, 0, 0);
    ui_window_text_out(wnd, x - 1, y, text, 0, 0, 0xb, 0xffffff, 0, 0);
    ui_window_text_out(wnd, x, y + 1, text, 0, 0, 0xb, 0xffffff, 0, 0);
    ui_window_text_out(wnd, x, y - 1, text, 0, 0, 0xb, 0xffffff, 0, 0);
    ui_window_text_out(wnd, x, y, text, 0, 0, 0xb, 0, 0, 0);
}

struct UIBasicItemCtrl {
    struct UIWindow base;
    int type_filter;
    int location_mask;
    BYTE source;
    struct String empty_tooltip;
    struct ItemInfo item;
};
OFFSET_ASSERT(UIBasicItemCtrl, empty_tooltip, 0x88);
struct DragInfo;
const struct UIWindowOps ui_basic_item_ctrl_ops = {
    .destroy = ui_basic_item_ctrl_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 0,
    .refreshes_children = 0,
    .move = ui_window_move_base,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_basic_item_ctrl_drag_and_drop,
    .store_info = ui_window_store_info_base,
    .save_pos = ui_window_save_pos_base,
    .move_delta = ui_window_move_delta_base,
    .set_show = ui_window_set_show_base,
    .on_create = ui_window_on_create_base,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_basic_item_ctrl_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_window_on_l_btn_down_base,
    .on_l_btn_dbl_clk = ui_basic_item_ctrl_on_l_btn_dbl_clk,
    .on_mouse_move = ui_window_on_mouse_move_base,
    .on_mouse_hover = ui_basic_item_ctrl_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_window_on_l_btn_up_base,
    .on_r_btn_down = ui_basic_item_ctrl_on_r_btn_down,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_window_snap_base,
    .send_msg = ui_window_send_msg_base,
    .invalidate = ui_window_invalidate_base,
    .render = ui_window_render_base,
    .has_custom_backdrop = 0,
    .draw_backdrop = ui_window_draw_backdrop_base,
    .get_records = ui_window_get_records_base,
    .type = UI_TYPE_WINDOW,
    .on_hover_enter = ui_window_on_hover_enter_base,
    .on_hover_leave = ui_window_on_hover_leave_base,
    .make_quad = ui_window_make_quad_base,
    .hit_test = ui_window_hit_test_base,
    .render_overlay = ui_window_render_overlay_base,
};

#define ITEM_ID(ctrl) atoi(string_cstr(&(ctrl)->item.id_text))

void ui_basic_item_ctrl_destroy(struct UIWindow *window)
{
    struct UIBasicItemCtrl *self = (struct UIBasicItemCtrl *)window;
    self->base.ops = &ui_basic_item_ctrl_ops;
    item_info_destruct(&self->item);
    string_free(&self->empty_tooltip);
    ui_window_destruct(&self->base);
    mem_free(self);
}

void ui_basic_item_ctrl_drag_and_drop(struct UIWindow *window, int x, int y, const struct DragInfo *drag)
{
    struct UIBasicItemCtrl *self = (struct UIBasicItemCtrl *)window;
    if (!drag->named && drag->identified == self->source) {
        struct ItemInfo info;
        int fits;

        item_info_assign(&self->item, session_get_inventory_item(&g_session1, &info, drag->index));
        item_info_destruct(&info);

        if (self->type_filter == 0x15 && session_is_consumable_item_type(&g_session1, self->item.type))
            fits = 1;
        else if (self->type_filter == 0x16 && session_is_equip_item_type(&g_session1, self->item.type))
            fits = 1;
        else if (self->type_filter == 0x17 && session_is_etc_item_type(&g_session1, self->item.type))
            fits = 1;
        else
            fits = self->item.type == self->type_filter || self->type_filter == -1;
        if (fits && (self->location_mask == -1 || (self->item.equip_location & self->location_mask)) &&
            (unsigned)self->item.index > 0)
            ui_invalidate_root(&self->base);
    }
    mode_send_message(client_current_mode(), MODE_MSG_DRAG_END, 0, 0, 0, 0);
}

static void ui_basic_item_ctrl_draw_icon(struct UIBasicItemCtrl *self, int x, int y, BYTE plain)
{
    struct UIWindow *wnd = &self->base;
    struct String text;
    char path[0x80];
    struct BitmapRes *bitmap;
    int id = ITEM_ID(self);

    if (id <= 0)
        return;
    string_init(&text);
    string_format(&text, "%d", id);
    session_get_item_bitmap_file_name(string_cstr(&text), path, self->item.identified);
    string_free(&text);
    bitmap = (struct BitmapRes *)res_mgr_get(g_resMgr(), path);
    if (bitmap)
        ui_window_draw_bitmap(wnd, x, y, bitmap, 1);

    if ((unsigned)self->item.index > 0 && self->item.count) {
        string_format(&text, "%d", self->item.count);
        if (plain) {
            ui_window_text_out(wnd, x + 0x10, y + 0x14, string_cstr(&text), 0, 0, 0xb, 0, 0, 0);
        } else {
            int tx = x + (self->item.count < 1000 ? 0xe : 0);

            draw_outlined_number(wnd, tx, y + 0xf, string_cstr(&text));
        }
        string_free(&text);
    }
}

void ui_basic_item_ctrl_on_draw(struct UIWindow *window)
{
    struct UIBasicItemCtrl *self = (struct UIBasicItemCtrl *)window;
    const char *frame;

    ui_window_clear_canvas(&self->base, UI_TRANSPARENT);
    if (ITEM_ID(self) > 0 && (unsigned)self->item.index == 0)
        frame = UI_SKIN "ExMacro\\no_item.bmp";
    else
        frame = UI_SKIN "basic_interface\\itemwin_mid.bmp";
    ui_window_draw_bitmap(&self->base, 0, 0, ui_skin_bitmap(frame), 0);
    ui_basic_item_ctrl_draw_icon(self, 4, 0, 0);
}

void ui_basic_item_ctrl_on_l_btn_dbl_clk(struct UIWindow *window, int x, int y)
{
    struct UIBasicItemCtrl *self = (struct UIBasicItemCtrl *)window;
    struct ItemInfo empty;

    item_info_assign(&self->item, item_info_construct_with_arg(&empty));
    item_info_destruct(&empty);
    ui_invalidate_root(&self->base);
}

void ui_basic_item_ctrl_on_mouse_hover(struct UIWindow *window, int x_unused, int y_unused)
{
    struct UIBasicItemCtrl *self = (struct UIBasicItemCtrl *)window;
    struct String text, line;

    struct String name = {0};
    int x = 0, y = 0;

    string_init(&text);
    if (ITEM_ID(self) <= 0) {
        string_assign(&text, string_cstr(&self->empty_tooltip), self->empty_tooltip.size);
    } else if ((unsigned)self->item.index > 0) {
        item_info_get_display_name(&self->item, &name);
        string_init(&line);
        if (self->item.option_count)
            string_format(&line, msg_str(MSG_ITEM_TOOLTIP_OPTIONS), string_cstr(&name),
                          self->item.option_count, self->item.count);
        else
            string_format(&line, msg_str(MSG_ITEM_TOOLTIP), string_cstr(&name), self->item.count);
        string_assign(&text, string_cstr(&line), line.size);
        string_free(&line);
        string_free(&name);
    } else {
        const char *p;

        item_info_get_display_name(&self->item, &name);
        p = string_cstr(&name);
        string_assign_cstr(&text, p);
        string_free(&name);
    }
    ui_window_to_screen(&self->base, &x, &y);
    if (text.size)
        mode_show_tooltip(client_current_mode(), string_cstr(&text), x, y, 0xffffffff, 0);
    string_free(&text);
}

void ui_basic_item_ctrl_on_r_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIBasicItemCtrl *self = (struct UIBasicItemCtrl *)window;
    if (ITEM_ID(self) > 0) {
        struct UIWindow *wnd = ui_window_open(ITEM_WND_ID);

        ui_window_send(wnd, NULL, ITEM_WND_MSG_SHOW, (int)&self->item, 0, 0, 0);
    }
}

struct UIBasicSkillCtrl {
    struct UIWindow base;
    int skill_id;
    int level;
};
SIZE_ASSERT(UIBasicSkillCtrl, 0x84);
const struct UIWindowOps ui_basic_skill_ctrl_ops = {
    .destroy = ui_basic_skill_ctrl_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 0,
    .refreshes_children = 0,
    .move = ui_window_move_base,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_basic_skill_ctrl_drag_and_drop,
    .store_info = ui_window_store_info_base,
    .save_pos = ui_window_save_pos_base,
    .move_delta = ui_window_move_delta_base,
    .set_show = ui_window_set_show_base,
    .on_create = ui_window_on_create_base,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_basic_skill_ctrl_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_window_on_l_btn_down_base,
    .on_l_btn_dbl_clk = ui_basic_skill_ctrl_on_l_btn_dbl_clk,
    .on_mouse_move = ui_window_on_mouse_move_base,
    .on_mouse_hover = ui_basic_skill_ctrl_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_window_on_l_btn_up_base,
    .on_r_btn_down = ui_basic_skill_ctrl_on_r_btn_down,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_window_snap_base,
    .send_msg = ui_window_send_msg_base,
    .invalidate = ui_window_invalidate_base,
    .render = ui_window_render_base,
    .has_custom_backdrop = 0,
    .draw_backdrop = ui_window_draw_backdrop_base,
    .get_records = ui_window_get_records_base,
    .type = UI_TYPE_WINDOW,
    .on_hover_enter = ui_window_on_hover_enter_base,
    .on_hover_leave = ui_window_on_hover_leave_base,
    .make_quad = ui_window_make_quad_base,
    .hit_test = ui_window_hit_test_base,
    .render_overlay = ui_window_render_overlay_base,
};

void ui_basic_skill_ctrl_destroy(struct UIWindow *window)
{
    struct UIBasicSkillCtrl *self = (struct UIBasicSkillCtrl *)window;
    self->base.ops = &ui_basic_skill_ctrl_ops;
    ui_window_destruct(&self->base);
    mem_free(self);
}

void ui_basic_skill_ctrl_drag_and_drop(struct UIWindow *window, int x, int y, const struct DragInfo *drag)
{
    struct UIBasicSkillCtrl *self = (struct UIBasicSkillCtrl *)window;
    if (drag->named) {
        struct SkillInfo info;

        session_get_skill_info(&info, drag->index);
        if (info.level && info.found && info.sp_cost) {
            self->skill_id = drag->index;
            self->level = drag->level;
            ui_invalidate_root(&self->base);
        }
        skill_info_destruct(&info);
    }
    mode_send_message(client_current_mode(), MODE_MSG_DRAG_END, 0, 0, 0, 0);
}

void ui_basic_skill_ctrl_on_draw(struct UIWindow *window)
{
    struct UIBasicSkillCtrl *self = (struct UIBasicSkillCtrl *)window;
    struct UIWindow *wnd = &self->base;
    BYTE drawn = 0;

    ui_window_clear_canvas(wnd, UI_TRANSPARENT);
    if (self->skill_id >= 0) {
        struct SkillInfo info;
        char buf[0x400];

        session_get_skill_info(&info, self->skill_id);
        if (info.found) {
            drawn = 1;
            if (self->level > 0)
                info.level = self->level;
            session_format_item_bitmap_path(info.id_name, buf);
            ui_window_draw_bitmap(wnd, 0, 0, (struct BitmapRes *)res_mgr_get(g_resMgr(), buf), 1);
            if (info.level) {
                sprintf(buf, "%d", info.level);
                draw_outlined_number(wnd, wnd->width - ui_window_text_width(wnd, buf, 0, 0, 0xb, 0, 0) - 1, 0xf,
                                     buf);
            }
        }
        skill_info_destruct(&info);
    }
    if (!drawn)
        ui_window_draw_bitmap(wnd, 0, 0, ui_skin_bitmap(UI_SKIN "basic_interface\\no_skill.bmp"), 0);
}

void ui_basic_skill_ctrl_on_l_btn_dbl_clk(struct UIWindow *window, int x, int y)
{
    struct UIBasicSkillCtrl *self = (struct UIBasicSkillCtrl *)window;
    self->skill_id = -1;
    ui_invalidate_root(&self->base);
}

void ui_basic_skill_ctrl_on_mouse_hover(struct UIWindow *window, int x_unused, int y_unused)
{
    struct UIBasicSkillCtrl *self = (struct UIBasicSkillCtrl *)window;
    struct SkillInfo info;
    struct String text, line;
    int x = 0, y = 0, sp, ap;

    if (self->skill_id < 0)
        return;
    ui_window_to_screen(&self->base, &x, &y);
    session_get_skill_info(&info, self->skill_id);
    sp = skill_info_get_sp_amount(&info, info.level);
    ap = skill_info_get_ap_amount(&info, info.level);
    string_init(&text);
    if (session_is_space(&g_session1, info.id)) {
        string_init(&line);
        if (ap)
            string_format(&line, "%s Use Lv %d (Sp : %d%%, Ap : %d)", skill_info_get_name(&info),
                          info.level, sp, ap);
        else
            string_format(&line, "%s Use Lv %d (Sp : %d%%)", skill_info_get_name(&info), info.level, sp);
    } else {
        string_init(&line);
        if (ap)
            string_format(&line, "%s Use Lv %d (Sp : %d, Ap : %d)", skill_info_get_name(&info),
                          info.level, sp, ap);
        else
            string_format(&line, "%s Use Lv %d (Sp : %d)", skill_info_get_name(&info), info.level, sp);
    }
    string_assign(&text, string_cstr(&line), line.size);
    string_free(&line);
    if (text.size)
        mode_show_tooltip(client_current_mode(), string_cstr(&text), x, y, 0xffffffff, 0);
    string_free(&text);
    skill_info_destruct(&info);
}

void ui_basic_skill_ctrl_on_r_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIBasicSkillCtrl *self = (struct UIBasicSkillCtrl *)window;
    struct UIWindow *wnd;
    int sx = 0, sy = 0;

    if (self->skill_id < 0)
        return;
    wnd = ui_make_window(SKILL_WND_ID);
    ui_window_send_msg(wnd, NULL, SKILL_WND_MSG_SHOW, self->skill_id, 0, 0, 0);
    ui_window_to_screen(&self->base, &sx, &sy);
    ui_window_move(wnd, sx + x + 0x18, sy + y);
}

struct UIStaticItem {
    struct UIWindow base;
    struct ItemInfo item;
};
const struct UIWindowOps ui_static_item_ops = {
    .destroy = ui_static_item_destroy,
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
    .on_create = ui_window_on_create_base,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_static_item_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_window_on_l_btn_down_base,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_window_on_mouse_move_base,
    .on_mouse_hover = ui_static_item_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_window_on_l_btn_up_base,
    .on_r_btn_down = ui_static_item_on_r_btn_down,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_window_snap_base,
    .send_msg = ui_window_send_msg_base,
    .invalidate = ui_window_invalidate_base,
    .render = ui_window_render_base,
    .has_custom_backdrop = 0,
    .draw_backdrop = ui_window_draw_backdrop_base,
    .get_records = ui_window_get_records_base,
    .type = UI_TYPE_WINDOW,
    .on_hover_enter = ui_window_on_hover_enter_base,
    .on_hover_leave = ui_window_on_hover_leave_base,
    .make_quad = ui_window_make_quad_base,
    .hit_test = ui_window_hit_test_base,
    .render_overlay = ui_window_render_overlay_base,
};
const struct UIWindowOps ui_static_item_illust_ops = {
    .destroy = ui_static_item_illust_destroy,
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
    .on_create = ui_window_on_create_base,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_static_item_illust_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_window_on_l_btn_down_base,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_window_on_mouse_move_base,
    .on_mouse_hover = ui_static_item_illust_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_window_on_l_btn_up_base,
    .on_r_btn_down = ui_static_item_on_r_btn_down,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_window_snap_base,
    .send_msg = ui_window_send_msg_base,
    .invalidate = ui_window_invalidate_base,
    .render = ui_window_render_base,
    .has_custom_backdrop = 0,
    .draw_backdrop = ui_window_draw_backdrop_base,
    .get_records = ui_window_get_records_base,
    .type = UI_TYPE_WINDOW,
    .on_hover_enter = ui_window_on_hover_enter_base,
    .on_hover_leave = ui_window_on_hover_leave_base,
    .make_quad = ui_window_make_quad_base,
    .hit_test = ui_window_hit_test_base,
    .render_overlay = ui_window_render_overlay_base,
};

void ui_static_item_destruct(struct UIStaticItem *self)
{
    self->base.ops = &ui_static_item_ops;
    item_info_destruct(&self->item);
    ui_window_destruct(&self->base);
}

void ui_static_item_illust_destruct(struct UIStaticItem *self)
{
    self->base.ops = &ui_static_item_illust_ops;
    ui_static_item_destruct(self);
}

void ui_static_item_destroy(struct UIWindow *window)
{
    struct UIStaticItem *self = (struct UIStaticItem *)window;
    ui_static_item_destruct(self);
    mem_free(self);
}

void ui_static_item_illust_destroy(struct UIWindow *window)
{
    struct UIStaticItem *self = (struct UIStaticItem *)window;
    ui_static_item_illust_destruct(self);
    mem_free(self);
}

void ui_static_item_on_draw(struct UIWindow *window)
{
    struct UIStaticItem *self = (struct UIStaticItem *)window;
    char path[0x104];
    struct BitmapRes *bitmap;

    memset(path, 0, sizeof path);
    session_get_item_bitmap_file_name(string_cstr(&self->item.id_text), path,
                                   self->item.identified);
    bitmap = (struct BitmapRes *)res_mgr_get(g_resMgr(), path);
    if (!bitmap)
        bitmap = (struct BitmapRes *)res_mgr_get(g_resMgr(), UI_SKIN "item\\사과.bmp");
    if (bitmap)
        ui_window_draw_bitmap(&self->base, 0, 0, bitmap, 1);
}

void ui_static_item_illust_on_draw(struct UIWindow *window)
{
    struct UIStaticItem *self = (struct UIStaticItem *)window;
    struct String name, path;
    const char *res = item_info_get_res_name(&self->item);
    struct BitmapRes *bitmap;

    string_init(&name);
    string_assign_cstr(&name, res);
    string_insert(&name, 0, UI_SKIN "collection\\", 0x1a);
    path = name;
    string_init(&name);
    string_append_cstr(&path, ".bmp");
    bitmap = (struct BitmapRes *)res_mgr_get(g_resMgr(), string_cstr(&path));
    if (!bitmap)
        bitmap = (struct BitmapRes *)res_mgr_get(g_resMgr(), UI_SKIN "collection\\사과.bmp");
    if (bitmap)
        ui_window_draw_bitmap(&self->base, 0, 0, bitmap, 1);
    string_free(&path);
    string_free(&name);
}

void ui_static_item_on_mouse_hover(struct UIWindow *window, int x_unused, int y_unused)
{
    struct UIStaticItem *self = (struct UIStaticItem *)window;
    const char *name;
    int x = 0, y = -20;

    name = item_info_get_name(&self->item);
    if (!name[0])
        return;
    ui_window_to_screen(&self->base, &x, &y);
    mode_show_tooltip(client_current_mode(), name, x, y, 0xffffffff, 0);
}

void ui_static_item_illust_on_mouse_hover(struct UIWindow *self, int x, int y)
{
}

void ui_static_item_on_r_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIStaticItem *self = (struct UIStaticItem *)window;
    struct UIWindow *wnd = ui_window_open(ITEM_WND_ID);

    ui_window_send(wnd, NULL, ITEM_WND_MSG_SHOW, (int)&self->item, 0, 0, 0);
}

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
        .get_records = ui_window_get_records_base,
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
            _snprintf(name, (size_t)-1, SLOT_DIR "img_slot_select%d.bmp", self->anim_tick / self->anim_divisor);
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

int ui_slot_bmp_button_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1,
                                       int val2, int val3, int val4)
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
