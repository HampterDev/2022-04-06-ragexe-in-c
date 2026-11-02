#include "ui/ui_std.h"
#include "session/state.h"
void ui_scroll_bar_resize(struct UIWindow * arg0, int arg1, int arg2) { abort(); }
void ui_window_on_create_base(struct UIWindow * arg0, int arg1, int arg2) { abort(); }
struct UIScrollBar * ui_scroll_bar_construct(struct UIScrollBar * arg0) { abort(); }
void ui_window_clear_canvas(struct UIWindow * arg0, DWORD arg1) { abort(); }
struct UIWindow * ui_window_construct(struct UIWindow * arg0, int arg1) { abort(); }
void ui_window_destruct(struct UIWindow * arg0) { abort(); }
void ui_window_drag_and_drop_base(struct UIWindow * arg0, int arg1, int arg2, const struct DragInfo * arg3) { abort(); }
void ui_window_draw_backdrop_base(struct UIWindow * arg0) { abort(); }
void ui_window_fill_rect(struct UIWindow * arg0, int arg1, int arg2, int arg3, int arg4, COLORREF arg5) { abort(); }
struct UIWindow * ui_window_get_ancestor(struct UIWindow * arg0) { abort(); }
BYTE ui_window_get_backdrop_box_base(struct UIWindow * arg0, struct UIBackdropBox * arg1) { abort(); }
DWORD ui_window_get_color(struct UIWindow * arg0) { abort(); }

struct UIWindow * ui_window_hit_test_base(struct UIWindow * arg0, int arg1, int arg2) { abort(); }
BYTE ui_window_is_hit_testable_base(struct UIWindow * arg0) { abort(); }
struct TexDrawPrim * ui_window_make_quad_base(struct UIWindow * arg0) { abort(); }
void ui_window_move_delta_base(struct UIWindow * arg0, int arg1, int arg2) { abort(); }
void ui_window_on_destroy_base(struct UIWindow * arg0) { abort(); }
void ui_window_on_l_btn_up_base(struct UIWindow * arg0, int arg1, int arg2) { abort(); }
void ui_window_on_mouse_hover_base(struct UIWindow * arg0, int arg1, int arg2) { abort(); }
void ui_window_on_mouse_move_base(struct UIWindow * arg0, int arg1, int arg2) { abort(); }
void ui_window_on_mouse_shape_base(struct UIWindow * arg0, int arg1, int arg2) { abort(); }
void ui_window_on_process_base(struct UIWindow * arg0) { abort(); }
BYTE ui_window_render_base(struct UIWindow * arg0, int arg1) { abort(); }
void ui_window_save_pos_base(struct UIWindow * arg0) { abort(); }
intptr_t ui_window_send_msg_base(struct UIWindow * arg0, struct UIWindow * arg1, int arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6) { abort(); }
void ui_window_set_show_base(struct UIWindow * arg0, int arg1) { abort(); }
void ui_window_render_overlay_base(struct UIWindow * arg0) { abort(); }
void ui_window_snap_base(struct UIWindow * arg0) { abort(); }
void ui_window_store_info_base(struct UIWindow * arg0) { abort(); }
void ui_window_text_out_runs(struct UIWindow * arg0, int arg1, int arg2, const char * arg3, int arg4, COLORREF * arg5, int arg6, int arg7, BYTE arg8) { abort(); }
void ui_window_on_focus_base(struct UIWindow * arg0) { abort(); }
void ui_window_on_focus_lost_base(struct UIWindow * arg0) { abort(); }
void ui_window_track_mouse_base(struct UIWindow * arg0, int arg1, int arg2) { abort(); }
void ui_window_on_r_btn_down_base(struct UIWindow * arg0, int arg1, int arg2) { abort(); }
void ui_window_on_r_btn_up_base(struct UIWindow * arg0, int arg1, int arg2) { abort(); }
void ui_window_on_hover_enter_base(struct UIWindow * arg0) { abort(); }
void ui_window_on_hover_leave_base(struct UIWindow * arg0) { abort(); }
BYTE ui_window_is_focusable_base(struct UIWindow * arg0) { abort(); }
BYTE ui_window_is_click_through_base(struct UIWindow * arg0) { abort(); }
struct Session g_session1;
void * pod_emplace_realloc(struct Vec * arg0, BYTE * arg1, const void * arg2, size_t arg3, size_t arg4) { abort(); }

void * res_mgr_get(struct ResMgr * arg0, const char * arg1) { abort(); }
struct UIFrameWnd * ui_frame_wnd_construct(struct UIFrameWnd * arg0, int arg1) { abort(); }
void ui_frame_wnd_destruct(struct UIFrameWnd * arg0) { abort(); }

void ui_frame_wnd_move(struct UIWindow * arg0, int arg1, int arg2) { abort(); }
void ui_frame_wnd_move_delta(struct UIWindow * arg0, int arg1, int arg2) { abort(); }
void ui_frame_wnd_on_l_btn_down(struct UIWindow * arg0, int arg1, int arg2) { abort(); }
void ui_frame_wnd_on_l_btn_up(struct UIWindow * arg0, int arg1, int arg2) { abort(); }
void ui_frame_wnd_on_mouse_hover(struct UIWindow * arg0, int arg1, int arg2) { abort(); }
void ui_frame_wnd_on_mouse_move(struct UIWindow * arg0, int arg1, int arg2) { abort(); }
void ui_frame_wnd_save_pos(struct UIWindow * arg0) { abort(); }
void ui_frame_wnd_snap(struct UIWindow * arg0) { abort(); }
void ui_list_box_add_item(struct UIListBox * arg0, const char * arg1) { abort(); }
void ui_list_box_clear_items(struct UIListBox * arg0) { abort(); }
struct UIListBox * ui_list_box_construct(struct UIListBox * arg0) { abort(); }
void ui_list_box_create(struct UIListBox * arg0, int arg1, int arg2, int arg3, int arg4, BYTE arg5) { abort(); }
void ui_list_box_destruct(struct UIListBox * arg0) { abort(); }
int ui_list_box_hit_line(struct UIListBox * arg0, int arg1, int arg2) { abort(); }
void ui_list_box_on_create(struct UIWindow * arg0, int arg1, int arg2) { abort(); }
void ui_list_box_on_wheel(struct UIWindow * arg0, int arg1) { abort(); }
void ui_list_box_resize(struct UIWindow * arg0, int arg1, int arg2) { abort(); }
intptr_t ui_list_box_send_msg(struct UIWindow * arg0, struct UIWindow * arg1, int arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6) { abort(); }
void ui_list_box_on_r_btn_down(struct UIWindow * arg0, int arg1, int arg2) { abort(); }
void ui_skin_color_chip(struct SkinMgr * arg0, int arg1, int arg2, int * arg3, int * arg4, int * arg5) { abort(); }
const char * ui_skin_path(const char * arg0) { abort(); }
BYTE ui_window_mgr_post_quit1(struct UIWindowMgr * arg0, int arg1) { abort(); }
void ui_window_draw_bitmap(struct UIWindow * arg0, int arg1, int arg2, struct BitmapRes * arg3, int arg4) { abort(); }
void ui_window_on_l_btn_dbl_clk_base(struct UIWindow * arg0, int arg1, int arg2) { abort(); }
void ui_window_on_wheel_base(struct UIWindow * arg0, int arg1) { abort(); }
void ui_window_text_out(struct UIWindow * arg0, int arg1, int arg2, const char * arg3, int arg4, int arg5, int arg6, COLORREF arg7, int arg8, int arg9) { abort(); }
struct ResMgr * g_resMgr(void) { abort(); }
struct SkinMgr g_skinMgr;
struct UIWindowMgr g_windowMgr;
struct UIWindow * ui_window_find(int arg0) { abort(); }
