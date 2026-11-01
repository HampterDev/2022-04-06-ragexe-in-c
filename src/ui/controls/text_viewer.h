#ifndef UI_CONTROLS_TEXT_VIEWER_H
#define UI_CONTROLS_TEXT_VIEWER_H

#include "ui/window.h"
#include "ui/draw.h"
#include "ui/skin.h"
#include "ui/controls/scroll.h"
#include "ui/controls/listbox.h"

struct Vec;
struct UIListBox;
struct UITextViewEX;
struct UIString;
struct UIWindow;

struct UITextViewer {
    struct UIListBox list;
    struct Vec embeds;
    DWORD background;
    BYTE coloured;
    BYTE coloured_arg;
};
OFFSET_ASSERT(UITextViewer, embeds, 0xd0);
OFFSET_ASSERT(UITextViewer, background, 0xdc);
#define UI_MSG_ADD_TEXT 0x1d
#define UI_TEXT_VIEWER_BACKGROUND(viewer) (((struct UITextViewer *)(viewer))->background)
struct UIWindow *ui_text_viewer_construct(struct UIWindow *self);


void ui_text_view_ex_destroy(struct UIWindow *window);
int ui_text_view_ex_add_words(struct UITextViewEX *self, struct TextView text, int joined,
                                     int r, int g, int b);
void ui_text_view_ex_set_text(struct UITextViewEX *self, struct TextView text);
void ui_text_view_ex_add_row(struct UITextViewEX *self, struct UIString **first,
                                    struct UIString **last);
void ui_text_view_ex_break_rows(struct UITextViewEX *self);
void ui_text_view_ex_create(struct UIListBox *base_list, int x, int y, int width, int height,
                                    BYTE transparent);
void ui_text_view_ex_clear(struct UIListBox *base_list);
void ui_text_view_ex_update_scroll_bar(struct UIListBox *base_list);
void ui_text_view_ex_on_create(struct UIWindow *self, int width, int height);
void ui_text_view_ex_on_draw(struct UIWindow *self);
int ui_text_view_ex_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg,
                                    int val1, int val2, int val3, int val4);

void ui_text_viewer_destroy(struct UIWindow *window);
void ui_text_viewer_on_draw(struct UIWindow *self);
void ui_text_viewer_ignore_click(struct UIWindow *self, int x, int y);

void ui_text_viewer_clear_items(struct UIListBox *base_list);

int ui_text_viewer_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg,
                                    int val1, int val2, int val3, int val4);

void ui_text_viewer_on_create(struct UIWindow *window, int width, int height);

#endif
