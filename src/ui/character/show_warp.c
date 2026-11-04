#include "ui/controls/listbox.h"
#include "render/rp_pool.h"
#include "session/map_names.h"
#include "core/object_type_tables.h"
#include "ui/character/character.h"
#include "core/msg_str.h"
#include "diag/perf.h"
#include "session/session_names.h"
#include "ui/ui_std.h"

struct UIShowWarpWnd;
struct UIWindow;
void ui_show_warp_wnd_destroy(struct UIWindow *window);
void ui_show_warp_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_show_warp_wnd_on_draw(struct UIWindow *self);
intptr_t ui_show_warp_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4);
const struct UIWindowOps ui_show_warp_wnd_ops = {
    .destroy = ui_show_warp_wnd_destroy,
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
    .on_create = ui_show_warp_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_show_warp_wnd_on_draw,
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
    .send_msg = ui_show_warp_wnd_send_msg,
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

#define SHOW_WARP_WND_ID 0x134
#define SHOW_WARP_MSG_ADD 0x22
#define LIST_MSG_SELECT_PREV 0x12
#define LIST_MSG_SELECT_NEXT 0x13
#define MSG_WARP_RANDOM 0xd4
#define MSG_WARP_CANCEL 0xda
#define SHOW_WARP_LIST_BACK 0xf0

struct UIShowWarpWnd {
    struct UIFrameWnd frame;
    struct UIListBox *list;
};

struct UIShowWarpWnd *ui_show_warp_wnd_construct(struct UIShowWarpWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_show_warp_wnd_ops;
    return self;
}

struct UIWindow *ui_show_warp_wnd_new(void)
{
    return (struct UIWindow *)ui_show_warp_wnd_construct(mem_alloc(sizeof(struct UIShowWarpWnd)));
}

void ui_show_warp_wnd_destroy(struct UIWindow *window)
{
    struct UIShowWarpWnd *self = (struct UIShowWarpWnd *)window;
    self->frame.base.ops = &ui_show_warp_wnd_ops;
    ui_frame_wnd_destruct(&self->frame);
    mem_free(self);
}

void ui_show_warp_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UIShowWarpWnd *self = (struct UIShowWarpWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct UIListBox *list = mem_alloc(sizeof(struct UIListBox));
    struct UIBitmapButton *button;
    struct String skin, name, path;
    static const char *const states[3] = { "btn_close", "btn_close_a", "btn_close_b" };
    int list_w = width - 0x18, list_h = height - 0x32, x, y, state;

    if (list)
        list = ui_list_box_construct(list);
    self->list = list;
    ui_list_box_create_window(list, 0xc, 0x14, list_w < 0x14 ? 0x14 : list_w, list_h < 0x1e ? 0x1e : list_h, 0);
    self->list->r = SHOW_WARP_LIST_BACK;
    self->list->g = SHOW_WARP_LIST_BACK;
    self->list->b = SHOW_WARP_LIST_BACK;
    self->list->selected = -1;
    ui_window_invalidate_base(&self->list->base);
    self->list->selectable = 0;
    ui_window_add_child(wnd, &self->list->base);

    string_init(&skin);
    string_assign(&skin, UI_SKIN, sizeof UI_SKIN - 1);
    x = wnd->width - 0x2e;
    y = wnd->height - 0x18;
    button = mem_alloc(sizeof(struct UIBitmapButton));
    if (button)
        button = ui_bitmap_button_construct(button);
    for (state = 0; state < 3; state++) {
        string_copy(&name, &skin);
        string_append_cstr(&name, states[state]);
        string_append_cstr(&name, ".bmp");
        path = name;
        string_init(&name);
        ui_bitmap_button_set_bitmap(button, string_cstr(&path), state, 0);
        string_free(&path);
        string_free(&name);
    }
    ui_window_create(&button->button.base, button->bitmap_width, button->bitmap_height);
    ui_window_move_base(&button->button.base, x, y);
    button->button.base.id = UI_ID_CLOSE;
    ui_window_add_child(wnd, &button->button.base);
    UI_FRAME_DEFAULT_ID(self) = UI_ID_OK;
    string_free(&skin);
}

void ui_show_warp_wnd_on_draw(struct UIWindow *window)
{
    struct UIShowWarpWnd *self = (struct UIShowWarpWnd *)window;
    ui_window_draw_sys_box(&self->frame.base, 0, 0, 0, 0);
}

static int text_is(const char *text, const char *word)
{
    struct String s;
    BYTE equal;

    string_init(&s);
    string_assign_cstr(&s, text);
    equal = string_equal_cstr(&s, word);
    string_free(&s);
    return equal;
}

intptr_t ui_show_warp_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, intptr_t val1,
                                     intptr_t val2, intptr_t val3, intptr_t val4)
{
    struct UIShowWarpWnd *self = (struct UIShowWarpWnd *)window;
    const char *line;

    switch (msg) {
    case UI_MSG_COMMAND:

        if (val1 == UI_ID_CLOSE)
            ui_window_mgr_post_quit1(&g_windowMgr, SHOW_WARP_WND_ID);
        return 0;
    case LIST_MSG_SELECT_PREV:
    case LIST_MSG_SELECT_NEXT:
        ui_window_send_msg(&self->list->base, NULL, msg, 0, 0, 0, 0);
        return 0;
    case SHOW_WARP_MSG_ADD:
        if (text_is((const char *)val1, "Random"))
            line = msg_str(MSG_WARP_RANDOM);
        else if (text_is((const char *)val1, "cancel"))
            line = msg_str(MSG_WARP_CANCEL);
        else
            line = session_map_display_name(session_get_rsw_name((const char *)val1));
        ui_list_box_append_text(self->list, line);
        return 0;
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
}
