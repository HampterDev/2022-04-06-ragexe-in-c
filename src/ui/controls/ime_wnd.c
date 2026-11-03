#include "ui/controls/edit.h"
#include "render/rp_pool.h"
#include "core/object_type_tables.h"
#include "ui/controls/controls.h"
#include "platform/clientinfo.h"
#include "diag/perf.h"

struct UICandidateWnd;
struct UIWindow;
const struct UIWindowOps ui_candidate_wnd_ops = {
    .destroy = ui_candidate_wnd_destroy,
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
    .on_create = ui_candidate_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_candidate_wnd_on_draw,
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
    .send_msg = ui_candidate_wnd_send_msg,
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

#define IME_FONT_HEIGHT 12

#define CANDIDATE_MAX 9

struct UICandidateWnd {
    struct UIFrameWnd frame;
    struct String text[CANDIDATE_MAX];
    int selected;
    int page, last_page;
    int count;
};

struct UICandidateWnd *ui_candidate_wnd_construct(struct UICandidateWnd *self)
{
    int i;

    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_candidate_wnd_ops;
    for (i = 0; i < CANDIDATE_MAX; i++)
        string_init(&self->text[i]);
    return self;
}

struct UIWindow *ui_candidate_wnd_new(void)
{
    return (struct UIWindow *)ui_candidate_wnd_construct(mem_alloc(sizeof(struct UICandidateWnd)));
}

void ui_candidate_wnd_destroy(struct UIWindow *window)
{
    struct UICandidateWnd *self = (struct UICandidateWnd *)window;
    int i;

    self->frame.base.ops = &ui_candidate_wnd_ops;
    for (i = CANDIDATE_MAX - 1; i >= 0; i--)
        string_free(&self->text[i]);
    ui_frame_wnd_destruct(&self->frame);
    mem_free(self);
}

void ui_candidate_wnd_on_create(struct UIWindow *self, int width_unused, int height_unused)
{
}

static void candidate_label(struct UICandidateWnd *self, int i, char *buf)
{
    sprintf(buf, "%d %s", i + 1, string_cstr(&self->text[i]));
}

/* The client draws no candidate text: the Korean (horizontal), Japanese (vertical) and
   Chinese layouts belonged to other service types, so only the frame is drawn. */
void ui_candidate_wnd_on_draw(struct UIWindow *window)
{
    ui_window_draw_sys_box(window, 0, 0, 0, 0);
}

int ui_candidate_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1,
                                      int val2, int val3, int val4)
{
    struct UICandidateWnd *self = (struct UICandidateWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    const struct String *text = (const struct String *)val1;
    const struct CandidatePage *page = (const struct CandidatePage *)val2;
    char buf[64];
    int i, width;

    if (msg != 0x41)
        return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);

    for (i = 0; i < CANDIDATE_MAX; i++)
        string_assign(&self->text[i], string_cstr(&text[i]), text[i].size);
    self->selected = page->selected;
    self->page = page->page;
    self->last_page = page->last_page;
    self->count = page->count;

    width = 0;
    for (i = 0; i < self->count; i++) {
        candidate_label(self, i, buf);
        width += ui_window_text_width(wnd, buf, 0, 0, IME_FONT_HEIGHT, 0, 0) + 8;
    }
    ui_window_resize_base(wnd, width + 0x16, wnd->height);
    ui_window_invalidate_base(wnd);
    return 0;
}
