#include "ui/controls/listbox.h"
#include "render/rp_pool.h"
#include "session/state.h"
#include "core/object_type_tables.h"
#include "game/context.h"
#include <stdlib.h>
#include <string.h>
#include "ui/character/character.h"
#include "ui/window_mgr.h"
#include "ui/game.h"
#include "ui/ui_items.h"
#include "ui/controls/controls.h"
#include "platform/clientinfo.h"
#include "core/msg_str.h"
#include "diag/perf.h"

static int g_loadingScreenNext = 0;

#define MSGBOX_BITMAP UI_SKIN "win_msgbox.bmp"

#define WND_ID_RESTART 0x13
#define WND_ID_QUIT 0x30
#define WND_ID_REBIRTH 0x81

#define GAME_REQ_RESTART 0x19
#define RESTART_TO_SAVE_POINT 0
#define RESTART_TO_CHAR_SELECT 1
#define GAME_REQ_DISCONNECT 0x58
#define GAME_REQ_RISE_HERE 0xfa

struct UIQuitWnd;
struct UIWindow;
void ui_quit_wnd_destroy(struct UIWindow *window);
void ui_quit_wnd_on_create(struct UIWindow *self, int width_unused, int height_unused);
void ui_quit_wnd_on_draw(struct UIWindow *self);
intptr_t ui_quit_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4);
const struct UIWindowOps ui_quit_wnd_ops = {
    .destroy = ui_quit_wnd_destroy,
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
    .on_create = ui_quit_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_quit_wnd_on_draw,
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
    .send_msg = ui_quit_wnd_send_msg,
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
struct UIRestartWnd;
struct UIWindow;
void ui_restart_wnd_destroy(struct UIWindow *window);
void ui_restart_wnd_on_create(struct UIWindow *self, int width_unused, int height_unused);
void ui_restart_wnd_on_draw(struct UIWindow *self);
intptr_t ui_restart_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4);
const struct UIWindowOps ui_restart_wnd_ops = {
    .destroy = ui_restart_wnd_destroy,
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
    .on_create = ui_restart_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_restart_wnd_on_draw,
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
    .send_msg = ui_restart_wnd_send_msg,
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
struct UIRebirthWnd;
void ui_rebirth_wnd_destroy(struct UIWindow *window);
void ui_rebirth_wnd_on_create(struct UIWindow *self, int width_unused, int height_unused);
void ui_rebirth_wnd_on_draw(struct UIWindow *self);
intptr_t ui_rebirth_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4);
const struct UIWindowOps ui_rebirth_wnd_ops = {
    .destroy = ui_rebirth_wnd_destroy,
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
    .on_create = ui_rebirth_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_rebirth_wnd_on_draw,
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
    .send_msg = ui_rebirth_wnd_send_msg,
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
struct UIWaitWnd;
struct UIWindow;
void ui_wait_wnd_destroy(struct UIWindow *window);
void ui_wait_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_wait_wnd_on_draw(struct UIWindow *self);
intptr_t ui_wait_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4);
const struct UIWindowOps ui_wait_wnd_ops = {
    .destroy = ui_wait_wnd_destroy,
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
    .on_create = ui_wait_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_wait_wnd_on_draw,
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
    .send_msg = ui_wait_wnd_send_msg,
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
struct UILoadingWnd;
void ui_loading_wnd_destroy(struct UIWindow *window);
void ui_loading_wnd_on_create(struct UIWindow *self, int width_unused, int height_unused);
void ui_loading_wnd_on_draw(struct UIWindow *self);
intptr_t ui_loading_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4);
const struct UIWindowOps ui_loading_wnd_ops = {
    .destroy = ui_loading_wnd_destroy,
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
    .on_create = ui_loading_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_loading_wnd_on_draw,
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
    .send_msg = ui_loading_wnd_send_msg,
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
struct UIDisconnectedServerMsgWnd;
struct UIFrameWnd;
void ui_disconnected_server_msg_wnd_destroy(struct UIWindow *window);
void ui_disconnected_server_msg_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_disconnected_server_msg_wnd_on_draw(struct UIWindow *self);
intptr_t ui_disconnected_server_msg_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4);
const struct UIWindowOps ui_disconnected_server_msg_wnd_ops = {
    .destroy = ui_disconnected_server_msg_wnd_destroy,
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
    .on_create = ui_disconnected_server_msg_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_disconnected_server_msg_wnd_on_draw,
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
    .send_msg = ui_disconnected_server_msg_wnd_send_msg,
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

static struct UIBitmapButton *make_button(const char *normal, const char *hover, const char *pressed,
                                          int id, int x, int y)
{
    struct UIBitmapButton *button = ui_bitmap_button(normal, hover, pressed);

    ui_window_move_base(&button->button.base, x, y);
    button->button.base.id = id;
    return button;
}

static struct UIWindow *add_message_text(struct UIWindow *wnd, int width, int height, int text_id)
{
    struct UIWindow *text = mem_alloc(sizeof(struct UITextViewer));

    if (text)
        text = ui_text_viewer_construct(text);
    ui_list_box_create_window((struct UIListBox *)text, 0xa, 0x16, width - 0x14, height - 0x37, 0);
    UI_TEXT_VIEWER_BACKGROUND(text) = ui_skin_argb(2, 2);
    ui_window_invalidate(text);
    ui_window_add_child(wnd, text);
    ui_window_send_msg(text, NULL, UI_MSG_ADD_TEXT, (intptr_t)msg_str(text_id), 0x26, 0, 0);
    return text;
}

static void draw_msgbox(struct UIWindow *wnd)
{
    ui_window_draw_bitmap(wnd, 0, 0, ui_skin_bitmap(MSGBOX_BITMAP), 0);
}

struct EscButton {
    const char *bitmaps[3];
    int id;
    int x, y;
};

static void add_esc_buttons(struct UIWindow *wnd, const struct EscButton *row, int count)
{
    int i;

    for (i = 0; i < count; i++)
        ui_window_add_child(wnd, &make_button(row[i].bitmaps[0], row[i].bitmaps[1], row[i].bitmaps[2],
                                                  row[i].id, row[i].x, row[i].y)->button.base);
}

enum {
    QUIT_ID_CHAR_SELECT = 0xd4,
    QUIT_ID_EXIT = 0xd5,
    QUIT_ID_CANCEL = 0xb1,
};

struct UIQuitWnd {
    struct UIFrameWnd frame;
};

struct UIQuitWnd *ui_quit_wnd_construct(struct UIQuitWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_quit_wnd_ops;
    return self;
}

struct UIWindow *ui_quit_wnd_new(void)
{
    return (struct UIWindow *)ui_quit_wnd_construct(mem_alloc(sizeof(struct UIQuitWnd)));
}

void ui_quit_wnd_destroy(struct UIWindow *window)
{
    struct UIQuitWnd *self = (struct UIQuitWnd *)window;
    self->frame.base.ops = &ui_quit_wnd_ops;
    ui_frame_wnd_destruct(&self->frame);
    mem_free(self);
}

void ui_quit_wnd_on_create(struct UIWindow *window, int width_unused, int height_unused)
{
    struct UIQuitWnd *self = (struct UIQuitWnd *)window;
    static const struct EscButton rows[] = {
        { { UI_SKIN "esc_01a.bmp", UI_SKIN "esc_01b.bmp", UI_SKIN "esc_01c.bmp" }, QUIT_ID_CHAR_SELECT, 0x1d, 0x1e },
        { { UI_SKIN "esc_03a.bmp", UI_SKIN "esc_03b.bmp", UI_SKIN "esc_03c.bmp" }, QUIT_ID_EXIT, 0x1d, 0x3d },
        { { UI_SKIN "esc_02a.bmp", UI_SKIN "esc_02b.bmp", UI_SKIN "esc_02c.bmp" }, QUIT_ID_CANCEL, 0x1d, 0x60 },
    };

    add_esc_buttons(&self->frame.base, rows, 3);
    UI_FRAME_DEFAULT_ID(self) = QUIT_ID_EXIT;
}

void ui_quit_wnd_on_draw(struct UIWindow *window)
{
    struct UIQuitWnd *self = (struct UIQuitWnd *)window;
    draw_msgbox(&self->frame.base);
}

intptr_t ui_quit_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2,
                                 intptr_t val3, intptr_t val4)
{
    struct UIQuitWnd *self = (struct UIQuitWnd *)window;
    struct Mode *game = game_current_base_mode();

    if (msg != UI_MSG_COMMAND)
        return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
    switch (val1) {
    case QUIT_ID_CHAR_SELECT:
        mode_send_message(game, GAME_REQ_RESTART, RESTART_TO_CHAR_SELECT, 0, 0, 0);
        break;
    case QUIT_ID_EXIT:
        mode_send_message(game, GAME_REQ_DISCONNECT, 0, 0, 0, 0);
        break;
    case QUIT_ID_CANCEL:
        break;
    default:
        return 0;
    }
    ui_window_mgr_post_quit1(&g_windowMgr, WND_ID_QUIT);
    return 0;
}

enum {
    RESTART_ID_SAVE_POINT = 0xd4,
    RESTART_ID_CHAR_SELECT = 0xf4,
    RESTART_ID_EXIT = 0xf3,
    RESTART_ID_CANCEL = 0xb1,
};

struct UIRestartWnd {
    struct UIFrameWnd frame;
};

struct UIRestartWnd *ui_restart_wnd_construct(struct UIRestartWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_restart_wnd_ops;
    return self;
}

struct UIWindow *ui_restart_wnd_new(void)
{
    return (struct UIWindow *)ui_restart_wnd_construct(mem_alloc(sizeof(struct UIRestartWnd)));
}

void ui_restart_wnd_destroy(struct UIWindow *window)
{
    struct UIRestartWnd *self = (struct UIRestartWnd *)window;
    self->frame.base.ops = &ui_restart_wnd_ops;
    ui_frame_wnd_destruct(&self->frame);
    mem_free(self);
}

void ui_restart_wnd_on_create(struct UIWindow *window, int width_unused, int height_unused)
{
    struct UIRestartWnd *self = (struct UIRestartWnd *)window;
    static const struct EscButton rows[] = {
        { { UI_SKIN "esc_04a.bmp", UI_SKIN "esc_04b.bmp", UI_SKIN "esc_04c.bmp" }, RESTART_ID_SAVE_POINT, 0x1d, 0x16 },
        { { UI_SKIN "esc_01a.bmp", UI_SKIN "esc_01b.bmp", UI_SKIN "esc_01c.bmp" }, RESTART_ID_CHAR_SELECT, 0x1d, 0x2d },
        { { UI_SKIN "esc_03a.bmp", UI_SKIN "esc_03b.bmp", UI_SKIN "esc_03c.bmp" }, RESTART_ID_EXIT, 0x1d, 0x44 },
        { { UI_SKIN "esc_02a.bmp", UI_SKIN "esc_02b.bmp", UI_SKIN "esc_02c.bmp" }, RESTART_ID_CANCEL, 0x1d, 0x60 },
    };

    add_esc_buttons(&self->frame.base, rows, 4);
    UI_FRAME_DEFAULT_ID(self) = RESTART_ID_CHAR_SELECT;
}

void ui_restart_wnd_on_draw(struct UIWindow *window)
{
    struct UIRestartWnd *self = (struct UIRestartWnd *)window;
    draw_msgbox(&self->frame.base);
}

intptr_t ui_restart_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, intptr_t val1,
                                    intptr_t val2, intptr_t val3, intptr_t val4)
{
    struct UIRestartWnd *self = (struct UIRestartWnd *)window;
    struct Mode *game = game_current_base_mode();

    if (msg != UI_MSG_COMMAND)
        return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
    switch (val1) {
    case RESTART_ID_CANCEL:
        ui_window_mgr_post_quit1(&g_windowMgr, WND_ID_RESTART);
        break;
    case RESTART_ID_SAVE_POINT:
        mode_send_message(game, GAME_REQ_RESTART, RESTART_TO_SAVE_POINT, 0, 0, 0);
        ui_window_mgr_post_quit1(&g_windowMgr, WND_ID_RESTART);
        break;
    case RESTART_ID_EXIT:
        ui_window_mgr_post_quit1(&g_windowMgr, WND_ID_RESTART);
        mode_send_message(game, GAME_REQ_DISCONNECT, 0, 0, 0, 0);
        break;
    case RESTART_ID_CHAR_SELECT:
        mode_send_message(game, GAME_REQ_RESTART, RESTART_TO_CHAR_SELECT, 0, 0, 0);
        ui_window_mgr_post_quit1(&g_windowMgr, WND_ID_RESTART);
        break;
    }
    return 0;
}

enum {
    REBIRTH_ID_RISE_HERE = 0x159,
    REBIRTH_ID_SAVE_POINT = 0xd4,
    REBIRTH_ID_CHAR_SELECT = 0xf4,
    REBIRTH_ID_EXIT = 0xf3,
    REBIRTH_ID_CANCEL = 0xb1,
};

struct UIRebirthWnd {
    struct UIFrameWnd frame;
};

struct UIRebirthWnd *ui_rebirth_wnd_construct(struct UIRebirthWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_rebirth_wnd_ops;
    return self;
}

struct UIWindow *ui_rebirth_wnd_new(void)
{
    return (struct UIWindow *)ui_rebirth_wnd_construct(mem_alloc(sizeof(struct UIRebirthWnd)));
}

void ui_rebirth_wnd_destroy(struct UIWindow *window)
{
    struct UIRebirthWnd *self = (struct UIRebirthWnd *)window;
    self->frame.base.ops = &ui_rebirth_wnd_ops;
    ui_frame_wnd_destruct(&self->frame);
    mem_free(self);
}

void ui_rebirth_wnd_on_create(struct UIWindow *window, int width_unused, int height_unused)
{
    struct UIRebirthWnd *self = (struct UIRebirthWnd *)window;
    static const struct EscButton rows[] = {
        { { UI_SKIN "esc_05a.bmp", UI_SKIN "esc_05b.bmp", UI_SKIN "esc_05c.bmp" }, REBIRTH_ID_RISE_HERE, 0x1d, 0x16 },
        { { UI_SKIN "esc_04a.bmp", UI_SKIN "esc_04b.bmp", UI_SKIN "esc_04c.bmp" }, REBIRTH_ID_SAVE_POINT, 0x1d, 0x2d },
        { { UI_SKIN "esc_01a.bmp", UI_SKIN "esc_01b.bmp", UI_SKIN "esc_01c.bmp" }, REBIRTH_ID_CHAR_SELECT, 0x1d, 0x44 },
        { { UI_SKIN "esc_03a.bmp", UI_SKIN "esc_03b.bmp", UI_SKIN "esc_03c.bmp" }, REBIRTH_ID_EXIT, 0x1d, 0x5b },
        { { UI_SKIN "esc_02a.bmp", UI_SKIN "esc_02b.bmp", UI_SKIN "esc_02c.bmp" }, REBIRTH_ID_CANCEL, 0x1d, 0x77 },
    };

    add_esc_buttons(&self->frame.base, rows, 5);
    UI_FRAME_DEFAULT_ID(self) = REBIRTH_ID_CHAR_SELECT;
}

void ui_rebirth_wnd_on_draw(struct UIWindow *window)
{
    struct UIRebirthWnd *self = (struct UIRebirthWnd *)window;
    ui_window_draw_bitmap(&self->frame.base, 0, 0, ui_skin_bitmap(UI_SKIN "win_msgbox1.bmp"), 0);
}

intptr_t ui_rebirth_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, intptr_t val1,
                                    intptr_t val2, intptr_t val3, intptr_t val4)
{
    struct UIRebirthWnd *self = (struct UIRebirthWnd *)window;
    struct Mode *game = game_current_base_mode();

    if (msg != UI_MSG_COMMAND)
        return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
    switch (val1) {
    case REBIRTH_ID_CANCEL:
        ui_window_mgr_post_quit1(&g_windowMgr, WND_ID_REBIRTH);
        break;
    case REBIRTH_ID_SAVE_POINT:
        mode_send_message(game, GAME_REQ_RESTART, RESTART_TO_SAVE_POINT, 0, 0, 0);
        ui_window_mgr_post_quit1(&g_windowMgr, WND_ID_REBIRTH);
        break;
    case REBIRTH_ID_EXIT:
        ui_window_mgr_post_quit1(&g_windowMgr, WND_ID_REBIRTH);
        mode_send_message(game, GAME_REQ_DISCONNECT, 0, 0, 0, 0);
        break;
    case REBIRTH_ID_CHAR_SELECT:
        mode_send_message(game, GAME_REQ_RESTART, RESTART_TO_CHAR_SELECT, 0, 0, 0);
        ui_window_mgr_post_quit1(&g_windowMgr, WND_ID_REBIRTH);
        break;
    case REBIRTH_ID_RISE_HERE:
        mode_send_message(game, GAME_REQ_RISE_HERE, 0, 0, 0, 0);
        ui_window_mgr_post_quit1(&g_windowMgr, WND_ID_REBIRTH);
        break;
    }
    return 0;
}

#define WAIT_ID_CANCEL 0xb1
#define MSG_WAIT_TEXT 0x79
#define WAIT_MSG_BUTTON_TO_BOTTOM 0x58
#define MODE_REQ_CANCEL_WAIT 0x2725

struct UIWaitWnd {
    struct UIFrameWnd frame;
    struct UIWindow *text;
    struct UIBitmapButton *cancel;
};

struct UIWaitWnd *ui_wait_wnd_construct(struct UIWaitWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_wait_wnd_ops;
    self->cancel = NULL;
    return self;
}

struct UIWindow *ui_wait_wnd_new(void)
{
    return (struct UIWindow *)ui_wait_wnd_construct(mem_alloc(sizeof(struct UIWaitWnd)));
}

void ui_wait_wnd_destroy(struct UIWindow *window)
{
    struct UIWaitWnd *self = (struct UIWaitWnd *)window;
    self->frame.base.ops = &ui_wait_wnd_ops;
    ui_frame_wnd_destruct(&self->frame);
    mem_free(self);
}

void ui_wait_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UIWaitWnd *self = (struct UIWaitWnd *)window;
    struct UIBitmapButton *cancel = ui_bitmap_button(UI_SKIN "btn_cancel.bmp", UI_SKIN "btn_cancel_a.bmp",
                                                     UI_SKIN "btn_cancel_b.bmp");

    self->cancel = cancel;
    ui_window_move_base(&cancel->button.base, (width - cancel->bitmap_width) / 2, 0x32);
    cancel->button.base.id = WAIT_ID_CANCEL;
    ui_bitmap_button_set_tool_tip(cancel, "");
    ui_window_add_child(&self->frame.base, &cancel->button.base);

    self->text = add_message_text(&self->frame.base, width, height, MSG_WAIT_TEXT);
}

void ui_wait_wnd_on_draw(struct UIWindow *window)
{
    struct UIWaitWnd *self = (struct UIWaitWnd *)window;
    draw_msgbox(&self->frame.base);
}

intptr_t ui_wait_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2,
                                 intptr_t val3, intptr_t val4)
{
    struct UIWaitWnd *self = (struct UIWaitWnd *)window;
    struct UIWindow *wnd = &self->frame.base;

    if (msg == UI_MSG_COMMAND) {
        if (val1 == WAIT_ID_CANCEL)
            mode_send_message(g_modeMgr.curMode, MODE_REQ_CANCEL_WAIT, 0, 0, 0, 0);
    } else if (msg == WAIT_MSG_BUTTON_TO_BOTTOM) {
        ui_window_move_base(&self->cancel->button.base, (wnd->width - self->cancel->bitmap_width) / 2,
                 wnd->height - self->cancel->bitmap_height - 5);
    }
    return 0;
}

#define LOADING_MSG_PROGRESS 0x11

struct UILoadingWnd {
    struct UIFrameWnd frame;
    struct UIWindow *bar;
};

struct UILoadingWnd *ui_loading_wnd_construct(struct UILoadingWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_loading_wnd_ops;
    self->bar = NULL;
    return self;
}

struct UIWindow *ui_loading_wnd_new(void)
{
    return (struct UIWindow *)ui_loading_wnd_construct(mem_alloc(sizeof(struct UILoadingWnd)));
}

void ui_loading_wnd_destroy(struct UIWindow *window)
{
    struct UILoadingWnd *self = (struct UILoadingWnd *)window;
    self->frame.base.ops = &ui_loading_wnd_ops;
    ui_frame_wnd_destruct(&self->frame);
    mem_free(self);
}

void ui_loading_wnd_on_create(struct UIWindow *window, int width_unused, int height_unused)
{
    struct UILoadingWnd *self = (struct UILoadingWnd *)window;
    struct UIWindow *bar = mem_alloc(sizeof(struct UIBarGraph2));
    const struct String *names = g_loadingScreens.first;
    int count = (int)VEC_COUNT(&g_loadingScreens, struct String);
    struct UIBarGraph2 *graph;
    char path[MAX_PATH];
    int r, g, b;

    if (bar)
        bar = ui_bar_graph2_construct(bar);
    self->bar = bar;
    ui_window_create(bar, 0xf0, 0xf);
    ui_window_move(bar, 0, 0);
    ui_bar_graph_set_pos(bar, 0.0f);
    ui_skin_color_chip(&g_skinMgr, 0x16, 2, &r, &g, &b);
    graph = (struct UIBarGraph2 *)self->bar;
    graph->bar_red = r;
    graph->bar_green = g;
    graph->bar_blue = b;
    ui_window_add_child(&self->frame.base, self->bar);

    if (g_session1.loadingAirplane == 1) {
        strcpy(path, UI_SKIN "loading_airplane.jpg");
    } else if (!g_session1.loadingScreenRandom) {
        if (g_loadingScreenNext < count)
            strcpy(path, string_cstr(&names[g_loadingScreenNext++]));
        else {
            g_loadingScreenNext = 0;
            strcpy(path, UI_SKIN "loading00.jpg");
        }
    } else if (count != 0) {
        strcpy(path, string_cstr(&names[rand() % count]));
    } else {
        strcpy(path, UI_SKIN "loading00.jpg");
    }
    ui_skin_bitmap(path);
    ui_window_mgr_set_backdrop(&g_windowMgr, ui_skin_bitmap(path));
}

void ui_loading_wnd_on_draw(struct UIWindow *self)
{
}

intptr_t ui_loading_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, intptr_t val1,
                                    intptr_t val2, intptr_t val3, intptr_t val4)
{
    struct UILoadingWnd *self = (struct UILoadingWnd *)window;

    if (msg == LOADING_MSG_PROGRESS)
        ui_bar_graph_set_pos(self->bar, (float)val1);
    return 0;
}

#define DISCONNECTED_ID_OK 0xb0
#define MSG_DISCONNECTED_TEXT 2

struct UIDisconnectedServerMsgWnd {
    struct UIFrameWnd frame;
    struct UIWindow *text;
};

struct UIDisconnectedServerMsgWnd *ui_disconnected_server_msg_wnd_construct(
    struct UIDisconnectedServerMsgWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_disconnected_server_msg_wnd_ops;
    self->text = NULL;
    return self;
}

struct UIWindow *ui_disconnected_server_msg_wnd_new(void)
{
    return (struct UIWindow *)ui_disconnected_server_msg_wnd_construct(mem_alloc(sizeof(struct UIDisconnectedServerMsgWnd)));
}

void ui_disconnected_server_msg_wnd_destroy(struct UIWindow *window)
{
    struct UIDisconnectedServerMsgWnd *self = (struct UIDisconnectedServerMsgWnd *)window;
    self->frame.base.ops = &ui_disconnected_server_msg_wnd_ops;
    ui_frame_wnd_destruct(&self->frame);
    mem_free(self);
}

void ui_disconnected_server_msg_wnd_on_create(struct UIWindow *window, int width,
                                                    int height)
{
    struct UIDisconnectedServerMsgWnd *self = (struct UIDisconnectedServerMsgWnd *)window;
    struct UIBitmapButton *ok = make_button(UI_SKIN "btn_ok.bmp", UI_SKIN "btn_ok_a.bmp", UI_SKIN "btn_ok_b.bmp",
                                            DISCONNECTED_ID_OK, 0xea, 0x60);

    ui_window_add_child(&self->frame.base, &ok->button.base);
    UI_FRAME_DEFAULT_ID(self) = DISCONNECTED_ID_OK;
    self->text = add_message_text(&self->frame.base, width, height, MSG_DISCONNECTED_TEXT);
}

void ui_disconnected_server_msg_wnd_on_draw(struct UIWindow *window)
{
    struct UIDisconnectedServerMsgWnd *self = (struct UIDisconnectedServerMsgWnd *)window;
    draw_msgbox(&self->frame.base);
}

#define UI_MSG_BUTTON 6
intptr_t ui_disconnected_server_msg_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender,
                                                  int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4)
{
    struct UIFrameWnd *self = (struct UIFrameWnd *)window;
    if (msg != UI_MSG_BUTTON)
        return ui_frame_wnd_send_msg((struct UIWindow *)(self), sender, msg, val1, val2, val3, val4);
    if (val1 == 0xb0) {
        game_send_message(0x8d, 0, 0, 0, 0);
    }
    return 0;
}

void ui_wait_wnd_set_text(struct UIWindow *wnd, const char *text)
{
    struct UIWaitWnd *self = (struct UIWaitWnd *)wnd;

    ui_list_box_clear((struct UIListBox *)self->text);
    ui_window_send_msg(self->text, NULL, 0x1d, (intptr_t)text, 0x26, 0, 0);
}
