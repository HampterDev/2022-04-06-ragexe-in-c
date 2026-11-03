#ifndef UI_CONTROLS_UI_HOOKS_H
#define UI_CONTROLS_UI_HOOKS_H

struct ItemInfo;

/* The calls the generic widgets in ui/controls make into feature code above them. There
   is one instance, g_uiHooks (ui/controls/ui_hooks.c); client_install_hooks() in
   core/client_init.c fills it in once at startup, before any window exists. Each member
   says who provides it and what the widgets do while it is NULL. */
struct UIHooks {
    /* A click on an item link in a text button, an edit line or a list: shows `item`
       in the item collection window and, unless `no_compare`, the worn item it would
       replace beside it. item_collection_show_link (ui/items/item_collection.c).
       NULL: the click opens nothing. */
    void (*show_item_link)(struct ItemInfo *item, int no_compare);
};

extern struct UIHooks g_uiHooks;

#endif
