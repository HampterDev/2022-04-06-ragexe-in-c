#ifndef UI_CONTROLS_ITEM_TAG_BUTTON_H
#define UI_CONTROLS_ITEM_TAG_BUTTON_H

#include "ui/controls/buttons.h"
#include "iteminfo/iteminfo.h"

/* A text button that carries an item: the <item> tag shown in chat, mail and megaphone
   edit lines. A click sends its parent message 0x62 with the button's ID, value and
   item. */
struct UIItemTagButton {
    struct UITextButton text;
    struct ItemInfo item;
    int line_x;
    int line;
    int text_y;
    int aid;
};

struct UIItemTagButton *ui_item_tag_button_construct(struct UIItemTagButton *self);
void ui_item_tag_button_destroy(struct UIWindow *window);
void ui_item_tag_button_on_draw(struct UIWindow *self);
void ui_item_tag_button_on_l_btn_down(struct UIWindow *self, int x, int y);
void ui_item_tag_button_on_mouse_hover(struct UIWindow *self, int x, int y);

#endif
