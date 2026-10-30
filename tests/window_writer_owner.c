#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "ui/window.h"
#include "replay/snapshot.h"
#include "window_writer_cases.h"
static void fixture_text(struct String *s, const char *text)
{
    memset(s, 0, sizeof *s);
    s->size = s->capacity = strlen(text);
    s->data = (char *)text;
}
#ifndef WINDOW_WRITER_ORACLE
#define malloc window_allocate
#define realloc window_resize
#define free window_release
#endif

#if WINDOW_WRITER_OWNER == 1
#include "../src/ui/chat/chat_room.c"
struct MemberNode {
    struct TreeNode node;
    struct ChatRoomMember member;
};
int test_write_UIChatRoomWnd_GetRecords(int variant, struct ReplayBuffer *out)
{
    struct UIChatRoomWnd self = {0};
    struct WindowInfo info = {11, -22, 333, 444};
    struct TreeNode head = {0};
    struct MemberNode members[2] = {0};
    self.info = &info;
    fixture_text(&self.room.title, "A saved room");
    fixture_text(&self.room.password, "secret");
    self.room.type = 2;
    self.room.users = 3;
    self.room.max_users = 10;
    self.room.room_id = 0x12345678;
    self.members.head = &head;
    self.members.size = 2;
    head.left = &members[0].node;
    fixture_text(&members[0].member.name, "alpha");
    fixture_text(&members[1].member.name, "beta");
    members[1].member.value = 1;
    window_test_tree[0] = &head;
    window_test_tree[1] = &members[0].node;
    window_test_tree[2] = &members[1].node;
    (void)variant;
#ifdef WINDOW_WRITER_ORACLE
    {
        struct Vec bytes = {0};
        ui_chat_room_wnd_get_records(&self, &bytes);
        out->data = bytes.first;
        out->size = bytes.first ? (size_t)((char *)bytes.last - (char *)bytes.first) : 0;
        return 1;
    }
#else
    return ui_chat_room_wnd_get_records((struct UIWindow *)(&self), out);
#endif
}

#endif

#if WINDOW_WRITER_OWNER == 2
#include "../src/ui/chat/friend_request.c"
int test_write_UIJoinFriendAcceptWnd_GetRecords(int variant, struct ReplayBuffer *out)
{
    struct UIJoinFriendAcceptWnd self = {0};
    self.accept.text = (struct UIWindow *)window_test_dialog_text;
    (void)variant;
#ifdef WINDOW_WRITER_ORACLE
    {
        struct Vec bytes = {0};
        ui_join_friend_accept_wnd_get_records(&self, &bytes);
        out->data = bytes.first;
        out->size = bytes.first ? (size_t)((char *)bytes.last - (char *)bytes.first) : 0;
        return 1;
    }
#else
    return ui_join_friend_accept_wnd_get_records((struct UIWindow *)(&self), out);
#endif
}

#endif

#if WINDOW_WRITER_OWNER == 3
#include "../src/ui/controls/window_defaults.c"
int test_write_UIWindow_GetRecords(int variant, struct ReplayBuffer *out)
{
    struct UIWindow self = {0};

    (void)variant;
#ifdef WINDOW_WRITER_ORACLE
    {
        struct Vec bytes = {0};
        ui_window_get_records_base(&self, &bytes);
        out->data = bytes.first;
        out->size = bytes.first ? (size_t)((char *)bytes.last - (char *)bytes.first) : 0;
        return 1;
    }
#else
    return ui_window_get_records_base(&self, out);
#endif
}
int test_write_UIFrameWnd_GetRecords(int variant, struct ReplayBuffer *out)
{
    struct UIWindow self = {0};

    (void)variant;
#ifdef WINDOW_WRITER_ORACLE
    {
        struct Vec bytes = {0};
        ui_frame_wnd_get_records(&self, &bytes);
        out->data = bytes.first;
        out->size = bytes.first ? (size_t)((char *)bytes.last - (char *)bytes.first) : 0;
        return 1;
    }
#else
    return ui_frame_wnd_get_records(&self, out);
#endif
}

#endif

#if WINDOW_WRITER_OWNER == 4
#include "../src/ui/items/item_identify.c"
int test_write_UIItemIdentifyWnd_GetRecords(int variant, struct ReplayBuffer *out)
{
    struct UIItemIdentifyWnd self = {0};
    struct { struct ListNode links; struct ItemInfo info; } head = {0}, nodes[3] = {0};
    int i;
    self.items.head = &head.links;
    self.items.size = 3;
    head.links.next = &nodes[0].links;
    for (i = 0; i < 3; ++i) {
        nodes[i].links.next = i == 2 ? &head.links : &nodes[i + 1].links;
        nodes[i].info.index = i == 0 ? 1 : i == 1 ? 0x1234 : -1;
    }

    (void)variant;
#ifdef WINDOW_WRITER_ORACLE
    {
        struct Vec bytes = {0};
        ui_item_identify_wnd_get_records(&self, &bytes);
        out->data = bytes.first;
        out->size = bytes.first ? (size_t)((char *)bytes.last - (char *)bytes.first) : 0;
        return 1;
    }
#else
    return ui_item_identify_wnd_get_records((struct UIWindow *)(&self), out);
#endif
}

#endif

#if WINDOW_WRITER_OWNER == 5
#include "../src/ui/items/item_list_windows.c"
int test_write_UIMakingArrowListWnd_GetRecords(int variant, struct ReplayBuffer *out)
{
    struct UIMakingArrowListWnd self = {0};
    struct { struct ListNode links; struct ItemInfo info; } head = {0}, nodes[3] = {0};
    int i;
    self.list.items.head = &head.links;
    self.list.items.size = 3;
    head.links.next = &nodes[0].links;
    for (i = 0; i < 3; ++i) {
        nodes[i].links.next = i == 2 ? &head.links : &nodes[i + 1].links;
        nodes[i].info.index = i == 0 ? 1 : i == 1 ? 0x1234 : -1;
    }
    fixture_text(&nodes[0].info.id_text, "501");
    fixture_text(&nodes[1].info.id_text, "-30");
    fixture_text(&nodes[2].info.id_text, "70000");
    self.kind = 7;
    (void)variant;
#ifdef WINDOW_WRITER_ORACLE
    {
        struct Vec bytes = {0};
        ui_making_arrow_list_wnd_get_records(&self, &bytes);
        out->data = bytes.first;
        out->size = bytes.first ? (size_t)((char *)bytes.last - (char *)bytes.first) : 0;
        return 1;
    }
#else
    return ui_making_arrow_list_wnd_get_records((struct UIWindow *)(&self), out);
#endif
}
int test_write_UIItemCompositionWnd_GetRecords(int variant, struct ReplayBuffer *out)
{
    struct UIItemCompositionWnd self = {0};
    struct { struct ListNode links; struct ItemInfo info; } head = {0}, nodes[3] = {0};
    int i;
    self.list.items.head = &head.links;
    self.list.items.size = 3;
    head.links.next = &nodes[0].links;
    for (i = 0; i < 3; ++i) {
        nodes[i].links.next = i == 2 ? &head.links : &nodes[i + 1].links;
        nodes[i].info.index = i == 0 ? 1 : i == 1 ? 0x1234 : -1;
    }

    (void)variant;
#ifdef WINDOW_WRITER_ORACLE
    {
        struct Vec bytes = {0};
        ui_item_composition_wnd_get_records(&self, &bytes);
        out->data = bytes.first;
        out->size = bytes.first ? (size_t)((char *)bytes.last - (char *)bytes.first) : 0;
        return 1;
    }
#else
    return ui_item_composition_wnd_get_records((struct UIWindow *)(&self), out);
#endif
}

#endif

#if WINDOW_WRITER_OWNER == 6
#include "../src/ui/items/make_target.c"
int test_write_UIMakeTargetListWnd_GetRecords(int variant, struct ReplayBuffer *out)
{
    struct UIMakeTargetListWnd self = {0};
    int ids[] = {-10, 501, 70000};
    self.ids.first = ids;
    self.ids.last = self.ids.end = ids + 3;
    (void)variant;
#ifdef WINDOW_WRITER_ORACLE
    {
        struct Vec bytes = {0};
        ui_make_target_list_wnd_get_records(&self, &bytes);
        out->data = bytes.first;
        out->size = bytes.first ? (size_t)((char *)bytes.last - (char *)bytes.first) : 0;
        return 1;
    }
#else
    return ui_make_target_list_wnd_get_records((struct UIWindow *)(&self), out);
#endif
}
int test_write_UIMakeTargetProcessWnd_GetRecords(int variant, struct ReplayBuffer *out)
{
    struct UIMakeTargetProcessWnd self = {0};
    self.item_id = 0x12345678;
    (void)variant;
#ifdef WINDOW_WRITER_ORACLE
    {
        struct Vec bytes = {0};
        ui_make_target_process_wnd_get_records(&self, &bytes);
        out->data = bytes.first;
        out->size = bytes.first ? (size_t)((char *)bytes.last - (char *)bytes.first) : 0;
        return 1;
    }
#else
    return ui_make_target_process_wnd_get_records((struct UIWindow *)(&self), out);
#endif
}

#endif

#if WINDOW_WRITER_OWNER == 7
#include "../src/ui/items/repair_refine.c"
int test_write_UIItemRepairWnd_GetRecords(int variant, struct ReplayBuffer *out)
{
    struct UIItemRepairWnd self = {0};
    struct { struct ListNode links; struct RepairItem item; } head = {0}, nodes[2] = {0};
    self.items.head = &head.links;
    self.items.size = 2;
    head.links.next = &nodes[0].links;
    nodes[0].links.next = &nodes[1].links;
    nodes[1].links.next = &head.links;
    nodes[0].item.index = 3;
    nodes[0].item.item_id = 501;
    memset(nodes[0].item.cards, 0x43, sizeof nodes[0].item.cards);
    nodes[0].item.refine = nodes[0].item.grade = 0x43;
    nodes[1].item.index = -7;
    nodes[1].item.item_id = 70000;
    memset(nodes[1].item.cards, 0x98, sizeof nodes[1].item.cards);
    nodes[1].item.refine = nodes[1].item.grade = 0x98;
    (void)variant;
#ifdef WINDOW_WRITER_ORACLE
    {
        struct Vec bytes = {0};
        ui_item_repair_wnd_get_records(&self, &bytes);
        out->data = bytes.first;
        out->size = bytes.first ? (size_t)((char *)bytes.last - (char *)bytes.first) : 0;
        return 1;
    }
#else
    return ui_item_repair_wnd_get_records((struct UIWindow *)(&self), out);
#endif
}
int test_write_UIWeaponRefineWnd_GetRecords(int variant, struct ReplayBuffer *out)
{
    struct UIWeaponRefineWnd self = {0};
    int ids[] = {501, 502, 70000}, indices[] = {1, -1, 0x1234};
    self.pick.item_ids.first = ids;
    self.pick.item_ids.last = self.pick.item_ids.end = ids + 3;
    self.pick.indices.first = indices;
    self.pick.indices.last = self.pick.indices.end = indices + 3;
    (void)variant;
#ifdef WINDOW_WRITER_ORACLE
    {
        struct Vec bytes = {0};
        ui_weapon_refine_wnd_get_records(&self, &bytes);
        out->data = bytes.first;
        out->size = bytes.first ? (size_t)((char *)bytes.last - (char *)bytes.first) : 0;
        return 1;
    }
#else
    return ui_weapon_refine_wnd_get_records((struct UIWindow *)(&self), out);
#endif
}

#endif

#if WINDOW_WRITER_OWNER == 8
#include "../src/ui/shops/merchant.c"
int test_write_UIMerchantMirrorItemWnd_GetRecords(int variant, struct ReplayBuffer *out)
{
    struct UIMerchantMirrorItemWnd self = {0};

    (void)variant;
#ifdef WINDOW_WRITER_ORACLE
    {
        struct Vec bytes = {0};
        ui_merchant_mirror_item_wnd_get_records(&self, &bytes);
        out->data = bytes.first;
        out->size = bytes.first ? (size_t)((char *)bytes.last - (char *)bytes.first) : 0;
        return 1;
    }
#else
    return ui_merchant_mirror_item_wnd_get_records((struct UIWindow *)(&self), out);
#endif
}

#endif

#if WINDOW_WRITER_OWNER == 9
#include "../src/ui/shops/merchant_purchase.c"
int test_write_UIMerchantItemPurchaseWnd_GetRecords(int variant, struct ReplayBuffer *out)
{
    struct UIMerchantItemPurchaseWnd self = {0};
    self.variant = (BYTE)variant;
    (void)variant;
#ifdef WINDOW_WRITER_ORACLE
    {
        struct Vec bytes = {0};
        ui_merchant_item_purchase_wnd_get_records(&self, &bytes);
        out->data = bytes.first;
        out->size = bytes.first ? (size_t)((char *)bytes.last - (char *)bytes.first) : 0;
        return 1;
    }
#else
    return ui_merchant_item_purchase_wnd_get_records((struct UIWindow *)(&self), out);
#endif
}

#endif

#if WINDOW_WRITER_OWNER == 10
#include "../src/ui/shops/merchant_shop.c"
int test_write_UIMerchantItemShopWnd_GetRecords(int variant, struct ReplayBuffer *out)
{
    struct UIMerchantItemShopWnd self = {0};
    self.variant = (BYTE)variant;
    self.notice_amount = -1234567;
    (void)variant;
#ifdef WINDOW_WRITER_ORACLE
    {
        struct Vec bytes = {0};
        ui_merchant_item_shop_wnd_get_records(&self, &bytes);
        out->data = bytes.first;
        out->size = bytes.first ? (size_t)((char *)bytes.last - (char *)bytes.first) : 0;
        return 1;
    }
#else
    return ui_merchant_item_shop_wnd_get_records((struct UIWindow *)(&self), out);
#endif
}
int test_write_UIMerchantItemMyShopWnd_GetRecords(int variant, struct ReplayBuffer *out)
{
    struct UIMerchantItemMyShopWnd self = {0};
    self.variant = (BYTE)variant;
    (void)variant;
#ifdef WINDOW_WRITER_ORACLE
    {
        struct Vec bytes = {0};
        ui_merchant_item_my_shop_wnd_get_records(&self, &bytes);
        out->data = bytes.first;
        out->size = bytes.first ? (size_t)((char *)bytes.last - (char *)bytes.first) : 0;
        return 1;
    }
#else
    return ui_merchant_item_my_shop_wnd_get_records((struct UIWindow *)(&self), out);
#endif
}

#endif

#if WINDOW_WRITER_OWNER == 11
#include "../src/ui/shops/merchant_shop_make.c"
int test_write_UIMerchantShopMakeWnd_GetRecords(int variant, struct ReplayBuffer *out)
{
    struct UIMerchantShopMakeWnd self = {0};
    self.buying = (BYTE)variant;
    (void)variant;
#ifdef WINDOW_WRITER_ORACLE
    {
        struct Vec bytes = {0};
        ui_merchant_shop_make_wnd_get_records(&self, &bytes);
        out->data = bytes.first;
        out->size = bytes.first ? (size_t)((char *)bytes.last - (char *)bytes.first) : 0;
        return 1;
    }
#else
    return ui_merchant_shop_make_wnd_get_records((struct UIWindow *)(&self), out);
#endif
}

#endif

#if WINDOW_WRITER_OWNER == 12
#include "../src/ui/shops/npc_shop_purchase.c"
int test_write_UIItemPurchaseWnd_GetRecords(int variant, struct ReplayBuffer *out)
{
    struct UIItemPurchaseWnd self = {0};

    (void)variant;
#ifdef WINDOW_WRITER_ORACLE
    {
        struct Vec bytes = {0};
        ui_item_purchase_wnd_get_records(&self, &bytes);
        out->data = bytes.first;
        out->size = bytes.first ? (size_t)((char *)bytes.last - (char *)bytes.first) : 0;
        return 1;
    }
#else
    return ui_item_purchase_wnd_get_records((struct UIWindow *)(&self), out);
#endif
}

#endif

#if WINDOW_WRITER_OWNER == 13
#include "../src/ui/shops/npc_shop_sell.c"
int test_write_UIItemSellWnd_GetRecords(int variant, struct ReplayBuffer *out)
{
    struct UIItemSellWnd self = {0};

    (void)variant;
#ifdef WINDOW_WRITER_ORACLE
    {
        struct Vec bytes = {0};
        ui_item_sell_wnd_get_records(&self, &bytes);
        out->data = bytes.first;
        out->size = bytes.first ? (size_t)((char *)bytes.last - (char *)bytes.first) : 0;
        return 1;
    }
#else
    return ui_item_sell_wnd_get_records((struct UIWindow *)(&self), out);
#endif
}
int test_write_UIItemSellWnd2_GetRecords(int variant, struct ReplayBuffer *out)
{
    struct UIItemSellWnd2 self = {0};
    self.window_type = -1234567;
    (void)variant;
#ifdef WINDOW_WRITER_ORACLE
    {
        struct Vec bytes = {0};
        ui_item_sell_wnd2_get_records(&self, &bytes);
        out->data = bytes.first;
        out->size = bytes.first ? (size_t)((char *)bytes.last - (char *)bytes.first) : 0;
        return 1;
    }
#else
    return ui_item_sell_wnd2_get_records((struct UIWindow *)(&self), out);
#endif
}

#endif

#if WINDOW_WRITER_OWNER == 14
#include "../src/ui/shops/storage.c"
int test_write_UIItemStoreWnd_GetRecords(int variant, struct ReplayBuffer *out)
{
    struct UIItemStoreWnd self = {0};

    (void)variant;
#ifdef WINDOW_WRITER_ORACLE
    {
        struct Vec bytes = {0};
        ui_item_store_wnd_get_records(&self, &bytes);
        out->data = bytes.first;
        out->size = bytes.first ? (size_t)((char *)bytes.last - (char *)bytes.first) : 0;
        return 1;
    }
#else
    return ui_item_store_wnd_get_records((struct UIWindow *)(&self), out);
#endif
}

#endif

#if WINDOW_WRITER_OWNER == 15
#include "../src/ui/shops/trade.c"
int test_write_UIExchangeWnd_GetRecords(int variant, struct ReplayBuffer *out)
{
    struct UIExchangeWnd self = {0};
    self.partner_cid = 0x12345678;
    self.partner_level = 321;
    fixture_text(&self.partner_name, "Snapshot Trader");
    self.locked_mine = 1;
    self.locked_partner = 2;
    self.pressed_ok = 0;
    self.pressed_exchange = -1;
    self.pressed_cancel = 7;
    (void)variant;
#ifdef WINDOW_WRITER_ORACLE
    {
        struct Vec bytes = {0};
        ui_exchange_wnd_get_records(&self, &bytes);
        out->data = bytes.first;
        out->size = bytes.first ? (size_t)((char *)bytes.last - (char *)bytes.first) : 0;
        return 1;
    }
#else
    return ui_exchange_wnd_get_records((struct UIWindow *)(&self), out);
#endif
}

#endif

#if WINDOW_WRITER_OWNER == 16
#include "../src/ui/social/accept.c"
int test_write_UIJoinPartyAcceptWnd_GetRecords(int variant, struct ReplayBuffer *out)
{
    struct UIAcceptWnd self = {0};
    self.text = (struct UIWindow *)window_test_dialog_text;
    (void)variant;
#ifdef WINDOW_WRITER_ORACLE
    {
        struct Vec bytes = {0};
        ui_join_party_accept_wnd_get_records(&self, &bytes);
        out->data = bytes.first;
        out->size = bytes.first ? (size_t)((char *)bytes.last - (char *)bytes.first) : 0;
        return 1;
    }
#else
    return ui_join_party_accept_wnd_get_records((struct UIWindow *)(&self), out);
#endif
}
int test_write_UIJoinGuildAcceptWnd_GetRecords(int variant, struct ReplayBuffer *out)
{
    struct UIAcceptWnd self = {0};
    self.text = (struct UIWindow *)window_test_dialog_text;
    (void)variant;
#ifdef WINDOW_WRITER_ORACLE
    {
        struct Vec bytes = {0};
        ui_join_guild_accept_wnd_get_records(&self, &bytes);
        out->data = bytes.first;
        out->size = bytes.first ? (size_t)((char *)bytes.last - (char *)bytes.first) : 0;
        return 1;
    }
#else
    return ui_join_guild_accept_wnd_get_records((struct UIWindow *)(&self), out);
#endif
}
int test_write_UIBabyAcceptWnd_GetRecords(int variant, struct ReplayBuffer *out)
{
    struct UIAcceptWnd self = {0};
    self.text = (struct UIWindow *)window_test_dialog_text;
    (void)variant;
#ifdef WINDOW_WRITER_ORACLE
    {
        struct Vec bytes = {0};
        ui_baby_accept_wnd_get_records(&self, &bytes);
        out->data = bytes.first;
        out->size = bytes.first ? (size_t)((char *)bytes.last - (char *)bytes.first) : 0;
        return 1;
    }
#else
    return ui_baby_accept_wnd_get_records((struct UIWindow *)(&self), out);
#endif
}
int test_write_UIBabyAcceptWnd2_GetRecords(int variant, struct ReplayBuffer *out)
{
    struct UIAcceptWnd self = {0};

    (void)variant;
#ifdef WINDOW_WRITER_ORACLE
    {
        struct Vec bytes = {0};
        ui_baby_accept_wnd2_get_records(&self, &bytes);
        out->data = bytes.first;
        out->size = bytes.first ? (size_t)((char *)bytes.last - (char *)bytes.first) : 0;
        return 1;
    }
#else
    return ui_baby_accept_wnd2_get_records((struct UIWindow *)(&self), out);
#endif
}
int test_write_UIStarPlaceAcceptWnd_GetRecords(int variant, struct ReplayBuffer *out)
{
    struct UIStarPlaceAcceptWnd self = {0};
    self.kind = 10;
    (void)variant;
#ifdef WINDOW_WRITER_ORACLE
    {
        struct Vec bytes = {0};
        ui_star_place_accept_wnd_get_records(&self, &bytes);
        out->data = bytes.first;
        out->size = bytes.first ? (size_t)((char *)bytes.last - (char *)bytes.first) : 0;
        return 1;
    }
#else
    return ui_star_place_accept_wnd_get_records((struct UIWindow *)(&self), out);
#endif
}

#endif

#if WINDOW_WRITER_OWNER == 17
#include "../src/ui/social/accept_base.c"
int test_write_UIChooseSellBuyWnd_GetRecords(int variant, struct ReplayBuffer *out)
{
    struct UIAcceptWnd self = {0};

    (void)variant;
#ifdef WINDOW_WRITER_ORACLE
    {
        struct Vec bytes = {0};
        ui_choose_sell_buy_wnd_get_records(&self, &bytes);
        out->data = bytes.first;
        out->size = bytes.first ? (size_t)((char *)bytes.last - (char *)bytes.first) : 0;
        return 1;
    }
#else
    return ui_choose_sell_buy_wnd_get_records((struct UIWindow *)(&self), out);
#endif
}
int test_write_UIExchangeAcceptWnd_GetRecords(int variant, struct ReplayBuffer *out)
{
    struct UIAcceptWnd self = {0};
    self.text = (struct UIWindow *)window_test_dialog_text;
    (void)variant;
#ifdef WINDOW_WRITER_ORACLE
    {
        struct Vec bytes = {0};
        ui_exchange_accept_wnd_get_records(&self, &bytes);
        out->data = bytes.first;
        out->size = bytes.first ? (size_t)((char *)bytes.last - (char *)bytes.first) : 0;
        return 1;
    }
#else
    return ui_exchange_accept_wnd_get_records((struct UIWindow *)(&self), out);
#endif
}

#endif

#if WINDOW_WRITER_OWNER == 18
#include "../src/ui/wndmgr/records.c"
int test_write_UIWindowMgr_GetOptionRecords(int variant, struct ReplayBuffer *out)
{
    struct UIWindowMgr self = {0};
    self.drag_all = 0x12345678;
    (void)variant;
#ifdef WINDOW_WRITER_ORACLE
    {
        struct Vec bytes = {0};
        ui_window_mgr_get_option_records(&self, &bytes);
        out->data = bytes.first;
        out->size = bytes.first ? (size_t)((char *)bytes.last - (char *)bytes.first) : 0;
        return 1;
    }
#else
    return ui_window_mgr_get_option_records(&self, out);
#endif
}
int test_write_UIWindowMgr_GetEmptyRecords(int variant, struct ReplayBuffer *out)
{
    struct UIWindowMgr self = {0};

    (void)variant;
#ifdef WINDOW_WRITER_ORACLE
    {
        struct Vec bytes = {0};
        ui_window_mgr_get_empty_records(&self, &bytes);
        out->data = bytes.first;
        out->size = bytes.first ? (size_t)((char *)bytes.last - (char *)bytes.first) : 0;
        return 1;
    }
#else
    return ui_window_mgr_get_empty_records(&self, out);
#endif
}
int test_write_UIWindowMgr_GetWindowRecords(int variant, struct ReplayBuffer *out)
{
    struct UIWindowMgr self = {0};
    struct ListNode head = {0};
    struct { struct ListNode links; struct UIWindow *window; } nodes[4] = {0};
    struct UIWindow windows[4] = {0};
    struct UIWindowOps table = {0};
    int i;
    table.get_records = ui_window_get_records_base;
    self.windows.head = &head;
    self.windows.size = 4;
    head.next = &nodes[0].links;
    for (i = 0; i < 4; ++i) {
        nodes[i].links.next = i == 3 ? &head : &nodes[i + 1].links;
        nodes[i].window = &windows[i];
        windows[i].ops = &table;
    }
    windows[0].id = 3;
    windows[1].id = 7;
    windows[2].id = 0x166;
    windows[3].id = 8;
    self.window_flags[3][7] = self.window_flags[7][7] = 1;
    (void)variant;
#ifdef WINDOW_WRITER_ORACLE
    {
        struct Vec bytes = {0};
        ui_window_mgr_get_window_records(&self, &bytes);
        out->data = bytes.first;
        out->size = bytes.first ? (size_t)((char *)bytes.last - (char *)bytes.first) : 0;
        return 1;
    }
#else
    return ui_window_mgr_get_window_records(&self, out);
#endif
}

#endif
#include "window_writer_unreached.h"
