#include "text_buttons_unreached.h"
UINT g_codePage;
void tag_links_free(struct Vec *links) { abort(); }
int tag_links_snap_to_link(int pos, const struct Vec *links) { abort(); }
struct String *tag_list_box_split_tags(struct String *out, const struct String *text,
                                       struct Vec *links) { abort(); }
int tag_text_line_length(const char *text, int len, int width) { abort(); }
struct String *tag_text_strip_codes(struct String *out, const char *text, int len) { abort(); }
void ui_list_box_add_item(struct UIListBox *self, const char *text) { abort(); }
void ui_list_box_clear_items(struct UIListBox *self) { abort(); }
void ui_list_box_create(struct UIListBox *self, int x, int y, int width, int height,
                                 BYTE transparent) { abort(); }
void ui_list_box_destruct(struct UIListBox *self) { abort(); }
void ui_list_box_on_create(struct UIWindow *self, int width, int height) { abort(); }
void ui_list_box_on_wheel(struct UIWindow *self, int delta) { abort(); }
void ui_list_box_resize(struct UIWindow *self, int width, int height) { abort(); }
int ui_list_box_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg,
                                 int val1, int val2, int val3, int val4) { abort(); }
void ui_list_box_update_scroll_bar(struct UIListBox *self) { abort(); }
void ui_list_box_on_r_btn_down(struct UIWindow *self, int x, int y) { abort(); }
void ui_window_add_child(struct UIWindow *self, struct UIWindow *child) { abort(); }
void ui_window_break_lines(struct UIWindow *self, const char *text,
                                    struct TextArray *lines, int width) { abort(); }
void ui_window_remove_child(struct UIWindow *self,
                                     struct UIWindow *child, BYTE destroy) { abort(); }
void ui_window_text_out_runs(struct UIWindow *self, int x, int y, const char *text, int len,
                           COLORREF *colour, int font_face, int font_height, BYTE shadow) { abort(); }
const char *text_find_colour_code(const char *text, const char *end, unsigned code_page) { abort(); }
const char *text_read_colour_code(const char *text, const char *end, unsigned long *colour) { abort(); }
void text_split_lines(const char *text, struct TextArray *lines, int width) { abort(); }
