#ifndef UI_ITEMS_ITEM_TAGS_INTERNAL_H
#define UI_ITEMS_ITEM_TAGS_INTERNAL_H

#include "ui/items/item_tags.h"

struct ItemTagActions {
    BYTE (*can_add)(struct UIItemTagCtrl *self, struct TextView text, int width);
    void (*attach)(struct UIItemTagCtrl *self, struct UIWindow *button);
    void (*detach)(struct UIItemTagCtrl *self, struct UIWindow *button);
    void (*place)(struct UIItemTagCtrl *self, struct UIWindow *button,
                  struct TextView before, int x);
};

void ui_item_tag_ctrl_destruct(struct UIItemTagCtrl *self);
BYTE ui_item_tag_ctrl_can_tag(struct UIItemTagCtrl *self, struct TextView text, int width);
void ui_item_tag_ctrl_detach_tag(struct UIItemTagCtrl *self, struct UIWindow *button);
void ui_item_tag_ctrl_place_tag(struct UIItemTagCtrl *self, struct UIWindow *button,
                          struct TextView before, int x);

BYTE ui_item_tag_on_chat_can_tag(struct UIItemTagCtrl *self, struct TextView text, int width);
void ui_item_tag_on_chat_attach_tag(struct UIItemTagCtrl *self, struct UIWindow *button);
void ui_item_tag_on_chat_detach_tag(struct UIItemTagCtrl *self, struct UIWindow *button);
void ui_item_tag_on_chat_place_tag(struct UIItemTagCtrl *self, struct UIWindow *button,
                            struct TextView before, int x);

BYTE ui_item_tag_on_mail_can_tag(struct UIItemTagCtrl *ctrl, struct TextView text, int width);
void ui_item_tag_on_mail_attach_tag(struct UIItemTagCtrl *ctrl, struct UIWindow *button);
void ui_item_tag_on_mail_detach_tag(struct UIItemTagCtrl *ctrl, struct UIWindow *button);
void ui_item_tag_on_mail_destroy(struct UIWindow *window);
void ui_item_tag_on_mail_place_tag(struct UIItemTagCtrl *ctrl, struct UIWindow *button,
                            struct TextView before, int x);

BYTE ui_item_tag_on_mega_phone_can_tag(struct UIItemTagCtrl *ctrl, struct TextView text, int width);
void ui_item_tag_on_mega_phone_attach_tag(struct UIItemTagCtrl *ctrl, struct UIWindow *button);
void ui_item_tag_on_mega_phone_detach_tag(struct UIItemTagCtrl *ctrl, struct UIWindow *button);
void ui_item_tag_on_mega_phone_place_tag(struct UIItemTagCtrl *ctrl, struct UIWindow *button,
                                 struct TextView before, int x);

void ui_item_tag_on_chat_destroy(struct UIWindow *window);
void ui_item_tag_on_mega_phone_destroy(struct UIWindow *window);

#endif
