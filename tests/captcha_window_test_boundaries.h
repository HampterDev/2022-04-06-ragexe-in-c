#include "world/tooltip.h"
#include "render/rp_pool.h"

void mode_show_tooltip(struct Mode *self, const char *text, int x, int y,
                                  DWORD colour, BYTE flag) { fprintf(stderr, "Unexpected test boundary: %s\n", __FUNCTION__); abort(); }

void ui_frame_wnd_move_delta(struct UIWindow *self, int dx, int dy) { fprintf(stderr, "Unexpected test boundary: %s\n", __FUNCTION__); abort(); }
void ui_frame_wnd_on_l_btn_down(struct UIWindow *self, int x, int y) { fprintf(stderr, "Unexpected test boundary: %s\n", __FUNCTION__); abort(); }
void ui_frame_wnd_on_l_btn_up(struct UIWindow *self, int x, int y) { fprintf(stderr, "Unexpected test boundary: %s\n", __FUNCTION__); abort(); }
void ui_frame_wnd_on_mouse_hover(struct UIWindow *self, int x, int y) { fprintf(stderr, "Unexpected test boundary: %s\n", __FUNCTION__); abort(); }
void ui_frame_wnd_on_mouse_move(struct UIWindow *self, int x, int y) { fprintf(stderr, "Unexpected test boundary: %s\n", __FUNCTION__); abort(); }
void ui_frame_wnd_save_pos(struct UIWindow *self) { fprintf(stderr, "Unexpected test boundary: %s\n", __FUNCTION__); abort(); }
void ui_frame_wnd_snap(struct UIWindow *self) { fprintf(stderr, "Unexpected test boundary: %s\n", __FUNCTION__); abort(); }
int ui_window_mgr_send_msg_(struct UIWindowMgr *self, int msg, int val1, int val2,
                                   int val3, int val4) { fprintf(stderr, "Unexpected test boundary: %s\n", __FUNCTION__); abort(); }
void ui_window_clear_canvas(struct UIWindow *self, DWORD colour) { fprintf(stderr, "Unexpected test boundary: %s\n", __FUNCTION__); abort(); }
void ui_window_drag_and_drop_base(struct UIWindow *self, int x, int y, const struct DragInfo *drag) { fprintf(stderr, "Unexpected test boundary: %s\n", __FUNCTION__); abort(); }
int ui_window_draw_button_bar(struct UIWindow *self, int variant,
                                      int width, int rows) { fprintf(stderr, "Unexpected test boundary: %s\n", __FUNCTION__); abort(); }
void ui_window_draw_outline(struct UIWindow *self, int x, int y, int width, int height,
                           COLORREF colour) { fprintf(stderr, "Unexpected test boundary: %s\n", __FUNCTION__); abort(); }
int ui_window_draw_title_bar(struct UIWindow *self, int sys_icon, const char *title,
                                     int width) { fprintf(stderr, "Unexpected test boundary: %s\n", __FUNCTION__); abort(); }
void ui_window_fill_rect(struct UIWindow *self, int x, int y, int width,
                                  int height, COLORREF colour) { fprintf(stderr, "Unexpected test boundary: %s\n", __FUNCTION__); abort(); }
void ui_window_fill_round_rect(struct UIWindow *self, int x, int y, int width,
                                       int height, COLORREF colour) { fprintf(stderr, "Unexpected test boundary: %s\n", __FUNCTION__); abort(); }
DWORD ui_window_get_color(struct UIWindow *self) { fprintf(stderr, "Unexpected test boundary: %s\n", __FUNCTION__); abort(); }

struct UIWindow *ui_window_hit_test_base(struct UIWindow *self, int x, int y) { fprintf(stderr, "Unexpected test boundary: %s\n", __FUNCTION__); abort(); }
BYTE ui_window_is_hit_testable_base(struct UIWindow *self) { fprintf(stderr, "Unexpected test boundary: %s\n", __FUNCTION__); abort(); }
struct TexDrawPrim *ui_window_make_quad_base(struct UIWindow *self) { fprintf(stderr, "Unexpected test boundary: %s\n", __FUNCTION__); abort(); }
void ui_window_move_delta_base(struct UIWindow *self, int dx, int dy) { fprintf(stderr, "Unexpected test boundary: %s\n", __FUNCTION__); abort(); }
void ui_window_on_create_base(struct UIWindow *self, int width, int height) { (void)self; (void)width; (void)height; }
void ui_window_on_destroy_base(struct UIWindow *self) { fprintf(stderr, "Unexpected test boundary: %s\n", __FUNCTION__); abort(); }
void ui_window_on_l_btn_dbl_clk_base(struct UIWindow *self, int x, int y) { fprintf(stderr, "Unexpected test boundary: %s\n", __FUNCTION__); abort(); }
void ui_window_on_l_btn_down_base(struct UIWindow *self, int x, int y) { fprintf(stderr, "Unexpected test boundary: %s\n", __FUNCTION__); abort(); }
void ui_window_on_l_btn_up_base(struct UIWindow *self, int x, int y) { fprintf(stderr, "Unexpected test boundary: %s\n", __FUNCTION__); abort(); }
void ui_window_on_mouse_move_base(struct UIWindow *self, int x, int y) { fprintf(stderr, "Unexpected test boundary: %s\n", __FUNCTION__); abort(); }
void ui_window_on_mouse_shape_base(struct UIWindow *self, int x, int y) { fprintf(stderr, "Unexpected test boundary: %s\n", __FUNCTION__); abort(); }
void ui_window_on_process_base(struct UIWindow *self) { fprintf(stderr, "Unexpected test boundary: %s\n", __FUNCTION__); abort(); }
void ui_window_on_wheel_base(struct UIWindow *self, int delta) { fprintf(stderr, "Unexpected test boundary: %s\n", __FUNCTION__); abort(); }
BYTE ui_window_render_base(struct UIWindow *self, int force) { fprintf(stderr, "Unexpected test boundary: %s\n", __FUNCTION__); abort(); }
void ui_window_resize_base(struct UIWindow *self, int width, int height) { fprintf(stderr, "Unexpected test boundary: %s\n", __FUNCTION__); abort(); }
void ui_window_save_pos_base(struct UIWindow *self) { fprintf(stderr, "Unexpected test boundary: %s\n", __FUNCTION__); abort(); }
void ui_window_render_overlay_base(struct UIWindow *self) { fprintf(stderr, "Unexpected test boundary: %s\n", __FUNCTION__); abort(); }
void ui_window_snap_base(struct UIWindow *self) { fprintf(stderr, "Unexpected test boundary: %s\n", __FUNCTION__); abort(); }
void ui_window_store_info_base(struct UIWindow *self) { fprintf(stderr, "Unexpected test boundary: %s\n", __FUNCTION__); abort(); }
void ui_window_text_out(struct UIWindow *self, int x, int y, const char *text, int len,
                                 int font_face, int font_height, COLORREF colour, int font_arg3, int font_arg4) { fprintf(stderr, "Unexpected test boundary: %s\n", __FUNCTION__); abort(); }
void ui_window_text_out_right(struct UIWindow *self, int x, int y, const char *text,
                                      int len, int font_face, int font_height, COLORREF colour, int font_arg) { fprintf(stderr, "Unexpected test boundary: %s\n", __FUNCTION__); abort(); }
void ui_window_to_screen(struct UIWindow *self, int *x, int *y) { fprintf(stderr, "Unexpected test boundary: %s\n", __FUNCTION__); abort(); }
void ui_window_on_focus_base(struct UIWindow *self) { fprintf(stderr, "Unexpected test boundary: %s\n", __FUNCTION__); abort(); }
void ui_window_on_focus_lost_base(struct UIWindow *self) { fprintf(stderr, "Unexpected test boundary: %s\n", __FUNCTION__); abort(); }
void ui_window_track_mouse_base(struct UIWindow *self, int x, int y) { fprintf(stderr, "Unexpected test boundary: %s\n", __FUNCTION__); abort(); }
void ui_window_on_r_btn_down_base(struct UIWindow *self, int x, int y) { fprintf(stderr, "Unexpected test boundary: %s\n", __FUNCTION__); abort(); }
void ui_window_on_r_btn_up_base(struct UIWindow *self, int x, int y) { fprintf(stderr, "Unexpected test boundary: %s\n", __FUNCTION__); abort(); }
void ui_window_draw_backdrop_base(struct UIWindow *self) { fprintf(stderr, "Unexpected test boundary: %s\n", __FUNCTION__); abort(); }
void ui_window_on_hover_enter_base(struct UIWindow *self) { fprintf(stderr, "Unexpected test boundary: %s\n", __FUNCTION__); abort(); }
void ui_window_on_hover_leave_base(struct UIWindow *self) { fprintf(stderr, "Unexpected test boundary: %s\n", __FUNCTION__); abort(); }
BYTE ui_window_is_focusable_base(struct UIWindow *self) { fprintf(stderr, "Unexpected test boundary: %s\n", __FUNCTION__); abort(); }
BYTE ui_window_get_backdrop_box_base(struct UIWindow *self, struct UIBackdropBox *arg) { fprintf(stderr, "Unexpected test boundary: %s\n", __FUNCTION__); abort(); }
BYTE ui_window_is_click_through_base(struct UIWindow *self) { fprintf(stderr, "Unexpected test boundary: %s\n", __FUNCTION__); abort(); }
struct Language *g_language;
struct ModeMgr g_modeMgr;
struct UIWindowMgr g_windowMgr;
