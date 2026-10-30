#ifndef TEST_WINDOW_WRITER_CASES_H
#define TEST_WINDOW_WRITER_CASES_H
#include <stddef.h>
struct ReplayBuffer;
struct TreeNode;
extern const char *window_test_dialog_text;
extern struct TreeNode *window_test_tree[3];
void *window_allocate(size_t size);
void *window_resize(void *data, size_t size);
void window_release(void *data);
int test_write_UIChatRoomWnd_GetRecords(int variant, struct ReplayBuffer *out);
int test_write_UIJoinFriendAcceptWnd_GetRecords(int variant, struct ReplayBuffer *out);
int test_write_UIWindow_GetRecords(int variant, struct ReplayBuffer *out);
int test_write_UIFrameWnd_GetRecords(int variant, struct ReplayBuffer *out);
int test_write_UIItemIdentifyWnd_GetRecords(int variant, struct ReplayBuffer *out);
int test_write_UIMakingArrowListWnd_GetRecords(int variant, struct ReplayBuffer *out);
int test_write_UIItemCompositionWnd_GetRecords(int variant, struct ReplayBuffer *out);
int test_write_UIMakeTargetListWnd_GetRecords(int variant, struct ReplayBuffer *out);
int test_write_UIMakeTargetProcessWnd_GetRecords(int variant, struct ReplayBuffer *out);
int test_write_UIItemRepairWnd_GetRecords(int variant, struct ReplayBuffer *out);
int test_write_UIWeaponRefineWnd_GetRecords(int variant, struct ReplayBuffer *out);
int test_write_UIMerchantMirrorItemWnd_GetRecords(int variant, struct ReplayBuffer *out);
int test_write_UIMerchantItemPurchaseWnd_GetRecords(int variant, struct ReplayBuffer *out);
int test_write_UIMerchantItemShopWnd_GetRecords(int variant, struct ReplayBuffer *out);
int test_write_UIMerchantItemMyShopWnd_GetRecords(int variant, struct ReplayBuffer *out);
int test_write_UIMerchantShopMakeWnd_GetRecords(int variant, struct ReplayBuffer *out);
int test_write_UIItemPurchaseWnd_GetRecords(int variant, struct ReplayBuffer *out);
int test_write_UIItemSellWnd_GetRecords(int variant, struct ReplayBuffer *out);
int test_write_UIItemSellWnd2_GetRecords(int variant, struct ReplayBuffer *out);
int test_write_UIItemStoreWnd_GetRecords(int variant, struct ReplayBuffer *out);
int test_write_UIExchangeWnd_GetRecords(int variant, struct ReplayBuffer *out);
int test_write_UIJoinPartyAcceptWnd_GetRecords(int variant, struct ReplayBuffer *out);
int test_write_UIJoinGuildAcceptWnd_GetRecords(int variant, struct ReplayBuffer *out);
int test_write_UIBabyAcceptWnd_GetRecords(int variant, struct ReplayBuffer *out);
int test_write_UIBabyAcceptWnd2_GetRecords(int variant, struct ReplayBuffer *out);
int test_write_UIStarPlaceAcceptWnd_GetRecords(int variant, struct ReplayBuffer *out);
int test_write_UIChooseSellBuyWnd_GetRecords(int variant, struct ReplayBuffer *out);
int test_write_UIExchangeAcceptWnd_GetRecords(int variant, struct ReplayBuffer *out);
int test_write_UIWindowMgr_GetOptionRecords(int variant, struct ReplayBuffer *out);
int test_write_UIWindowMgr_GetEmptyRecords(int variant, struct ReplayBuffer *out);
int test_write_UIWindowMgr_GetWindowRecords(int variant, struct ReplayBuffer *out);
struct WindowWriterCase {
    const char *name;
    int (*write)(int variant, struct ReplayBuffer *out);
    int variants;
};
#ifdef WINDOW_WRITER_CASE_TABLE
static const struct WindowWriterCase window_writer_cases[] = {
    {"UIChatRoomWnd_GetRecords", test_write_UIChatRoomWnd_GetRecords, 1},
    {"UIJoinFriendAcceptWnd_GetRecords", test_write_UIJoinFriendAcceptWnd_GetRecords, 1},
    {"UIWindow_GetRecords", test_write_UIWindow_GetRecords, 1},
    {"UIFrameWnd_GetRecords", test_write_UIFrameWnd_GetRecords, 1},
    {"UIItemIdentifyWnd_GetRecords", test_write_UIItemIdentifyWnd_GetRecords, 1},
    {"UIMakingArrowListWnd_GetRecords", test_write_UIMakingArrowListWnd_GetRecords, 1},
    {"UIItemCompositionWnd_GetRecords", test_write_UIItemCompositionWnd_GetRecords, 1},
    {"UIMakeTargetListWnd_GetRecords", test_write_UIMakeTargetListWnd_GetRecords, 1},
    {"UIMakeTargetProcessWnd_GetRecords", test_write_UIMakeTargetProcessWnd_GetRecords, 1},
    {"UIItemRepairWnd_GetRecords", test_write_UIItemRepairWnd_GetRecords, 1},
    {"UIWeaponRefineWnd_GetRecords", test_write_UIWeaponRefineWnd_GetRecords, 1},
    {"UIMerchantMirrorItemWnd_GetRecords", test_write_UIMerchantMirrorItemWnd_GetRecords, 1},
    {"UIMerchantItemPurchaseWnd_GetRecords", test_write_UIMerchantItemPurchaseWnd_GetRecords, 2},
    {"UIMerchantItemShopWnd_GetRecords", test_write_UIMerchantItemShopWnd_GetRecords, 2},
    {"UIMerchantItemMyShopWnd_GetRecords", test_write_UIMerchantItemMyShopWnd_GetRecords, 2},
    {"UIMerchantShopMakeWnd_GetRecords", test_write_UIMerchantShopMakeWnd_GetRecords, 2},
    {"UIItemPurchaseWnd_GetRecords", test_write_UIItemPurchaseWnd_GetRecords, 1},
    {"UIItemSellWnd_GetRecords", test_write_UIItemSellWnd_GetRecords, 1},
    {"UIItemSellWnd2_GetRecords", test_write_UIItemSellWnd2_GetRecords, 1},
    {"UIItemStoreWnd_GetRecords", test_write_UIItemStoreWnd_GetRecords, 1},
    {"UIExchangeWnd_GetRecords", test_write_UIExchangeWnd_GetRecords, 1},
    {"UIJoinPartyAcceptWnd_GetRecords", test_write_UIJoinPartyAcceptWnd_GetRecords, 1},
    {"UIJoinGuildAcceptWnd_GetRecords", test_write_UIJoinGuildAcceptWnd_GetRecords, 1},
    {"UIBabyAcceptWnd_GetRecords", test_write_UIBabyAcceptWnd_GetRecords, 1},
    {"UIBabyAcceptWnd2_GetRecords", test_write_UIBabyAcceptWnd2_GetRecords, 1},
    {"UIStarPlaceAcceptWnd_GetRecords", test_write_UIStarPlaceAcceptWnd_GetRecords, 1},
    {"UIChooseSellBuyWnd_GetRecords", test_write_UIChooseSellBuyWnd_GetRecords, 1},
    {"UIExchangeAcceptWnd_GetRecords", test_write_UIExchangeAcceptWnd_GetRecords, 1},
    {"UIWindowMgr_GetOptionRecords", test_write_UIWindowMgr_GetOptionRecords, 1},
    {"UIWindowMgr_GetEmptyRecords", test_write_UIWindowMgr_GetEmptyRecords, 1},
    {"UIWindowMgr_GetWindowRecords", test_write_UIWindowMgr_GetWindowRecords, 1},
};
#endif
#endif
