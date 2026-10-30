#include "ui/controls/listbox.h"
#include "ui/window_access.h"
#include "render/rp_pool.h"
#include "core/object_type_tables.h"
#include <string.h>
#include "ui/window.h"
#include "ui/draw.h"
#include "ui/skin.h"
#include "ui/window_mgr.h"
#include "ui/controls/controls.h"
#include "core/msg_str.h"
#include "diag/perf.h"
#include "std/tree.h"

struct UINormalMB;
struct UIWindow;
void ui_normal_mb_destroy(struct UIWindow *window);
void ui_normal_mb_on_draw(struct UIWindow *self);
int ui_normal_mb_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_normal_mb_ops = {
    .destroy = ui_normal_mb_destroy,
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
    .on_create = ui_window_on_create_base,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_normal_mb_on_draw,
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
    .send_msg = ui_normal_mb_send_msg,
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

#define NORMAL_MB_OK 0
#define NORMAL_MB_YES_NO 2
#define NORMAL_MB_CANCEL 4
#define NORMAL_MB_ID_YES 0xb3
#define NORMAL_MB_ID_NO 0xb4
#define UI_ID_NORMAL_MB_ANSWER 0x1e0
#define NORMAL_MB_MSG_SET_TEXT 0x39
#define MSG_NORMAL_MB_TITLE 0xe74

struct NormalMBAnswer {
    int id;
    void (*run)(void);
};

struct UINormalMB {
    struct UIFrameWnd frame;
    struct UIWindow *owner;
    struct UIWindow *viewer;
    struct String text;
    struct String title;
    struct Tree answers;
};
OFFSET_ASSERT(UINormalMB, text, 0x9c);

static struct UIWindow *ui_normal_mb_construct(struct UINormalMB *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_normal_mb_ops;
    string_init(&self->text);
    string_init(&self->title);
    tree_init(&self->answers);
    self->owner = NULL;
    return &self->frame.base;
}

struct UIWindow *ui_normal_mb_new(void)
{
    struct UINormalMB *self = mem_alloc(sizeof *self);

    return self ? ui_normal_mb_construct(self) : NULL;
}

void ui_normal_mb_destroy(struct UIWindow *window)
{
    struct UINormalMB *self = (struct UINormalMB *)window;
    self->frame.base.ops = &ui_normal_mb_ops;
    tree_free(&self->answers, NULL);
    string_free(&self->title);
    string_free(&self->text);
    ui_frame_wnd_destruct(&self->frame);
    mem_free(self);
}

void ui_normal_mb_set_answer(struct UIWindow *window, int id, void (*run)(void))
{
    struct UINormalMB *self = (struct UINormalMB *)window;
    struct NormalMBAnswer *answer = tree_get(&self->answers, &id, sizeof id, sizeof *answer, tree_less_int);

    answer->run = run;
}

static struct UIBitmapButton *add_button(struct UINormalMB *self, const char *normal, const char *hover,
                                         const char *pressed, int x, int y, int id)
{
    struct UIBitmapButton *button = mem_alloc(sizeof(struct UIBitmapButton));

    if (button)
        button = ui_bitmap_button_construct(button);
    ui_bitmap_button_set_bitmap(button, normal, UI_BTN_UP, 0);
    ui_bitmap_button_set_bitmap(button, hover, UI_BTN_HOVER, 0);
    ui_bitmap_button_set_bitmap(button, pressed, UI_BTN_DOWN, 0);
    ui_window_create(&button->button.base, button->bitmap_width, button->bitmap_height);
    ui_window_move_base(&button->button.base, x, y);
    button->button.base.id = id;
    ui_window_add_child(&self->frame.base, &button->button.base);
    return button;
}

#define BTN_OK UI_SKIN "btn_ok.bmp", UI_SKIN "btn_ok_a.bmp", UI_SKIN "btn_ok_b.bmp"
#define BTN_CANCEL UI_SKIN "btn_cancel.bmp", UI_SKIN "btn_cancel_a.bmp", UI_SKIN "btn_cancel_b.bmp"

void ui_normal_mb_create(struct UIWindow *wnd, struct UIWindow *owner, const char *text,
                                  int type, int default_no, int title, int width, int height, int picture)
{
    struct UINormalMB *self = (struct UINormalMB *)wnd;
    const char *title_text = (const char *)title;
    struct UIWindow *viewer;
    int text_x = 0;

    self->owner = owner;
    string_assign_cstr(&self->text, text);
    if (!title_text)
        title_text = msg_str(MSG_NORMAL_MB_TITLE);
    string_assign_cstr(&self->title, title_text);
    ui_window_create(wnd, width, height);
    ui_window_on_create(wnd, width, height);
    ui_window_move(wnd, (int)((double)(g_renderer->width - width) * 0.5),
             (int)((double)(g_renderer->height - height) * 0.5));

    if (picture) {
        struct UIStaticBitmap *bitmap = mem_alloc(sizeof(struct UIStaticBitmap));

        if (bitmap)
            bitmap = ui_static_bitmap_construct(bitmap);
        ui_static_bitmap_set_bitmap(bitmap, (const char *)picture);
        ui_window_create(&bitmap->base, bitmap->base.width, bitmap->base.height);
        ui_window_move_base(&bitmap->base, 10, 0x16);
        ui_window_add_child(wnd, &bitmap->base);
        text_x = bitmap->base.width + bitmap->base.x;
    }

    viewer = mem_alloc(sizeof(struct UITextViewer));
    if (viewer)
        viewer = ui_text_viewer_construct(viewer);
    self->viewer = viewer;
    ui_list_box_create_window((struct UIListBox *)viewer, text_x + 10, 0x16, width - text_x - 0x14, height - 0x37, 0);
    UI_TEXT_VIEWER_BACKGROUND(self->viewer) = ui_skin_argb(2, 2);
    ui_window_invalidate(self->viewer);
    ui_window_add_child(wnd, self->viewer);
    ui_window_send_msg(self->viewer, NULL, UI_MSG_ADD_TEXT, (int)string_cstr(&self->text), (int)(width / 7.5),
             0, 0);

    switch (type) {
    case NORMAL_MB_OK:
        add_button(self, BTN_OK, width - 0x2e, height - 0x18, UI_ID_OK);
        UI_FRAME_DEFAULT_ID(self) = UI_ID_OK;
        break;
    case NORMAL_MB_YES_NO:
        add_button(self, BTN_OK, width - 0x5b, height - 0x18, NORMAL_MB_ID_YES);
        add_button(self, BTN_CANCEL, width - 0x2e, height - 0x18, NORMAL_MB_ID_NO);
        UI_FRAME_CANCEL_ID(self) = NORMAL_MB_ID_NO;
        UI_FRAME_DEFAULT_ID(self) = ((BYTE)default_no ^ 1) + NORMAL_MB_ID_YES;
        break;
    case NORMAL_MB_CANCEL:
        add_button(self, BTN_CANCEL, width - 0x2e, height - 0x18, UI_ID_CANCEL);
        UI_FRAME_DEFAULT_ID(self) = UI_ID_CANCEL;
        break;
    }
}

void ui_normal_mb_on_draw(struct UIWindow *window)
{
    struct UINormalMB *self = (struct UINormalMB *)window;
    ui_window_draw_title_bar(&self->frame.base, 1, string_cstr(&self->title), 0);
    ui_window_draw_body(&self->frame.base, 0x11, 0, "");
}

int ui_normal_mb_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1, int val2,
                                  int val3, int val4)
{
    struct UINormalMB *self = (struct UINormalMB *)window;
    if (msg == UI_MSG_COMMAND) {
        struct NormalMBAnswer *answer = tree_find(&self->answers, &val1, tree_less_int);

        if (answer)
            answer->run();
        else if (self->owner)
            ui_window_send_msg(self->owner, &self->frame.base, UI_MSG_COMMAND, UI_ID_NORMAL_MB_ANSWER, val1, 0, 0);
        ui_window_mgr_post_quit2(&g_windowMgr, &self->frame.base);
        return 0;
    }
    if (msg == NORMAL_MB_MSG_SET_TEXT) {
        ui_list_box_clear((struct UIListBox *)self->viewer);
        ui_window_send_msg(self->viewer, NULL, UI_MSG_ADD_TEXT, val1, 0x26, 0, 0);
        return 0;
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
}
