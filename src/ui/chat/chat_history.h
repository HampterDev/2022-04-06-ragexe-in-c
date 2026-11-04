#include "ui/window_manager_state.h"
#ifndef UI_CHAT_HISTORY_H
#define UI_CHAT_HISTORY_H

#include "core/markup_tags.h"
#include "ui/chat/readable_links.h"
#include "ui/chat/chat.h"

#define CHAT_HISTORY_MAX_LINES 0xf0

#define UI_ID_HISTORY_LINE 0x153
#define UI_MSG_HISTORY_DRAW 0x74

#define UI_ID_NAVI_LINK 0x1ae

struct UIChatHisBox {
    struct UIListBox list;
    struct List item_tags;
    struct List navi_links;
    struct Vec colours;
    unsigned max_lines;
    struct Vec line_extra;
    struct Vec log_text;
    struct Vec log_colour;
    struct Vec log_extra;
};

struct UIChatHisBox *ui_chat_his_box_construct(struct UIChatHisBox *self);
void ui_chat_his_box_destruct(struct UIChatHisBox *self);
void ui_chat_his_box_add_wrapped(struct UIChatHisBox *self, const char *text, COLORREF colour,
                                        const char *extra);
void ui_chat_his_box_log_message(struct UIChatHisBox *self, const char *text, COLORREF colour,
                                        const char *extra);
void ui_chat_his_box_rebuild(struct UIChatHisBox *self);
void ui_chat_his_box_resize_to(struct UIChatHisBox *self, int width, int height, BYTE rebuild);
int ui_chat_his_box_save_to_file(struct UIChatHisBox *self, const char *path);
void ui_chat_his_box_resolve_pending_name(struct UIChatHisBox *self, const char *text);
void ui_chat_his_box_copy_log(struct UIChatHisBox *self, struct UIChatHisBox *src);
const char *ui_chat_his_box_get_line_extra(struct UIChatHisBox *self, unsigned i);

void ui_chat_his_box_add_item_link_line(struct UIChatHisBox *self, const char *text, const COLORREF *colour,
                                             const int *mode);
void ui_chat_his_box_add_navi_link_line(struct UIChatHisBox *self, const char *text, const COLORREF *colour,
                                             const int *mode);
struct String *ui_chat_his_box_build_item_line(struct UIChatHisBox *self, struct String *out,
                                                        const char *line, struct String *rest);

struct UIChatHisBox2 {
    struct UIChatHisBox box;
    int back_r, back_g, back_b;
};
struct UIChatHisBox2 *ui_chat_his_box2_construct(struct UIChatHisBox2 *self);

void ui_chat_his_box_resize(struct UIWindow *window, int width, int height);
BYTE ui_chat_his_box2_is_click_through(struct UIWindow *self);
void ui_chat_his_box_set_show(struct UIWindow *window, int visible);
void ui_chat_his_box2_on_create(struct UIWindow *window, int width, int height);
void ui_chat_his_box_on_l_btn_dbl_clk(struct UIWindow *window, int x_unused, int y_unused);
void ui_chat_his_box_on_l_btn_up(struct UIWindow *self, int x_unused, int y_unused);
intptr_t ui_chat_his_box_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2,
                                    intptr_t val3, intptr_t val4);

void ui_chat_his_box2_create(struct UIListBox *base_list, int x, int y, int width, int height, BYTE transparent_unused);
void ui_chat_his_box_clear_items(struct UIListBox *base_list);
void ui_chat_his_box_add_line(struct UIChatHisBox *self, const char *text, COLORREF colour, const char *extra);

#endif
