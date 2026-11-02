#include "session/shop_inventory.h"
#include "ui/item_draw.h"
#include "iteminfo/item_api.h"
#include "contents/mcstore_requests.h"
#include "contents/item_info_mgr.h"
#include "session/state.h"
#ifndef UI_UI_ITEMS_H
#define UI_UI_ITEMS_H

#include "iteminfo/item_types.h"
#include "iteminfo/item_table.h"
#include "ui/window.h"
#include <stdint.h>

struct Session;

#define UI_MSG_SHOW_ITEM 0x18

#define ITEM_LINK_EDIT_WHISPER 0x1e2
#define ITEM_LINK_EDIT_RODEX 0x1e3
#define ITEM_LINK_EDIT_MEGAPHONE 0x1e4
#define ITEM_LINK_EDIT_CHAT 0x1e5
#define ITEM_LINK_EDIT_CHAT_ROOM 0x1e6
void ui_chat_edit_ctrl_add_item_link(struct UIWindow *self,
                                           const struct ItemInfo *info);
void ui_rodex_compose_wnd_add_item_link(struct UIWindow *self, const struct ItemInfo *info);
struct UIRefiningWnd;
struct UIGradeEnchantWnd;
void ui_grade_enchant_wnd_reset(struct UIGradeEnchantWnd *self);

int zeny_fee_percent(int64_t price);

struct McStoreBackup;

int item_type_rank(int type);

#endif
