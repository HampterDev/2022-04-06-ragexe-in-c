#include "ui/controls/scroll.h"
void ui_window_drag_and_drop_base(struct UIWindow *self, int x, int y, const struct DragInfo *drag) { abort(); }
DWORD ui_window_get_color(struct UIWindow *self) { abort(); }

struct UIWindow *ui_window_hit_test_base(struct UIWindow *self, int x, int y) { abort(); }
BYTE ui_window_is_hit_testable_base(struct UIWindow *self) { abort(); }
struct TexDrawPrim *ui_window_make_quad_base(struct UIWindow *self) { abort(); }
void ui_window_move_delta_base(struct UIWindow *self, int dx, int dy) { abort(); }
void ui_window_on_destroy_base(struct UIWindow *self) { abort(); }
void ui_window_on_l_btn_dbl_clk_base(struct UIWindow *self, int x, int y) { abort(); }
void ui_window_on_l_btn_down_base(struct UIWindow *self, int x, int y) { abort(); }
void ui_window_on_l_btn_up_base(struct UIWindow *self, int x, int y) { abort(); }
void ui_window_on_mouse_move_base(struct UIWindow *self, int x, int y) { abort(); }
void ui_window_on_process_base(struct UIWindow *self) { abort(); }
BYTE ui_window_render_base(struct UIWindow *self, int force) { abort(); }
void ui_window_save_pos_base(struct UIWindow *self) { abort(); }
void ui_window_render_overlay_base(struct UIWindow *self) { abort(); }
void ui_window_snap_base(struct UIWindow *self) { abort(); }
void ui_window_store_info_base(struct UIWindow *self) { abort(); }
void ui_window_on_focus_base(struct UIWindow *self) { abort(); }
void ui_window_on_focus_lost_base(struct UIWindow *self) { abort(); }
void ui_window_track_mouse_base(struct UIWindow *self, int x, int y) { abort(); }
void ui_window_on_r_btn_down_base(struct UIWindow *self, int x, int y) { abort(); }
void ui_window_on_r_btn_up_base(struct UIWindow *self, int x, int y) { abort(); }
void ui_window_draw_backdrop_base(struct UIWindow *self) { abort(); }
void ui_window_on_hover_enter_base(struct UIWindow *self) { abort(); }
void ui_window_on_hover_leave_base(struct UIWindow *self) { abort(); }
BYTE ui_window_is_focusable_base(struct UIWindow *self) { abort(); }
BYTE ui_window_get_backdrop_box_base(struct UIWindow *self, struct UIBackdropBox *arg) { abort(); }
BYTE ui_window_is_click_through_base(struct UIWindow *self) { abort(); }
char *std_copy_memmove(const char *first, const char *last,
                                  char *dest) { abort(); }
void store_search_items_destroy(void *first, void *last) { abort(); }
void store_search_item_copy(void *self, const void *src) { abort(); }
unsigned hex_digit_value(unsigned char c) { abort(); }
struct String *item_info_get_display_name(const struct ItemInfo *self, struct String *out) { abort(); }
void ui_button_on_l_btn_down(struct UIWindow *self, int x, int y) { abort(); }
void ui_button_on_l_btn_dbl_clk(struct UIWindow *self, int x, int y) { abort(); }
void ui_button_track_mouse(struct UIWindow *self, int x, int y) { abort(); }
void ui_button_on_mouse_shape(struct UIWindow *self, int x, int y) { abort(); }
void ui_button_set_show(struct UIButton *self, BYTE show) { abort(); }
void ui_window_set_show_base(struct UIWindow *self, int show) { abort(); }

int g_serviceType;
void ui_scroll_bar_resize(struct UIWindow *self, int width, int height) { abort(); }
const struct UIWindowOps ui_scroll_bar2_ops = { .destroy = ui_window_on_destroy_base };
const struct UIWindowOps ui_scroll_bar3_ops = { .destroy = ui_window_on_destroy_base };
struct String *ui_chat_his_box_build_item_line(struct UIChatHisBox *self, struct String *out,
                                                        const char *line, struct String *rest) { abort(); }
struct String *ui_chat_his_box_readable_item_links(struct UIWindow *window,
                                                struct String *out, struct TextView text) { abort(); }
void ui_list_box_add_item(struct UIListBox *self, const char *text) { abort(); }
void ui_list_box_clear_items(struct UIListBox *self) { abort(); }
struct UIListBox *ui_list_box_construct(struct UIListBox *self) { abort(); }
void ui_list_box_destruct(struct UIListBox *self) { abort(); }
int ui_list_box_hit_line(struct UIListBox *self, int x, int y) { abort(); }
void ui_list_box_on_wheel(struct UIWindow *self, int delta) { abort(); }
void ui_list_box_resize(struct UIWindow *self, int width, int height) { abort(); }
intptr_t ui_list_box_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg,
                                 intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4) { abort(); }
void ui_list_box_update_scroll_bar(struct UIListBox *self) { abort(); }
void ui_list_box_on_r_btn_down(struct UIWindow *self, int x, int y) { abort(); }
struct UIScrollBar *ui_scroll_bar_construct(struct UIScrollBar *self) { abort(); }
void ui_window_break_coloured_lines(struct UIWindow *self,
                                            const char *text, struct TextArray *lines,
                                            struct ByteArray *flags, int width) { abort(); }
void ui_window_break_lines(struct UIWindow *self, const char *text,
                                    struct TextArray *lines, int width) { abort(); }
