#ifndef UI_ITEMS_ITEM_TAGS_H
#define UI_ITEMS_ITEM_TAGS_H

#include "ui/controls/edit_types.h"
#include "ui/controls/buttons.h"
#include "iteminfo/iteminfo.h"
#include "ui/items/item_tag_list.h"

struct ItemTagActions;

struct UIEditCtrlOps;

struct UIItemTagButton {
    struct UITextButton text;
    struct ItemInfo item;
    int line_x;
    int line;
    int text_y;
    int aid;
};

struct UIItemTagCtrl {
    struct UIEditCtrl edit;
    unsigned ordered_count;
    struct ItemTagList tags;
    struct String text;
    struct String linked_text;
    int width;
    const struct ItemTagActions *actions;
};

struct UIItemTagOnMail {
    struct UIItemTagCtrl ctrl;
    struct TextArray lines;
};

struct UIItemTagOnMegaPhone {
    struct UIItemTagCtrl ctrl;
    int tag_width;
};

struct UIItemTagButton *ui_item_tag_button_construct(struct UIItemTagButton *self);
struct UIItemTagCtrl *ui_item_tag_ctrl_construct(struct UIItemTagCtrl *self);
struct UIItemTagCtrl *ui_item_tag_on_chat_construct(struct UIItemTagCtrl *self);
struct UIItemTagOnMail *ui_item_tag_on_mail_construct(struct UIItemTagOnMail *self);
struct UIItemTagOnMegaPhone *ui_item_tag_on_mega_phone_construct(struct UIItemTagOnMegaPhone *self);
void ui_item_tag_ctrl_clear_tags(struct UIItemTagCtrl *self);
void ui_item_tag_ctrl_set_text(struct UIItemTagCtrl *self, struct TextView text, int width);
void ui_item_tag_ctrl_update(struct UIItemTagCtrl *self);
struct String *ui_item_tag_ctrl_append_tag(struct UIItemTagCtrl *self, struct String *out,
                                       struct TextView name);
void ui_item_tag_ctrl_add_item(struct UIItemTagCtrl *self, const struct ItemInfo *info, int colour);
BYTE ui_item_tag_ctrl_can_add(struct UIItemTagCtrl *self, struct TextView name, int width);
void ui_item_tag_on_mail_set_lines(struct UIItemTagOnMail *self, const struct TextArray *lines);

#endif
