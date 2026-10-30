#ifndef UI_WINDOW_RECORDS_H
#define UI_WINDOW_RECORDS_H
#include "replay/snapshot.h"
#include "ui/record_stream.h"
struct UIAcceptWnd;
struct UIChatRoomWnd;
struct UIExchangeWnd;
struct UIItemCompositionWnd;
struct UIItemIdentifyWnd;
struct UIItemPurchaseWnd;
struct UIItemRepairWnd;
struct UIItemSellWnd;
struct UIItemSellWnd2;
struct UIItemStoreWnd;
struct UIJoinFriendAcceptWnd;
struct UIMakeTargetListWnd;
struct UIMakeTargetProcessWnd;
struct UIMakingArrowListWnd;
struct UIMerchantItemMyShopWnd;
struct UIMerchantItemPurchaseWnd;
struct UIMerchantItemShopWnd;
struct UIMerchantMirrorItemWnd;
struct UIMerchantShopMakeWnd;
struct UIStarPlaceAcceptWnd;
struct UIWeaponRefineWnd;
struct UIWindow;
struct UIWindowMgr;

int ui_chat_room_wnd_get_records(struct UIWindow *self, struct ReplayBuffer *out);
int ui_join_friend_accept_wnd_get_records(struct UIWindow *self, struct ReplayBuffer *out);
int ui_window_get_records_base(struct UIWindow *self, struct ReplayBuffer *out);
int ui_frame_wnd_get_records(struct UIWindow *self, struct ReplayBuffer *out);
int ui_item_identify_wnd_get_records(struct UIWindow *self, struct ReplayBuffer *out);
int ui_making_arrow_list_wnd_get_records(struct UIWindow *self, struct ReplayBuffer *out);
int ui_item_composition_wnd_get_records(struct UIWindow *self, struct ReplayBuffer *out);
int ui_make_target_list_wnd_get_records(struct UIWindow *self, struct ReplayBuffer *out);
int ui_make_target_process_wnd_get_records(struct UIWindow *self, struct ReplayBuffer *out);
int ui_item_repair_wnd_get_records(struct UIWindow *self, struct ReplayBuffer *out);
int ui_weapon_refine_wnd_get_records(struct UIWindow *self, struct ReplayBuffer *out);
int ui_merchant_mirror_item_wnd_get_records(struct UIWindow *self, struct ReplayBuffer *out);
int ui_merchant_item_purchase_wnd_get_records(struct UIWindow *self, struct ReplayBuffer *out);
int ui_merchant_item_shop_wnd_get_records(struct UIWindow *self, struct ReplayBuffer *out);
int ui_merchant_item_my_shop_wnd_get_records(struct UIWindow *self, struct ReplayBuffer *out);
int ui_merchant_shop_make_wnd_get_records(struct UIWindow *self, struct ReplayBuffer *out);
int ui_item_purchase_wnd_get_records(struct UIWindow *self, struct ReplayBuffer *out);
int ui_item_sell_wnd_get_records(struct UIWindow *self, struct ReplayBuffer *out);
int ui_item_sell_wnd2_get_records(struct UIWindow *self, struct ReplayBuffer *out);
int ui_item_store_wnd_get_records(struct UIWindow *self, struct ReplayBuffer *out);
int ui_exchange_wnd_get_records(struct UIWindow *self, struct ReplayBuffer *out);
int ui_join_party_accept_wnd_get_records(struct UIWindow *self, struct ReplayBuffer *out);
int ui_join_guild_accept_wnd_get_records(struct UIWindow *self, struct ReplayBuffer *out);
int ui_baby_accept_wnd_get_records(struct UIWindow *self, struct ReplayBuffer *out);
int ui_baby_accept_wnd2_get_records(struct UIWindow *self, struct ReplayBuffer *out);
int ui_star_place_accept_wnd_get_records(struct UIWindow *self, struct ReplayBuffer *out);
int ui_choose_sell_buy_wnd_get_records(struct UIWindow *self, struct ReplayBuffer *out);
int ui_exchange_accept_wnd_get_records(struct UIWindow *self, struct ReplayBuffer *out);
int ui_window_mgr_get_option_records(struct UIWindowMgr *self, struct ReplayBuffer *out);
int ui_window_mgr_get_empty_records(struct UIWindowMgr *self, struct ReplayBuffer *out);
int ui_window_mgr_get_window_records(struct UIWindowMgr *self, struct ReplayBuffer *out);
int ui_window_mgr_load_option_records(struct UIWindowMgr *self, const void *data, size_t size);
int ui_window_mgr_load_window_records(struct UIWindowMgr *self, const void *data, size_t size);
int ui_window_mgr_skip_empty_records(struct UIWindowMgr *self, const void *data, size_t size);
struct ReplayRecorder;
int ui_window_snapshots_restore(struct UIWindowMgr *self, struct ReplayRecorder *recorder);

int ui_dialog_records_save(struct UIWindow *viewer, const char *question, uint16_t begin, uint16_t name,
                           uint16_t end, struct ReplayBuffer *out);
#endif
