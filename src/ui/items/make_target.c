#include "ui/controls/listbox.h"
#include "game/drag_info.h"
#include "render/rp_pool.h"
#include "iteminfo/item_api.h"
#include "iteminfo/item_tables.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "core/object_type_tables.h"
#include "world/mode_manager.h"
#include "ui/window_access.h"
#include "game/context.h"
#include <string.h>
#include "ui/window.h"
#include "ui/draw.h"
#include "ui/skin.h"
#include "ui/controls/controls.h"
#include "ui/window_mgr.h"
#include "ui/game.h"
#include "ui/ui_items.h"
#include "ui/ui_std.h"
#include "core/msg_str.h"
#include "diag/perf.h"
#include "session/session_items.h"
#include "ui/wndmgr/make_window_internal.h"

#define MT_LIST_WND_ID 0x4f
#define MT_PROCESS_WND_ID 0x50
#define MT_RESULT_WND_ID 0x51

#define LIST_MSG_SELECT_PREV 0x12
#define LIST_MSG_SELECT_NEXT 0x13
#define MT_MSG_ADD_TARGET 0x1f
#define MT_MSG_SET_TARGET 0x28
#define MT_MSG_TAKE_BACK 0x4b
#define MT_REQ_MAKE 0x82
#define MT_REQ_SHOW_ITEM 0x86
#define MT_REQ_DRAG_END 0x12
#define MT_DRAG_ITEM 1
#define MT_ITEM_WND_REFRESH 0x17

#define MT_ORE_FIRST 994
#define MT_ORE_LAST 1000
#define MT_IS_ORE(id) ((unsigned)((id) - MT_ORE_FIRST) <= MT_ORE_LAST - MT_ORE_FIRST)

#define JOB_RUNE_KNIGHT 4054
#define JOB_RUNE_KNIGHT_H 4060
#define JOB_BABY_RUNE_KNIGHT 4096

#define MSG_MT_LIST_TITLE 0x1a9
#define MSG_MT_PROCESS_LINE1 0x1aa
#define MSG_MT_PROCESS_LINE2 0x1ab
#define MSG_MT_RESULT_FAIL1 0x1ac
#define MSG_MT_RESULT_OK1 0x1ad
#define MSG_MT_RESULT_FAIL2 0x1ae
#define MSG_MT_RESULT_OK2 0x1af

#define MT_STONES 3

struct UIMakeTargetListWnd {
    struct UIFrameWnd frame;
    struct UIListBox *list;
    struct Vec ids;
};
OFFSET_ASSERT(UIMakeTargetListWnd, ids, 0x98);
SIZE_ASSERT(UIMakeTargetListWnd, 0xa4);

struct UIMakeTargetProcessWnd {
    struct UIFrameWnd frame;
    int item_id;
    struct ItemInfo stones[MT_STONES];
};
OFFSET_ASSERT(UIMakeTargetProcessWnd, stones, 0x98);

struct UIMakeTargetResultWnd {
    struct UIFrameWnd frame;
    int item_id;
    BYTE success;
};
OFFSET_ASSERT(UIMakeTargetResultWnd, success, 0x98);
SIZE_ASSERT(UIMakeTargetResultWnd, 0x9c);

void ui_make_target_list_wnd_destroy(struct UIWindow *window);
void ui_make_target_list_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_make_target_list_wnd_on_draw(struct UIWindow *self);
int ui_make_target_list_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);

const struct UIWindowOps ui_make_target_list_wnd_ops = {
    .destroy = ui_make_target_list_wnd_destroy,
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
    .on_create = ui_make_target_list_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_make_target_list_wnd_on_draw,
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
    .send_msg = ui_make_target_list_wnd_send_msg,
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
void ui_make_target_process_wnd_destroy(struct UIWindow *window);
void ui_make_target_process_wnd_drag_and_drop(struct UIWindow *window, int x, int y, const struct DragInfo *drag);
void ui_make_target_process_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_make_target_process_wnd_on_draw(struct UIWindow *self);
void ui_make_target_process_wnd_on_l_btn_down(struct UIWindow *self, int x, int y);
int ui_make_target_process_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);

const struct UIWindowOps ui_make_target_process_wnd_ops = {
    .destroy = ui_make_target_process_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_make_target_process_wnd_drag_and_drop,
    .store_info = ui_window_store_info_base,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_make_target_process_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_make_target_process_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_make_target_process_wnd_on_l_btn_down,
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
    .send_msg = ui_make_target_process_wnd_send_msg,
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
void ui_make_target_result_wnd_destroy(struct UIWindow *window);
void ui_make_target_result_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_make_target_result_wnd_on_draw(struct UIWindow *self);
int ui_make_target_result_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_make_target_result_wnd_ops = {
    .destroy = ui_make_target_result_wnd_destroy,
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
    .on_create = ui_make_target_result_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_make_target_result_wnd_on_draw,
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
    .send_msg = ui_make_target_result_wnd_send_msg,
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

static void add_buttons(struct UIFrameWnd *frame, int width, int height, int count)
{
    static const char *const names[2][3] = {
        { "btn_ok", "btn_ok_a", "btn_ok_b" },
        { "btn_cancel", "btn_cancel_a", "btn_cancel_b" },
    };
    static const int ids[2] = { UI_ID_OK, UI_ID_CANCEL };
    const int xs[2] = { width - 0x5b, width - 0x2e };
    struct String dir;
    int i;

    ui_skin_dir(&dir);
    for (i = 0; i < count; i++)
        ui_add_named_skin_button(&frame->base, &dir, names[i], xs[i], height - 0x18, ids[i]);
    frame->default_id = UI_ID_OK;
    string_free(&dir);
}

static void draw_chrome(struct UIWindow *self, int sys_icon, const char *title)
{
    int top = ui_window_draw_title_bar(self, sys_icon, title, 0);

    ui_window_draw_body(self, top, 0, "");
    ui_window_draw_button_bar(self, 2, 0, 1);
}

static void draw_item_line(struct UIWindow *self, struct ItemInfo *info, int y, const char *text)
{
    int width = ui_window_draw_item_name(self, 0x10, y, info, 0, 0);

    ui_window_draw_shadow_text(self, width + 0x10, y, text, 0, 0, 0xffffff, 0, 0xc, 0);
}

static void target_info(struct ItemInfo *info, int item_id)
{
    item_info_construct(info);
    item_info_set_item_id(info, item_id);
    info->identified = 1;
    info->refine = 0;
}

static void send_make_empty(int item_id)
{
    struct ItemInfo items[MT_STONES];
    int i;

    for (i = 0; i < MT_STONES; i++)
        item_info_construct(&items[i]);
    game_send_message(MT_REQ_MAKE, item_id, (int)items, 0, 0);
    for (i = MT_STONES - 1; i >= 0; i--)
        item_info_destruct(&items[i]);
}

struct UIMakeTargetListWnd *ui_make_target_list_wnd_construct(struct UIMakeTargetListWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_make_target_list_wnd_ops;
    self->ids.first = self->ids.last = self->ids.end = NULL;
    return self;
}

struct UIWindow *ui_make_target_list_wnd_new(void)
{
    return (struct UIWindow *)ui_make_target_list_wnd_construct(mem_alloc(sizeof(struct UIMakeTargetListWnd)));
}

void ui_make_target_list_wnd_destroy(struct UIWindow *window)
{
    struct UIMakeTargetListWnd *self = (struct UIMakeTargetListWnd *)window;
    self->frame.base.ops = &ui_make_target_list_wnd_ops;
    vec_free(&self->ids);
    ui_frame_wnd_destruct(&self->frame);
    mem_free(self);
}

void ui_make_target_list_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UIMakeTargetListWnd *self = (struct UIMakeTargetListWnd *)window;
    void *mem = mem_alloc(sizeof(struct UIListBox));
    struct UIListBox *list = mem ? ui_list_box_construct(mem) : NULL;

    self->list = list;
    ui_list_box_create_window(list, 0xc, 0x16, width - 0x18, height - 0x37, 0);
    self->list->r = 0xf0;
    self->list->g = 0xf0;
    self->list->b = 0xf0;
    self->list->base.id = UI_ID_OK;
    ui_window_add_child(&self->frame.base, &self->list->base);
    add_buttons(&self->frame, width, height, 2);
}

void ui_make_target_list_wnd_on_draw(struct UIWindow *window)
{
    struct UIMakeTargetListWnd *self = (struct UIMakeTargetListWnd *)window;
    draw_chrome(&self->frame.base, 1, msg_str(MSG_MT_LIST_TITLE));
}

static void list_choose(struct UIMakeTargetListWnd *self)
{
    int item_id;
    struct UIWindow *wnd;

    item_id = ((int *)self->ids.first)[self->list->selected];
    switch (g_session1.job) {
    case JOB_RUNE_KNIGHT:
    case JOB_RUNE_KNIGHT_H:
    case JOB_BABY_RUNE_KNIGHT:
        send_make_empty(item_id);
        break;
    default:
        wnd = ui_window_open(MT_PROCESS_WND_ID);
        ui_window_send_msg(wnd, &self->frame.base, MT_MSG_SET_TARGET, item_id, 0, 0, 0);
        ui_window_move(wnd, self->frame.base.x, self->frame.base.y);
        break;
    }
    ui_close_window(MT_LIST_WND_ID);
}

int ui_make_target_list_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                           int val1, int val2, int val3, int val4)
{
    struct UIMakeTargetListWnd *self = (struct UIMakeTargetListWnd *)window;
    struct ItemInfo info;
    struct String name = {0};
    int result = 0, id;

    item_info_construct(&info);
    switch (msg) {
    case MT_MSG_ADD_TARGET:
        vec_push(&self->ids, &val1, sizeof val1);
        item_info_set_item_id(&info, val1);
        info.refine = 0;
        info.identified = 1;
        item_info_get_display_name(&info, &name);
        ui_list_box_append_text(self->list, string_cstr(&name));
        string_free(&name);
        break;
    case LIST_MSG_SELECT_PREV:
    case LIST_MSG_SELECT_NEXT:
        ui_window_send_msg(&self->list->base, NULL, msg, 0, 0, 0, 0);
        break;
    case UI_MSG_COMMAND:

        if (val1 == UI_ID_CANCEL) {
            send_make_empty(0);
            ui_close_window(MT_LIST_WND_ID);
        } else if (val1 == UI_ID_OK) {
            if (ui_list_box_count(self->list) == 0) {
                ui_close_window(MT_LIST_WND_ID);
                ui_close_window(MT_LIST_WND_ID);
            } else if (self->list->selected >= 0) {
                list_choose(self);
            }
        }
        break;

    default:
        result = ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
        break;
    }
    item_info_destruct(&info);
    return result;
}

struct UIMakeTargetProcessWnd *ui_make_target_process_wnd_construct(struct UIMakeTargetProcessWnd *self)
{
    int i;

    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_make_target_process_wnd_ops;
    for (i = 0; i < MT_STONES; i++)
        item_info_construct(&self->stones[i]);
    return self;
}

struct UIWindow *ui_make_target_process_wnd_new(void)
{
    return (struct UIWindow *)ui_make_target_process_wnd_construct(mem_alloc(sizeof(struct UIMakeTargetProcessWnd)));
}

void ui_make_target_process_wnd_destroy(struct UIWindow *window)
{
    struct UIMakeTargetProcessWnd *self = (struct UIMakeTargetProcessWnd *)window;
    int i;

    self->frame.base.ops = &ui_make_target_process_wnd_ops;
    for (i = MT_STONES - 1; i >= 0; i--)
        item_info_destruct(&self->stones[i]);
    ui_frame_wnd_destruct(&self->frame);
    mem_free(self);
}

void ui_make_target_process_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UIMakeTargetProcessWnd *self = (struct UIMakeTargetProcessWnd *)window;
    add_buttons(&self->frame, width, height, 2);
}

void ui_make_target_process_wnd_on_draw(struct UIWindow *window)
{
    struct UIMakeTargetProcessWnd *self = (struct UIMakeTargetProcessWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct ItemInfo info;
    const struct ItemTextLines *lines;
    struct BitmapRes *bar;
    unsigned i;
    int x;

    draw_chrome(wnd, 0, "");
    if (!MT_IS_ORE(self->item_id)) {
        for (x = 8, i = 0; x < 0x68; x += 0x20, i++) {
            bar = ui_skin_bitmap(UI_SKIN "basic_interface\\btnbar_right2.bmp");
            ui_window_draw_item_win_mid(wnd, x - 4, wnd->height - bar->height - 0x20);
            if (self->stones[i].index)
                ui_window_draw_item_icon(wnd, x, wnd->height - bar->height - 0x1c, &self->stones[i], 0);
        }
    }
    target_info(&info, self->item_id);
    draw_item_line(wnd, &info, 2, msg_str(MSG_MT_PROCESS_LINE1));
    draw_item_line(wnd, &info, 0x16, msg_str(MSG_MT_PROCESS_LINE2));
    lines = metal_process_get(self->item_id);
    for (i = 0; i < lines->count; i++)
        ui_window_text_out(wnd, 0x10, 0x2a + 0x14 * (int)i, lines->data[i], 0, 0, 0xc, 0, 0,
                         0);
    item_info_destruct(&info);
}

static struct ItemInfo *stone_at(struct UIMakeTargetProcessWnd *self, struct ItemInfo *out, int x, int y)
{
    int bottom = self->frame.base.height;

    if (!MT_IS_ORE(self->item_id) && y > bottom - 0x38 && y < bottom - 0x20) {
        if ((unsigned)(x - 4) <= 0x1e)
            return item_info_copy(out, &self->stones[0]);
        if ((unsigned)(x - 0x24) <= 0x1e)
            return item_info_copy(out, &self->stones[1]);
        if ((unsigned)(x - 0x44) <= 0x1e)
            return item_info_copy(out, &self->stones[2]);
    }
    return item_info_construct_with_arg(out);
}

void ui_make_target_process_wnd_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIMakeTargetProcessWnd *self = (struct UIMakeTargetProcessWnd *)window;
    struct ItemInfo info;

    stone_at(self, &info, x, y);
    if (info.index)
        mode_send_message(client_current_mode(), MT_REQ_SHOW_ITEM, (int)string_cstr(&info.id_text), 0, 0, 0);
    else
        ui_frame_wnd_on_l_btn_down((struct UIWindow *)(&self->frame), x, y);
    item_info_destruct(&info);
}

void ui_make_target_process_wnd_drag_and_drop(struct UIWindow *window, int x, int y, const struct DragInfo *drag)
{
    struct UIMakeTargetProcessWnd *self = (struct UIMakeTargetProcessWnd *)window;
    struct ItemInfo info;
    BYTE has_stone = 0;
    int i, free_slot = MT_STONES;

    if (MT_IS_ORE(self->item_id)) {
        game_send_message(MT_REQ_DRAG_END, 0, 0, 0, 0);
        return;
    }
    if (drag->source == MT_DRAG_ITEM) {
        for (i = 0; i < MT_STONES; i++) {
            if (self->stones[i].index)
                has_stone |= item_info_is_element_stone(&self->stones[i]);
            else if (free_slot == MT_STONES)
                free_slot = i;
        }
        if (free_slot < MT_STONES) {
            session_get_inventory_item(&g_session1, &info, drag->index);
            if (info.index && item_info_is_element_stone_or_star_crumb(&info)
                && !(has_stone && item_info_is_element_stone(&info))) {
                item_info_assign(&self->stones[free_slot], &info);
                self->stones[free_slot].count = 1;
                session_remove_inventory_item(&g_session1, info.index, 1);
                if (g_windowMgr.item_wnd)
                    ui_window_send_msg(g_windowMgr.item_wnd, NULL, MT_ITEM_WND_REFRESH, 0, 0, 0, 0);
                ui_window_invalidate_base(&self->frame.base);
            }
            item_info_destruct(&info);
        }
    }
    mode_send_message(client_current_mode(), MT_REQ_DRAG_END, 0, 0, 0, 0);
}

int ui_make_target_process_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                              int val1, int val2, int val3, int val4)
{
    struct UIMakeTargetProcessWnd *self = (struct UIMakeTargetProcessWnd *)window;
    BYTE returned = 0;
    int i;

    switch (msg) {
    case UI_MSG_COMMAND:

        if (val1 == UI_ID_OK) {
            game_send_message(MT_REQ_MAKE, self->item_id, (int)self->stones, 0, 0);
            ui_close_window(MT_PROCESS_WND_ID);
        } else if (val1 == UI_ID_CANCEL) {
            game_send_message(MT_REQ_MAKE, 0, (int)self->stones, 0, 0);
            for (i = 0; i < MT_STONES; i++) {
                if (self->stones[i].index) {
                    returned = 1;
                    session_add_inventory_item(&g_session1, &self->stones[i], 0);
                }
            }
            if (returned)
                ui_window_mgr_refresh_item_windows(&g_windowMgr);
            if (g_windowMgr.item_wnd)
                ui_window_send_msg(g_windowMgr.item_wnd, NULL, MT_ITEM_WND_REFRESH, 0, 0, 0, 0);
            ui_close_window(MT_PROCESS_WND_ID);
        }
        return 0;
    case MT_MSG_SET_TARGET:
        self->item_id = val1;
        ui_window_invalidate_base(&self->frame.base);
        return 0;
    case MT_MSG_TAKE_BACK:
        for (i = MT_STONES - 1; i >= 0; i--) {
            struct ItemInfo *stone = &self->stones[i];

            if (stone->index && stone->id_text.size == strlen((const char *)val1)
                && memcmp(string_cstr(&stone->id_text), (const char *)val1, stone->id_text.size) == 0) {
                session_add_inventory_item(&g_session1, stone, 1);
                item_info_clear(stone);
                ui_window_invalidate_base(&self->frame.base);
                return 0;
            }
        }
        return 0;
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
}

struct UIMakeTargetResultWnd *ui_make_target_result_wnd_construct(struct UIMakeTargetResultWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_make_target_result_wnd_ops;
    self->success = 0;
    self->item_id = 0;
    return self;
}

struct UIWindow *ui_make_target_result_wnd_new(void)
{
    return (struct UIWindow *)ui_make_target_result_wnd_construct(mem_alloc(sizeof(struct UIMakeTargetResultWnd)));
}

void ui_make_target_result_wnd_destroy(struct UIWindow *window)
{
    struct UIMakeTargetResultWnd *self = (struct UIMakeTargetResultWnd *)window;
    self->frame.base.ops = &ui_make_target_result_wnd_ops;
    ui_frame_wnd_destruct(&self->frame);
    mem_free(self);
}

void ui_make_target_result_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UIMakeTargetResultWnd *self = (struct UIMakeTargetResultWnd *)window;
    add_buttons(&self->frame, width, height, 1);
}

void ui_make_target_result_wnd_on_draw(struct UIWindow *window)
{
    struct UIMakeTargetResultWnd *self = (struct UIMakeTargetResultWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct ItemInfo info;

    draw_chrome(wnd, 0, "");
    target_info(&info, self->item_id);
    draw_item_line(wnd, &info, 2, msg_str(self->success == 1 ? MSG_MT_RESULT_OK1 : MSG_MT_RESULT_FAIL1));
    draw_item_line(wnd, &info, 0x2a, msg_str(self->success == 1 ? MSG_MT_RESULT_OK2 : MSG_MT_RESULT_FAIL2));
    item_info_destruct(&info);
}

int ui_make_target_result_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                             int val1, int val2, int val3, int val4)
{
    struct UIMakeTargetResultWnd *self = (struct UIMakeTargetResultWnd *)window;
    switch (msg) {
    case UI_MSG_COMMAND:
        ui_close_window(MT_RESULT_WND_ID);
        return 0;
    case MT_MSG_SET_TARGET:
        self->item_id = val1;
        if (val2)
            self->success = 1;
        ui_window_invalidate_base(&self->frame.base);
        return 0;
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
}
