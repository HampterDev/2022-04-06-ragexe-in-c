#include "contents/private_airplane.h"
#include "render/rp_pool.h"
#include "navi/navi_manager.h"
#include "ui/window_manager_state.h"
#include "core/object_type_tables.h"
#include "ui/quests/quests.h"
#include "core/msg_str.h"
#include "core/text_util.h"
#include "diag/perf.h"
#include "ui/quests/navigation.h"

#define AIRPLANE_BUTTON_ID 0x1ec
#define AIRPLANE_BUTTON_X 8
#define AIRPLANE_BUTTON_PITCH 0x1a
#define AIRPLANE_BUTTON_WIDTH 0xc0
#define AIRPLANE_DIR UI_SKIN "private_airplane\\"
#define AIRPLANE_MSG_DISABLE_ALL 0x23
#define AIRPLANE_MSG_NOTICE 0x9a

#define MSG_AIRPLANE_TICKET 0xd01
#define MSG_AIRPLANE_TITLE 0xd02

#define AIRPLANE_NOTICE_WIDTH 0xa4
#define AIRPLANE_LINE_HEIGHT 12
#define AIRPLANE_NOTICE_COLOUR 0x3512b5

#define MODAL_RESULT_AIRPLANE_CLOSED 0xb4

struct UIPrivateAirplaneWnd {
    struct UIFrameWnd frame;
    struct String notice;
    struct Vec buttons;
};
void ui_private_airplane_wnd_destroy(struct UIWindow *window);
void ui_private_airplane_wnd_on_create(struct UIWindow *self, int width_unused, int height_unused);
void ui_private_airplane_wnd_on_process(struct UIWindow *self);
void ui_private_airplane_wnd_on_draw(struct UIWindow *self);
int ui_private_airplane_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_private_airplane_wnd_ops = {
    .destroy = ui_private_airplane_wnd_destroy,
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
    .on_create = ui_private_airplane_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_private_airplane_wnd_on_process,
    .on_draw = ui_private_airplane_wnd_on_draw,
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
    .send_msg = ui_private_airplane_wnd_send_msg,
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

static struct UIPrivateAirplaneWnd *ui_private_airplane_wnd_construct(struct UIPrivateAirplaneWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_private_airplane_wnd_ops;
    string_init(&self->notice);
    self->buttons.first = self->buttons.last = self->buttons.end = NULL;
    return self;
}

struct UIPrivateAirplaneWnd *ui_private_airplane_wnd_new(void)
{
    struct UIPrivateAirplaneWnd *self = mem_alloc(sizeof *self);

    return self ? ui_private_airplane_wnd_construct(self) : NULL;
}

static void ui_private_airplane_wnd_destruct(struct UIPrivateAirplaneWnd *self)
{
    self->frame.base.ops = &ui_private_airplane_wnd_ops;
    vec_free(&self->buttons);
    string_free(&self->notice);
    ui_frame_wnd_destruct(&self->frame);
}

void ui_private_airplane_wnd_destroy(struct UIWindow *window)
{
    struct UIPrivateAirplaneWnd *self = (struct UIPrivateAirplaneWnd *)window;
    ui_private_airplane_wnd_destruct(self);
    mem_free(self);
}

void ui_private_airplane_wnd_on_create(struct UIWindow *window, int width_unused, int height_unused)
{
    struct UIPrivateAirplaneWnd *self = (struct UIPrivateAirplaneWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    int i, y;

    ui_window_add_sys_button(wnd);
    ui_window_add_close_button(wnd, "", 0);
    for (i = 0, y = AIRPLANE_BUTTON_PITCH; i < g_airshipTickets->need_items.size; i++, y += AIRPLANE_BUTTON_PITCH) {
        int item_id = private_airplane_need_item(g_airshipTickets, i);
        struct ItemInfo info;
        const char *name;
        struct UIBasicButton *button;
        struct UIWindow *added;
        struct String label;

        item_info_construct(&info);
        item_info_set_item_id(&info, item_id);

        name = item_info_get_name(&info);
        if (strlen(name)) {
            button = mem_alloc(sizeof *button);
            if (button)
                button = ui_basic_button_construct(button);
            string_init(&label);
            string_format(&label, msg_str(MSG_AIRPLANE_TICKET), name);
            ui_basic_button_set_label(&button->button.base, AIRPLANE_BUTTON_WIDTH, string_cstr(&label));
            string_free(&label);
            ui_window_move_base(&button->button.base, AIRPLANE_BUTTON_X, y);
            button->button.base.id = AIRPLANE_BUTTON_ID;
            button->button.cmd_value = item_id;
            ui_window_add_child(wnd, &button->button.base);
            added = &button->button.base;
            vec_push(&self->buttons, &added, sizeof added);
        }
        item_info_destruct(&info);
    }
}

static void destroy_string(void *value) { string_free(value); }

void ui_private_airplane_wnd_on_draw(struct UIWindow *window)
{
    struct UIPrivateAirplaneWnd *self = (struct UIPrivateAirplaneWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct List lines;
    struct List widths;
    struct String text;
    struct ListNode *node;
    int offset;

    ui_window_draw_bitmap(wnd, 0, 0, res_mgr_get(g_resMgr(), AIRPLANE_DIR "bg_pay.bmp"), 0);
    ui_window_text_out(wnd, 0x13, 4, msg_str(MSG_AIRPLANE_TITLE), 0, 0, 0xc, RGB(0xff, 0xff, 0xff), 0, 0);
    ui_window_text_out(wnd, 0x12, 3, msg_str(MSG_AIRPLANE_TITLE), 0, 0, 0xc, RGB(0, 0, 0), 0, 0);
    if (!self->notice.size)
        return;
    ui_window_draw_bitmap(wnd, 7, 0x56, res_mgr_get(g_resMgr(), AIRPLANE_DIR "ico_notic.bmp"), 1);
    list_init(&lines);
    string_copy(&text, &self->notice);
    ui_window_wrap_text_plain(wnd, &widths, text, &lines, AIRPLANE_NOTICE_WIDTH, 0, AIRPLANE_LINE_HEIGHT, 0);
    list_free(&widths, NULL);
    for (node = lines.head->next, offset = 0; node != lines.head; node = node->next, offset += AIRPLANE_LINE_HEIGHT) {
        int y = (int)((double)(unsigned)(32 - lines.size * AIRPLANE_LINE_HEIGHT) * 0.5 + 78.0 + (double)offset);

        ui_window_text_out(wnd, 0x1a, y, string_cstr(LIST_VALUE(node, struct String)), 0, 0,
                         AIRPLANE_LINE_HEIGHT, AIRPLANE_NOTICE_COLOUR, 0, 0);
    }
    list_free(&lines, destroy_string);
}

void ui_private_airplane_wnd_on_process(struct UIWindow *self)
{
}

int ui_private_airplane_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                            int val1, int val2, int val3, int val4)
{
    struct UIPrivateAirplaneWnd *self = (struct UIPrivateAirplaneWnd *)window;
    if (msg == UI_MSG_COMMAND) {
        if (val1 == UI_ID_CLOSE) {
            if (g_windowMgr.modal_wnd)
                g_windowMgr.modal_result = MODAL_RESULT_AIRPLANE_CLOSED;
        } else if (val1 == AIRPLANE_BUTTON_ID) {
            struct String name, map, arg;

            navi_manager_guided_map_name(&g_navi, &name);
            string_append_cstr(&name, ".gat");
            string_move(&map, &name);
            string_copy(&arg, &map);
            navi_manager_request_airship(&g_navi, arg, val2);
            string_free(&map);
        }
    } else if (msg == AIRPLANE_MSG_DISABLE_ALL) {
        struct UIButton **it;

        for (it = self->buttons.first; it != self->buttons.last; it++) {
            (*it)->disabled = 1;
            ui_window_invalidate_base(&(*it)->base);
        }
        ui_window_invalidate_base(&self->frame.base);
    } else if (msg == AIRPLANE_MSG_NOTICE) {
        const char *text = (const char *)val1;

        string_assign_cstr(&self->notice, text);
        ui_window_invalidate_base(&self->frame.base);
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
}
