#include "ui/controls/buttons.h"
#include "ui/controls/scroll.h"
#ifndef CONTENTS_ENCHANT_UI_H
#define CONTENTS_ENCHANT_UI_H

#include "base.h"
#include "ui/window_types.h"
#include "iteminfo/item_api.h"
#include "std/vec.h"
#include "std/std.h"

SIZE_ASSERT(UIWindow, 0x7c);

#define ENCHANT_UI_DIR  "유저인터페이스\\"
#define ENCHANT_BMP(n)  ENCHANT_UI_DIR "enchantUI\\" n

#define ENCHANT_COLORKEY    0xffff00ff

struct UIEnchantTab {
    struct UIWindow base;
    struct String label;
    char selected;
};
OFFSET_ASSERT(UIEnchantTab, label, 0x7c);

struct UIEnchantGroup {
    struct UIWindow base;
    char locked;
    int page;
};
OFFSET_ASSERT(UIEnchantGroup, page, 0x80);
SIZE_ASSERT(UIEnchantGroup, 0x84);

struct UIEnchantButton {
    struct UIButton button;
};

struct UIEnchantItem {
    struct UIWindow base;
    char large;
    char locked;
    struct ItemInfo item;
};
OFFSET_ASSERT(UIEnchantItem, large, 0x7c);
OFFSET_ASSERT(UIEnchantItem, locked, 0x7d);
OFFSET_ASSERT(UIEnchantItem, item, 0x80);

struct EnchantSocket {
    unsigned slot;
    struct UIEnchantItem *item;
};

struct EnchantSocketLabel {
    unsigned slot;
    struct UITagTextViewer *label;
};

struct UIEnchantScrollBar {
    struct UIScrollBar bar;
};

struct EnchantRow {
    struct ItemInfo item;
    char available;
};

struct UIEnchantList {
    struct UIWindow base;
    int scrollbar_width;
    int row_height;
    int visible_rows;
    int full_rows;
    int selected;
    int hover;
    int top;
    struct UIEnchantScrollBar *scrollbar;
    char locked;
    struct Vec rows;
};
OFFSET_ASSERT(UIEnchantList, row_height, 0x80);
OFFSET_ASSERT(UIEnchantList, selected, 0x8c);
OFFSET_ASSERT(UIEnchantList, scrollbar, 0x98);
OFFSET_ASSERT(UIEnchantList, locked, 0x9c);
OFFSET_ASSERT(UIEnchantList, rows, 0xa0);
SIZE_ASSERT(UIEnchantList, 0xac);

#define ENCHANT_ROW_COUNT(list) VEC_COUNT(&(list)->rows, struct EnchantRow)
#define ENCHANT_ROW(list, i)    (&((struct EnchantRow *)(list)->rows.first)[i])

struct UIEnchantInfo {
    struct UIWindow base;
    struct UIEnchantItem *target;
    struct Tree sockets;
    struct Tree socket_labels;
    struct Vec materials;
    struct UIEnchantButton *button;
    struct UIEnchantList *results;
    unsigned __int64 zeny;
    int enchant_slot;
};
OFFSET_ASSERT(UIEnchantInfo, sockets, 0x80);
OFFSET_ASSERT(UIEnchantInfo, materials, 0x90);
OFFSET_ASSERT(UIEnchantInfo, button, 0x9c);
OFFSET_ASSERT(UIEnchantInfo, zeny, 0xa8);
OFFSET_ASSERT(UIEnchantInfo, enchant_slot, 0xb0);
SIZE_ASSERT(UIEnchantInfo, 0xb8);

int enchant_item_cell_id(struct UIEnchantItem *self);
void enchant_group_select(struct UIEnchantGroup *self, int page);
int enchant_listbox_selected_item_id(struct UIEnchantList *self);
void enchant_listbox_select(struct UIEnchantList *self, unsigned row);
void enchant_info_set_target(struct UIEnchantInfo *self, const struct ItemInfo *item);
void enchant_info_set_materials(struct UIEnchantInfo *self,
                                           const struct Tree *needs, unsigned __int64 zeny);
void enchant_info_set_results(struct UIEnchantInfo *self,
                                         const struct Vec *results, char lock);
void enchant_info_clear_materials(struct UIEnchantInfo *self);
void enchant_info_update_button(struct UIEnchantInfo *self);
void enchant_info_lock(struct UIEnchantInfo *self);
void enchant_info_lock_sockets(struct UIEnchantInfo *self, char locked);
int enchant_info_enchant_slot_item_id(struct UIEnchantInfo *self);

struct EnchantResult {
    int item_id;
    char available;
};

struct UIEnchantList *enchant_listbox_construct(struct UIEnchantList *self);

void enchant_button_delete(struct UIWindow *window);
void enchant_scrollbar_delete(struct UIWindow *window);

void enchant_button_draw(struct UIWindow *window);
void enchant_scrollbar_draw(struct UIWindow *window);

#endif
