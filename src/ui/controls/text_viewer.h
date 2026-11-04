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
#define UI_MSG_ADD_TEXT 0x1d
#define UI_TEXT_VIEWER_BACKGROUND(viewer) (((struct UITextViewer *)(viewer))->background)
struct UIWindow *ui_text_viewer_construct(struct UIWindow *self);

void ui_text_viewer_destroy(struct UIWindow *window);
void ui_text_viewer_on_draw(struct UIWindow *self);
void ui_text_viewer_ignore_click(struct UIWindow *self, int x, int y);

void ui_text_viewer_clear_items(struct UIListBox *base_list);

intptr_t ui_text_viewer_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg,
                                    intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4);

void ui_text_viewer_on_create(struct UIWindow *window, int width, int height);

#endif
