#include "core/object_type_registry.h"
#include "core/object_type_tables.h"

/* The object types of src/ui/shops/: the shop, storage and vending windows and their controls.
   core/object_type.h describes the table. In the order of the class names. */
static const struct ObjectTypeRow k_rows[] = {
    {&ui_bank_new_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UIBank_NewWnd"},
    {&ui_barter_item_purchase_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UIBarterItemPurchaseWnd"},
    {&ui_bartermarket_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UIBartermarketWnd"},
    {&ui_bitmap_edit_ctrl_ops, 0, "UIBitmapEditCtrl"},
    {&ui_exchange_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UIExchangeWnd"},
    {&ui_expanded_barter_item_purchase_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UIExpandedBarterItemPurchaseWnd"},
    {&ui_expanded_barter_purchase_button_ops, 0, "UIExpandedBarterPurchaseButton"},
    {&ui_expanded_bartermarket_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UIExpandedBartermarketWnd"},
    {&ui_item_purchase_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UIItemPurchaseWnd"},
    {&ui_item_sell_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UIItemSellWnd"},
    {&ui_item_sell_wnd2_ops, OBJECT_TYPE(FRAME_WINDOW), "UIItemSellWnd2"},
    {&ui_item_shop_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UIItemShopWnd"},
    {&ui_item_shop_wnd2_ops, OBJECT_TYPE(FRAME_WINDOW), "UIItemShopWnd2"},
    {&ui_item_store_combo_box_ops, 0, "UIItemStoreComboBox"},
    {&ui_item_store_combo_box_wnd_ops, 0, "UIItemStoreComboBoxWnd"},
    {&ui_item_store_find_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UIItemStoreFindWnd"},
    {&ui_item_store_sub_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UIItemStoreSubWnd"},
    {&ui_item_store_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UIItemStoreWnd"},
    {&ui_merchant_item_log_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UIMerchantItemLogWnd"},
    {&ui_merchant_item_my_shop_wnd_ops, OBJECT_TYPE(FRAME_WINDOW) | OBJECT_TYPE(MY_SHOP_WINDOW), "UIMerchantItemMyShopWnd"},
    {&ui_merchant_item_purchase_msg_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UIMerchantItemPurchaseMSGWnd"},
    {&ui_merchant_item_purchase_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UIMerchantItemPurchaseWnd"},
    {&ui_merchant_item_shop_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UIMerchantItemShopWnd"},
    {&ui_merchant_item_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UIMerchantItemWnd"},
    {&ui_merchant_mirror_item_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UIMerchantMirrorItemWnd"},
    {&ui_merchant_shop_make_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UIMerchantShopMakeWnd"},
    {&ui_merchant_shop_title_ops, 0, "UIMerchantShopTitle"},
    {&ui_notice_message_box_lock_sell_item_ops, OBJECT_TYPE(FRAME_WINDOW), "UINoticeMessageBox_LockSellItem"},
    {&ui_para_item_purchase_wnd_ops, OBJECT_TYPE(FRAME_WINDOW) | OBJECT_TYPE(PARA_PURCHASE_WINDOW), "UIParaItemPurchaseWnd"},
    {&ui_para_item_shop_wnd_ops, OBJECT_TYPE(FRAME_WINDOW) | OBJECT_TYPE(PARA_SHOP_WINDOW), "UIParaItemShopWnd"},
    {&ui_para_result_wnd_ops, OBJECT_TYPE(FRAME_WINDOW) | OBJECT_TYPE(PARA_RESULT_WINDOW), "UIParaResultWnd"},
    {&ui_purchase_button_ops, 0, "UIPurchaseButton"},
    {&ui_serach_store_info_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UISerachStoreInfoWnd"},
};

const struct ObjectTypeRows g_shopsObjectTypes = OBJECT_TYPE_ROWS(k_rows);
