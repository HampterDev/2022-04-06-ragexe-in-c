#include "world/tooltip.h"
#include "render/rp_pool.h"
#include "ui/character/character.h"
#include "world/mode_manager.h"
#include <stdlib.h>

struct ModeMgr g_modeMgr;
struct SkinMgr g_skinMgr;
struct Mode;
struct SkinMgr;
struct Vec;
struct TexDrawPrim;
struct UIWindow;

void mode_show_tooltip(struct Mode * arg0, const char * arg1, int arg2, int arg3, unsigned long arg4, unsigned char arg5) { abort(); }
void ui_skin_color_chip(struct SkinMgr * arg0, int arg1, int arg2, int * arg3, int * arg4, int * arg5) { abort(); }
void ui_window_drag_and_drop_base(struct UIWindow *arg0, int arg1, int arg2, const struct DragInfo *arg3) { abort(); }
unsigned long ui_window_get_color(struct UIWindow * arg0) { abort(); }
int ui_window_get_records_base(struct UIWindow * arg0, struct ReplayBuffer * arg1) { abort(); }
struct UIWindow * ui_window_hit_test_base(struct UIWindow * arg0, int arg1, int arg2) { abort(); }
unsigned char ui_window_is_hit_testable_base(struct UIWindow * arg0) { abort(); }
struct TexDrawPrim * ui_window_make_quad_base(struct UIWindow * arg0) { abort(); }
void ui_window_move_base(struct UIWindow * arg0, int arg1, int arg2) { abort(); }
void ui_window_move_delta_base(struct UIWindow * arg0, int arg1, int arg2) { abort(); }
void ui_window_on_draw_base(struct UIWindow * arg0) { abort(); }
void ui_window_on_create_base(struct UIWindow * arg0, int arg1, int arg2) { abort(); }
void ui_window_on_destroy_base(struct UIWindow * arg0) { abort(); }
void ui_window_on_l_btn_dbl_clk_base(struct UIWindow * arg0, int arg1, int arg2) { abort(); }
void ui_window_on_l_btn_down_base(struct UIWindow * arg0, int arg1, int arg2) { abort(); }
void ui_window_on_l_btn_up_base(struct UIWindow * arg0, int arg1, int arg2) { abort(); }
void ui_window_on_mouse_hover_base(struct UIWindow * arg0, int arg1, int arg2) { abort(); }
void ui_window_on_mouse_move_base(struct UIWindow * arg0, int arg1, int arg2) { abort(); }
void ui_window_on_mouse_shape_base(struct UIWindow * arg0, int arg1, int arg2) { abort(); }
void ui_window_on_process_base(struct UIWindow * arg0) { abort(); }
void ui_window_on_wheel_base(struct UIWindow * arg0, int arg1) { abort(); }
unsigned char ui_window_render_base(struct UIWindow * arg0, int arg1) { abort(); }
void ui_window_resize_base(struct UIWindow * arg0, int arg1, int arg2) { abort(); }
void ui_window_save_pos_base(struct UIWindow * arg0) { abort(); }
int ui_window_send_msg_base(struct UIWindow * arg0, struct UIWindow * arg1, int arg2, int arg3, int arg4, int arg5, int arg6) { abort(); }
void ui_window_set_show_base(struct UIWindow * arg0, int arg1) { abort(); }
void ui_window_render_overlay_base(struct UIWindow * arg0) { abort(); }
void ui_window_snap_base(struct UIWindow * arg0) { abort(); }
void ui_window_store_info_base(struct UIWindow * arg0) { abort(); }
void ui_window_text_out(struct UIWindow * arg0, int arg1, int arg2, const char * arg3, int arg4, int arg5, int arg6, unsigned long arg7, int arg8, int arg9) { abort(); }
void ui_window_to_screen(struct UIWindow * arg0, int * arg1, int * arg2) { abort(); }
void ui_window_invalidate_base(struct UIWindow * arg0) { abort(); }
void ui_window_on_focus_base(struct UIWindow * arg0) { abort(); }
void ui_window_on_focus_lost_base(struct UIWindow * arg0) { abort(); }
void ui_window_track_mouse_base(struct UIWindow * arg0, int arg1, int arg2) { abort(); }
void ui_window_on_r_btn_down_base(struct UIWindow * arg0, int arg1, int arg2) { abort(); }
void ui_window_on_r_btn_up_base(struct UIWindow * arg0, int arg1, int arg2) { abort(); }
void ui_window_draw_backdrop_base(struct UIWindow * arg0) { abort(); }
void ui_window_on_hover_enter_base(struct UIWindow * arg0) { abort(); }
void ui_window_on_hover_leave_base(struct UIWindow * arg0) { abort(); }
unsigned char ui_window_is_focusable_base(struct UIWindow * arg0) { abort(); }
unsigned char ui_window_get_backdrop_box_base(struct UIWindow * arg0, struct UIBackdropBox * arg1) { abort(); }
unsigned char ui_window_is_click_through_base(struct UIWindow * arg0) { abort(); }
BYTE ui_window_query_false(struct UIWindow *self) { abort(); }
BYTE ui_window_query_true(struct UIWindow *self) { abort(); }
void ui_window_abstract_on_draw(struct UIWindow *self) { abort(); }
