#include "ui/window_manager_state.h"
#ifndef UI_SHOPS_NPC_SHOP_INTERNAL_H
#define UI_SHOPS_NPC_SHOP_INTERNAL_H
#include "ui/shops/shops.h"

#define SHOP_TITLE_X 0x10
#define MSG_SHOP_TITLE 0xa7
#define MSG_SHOP_CHECK 0x127
#define MSG_PURCHASE_TITLE 0xa6
#define MSG_SELL_TITLE 0xa8
#define MSG_SELL2_TITLE 0x621
#define MSG_SHOP2_CONVERT_TITLE 0x620
#define MSG_PRICE_COUNT 0x24c
#define MSG_POINTS_CASH 0x4ba
#define MSG_POINTS_FREE 0x519
#define MSG_TIP_COUNT_OPTIONS 0xac6
#define MSG_TIP_COUNT_OPTIONS_SAME 0xac7
#define MSG_TIP_OPTIONS 0xac8
#define MSG_TIP_OPTIONS_SAME 0xac9

#define DRAG_SHOP_GOODS 3
#define DRAG_SHOP_PURCHASE 4
#define COUNT_FOR_SELL 2
#define COUNT_FOR_SELL2 0x10

#define PURCHASE_DEFAULT_ID 0x107
#define SHOP_MSG_SET_MODE 0x24
#define SHOP_RESIZE_EDGE 7
#define SHOP_MIN_WIDTH 0xe8
#define SHOP_MAX_WIDTH 0x140
#define SHOP_MIN_HEIGHT 0x78
#define SHOP_MAX_HEIGHT 0x150
#define SHOP_WIDTH_BASE 0x118

#define DRAG_SELL_LIST 5
#define COUNT_FOR_SHOP_PURCHASE 3
#define COUNT_FOR_SHOP_SELL 4
#define COUNT_FOR_SHOP2 0xf
#define COUNT_FOR_PURCHASE 1
#define MSG_SHOP_TITLE_COUNTED 0xb9
#define MSG_SHOP_TITLE_PLAIN 0xba
#define SHOP_CASH_EXTRA 0x14
#define MSG_PURCHASE_POINTS 0x51a
#define MSG_PURCHASE_HAVE_ONE 0x77
#define MSG_PURCHASE_LIMIT 0x6a8
#define MSG_NO_ZENY 0x37
#define MSG_NO_CASH 0x4e6
#define MSG_CASH_NO_FREE 0x531
#define MSG_CASH_CONFIRM 0xebb
#define MSG_CASH_CONFIRM_FREE 0xebc
#define MSG_SELL2_CONFIRM 0x630
#define PURCHASE_STACK_LIMIT 30000
#define CASH_PURCHASE_MAX 10
#define PURCHASE_ROWS_MARGIN 0x3a
#define WND_ID_SHOP2 0x16
#define WND_ID_PURCHASE 0x17
#define WND_ID_SELL 0x18
#define WND_ID_SELL2_SHOP 0xa0
#define WND_ID_SELL2 0xa1
#define MODE_REQ_SELL_CHECKED 0x23
#define MODE_REQ_BUY 0x22
#define MODE_REQ_SHOP_CLOSE 0x28
#define MODE_REQ_CASH_BUY 0xf9
#define MODE_REQ_SELL2_YES 0x10a
#define MODE_REQ_SELL2_NO 0x10b
#define UIWM_MSG_SHOW_CHAT 3
#define CHAT_RED 0xff

int purchase_info_usable(struct WindowInfo *self);
int sell_info_usable(struct WindowInfo *self);
void purchase_info_default(struct WindowInfo *self);
void sell_info_default(struct WindowInfo *self);

void npc_shop_price_unit_long(char *buf, int cash);
void npc_shop_show_price_tooltip(struct UIItemBaseWnd *base, int x, int y, const char *unit, int with_count,
                                 int options_ok);
void shop_draw_backdrop(struct UIItemBaseWnd *base, int body, int cell_rows);
void shop_draw_lines(struct UIItemBaseWnd *base, int hover, int priced, const char *unit);
void npc_shop_store_info(struct UIWindow *wnd, struct WindowInfo *info);
void npc_shop_drop_or_count(struct UIItemBaseWnd *base, int at_once, int val1, int for_what);
void npc_shop_pass_on(struct UIItemBaseWnd *base, int x, int y, struct UIWindow *target, int target_id);
void npc_shop_list_press(struct UIItemBaseWnd *base, int *hover, int x, int y, int from_sell);
void npc_shop_describe_hovered(struct UIItemBaseWnd *base, int *hover, int x, int y);
void npc_shop_describe_at(struct UIItemBaseWnd *base, int x, int y, int asks_mode);
int npc_shop_refill(struct UIItemBaseWnd *base, int count,
                    struct ItemInfo *(*at)(struct Session *, struct ItemInfo *, int));
void npc_shop_list_resize(struct UIItemBaseWnd *base, int width, int height, int margin);
void npc_shop_list_place_buttons(struct UIItemBaseWnd *base);
void npc_shop_list_set_info(struct UIItemBaseWnd *base, struct WindowInfo *info, int edge,
                            int (*usable)(struct WindowInfo *),
                            void (*make_default)(struct WindowInfo *));
int npc_shop_records_reach(const char *buf, unsigned size, short begin_tag, short end_tag);

#endif
