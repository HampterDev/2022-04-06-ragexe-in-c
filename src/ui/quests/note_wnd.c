#include "ui/controls/scroll.h"
#include "render/rp_pool.h"
#include "core/object_type_tables.h"
#include "ui/quests/quests.h"
#include "core/msg_str.h"
#include "diag/perf.h"

#define MSG_NOTE_CLOSE 0xbe5
#define NOTE_MSG_SHOW_LINE 0x22

#define NOTE_SCROLL_WHEEL_STEP 12

struct UINoteWnd;
struct UIWindow;
void ui_note_wnd_destroy(struct UIWindow *window);
void ui_note_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_note_wnd_on_draw(struct UIWindow *self);
int ui_note_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_note_wnd_ops = {
    .destroy = ui_note_wnd_destroy,
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
    .on_create = ui_note_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_note_wnd_on_draw,
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
    .send_msg = ui_note_wnd_send_msg,
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
struct UINoteWnd_Scroll;
struct UIScrollWnd;
struct UIWindow;
void ui_note_wnd_scroll_destroy(struct UIWindow *window);
void ui_note_wnd_scroll_on_draw(struct UIWindow *self);
void ui_note_wnd_scroll_on_l_btn_down(struct UIWindow *self, int x, int y);
void ui_note_wnd_scroll_on_l_btn_up(struct UIWindow *self, int x, int y);
void ui_note_wnd_scroll_on_wheel(struct UIWindow *self, int delta);
const struct UIWindowOps ui_note_wnd_scroll_ops = {
    .destroy = ui_note_wnd_scroll_destroy,
    .resize = ui_scroll_wnd_resize,
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
    .on_create = ui_scroll_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_note_wnd_scroll_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_note_wnd_scroll_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_window_on_mouse_move_base,
    .on_mouse_hover = ui_window_on_mouse_hover_base,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_note_wnd_scroll_on_l_btn_up,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_note_wnd_scroll_on_wheel,
    .snap = ui_window_snap_base,
    .send_msg = ui_scroll_wnd_send_msg,
    .invalidate = ui_window_invalidate_base,
    .render = ui_scroll_wnd_render,
    .has_custom_backdrop = 0,
    .draw_backdrop = ui_window_draw_backdrop_base,
    .get_records = ui_window_get_records_base,
    .type = UI_TYPE_WINDOW,
    .on_hover_enter = ui_window_on_hover_enter_base,
    .on_hover_leave = ui_window_on_hover_leave_base,
    .make_quad = ui_scroll_wnd_make_quad,
    .hit_test = ui_window_hit_test_base,
    .render_overlay = ui_window_render_overlay_base,
};

struct UINoteWnd_Scroll {
    struct UIScrollWnd scroll;
    struct UIStaticTextLF *text;
};
OFFSET_ASSERT(UINoteWnd_Scroll, text, 0xb0);
SIZE_ASSERT(UINoteWnd_Scroll, 0xb4);

struct UINoteWnd {
    struct UIFrameWnd frame;
    struct UINoteWnd_Scroll *pane;
};
SIZE_ASSERT(UINoteWnd, 0x98);

static struct UINoteWnd_Scroll *ui_note_wnd_scroll_construct(struct UINoteWnd_Scroll *self, int width,
                                                                      int height)
{
    ui_scroll_wnd_construct(&self->scroll, width, height, 0);
    self->scroll.base.ops = &ui_note_wnd_scroll_ops;
    self->text = NULL;
    ui_window_create(&self->scroll.base, width, height);
    return self;
}

static void ui_note_wnd_scroll_destruct(struct UINoteWnd_Scroll *self)
{
    self->scroll.base.ops = &ui_note_wnd_scroll_ops;
    ui_scroll_wnd_destruct(&self->scroll);
}

void ui_note_wnd_scroll_destroy(struct UIWindow *window)
{
    struct UINoteWnd_Scroll *self = (struct UINoteWnd_Scroll *)window;
    ui_note_wnd_scroll_destruct(self);
    mem_free(self);
}

void ui_note_wnd_on_draw(struct UIWindow *window)
{
    struct UINoteWnd *self = (struct UINoteWnd *)window;
    struct UIWindow *wnd = &self->frame.base;

    ui_window_clear_canvas(wnd, 0xffff00ff);
    ui_window_fill_round_rect(wnd, 0, 0, wnd->width, wnd->height, 0xffffff);
    ui_window_draw_round_frame(wnd, 2, 2, wnd->width - 4, wnd->height - 4, 0xc8c8c8);
    ui_window_fill_rect(wnd, 8, 8, wnd->width - 0x10, wnd->height - 0x26, 0xf7f7f7);
}

void ui_note_wnd_scroll_on_draw(struct UIWindow *window)
{
    struct UINoteWnd_Scroll *self = (struct UINoteWnd_Scroll *)window;
    ui_window_clear_canvas(&self->scroll.base, RGB(0xf7, 0xf7, 0xf7));
}

void ui_note_wnd_scroll_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UINoteWnd_Scroll *self = (struct UINoteWnd_Scroll *)window;
    struct UIWindow *note = self->scroll.base.parent;

    if (note)
        ui_window_on_l_btn_down(note, x + 0x17, y + 0x1b);
}

void ui_note_wnd_scroll_on_l_btn_up(struct UIWindow *window, int x, int y)
{
    struct UINoteWnd_Scroll *self = (struct UINoteWnd_Scroll *)window;
    struct UIWindow *note = self->scroll.base.parent;

    if (note)
        ui_window_on_l_btn_up(note, x + 0x17, y + 0x1b);
}

void ui_note_wnd_scroll_on_wheel(struct UIWindow *window, int delta)
{
    struct UINoteWnd_Scroll *self = (struct UINoteWnd_Scroll *)window;
    struct UIScrollWnd *scroll = &self->scroll;

    if (scroll->vscroll_on != 1)
        return;
    ui_scroll_wnd_send_msg(&scroll->base, &scroll->vbar->base, UI_MSG_SCROLL_V,
             delta < 0 ? NOTE_SCROLL_WHEEL_STEP : -NOTE_SCROLL_WHEEL_STEP, 0, 0, 0);
}

static void ui_note_wnd_scroll_set_text(struct UINoteWnd_Scroll *self, struct String text)
{
    struct UIStaticTextLF *label = mem_alloc(sizeof(struct UIStaticTextLF));
    struct String copy;

    if (label)
        label = ui_static_text_lf_construct(label);
    self->text = label;
    ui_window_create(&label->base, self->scroll.base.width - 0xd, 12);
    ui_window_move_base(&self->text->base, 0, 0);
    string_copy(&copy, &text);
    ui_static_text_lf_set_text(self->text, copy);
    ui_scroll_wnd_add_child(&self->scroll, &self->text->base);
    ui_window_set_show_base(&self->scroll.hbar->base, 0);
    ui_window_set_show(&self->scroll.vbar->base, 0);
    ui_window_invalidate_base(&self->scroll.base);
    string_free(&text);
}

struct UINoteWnd *ui_note_wnd_construct(struct UINoteWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_note_wnd_ops;
    self->pane = NULL;
    return self;
}

struct UIWindow *ui_note_wnd_new(void)
{
    return (struct UIWindow *)ui_note_wnd_construct(mem_alloc(sizeof(struct UINoteWnd)));
}

static void ui_note_wnd_destruct(struct UINoteWnd *self)
{
    self->frame.base.ops = &ui_note_wnd_ops;
    ui_frame_wnd_destruct(&self->frame);
}

void ui_note_wnd_destroy(struct UIWindow *window)
{
    struct UINoteWnd *self = (struct UINoteWnd *)window;
    ui_note_wnd_destruct(self);
    mem_free(self);
}

void ui_note_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UINoteWnd *self = (struct UINoteWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct UIWindow *close = mem_alloc(sizeof(struct UIBasicButton));
    struct UINoteWnd_Scroll *pane;

    if (close)
        close = (struct UIWindow *)ui_basic_button_construct((struct UIBasicButton *)close);
    ui_basic_button_set_label(close, 0x3c, msg_str(MSG_NOTE_CLOSE));
    ui_window_move_base(close, width - 0x44, height - 0x1a);
    close->id = UI_ID_CLOSE;
    ui_window_add_child(wnd, close);

    pane = mem_alloc(sizeof *pane);
    if (pane)
        pane = ui_note_wnd_scroll_construct(pane, width - 0x23, height - 0x3b);
    self->pane = pane;
    ui_window_move_base(&pane->scroll.base, 0x10, 0x10);
    ui_window_add_child(wnd, &self->pane->scroll.base);
}

int ui_note_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1, int val2,
                                 int val3, int val4)
{
    struct UINoteWnd *self = (struct UINoteWnd *)window;
    if (msg == NOTE_MSG_SHOW_LINE) {
        const char *line = msg_str(val1);
        struct String text;

        string_init(&text);
        string_assign_cstr(&text, line);
        ui_note_wnd_scroll_set_text(self->pane, text);
        ui_window_invalidate_base(&self->frame.base);
    } else if (msg == UI_MSG_COMMAND && val1 == UI_ID_CLOSE) {
        ui_window_mgr_post_quit1(&g_windowMgr, self->frame.base.id);
    }
    return ui_window_send_msg_base(&self->frame.base, sender, msg, val1, val2, val3, val4);
}
