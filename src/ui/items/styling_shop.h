#include "iteminfo/item_api.h"
#include "session/character_name.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#ifndef UI_ITEMS_STYLING_SHOP_H
#define UI_ITEMS_STYLING_SHOP_H

#include "ui/items/items.h"
#include "ui/items/styling_shop_std.h"
#include "std/tree.h"

#define STYLE_SKIN UI_SKIN "styleshop\\"

enum StyleKind {
    STYLE_HEAD_PALETTE = 1,
    STYLE_HEAD_STYLE = 2,
    STYLE_BODY_PALETTE = 3,
    STYLE_TOP_ACCESSORY = 4,
    STYLE_MID_ACCESSORY = 5,
    STYLE_BOTTOM_ACCESSORY = 6,
    STYLE_BODY_STYLE = 7,
};

#define WND_STYLING_SHOP 0x119
#define STYLE_TAB_HAIR 0xc0
#define STYLE_TAB_MY_STYLE 0x1e8
#define STYLE_TAB_ACCESSORY 0x1e9
#define STYLE_TAB_2ND_COSTUME 0x1ea
#define STYLE_ID_PALETTE 0x1e7
#define STYLE_ID_BUY 0xd8
#define STYLE_ID_ROW 0xb5
#define STYLE_ID_RESET 0xb1
#define STYLE_ID_TURN_RIGHT 0xda
#define STYLE_ID_TURN_LEFT 0xdb
#define STYLE_ID_PALETTE_PREV 0x15a
#define STYLE_ID_PALETTE_NEXT 0x15b
#define STYLE_ID_FAVOURITE 0x15c
#define STYLE_ID_HAIR_PREV 0x1ba
#define STYLE_ID_HAIR_NEXT 0x1bb
#define STYLE_MSG_ACCESSORY 0x65
#define STYLE_MSG_MY_STYLE_APPLY 0x79
#define STYLE_PACKET_BUY 0xafc
#define STYLE_PACKET_CLOSE 0xa48

#pragma pack(push, 1)
struct PACKET_CZ_REQ_BUY_STYLINGSHOP2 {
    short packet_type;
    short look[STYLE_BODY_STYLE];
};
struct PACKET_CZ_CLOSE_UI_STYLINGSHOP {
    short packet_type;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_CZ_REQ_BUY_STYLINGSHOP2, 16);
SIZE_ASSERT(PACKET_CZ_CLOSE_UI_STYLINGSHOP, 2);

struct UIDisplayCharacter {
    struct UIWindow base;
    const char *shadow_bitmap;
    int slot;
    int direction;
    int robe;
    int head_style;
    int body_style;
    int head_palette;
    int body_palette;
    int accessory_top;
    int accessory_mid;
    int accessory_bottom;
    DWORD start_time;
    struct UIBitmapButton *favourite;
    BYTE no_buttons;
    int draw_x, draw_y;
    int goods_top, goods_mid, goods_bottom;
};
struct UIDisplayCharacter *ui_display_character_construct(struct UIDisplayCharacter *self);
void ui_display_character_copy_look(struct UIDisplayCharacter *self,
                                            const struct UIDisplayCharacter *other);

#define SHOW_INFO_ROWS 7
struct UIShowInfoWnd {
    struct UIWindow base;
    struct UIDisplayCharacter *display;
    struct UICheckBox *boxes[SHOW_INFO_ROWS];
    BYTE show_hint;
    BYTE draw_icons;
    BYTE draw_lines;
    int row_top;
    int font_height;
    BYTE all_ok;
    BYTE added[6];
    struct List kinds;
    BYTE added_body_style;
    int icon_x, icon_y;
    int text_x, text_y;
    const char *row_bitmap;
};
struct UIShowInfoWnd *ui_show_info_wnd_construct(struct UIShowInfoWnd *self);
void ui_show_info_wnd_setup(struct UIShowInfoWnd *self, const int *tab);
BYTE show_info_buy(struct UIShowInfoWnd *self);

struct UIStylingTab {
    struct UIWindow base;
    struct UIDisplayCharacter *display;
    struct UIShowInfoWnd *info;
};

struct UIBasicStylingWnd {
    struct UIStylingTab tab;
    struct Vec palette;
    struct UIBitmapButton *no_colour;
};

struct UI2ndCostumeWnd {
    struct UIStylingTab tab;
    struct UICheckBox *show_box;
    struct UICheckBox *hide_box;
};

#define MY_STYLE_SLOTS 3
struct UIMyStyleWnd {
    struct UIStylingTab tab;
    int selected;
    struct UIStylingTab *slots[MY_STYLE_SLOTS];
    struct UIBitmapButton *delete_btn[MY_STYLE_SLOTS];
    struct UIBitmapButton *rotate_left[MY_STYLE_SLOTS];
    struct UIBitmapButton *rotate_right[MY_STYLE_SLOTS];
};
void ui_my_style_wnd_load_slots(struct UIMyStyleWnd *self);
void ui_my_style_wnd_save_slots(struct UIMyStyleWnd *self);
BYTE my_style_add_slot(struct UIMyStyleWnd *self, struct UIDisplayCharacter *look);
BYTE my_style_all_ok(const struct UIMyStyleWnd *self);

struct UIAccessoryShowCase {
    struct UIScrollWnd scroll;
    struct Vec shadows;
};
void ui_accessory_show_case_fill_headgear(struct UIAccessoryShowCase *self);

struct UIAccBitmapButton {
    struct UIBitmapButton btn;
    int kind;
    int goods;
};

struct UIStylingShopWnd {
    struct UIFrameWnd frame;
    struct UIBitmapButton *buy;
    struct UIDisplayCharacter *display;
    struct Tree tabs;
};
struct StyleTab {
    struct UIWindow *button;
    struct UIStylingTab *tab;
};
struct UIStylingTab *styling_shop_find_tab(struct UIStylingShopWnd *shop, int id);
void styling_shop_set_buyable(struct UIStylingShopWnd *shop, BYTE all_ok);
BYTE styling_shop_save_look(struct UIStylingShopWnd *shop);
void styling_shop_refresh_favourite(struct UIStylingShopWnd *shop);
void display_set_favourite(struct UIDisplayCharacter *display, BYTE saved);
intptr_t ui_display_character_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                          intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4);
void ui_display_character_destroy(struct UIWindow *window);

struct String *style_kind_name(struct String *out, int kind);
int style_kind_value(const struct UIDisplayCharacter *display, int kind);
struct UIBitmapButton *style_add_button(struct UIWindow *parent, struct UIBitmapButton *btn, const char *up,
                                        const char *over, const char *down, int x, int y, int id);
void style_lua_size(const char *name, int *count);
void style_lua_seq(const char *name, int goods, int *seq);
struct StyleTab *style_my_style_tab(struct UIStylingShopWnd *shop);
struct UIDisplayCharacter *style_display_construct(struct UIDisplayCharacter *self, BYTE no_buttons);

extern const char g_styleNoColourText[];

void ui_stylingshop_tab_parent_wnd_destroy(struct UIWindow *window);
void ui_accessory_cus_wnd_destroy(struct UIWindow *window);
void ui_accessory_show_case_destroy(struct UIWindow *window);
void ui2nd_costume_wnd_destroy(struct UIWindow *window);
void ui_my_style_wnd_destroy(struct UIWindow *window);
void ui_slot_for_select_wnd_destroy(struct UIWindow *window);

void ui2nd_costume_wnd_on_create(struct UIWindow *window, int width, int height);
void ui2nd_costume_wnd_on_draw(struct UIWindow *window);
intptr_t ui2nd_costume_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, intptr_t val1,
                                       intptr_t val2, intptr_t val3, intptr_t val4);
void ui_accessory_cus_wnd_on_create(struct UIWindow *window, int width, int height);
void ui_accessory_cus_wnd_on_draw(struct UIWindow *window);
intptr_t ui_accessory_cus_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, intptr_t val1,
                                         intptr_t val2, intptr_t val3, intptr_t val4);
void ui_accessory_show_case_on_draw(struct UIWindow *window);
void ui_my_style_wnd_on_create(struct UIWindow *window, int width, int height);
void ui_my_style_wnd_on_draw(struct UIWindow *window);
intptr_t ui_my_style_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2,
                                    intptr_t val3, intptr_t val4);
void ui_stylingshop_tab_parent_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_stylingshop_tab_parent_wnd_on_draw(struct UIWindow *self);
intptr_t ui_stylingshop_tab_parent_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg,
                                                 intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4);

void ui_slot_for_select_wnd_on_l_btn_dbl_clk(struct UIWindow *self, int x, int y);
void ui_slot_for_select_wnd_on_l_btn_down(struct UIWindow *self, int x, int y);

struct UIStylingShopWnd *ui_styling_shop_wnd_construct(struct UIStylingShopWnd *self);
struct UIWindow *ui_styling_shop_wnd_new(void);

#endif
