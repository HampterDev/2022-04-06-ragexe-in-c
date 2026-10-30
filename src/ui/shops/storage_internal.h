#include "ui/record_tags.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#ifndef UI_SHOPS_STORAGE_INTERNAL_H
#define UI_SHOPS_STORAGE_INTERNAL_H

#include "ui/shops/shops.h"

#define STORE_WND_ID 0x21
#define STORE_FIND_WND_ID 0x99
#define STORE_TAB_WND_FIRST 0x92
#define STORE_TAB_WND_LAST_ID 0x135
#define STORE_TABS 8

enum StoreCategory {
    STORE_CAT_CONSUMABLE = 0,
    STORE_CAT_CASH = 1,
    STORE_CAT_ARMOR = 2,
    STORE_CAT_WEAPON = 3,
    STORE_CAT_COSTUME = 4,
    STORE_CAT_AMMO = 5,
    STORE_CAT_CARD = 6,
    STORE_CAT_ETC = 7,
};

#define STORE_MSG_REFILL 0x17
#define STORE_MSG_SORT_CHOSEN 0x27
#define STORE_MSG_COUNT 0x37
#define STORE_MSG_UNSORT 0xee
#define STORE_MSG_SORT 0x164
#define STORE_MSG_SEARCH 0x165
#define STORE_ID_SEARCH 0x165
#define STORE_ID_FIND 0x85
#define STORE_ID_UNSORT 0xee

#define MODE_REQ_TO_STORAGE 0x37
#define MODE_REQ_CART_TO_STORAGE 0x4f
#define MODE_REQ_FROM_STORAGE 0x38
#define MODE_REQ_STORAGE_TO_CART 0x4e
#define MODE_REQ_CLOSE_STORAGE 0x39
#define DRAG_CART 9

struct StoreWndInfo {
    struct WindowInfo frame;
    unsigned open_tabs;
};

struct UIItemStoreFindWnd {
    struct UIItemBaseWnd base;
    struct UIWindow *close;
    struct UIResizer *resizer;
    struct UIWindow *title;
    int line_height;
    struct String count_text;
    struct String search;
    struct WindowInfo *info;
    BYTE sorting;
};

struct UIItemStoreSubWnd {
    struct UIItemBaseWnd base;
    struct UIResizer *resizer;
    struct UIWindow *title;
    int title_height;
    struct String count_text;
    struct String search;
    struct WindowInfo *info;
    int category;
    BYTE sorting;
};

struct UIItemStoreWnd {
    struct UIItemBaseWnd base;
    struct UIBitmapEditCtrl *edit;
    struct UIWindow *combo;
    BYTE tab_flags[8];
    struct UIWindow *boxes[STORE_TABS];
    struct UIWindow *tab_wnds[STORE_TABS];
    struct UIWindow *find_wnd;
    struct String count_text;
    struct UIWindow *title;
    struct StoreWndInfo *info;
    struct UITab *tabs;
    int margins[4];
    int spacing;
    int category;
    BYTE sorted;
    int count;
    int capacity;
    BYTE list_received;
};
OFFSET_ASSERT(UIItemStoreWnd, boxes, 0xe0);
OFFSET_ASSERT(UIItemStoreWnd, find_wnd, 0x120);
OFFSET_ASSERT(UIItemStoreWnd, count_text, 0x124);

void storage_draw_items(struct UIItemBaseWnd *base, int x0, int name_width);
void storage_on_lbtn_down(struct UIItemBaseWnd *base, int x, int y, int left, int replay_stops);
void storage_on_mouse_hover(struct UIItemBaseWnd *base, int x, int y, int left);
void storage_on_r_btn_down(struct UIItemBaseWnd *base, int x, int y, int left);
void storage_on_wheel(struct UIItemBaseWnd *base, int delta);
void storage_drop(struct UIItemBaseWnd *base, int x, int y, const struct DragInfo *drag);
void storage_add_dragged(void);
void storage_store_info(struct UIWindow *wnd, struct WindowInfo *info);
void storage_set_count_text(struct UIWindow *wnd, struct String *text, int count, int most);
void storage_fill(struct UIItemBaseWnd *base, int category);
int storage_sort(struct UIItemBaseWnd *base, int descending);
int storage_filter(struct UIItemBaseWnd *base, const struct String *needle, int by_mbsstr);
void storage_sort_chosen(struct UIItemBaseWnd *base);
void storage_list_create(struct UIItemBaseWnd *base, struct UIWindow **title, struct UIResizer **resizer);
void storage_list_draw(struct UIItemBaseWnd *base, const struct String *count_text);
void storage_list_resize(struct UIItemBaseWnd *base, struct UIResizer *resizer, struct UIWindow *close,
                         int width, int height);
int storage_list_info_usable(const struct WindowInfo *info);
void storage_apply_info(struct UIWindow *wnd, const struct WindowInfo *info);
void storage_find_set_search(struct UIItemStoreFindWnd *self, struct String text);

struct UIItemStoreSubWnd *ui_item_store_sub_wnd_construct(struct UIItemStoreSubWnd *self);
struct UIWindow *ui_item_store_sub_wnd_new(void);
void ui_item_store_sub_wnd_set_category(struct UIItemStoreSubWnd *self, int category);
void ui_item_store_sub_wnd_set_title(struct UIItemStoreSubWnd *self, struct String title);

struct UIItemStoreFindWnd *ui_item_store_find_wnd_construct(struct UIItemStoreFindWnd *self);
struct UIWindow *ui_item_store_find_wnd_new(void);
void ui_item_store_find_wnd_set_title(struct UIItemStoreFindWnd *self, struct String title);

#endif
