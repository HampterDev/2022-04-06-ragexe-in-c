#include "world/tooltip.h"
void mode_show_tooltip(struct Mode * arg0, const char * arg1, int arg2, int arg3, DWORD arg4, BYTE arg5) { abort(); }
int rag_connection_get_packet_length(struct RagConnection * arg0, int arg1) { abort(); }
char rag_connection_send_packet(struct RagConnection * arg0, int arg1, const void * arg2) { abort(); }
void * res_mgr_get(struct ResMgr * arg0, const char * arg1) { abort(); }
COLORREF session_get_item_dis_color(const struct ItemInfo * arg1) { abort(); }
struct ItemInfo * session_get_mail_item(struct Session * arg0, struct ItemInfo * arg1, int arg2) { abort(); }
char * format_money64(long long arg0, char * arg1, int arg2) { abort(); }
char game_mode_is_blocking_window_open(void) { abort(); }
void item_info_destroy(void *item) { abort(); }
struct ItemInfo * item_info_construct_with_arg(struct ItemInfo * arg0) { abort(); }
struct ItemInfo * item_info_copy(struct ItemInfo * arg0, const struct ItemInfo * arg1) { abort(); }
struct String * item_info_get_display_name(const struct ItemInfo * arg0, struct String * arg1) { abort(); }
void * tree_find(const struct Tree * arg0, const void * arg1, TreeLessFn arg2) { abort(); }
int mail_id_greater(const void * arg0, const void * arg1) { abort(); }
struct RagConnection * rag_connection_instance(void) { abort(); }
struct UIFrameWnd * ui_frame_wnd_construct(struct UIFrameWnd * arg0, int arg1) { abort(); }
void ui_frame_wnd_destruct(struct UIFrameWnd * arg0) { abort(); }
int ui_frame_wnd_get_records(struct UIWindow * arg0, struct ReplayBuffer * arg1) { abort(); }
void ui_frame_wnd_move(struct UIWindow *arg0, int arg1, int arg2) { abort(); }
void ui_frame_wnd_move_delta(struct UIWindow *arg0, int arg1, int arg2) { abort(); }
void ui_frame_wnd_on_l_btn_down(struct UIWindow *arg0, int arg1, int arg2) { abort(); }
void ui_frame_wnd_on_l_btn_up(struct UIWindow *arg0, int arg1, int arg2) { abort(); }
void ui_frame_wnd_on_mouse_move(struct UIWindow *arg0, int arg1, int arg2) { abort(); }
void ui_frame_wnd_save_pos(struct UIWindow *arg0) { abort(); }
int ui_frame_wnd_send_msg(struct UIWindow *arg0, struct UIWindow * arg1, int arg2, int arg3, int arg4, int arg5, int arg6) { abort(); }
void ui_frame_wnd_snap(struct UIWindow *arg0) { abort(); }
const char * ui_skin_path(const char * arg0) { abort(); }
int ui_window_mgr_message_box(struct UIWindowMgr * arg0, const char * arg1, int arg2, int arg3, int arg4, int arg5, const char * arg6, int arg7, int arg8, int arg9) { abort(); }
BYTE ui_window_mgr_post_quit1(struct UIWindowMgr * arg0, int arg1) { abort(); }
int ui_window_mgr_send_msg_(struct UIWindowMgr * arg0, int arg1, int arg2, int arg3, int arg4, int arg5) { abort(); }
struct UIWindow * ui_window_add_close_button(struct UIWindow * arg0, const char * arg1, int arg2) { abort(); }
struct UIWindow * ui_window_add_sys_button(struct UIWindow * arg0) { abort(); }
void ui_window_clear_canvas(struct UIWindow * arg0, DWORD arg1) { abort(); }
void ui_window_drag_and_drop_base(struct UIWindow *arg0, int arg1, int arg2, const struct DragInfo *arg3) { abort(); }
void ui_window_draw_bitmap(struct UIWindow * arg0, int arg1, int arg2, struct BitmapRes * arg3, int arg4) { abort(); }
void ui_window_draw_item_icon_count(struct UIWindow * arg0, int arg1, int arg2, const struct ItemInfo * arg3, BYTE arg4, BYTE arg5) { abort(); }
int ui_window_draw_title_bar(struct UIWindow * arg0, int arg1, const char * arg2, int arg3) { abort(); }
DWORD ui_window_get_color(struct UIWindow * arg0) { abort(); }
struct UIWindow * ui_window_hit_test_base(struct UIWindow * arg0, int arg1, int arg2) { abort(); }
BYTE ui_window_is_hit_testable_base(struct UIWindow * arg0) { abort(); }
struct TexDrawPrim * ui_window_make_quad_base(struct UIWindow * arg0) { abort(); }
void ui_window_on_destroy_base(struct UIWindow * arg0) { abort(); }
void ui_window_on_l_btn_dbl_clk_base(struct UIWindow * arg0, int arg1, int arg2) { abort(); }
void ui_window_on_mouse_shape_base(struct UIWindow * arg0, int arg1, int arg2) { abort(); }
void ui_window_on_process_base(struct UIWindow * arg0) { abort(); }
void ui_window_on_wheel_base(struct UIWindow * arg0, int arg1) { abort(); }
BYTE ui_window_render_base(struct UIWindow * arg0, int arg1) { abort(); }
void ui_window_resize_base(struct UIWindow * arg0, int arg1, int arg2) { abort(); }
void ui_window_set_show_base(struct UIWindow * arg0, int arg1) { abort(); }
void ui_window_render_overlay_base(struct UIWindow * arg0) { abort(); }
void ui_window_store_info_base(struct UIWindow * arg0) { abort(); }
void ui_window_text_out(struct UIWindow * arg0, int arg1, int arg2, const char * arg3, int arg4, int arg5, int arg6, COLORREF arg7, int arg8, int arg9) { abort(); }
void ui_window_to_screen(struct UIWindow * arg0, int * arg1, int * arg2) { abort(); }
void ui_window_on_focus_base(struct UIWindow * arg0) { abort(); }
void ui_window_on_focus_lost_base(struct UIWindow * arg0) { abort(); }
void ui_window_track_mouse_base(struct UIWindow * arg0, int arg1, int arg2) { abort(); }
void ui_window_on_r_btn_up_base(struct UIWindow * arg0, int arg1, int arg2) { abort(); }
void ui_window_draw_backdrop_base(struct UIWindow * arg0) { abort(); }
void ui_window_on_hover_enter_base(struct UIWindow * arg0) { abort(); }
void ui_window_on_hover_leave_base(struct UIWindow * arg0) { abort(); }
BYTE ui_window_is_focusable_base(struct UIWindow * arg0) { abort(); }
BYTE ui_window_get_backdrop_box_base(struct UIWindow * arg0, struct UIBackdropBox * arg1) { abort(); }
BYTE ui_window_is_click_through_base(struct UIWindow * arg0) { abort(); }
struct Mode * client_current_mode(void) { abort(); }
struct ResMgr * g_resMgr(void) { abort(); }
struct Session g_session1;
struct UIWindowMgr g_windowMgr;
struct Mode * game_current_base_mode(void) { abort(); }
int game_send_message(int arg0, int arg1, int arg2, int arg3, int arg4) { abort(); }
int mode_send_message(struct Mode * arg0, int arg1, int arg2, int arg3, int arg4, int arg5) { abort(); }
struct UIWindow * ui_window_open(int arg0) { abort(); }
int ui_window_send(struct UIWindow * arg0, struct UIWindow * arg1, int arg2, int arg3, int arg4, int arg5, int arg6) { abort(); }
