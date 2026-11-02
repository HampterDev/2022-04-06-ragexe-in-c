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
void ui_window_on_mouse_hover_base(struct UIWindow *self, int x, int y) { abort(); }
void ui_window_on_mouse_move_base(struct UIWindow *self, int x, int y) { abort(); }
void ui_window_on_mouse_shape_base(struct UIWindow *self, int x, int y) { abort(); }
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

void mode_set_cursor_action(struct Mode *self, int cursor) { abort(); }
void *res_mgr_get(struct ResMgr *self, const char *name) { abort(); }
void session_get_item_bitmap_file_name(const char *id_text,
                                              char *buf, BYTE identified) { abort(); }
struct ItemInfo *item_info_construct(struct ItemInfo *self) { abort(); }
struct ItemInfo *item_info_copy(struct ItemInfo *self, const struct ItemInfo *src) { abort(); }
struct String *item_info_get_display_name(const struct ItemInfo *self, struct String *out) { abort(); }
void store_search_item_copy(void *self, const void *src) { abort(); }
void store_search_items_destroy(void *first, void *last) { abort(); }
void ui_list_box_on_create(struct UIWindow *self, int width, int height) { abort(); }
void ui_list_box_on_wheel(struct UIWindow *self, int delta) { abort(); }
void ui_list_box_resize(struct UIWindow *self, int width, int height) { abort(); }
intptr_t ui_list_box_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg,
                                 intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4) { abort(); }
void ui_list_box_on_r_btn_down(struct UIWindow *self, int x, int y) { abort(); }
#ifdef UI_STRING_REFERENCE
void UITextViewer_AddItem(struct UITextViewer *self, const char *text) { abort(); }
#else
void ui_list_box_add_item(struct UIListBox *self, const char *text) { abort(); }
#endif
void ui_text_viewer_ignore_click(struct UIWindow *self, int x, int y) { abort(); }
void ui_window_draw_bitmap(struct UIWindow *self, int x, int y, struct BitmapRes *bitmap,
                                    int keep_alpha) { abort(); }
void ui_window_on_wheel_base(struct UIWindow *self, int delta) { abort(); }
intptr_t ui_window_send_msg_base(struct UIWindow *self, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2,
                                intptr_t val3, intptr_t val4) { abort(); }
void ui_window_text_out_runs(struct UIWindow *self, int x, int y, const char *text, int len,
                           COLORREF *colour, int font_face, int font_height, BYTE shadow) { abort(); }
struct Mode *client_current_mode(void) { abort(); }
struct ResMgr *g_resMgr(void) { abort(); }
char *std_copy_memmove(const char *first, const char *last,
                                  char *dest) { abort(); }
const char *text_find_colour_code(const char *text, const char *end, unsigned code_page) { abort(); }
const char *text_read_colour_code(const char *text, const char *end, unsigned long *colour) { abort(); }
struct UIWindow *ui_window_find(int id) { abort(); }
struct UIWindow *ui_window_open(int id) { abort(); }
intptr_t ui_window_send(struct UIWindow *window, struct UIWindow *sender, int message,
                   intptr_t a, intptr_t b, intptr_t c, intptr_t d) { abort(); }
