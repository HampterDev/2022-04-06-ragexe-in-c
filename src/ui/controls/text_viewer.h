#ifndef UI_CONTROLS_TEXT_VIEWER_H
#define UI_CONTROLS_TEXT_VIEWER_H

#include "ui/window.h"
#include "ui/draw.h"
#include "ui/skin.h"
#include "ui/controls/scroll.h"
#include "ui/controls/listbox.h"

struct Vec;
struct UIListBox;
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

void ui_text_viewer_destroy(struct UIWindow *window);
void ui_text_viewer_on_draw(struct UIWindow *self);
void ui_text_viewer_ignore_click(struct UIWindow *self, int x, int y);

void ui_text_viewer_clear_items(struct UIListBox *base_list);

int ui_text_viewer_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg,
                                    int val1, int val2, int val3, int val4);

void ui_text_viewer_on_create(struct UIWindow *window, int width, int height);

#endif
