void ui_window_drag_and_drop_base(struct UIWindow *self, int x, int y, const struct DragInfo *drag) { abort(); }
DWORD ui_window_get_color(struct UIWindow *self) { abort(); }

struct UIWindow *ui_window_hit_test_base(struct UIWindow *self, int x, int y) { abort(); }
BYTE ui_window_is_hit_testable_base(struct UIWindow *self) { abort(); }
struct TexDrawPrim *ui_window_make_quad_base(struct UIWindow *self) { abort(); }
void ui_window_move_delta_base(struct UIWindow *self, int dx, int dy) { abort(); }
void ui_window_on_create_base(struct UIWindow *self, int width, int height) { abort(); }
void ui_window_on_destroy_base(struct UIWindow *self) { abort(); }
void ui_window_on_mouse_hover_base(struct UIWindow *self, int x, int y) { abort(); }
void ui_window_on_process_base(struct UIWindow *self) { abort(); }
BYTE ui_window_render_base(struct UIWindow *self, int force) { abort(); }
void ui_window_save_pos_base(struct UIWindow *self) { abort(); }
intptr_t ui_window_send_msg_base(struct UIWindow *self, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2,
                                intptr_t val3, intptr_t val4) { abort(); }
void ui_window_set_show_base(struct UIWindow *self, int visible) { abort(); }
void ui_window_render_overlay_base(struct UIWindow *self) { abort(); }
void ui_window_snap_base(struct UIWindow *self) { abort(); }
void ui_window_store_info_base(struct UIWindow *self) { abort(); }
void ui_window_on_focus_base(struct UIWindow *self) { abort(); }
void ui_window_on_focus_lost_base(struct UIWindow *self) { abort(); }
void ui_window_track_mouse_base(struct UIWindow *self, int x, int y) { abort(); }
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
