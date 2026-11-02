#include "render/rp_pool.h"
#include "render/texture_types.h"
#include "ui/window_list.h"
#include "ui/window_paint.h"
#ifndef UI_WINDOW_H
#define UI_WINDOW_H

#define WIN32_LEAN_AND_MEAN
#include "platform/win_types.h"
#include "render/renderer.h"
#include "base.h"
#include "ui/ui_std.h"

#include "ui/window_types.h"
#include "diag/perf.h"

int window_info_is_on_screen(struct WindowInfo *self);
void window_info_set_default(struct WindowInfo *self);

static __inline int ui_info_on_screen(const struct WindowInfo *info)
{
    if (info->width <= 0 || info->height <= 0)
        return 0;
    if (info->x < 0 || info->x > g_renderer->width - info->width)
        return 0;
    if (info->y < 0 || info->y > g_renderer->height - info->height)
        return 0;
    return 1;
}

struct UIFrameWnd {
    struct UIWindow base;
    int press_x, press_y;
    int saved_x, saved_y;
    int default_id;
    int cancel_id;
};
#define UI_FRAME_DEFAULT_ID(frame) (((struct UIFrameWnd *)(frame))->default_id)
#define UI_FRAME_CANCEL_ID(frame) (((struct UIFrameWnd *)(frame))->cancel_id)
#define UI_FRAME_DRAG_ALPHA 0x80

struct UIWindow *ui_window_construct(struct UIWindow *self, int composites);
void ui_window_create(struct UIWindow *self, int width, int height);
void ui_window_add_child(struct UIWindow *self, struct UIWindow *child);
void ui_window_remove_child(struct UIWindow *self,
                                     struct UIWindow *child, BYTE destroy);

void ui_window_destruct(struct UIWindow *self);
void ui_window_destroy_base(struct UIWindow *window);
void ui_window_resize_base(struct UIWindow *self, int width, int height);
void ui_window_move_base(struct UIWindow *self, int x, int y);
void ui_window_drag_and_drop_base(struct UIWindow *self, int x, int y, const struct DragInfo *drag);
void ui_window_on_mouse_shape_base(struct UIWindow *self, int x, int y);
int ui_window_send_msg_base(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2,
                                int val3, int val4);
DWORD ui_window_get_color(struct UIWindow *self);
struct UIWindow *ui_window_hit_test_base(struct UIWindow *self, int x, int y);
void ui_window_render_overlay_base(struct UIWindow *self);
struct UIWindow *ui_window_get_ancestor(struct UIWindow *self);
void ui_window_to_screen(struct UIWindow *self, int *x, int *y);

struct UIFrameWnd *ui_frame_wnd_construct(struct UIFrameWnd *self,
                                                   int composites);
void ui_frame_wnd_destruct(struct UIFrameWnd *self);
int ui_frame_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender,
                                  int msg, int val1, int val2, int val3, int val4);

void ui_frame_wnd_destroy(struct UIWindow *window);
void ui_frame_wnd_move(struct UIWindow *self, int x, int y);
void ui_frame_wnd_save_pos(struct UIWindow *self);
void ui_frame_wnd_move_delta(struct UIWindow *self, int dx, int dy);
void ui_frame_wnd_on_l_btn_down(struct UIWindow *self, int x, int y);
void ui_frame_wnd_on_mouse_move(struct UIWindow *self, int x, int y);
void ui_frame_wnd_on_mouse_hover(struct UIWindow *self, int x, int y);
void ui_frame_wnd_on_l_btn_up(struct UIWindow *self, int x, int y);
void ui_frame_wnd_snap(struct UIWindow *self);

struct UIEditCtrl;
BYTE ui_window_query_false(struct UIWindow *self);
BYTE ui_window_query_true(struct UIWindow *self);
void ui_window_abstract_on_create(struct UIWindow *self, int width, int height);
void ui_window_abstract_on_draw(struct UIWindow *self);
BYTE ui_window_is_focusable_base(struct UIWindow *self);
BYTE ui_window_get_backdrop_box_base(struct UIWindow *self, struct UIBackdropBox *out);
BYTE ui_window_is_click_through_base(struct UIWindow *self);
BYTE ui_window_is_hit_testable_base(struct UIWindow *self);
void ui_window_store_info_base(struct UIWindow *self);
void ui_window_save_pos_base(struct UIWindow *self);
void ui_window_move_delta_base(struct UIWindow *self, int dx, int dy);
void ui_window_set_show_base(struct UIWindow *self, int visible);
void ui_window_on_create_base(struct UIWindow *self, int width, int height);
void ui_window_on_destroy_base(struct UIWindow *self);
void ui_window_on_process_base(struct UIWindow *self);
void ui_window_on_focus_base(struct UIWindow *self);
void ui_window_on_focus_lost_base(struct UIWindow *self);
void ui_window_on_l_btn_down_base(struct UIWindow *self, int x, int y);
void ui_window_on_l_btn_dbl_clk_base(struct UIWindow *self, int x, int y);
void ui_window_on_mouse_move_base(struct UIWindow *self, int x, int y);
void ui_window_on_mouse_hover_base(struct UIWindow *self, int x, int y);
void ui_window_track_mouse_base(struct UIWindow *self, int x, int y);
void ui_window_on_l_btn_up_base(struct UIWindow *self, int x, int y);
void ui_window_on_r_btn_down_base(struct UIWindow *self, int x, int y);
void ui_window_on_r_btn_up_base(struct UIWindow *self, int x, int y);
void ui_window_on_wheel_base(struct UIWindow *self, int delta);
void ui_window_snap_base(struct UIWindow *self);
void ui_window_draw_backdrop_base(struct UIWindow *self);

void ui_window_on_hover_enter_base(struct UIWindow *self);
void ui_window_on_hover_leave_base(struct UIWindow *self);
BYTE ui_edit_ctrl_is_focusable(struct UIWindow *self);
void ui_list_box_on_r_btn_down(struct UIWindow *self, int x, int y);
void ui_list_box_navi_search_on_l_btn_dbl_clk(struct UIWindow *self, int x, int y);
BYTE ui_balloon_text_is_hit_testable(struct UIWindow *self);
void ui_balloon_text_on_process(struct UIWindow *self);

struct DragInfo;
struct List;
struct String;
struct Vec;
struct UIAcceptWnd;
struct UIFrameWnd;
struct UIItemBaseWnd;
struct TexDrawPrim;
struct UIWindow;
void ui_window_fill_canvas(struct UIWindow *self, DWORD colour, const RECT *rect);
void ui_window_prepend_child(struct UIWindow *self, struct UIWindow *child);
void ui_window_destroy_children(struct UIWindow *self);
struct UIWindow *ui_window_find_child_by_serial(struct UIWindow *self,
                                                       int serial);
struct UIWindow *ui_window_next_focusable(struct UIWindow *self,
                                                   struct UIWindow *child);
struct UIWindow *ui_window_prev_focusable(struct UIWindow *self,
                                                   struct UIWindow *child);
BYTE ui_window_has_ancestor(struct UIWindow *self,
                                     struct UIWindow *ancestor);
void ui_window_send_msg3_to_tree(struct UIWindow *self);
void ui_window_send_msg4_to_tree(struct UIWindow *self);
void ui_window_flush_quads(struct UIWindow *self);
BYTE ui_window_render_base(struct UIWindow *self, int force);
struct TexDrawPrim *ui_window_make_quad_base(struct UIWindow *self);

struct List *ui_window_wrap_text_plain(struct UIWindow *self,
                                                  struct List *out, struct String text,
                                                  struct List *lines, int width, int font_face,
                                                  int font_height, int font_arg);
struct List *ui_window_wrap_text(struct UIWindow *self,
                                             struct List *out, struct String text,
                                             struct List *lines, int width, int font_face,
                                             int font_height, BYTE bold);
void ui_window_parse_tag_links(struct UIWindow *self,
                                       struct String text, struct String open,
                                       struct String close, struct Vec *out);
void ui_window_parse_navi_links(struct UIWindow *self,
                                        struct String text, struct Vec *out);
void ui_window_parse_url_links(struct UIWindow *self,
                                       struct String text, struct Vec *out);

void ui_ondraw(struct UIWindow *self);
struct Texture;
void *ui_list_iterator_self(void *self);
void ui_empty_method(void *self);
struct Renderer;

static __inline void ui_invalidate_root(struct UIWindow *wnd)
{
    if (ui_window_get_ancestor(wnd)) {
        struct UIWindow *root = ui_window_get_ancestor(wnd);

        ui_window_invalidate(root);
        ui_window_invalidate_children(ui_window_get_ancestor(wnd));
    } else {
        ui_window_invalidate(wnd);
    }
}

#define UI_ADD_SYS_BUTTON(wnd) \
    (((struct UIWindow *(*)(struct UIWindow *))(void *)ui_window_add_sys_button)( \
        (struct UIWindow *)(wnd)))

#endif
