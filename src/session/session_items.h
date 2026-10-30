#include "session/shop_inventory.h"
#ifndef SESSION_SESSION_ITEMS_H
#define SESSION_SESSION_ITEMS_H

#include "session/csession.h"

unsigned equip_location_to_base(unsigned location);
int *equip_location_slots(int *out, unsigned location);
int *session_equip_slots_of(int *out, unsigned location);
BYTE session_set_worn_item(struct Session *self, const struct ItemInfo *item);
BYTE session_set_viewed_item(struct Session *self, const struct ItemInfo *item);
struct ItemInfo *session_get_inventory_item(struct Session *self, struct ItemInfo *out, int index);
BYTE session_add_inventory_item(struct Session *self, const struct ItemInfo *item, BYTE refresh);
BYTE session_add_cart_item(struct Session *self, const struct ItemInfo *item);
BYTE session_mirror_add(struct Session *self, const struct ItemInfo *item, int which, BYTE notify);
void session_add_own_store_item(struct Session *self, const struct ItemInfo *item);
void session_purchase_add(struct Session *self, const struct ItemInfo *item);
void session_add_other_store_item(struct Session *self, const struct ItemInfo *item);
void session_sellable_add(struct Session *self, const struct ItemInfo *item);
BYTE session_add_storage_item(struct Session *self, const struct ItemInfo *item, BYTE notify);
void session_add_shop_goods_by_name(struct Session *self, const struct ItemInfo *item);
void session_add_shop_buy_item(struct Session *self, const struct ItemInfo *item);
void session_add_shop_buy_item_keyed(struct Session *self, const struct ItemInfo *item);
struct ItemRecordA;
void session_append_market_goods(struct Session *self, const struct ItemRecordA *packetItem, int count, WORD weight);
void session_append_shop_goods_record(struct Session *self, const struct ItemRecordA *packetItem, int fromInventory);
void session_push_shop_goods(struct Session *self, const struct ItemInfo *item);
void session_add_shop_sell_item(struct Session *self, const struct ItemInfo *item);
void session_remove_inventory_other_types(struct Session *self);
void session_remove_inventory_filter_types(struct Session *self);
void session_clear_inventory(struct Session *self);
void session_clear_lists(struct Session *self);
void session_clear_purchase_lists(struct Session *self);
void session_clear_shop_buy_list(struct Session *self);
void session_clear_storage(struct Session *self);
void session_clear_cart(struct Session *self);
void session_clear_other_store_items(struct Session *self);
void session_clear_store_candidates(struct Session *self);
void session_clear_own_store_items(struct Session *self);
void session_clear_purchase_list(struct Session *self);
void session_clear_store_setup(struct Session *self);
void session_clear_item_list(struct Session *self, int which);
void session_clear_cash_shop_goods(struct Session *self);
void session_mirror_from_cart(struct Session *self);
void session_copy_store_purchase_to_sent(struct Session *self);
void session_clear_worn_items(struct Session *self);
void session_clear_viewed_items(struct Session *self);
void session_inventory_update(struct Session *self, const struct ItemInfo *item, BYTE notify);
void session_clear_inventory_new_flags(struct Session *self);
void session_fill_store_candidates(struct Session *self, int count, const short *indexes);

void session_remove_own_store_item_by_id(struct Session *self, struct ItemInfo *item);
void session_remove_shop_goods_by_id(struct Session *self, const struct ItemInfo *item);
void session_remove_inventory_item(struct Session *self, int index, int count);
BYTE session_remove_store_setup_item(struct Session *self, const struct ItemInfo *item, BYTE whole);
BYTE session_remove_cart_item(struct Session *self, const struct ItemInfo *item);
BYTE session_remove_storage_item(struct Session *self, const struct ItemInfo *item);
void session_remove_own_store_item(struct Session *self, struct ItemInfo *item);
void session_remove_store_purchase_item(struct Session *self, const struct ItemInfo *item);
void session_sell_list_remove(struct Session *self, const struct ItemInfo *item);
void session_remove_shop_goods(struct Session *self, const struct ItemInfo *item);
void session_remove_counted_shop_goods(struct Session *self, const struct ItemInfo *item);
void session_purchase_list_remove(struct Session *self, const struct ItemInfo *item);
void session_remove_shop_buy_item_keyed(struct Session *self, const struct ItemInfo *item);

struct ItemInfo *session_get_worn_counterpart(struct Session *self, const struct ItemInfo *item);
struct ItemInfo *session_find_costume_item(struct Session *self, struct ItemInfo *out, int index,
                                                  int *slotOut);
struct ItemInfo *session_find_equipped_item(struct Session *self, struct ItemInfo *out, int index,
                                                  int *slotOut);
struct ItemInfo *session_get_equipped_item(struct Session *self, struct ItemInfo *out,
                                                     unsigned slot);
struct ItemInfo *session_get_my_trade_item(struct Session *self, struct ItemInfo *out, int i);
struct ItemInfo *session_get_inventory_item_at(struct Session *self, struct ItemInfo *out, int n);
struct PartyMember *session_party_member_at(struct Session *self, struct PartyMember *out, int n);
struct PartyMember *session_get_friend_by_index(struct Session *self, struct PartyMember *out, int n);
WORD session_get_inventory_index_by_id(struct Session *self, int itemId);
struct Vec *session_inventory_indices(struct Session *self, struct Vec *out, int itemId,
                                                       BYTE onlyIdentified);
struct ItemInfo *session_get_inventory_item_by_id(struct Session *self, struct ItemInfo *out,
                                                          int itemId, BYTE requireIdentified);
int session_inventory_count(struct Session *self, int itemId);
DWORD session_get_item_dis_color(const struct ItemInfo *item);

struct ItemInfo *session_get_store_setup_item_by_index(struct Session *self, struct ItemInfo *out, int index);
struct ItemInfo *session_get_store_setup_item_at(struct Session *self, struct ItemInfo *out, int n);
int session_get_store_setup_count(struct Session *self, int index);
struct ItemInfo *session_cart_item_by_index(struct Session *self, struct ItemInfo *out, int index);
struct ItemInfo *session_get_cart_item_at(struct Session *self, struct ItemInfo *out, int n);
int session_get_cart_item_count(struct Session *self, int index);
struct ItemInfo *session_mirror_item_by_index(struct Session *self, struct ItemInfo *out, int index);
struct ItemInfo *session_mirror_item_at(struct Session *self, struct ItemInfo *out, int n);
int session_get_store_candidate_count(struct Session *self, int index);
struct ItemInfo *session_get_shop_item_by_index(struct Session *self, struct ItemInfo *out, int index);
struct ItemInfo *session_get_shop_item_by_id(struct Session *self, struct ItemInfo *out, int itemId);
struct ItemInfo *session_get_store_purchase_item_by_index(struct Session *self, struct ItemInfo *out, int index);
struct ItemInfo *session_purchase_item_by_id(struct Session *self, struct ItemInfo *out,
                                                      const char *name);
struct ItemInfo *session_purchase_item(struct Session *self, struct ItemInfo *out, int n);
int session_get_store_purchase_count_by_name(struct Session *self, const char *name);
int session_get_store_purchase_count(struct Session *self, int index);
__int64 session_purchase_total(struct Session *self);
struct ItemInfo *session_get_sent_store_purchase_item(struct Session *self, struct ItemInfo *out, int index);
struct ItemInfo *session_shop_list_item(struct Session *self, struct ItemInfo *out, int n);
struct ItemInfo *session_find_purchase_item(struct Session *self, struct ItemInfo *out,
                                                      const char *name);
struct ItemInfo *session_get_shop_buy_item_at(struct Session *self, struct ItemInfo *out, int n);
int session_get_shop_buy_count_by_name(struct Session *self, const char *name);
int session_get_shop_buy_count(struct Session *self, int index);
int session_get_shop_buy_total(struct Session *self);
struct ItemInfo *session_get_shop_sell_item_by_index(struct Session *self, struct ItemInfo *out, int index);
struct ItemInfo *session_get_shop_sell_item_at(struct Session *self, struct ItemInfo *out, int n);
int session_get_shop_sell_count(struct Session *self, int index);
int session_get_shop_goods_count_by_id(struct Session *self, const struct ItemInfo *item);
int session_get_shop_goods_count_by_index(struct Session *self, const struct ItemInfo *item);
WORD session_get_shop_goods_weight_by_id(struct Session *self, const struct ItemInfo *item);
WORD session_get_shop_goods_weight_by_index(struct Session *self, const struct ItemInfo *item);
WORD session_shop_buy_weight_by_id(struct Session *self);
WORD session_shop_buy_weight_by_index(struct Session *self);
struct ItemInfo *session_get_cash_goods(struct Session *self, struct ItemInfo *out, int k, int n);
int session_cash_goods_count(struct Session *self, int k);
int session_get_friend_count(struct Session *self);
int session_get_inventory_count(struct Session *self);
int session_get_store_setup_size(struct Session *self);
int session_get_cart_size(struct Session *self);
int session_mirror_count(struct Session *self);
int session_purchase_count(struct Session *self);
int session_shop_list_count(struct Session *self);
int session_get_party_member_count(struct Session *self);
int session_get_shop_buy_list_size(struct Session *self);
int session_get_shop_sell_list_size(struct Session *self);
int session_sellable_count(struct Session *self);
int session_storage_count(struct Session *self);
struct ItemInfo *session_get_partner_trade_item(struct Session *self, struct ItemInfo *out, int i);
struct ItemInfo *session_get_viewed_equip(struct Session *self, struct ItemInfo *out,
                                                      unsigned slot);
struct ItemInfo *session_sellable_at(struct Session *self, struct ItemInfo *out, int n);
int session_get_shop_goods_count(struct Session *self, int index);
struct ItemInfo *session_get_storage_item(struct Session *self, struct ItemInfo *out, int index);
struct ItemInfo *session_storage_item_at(struct Session *self, struct ItemInfo *out, int n);
int session_get_storage_item_count(struct Session *self, int index);
BYTE session_has_shop_goods(struct Session *self, int index);
BYTE session_has_cash_search_result(struct Session *self, int itemId);
BYTE session_has_cart_item_id(struct Session *self, int itemId);
int session_has_shop_sell_items(struct Session *self);
BYTE session_is_worn_index(struct Session *self, int index);
BYTE session_has_inventory_index(struct Session *self, int index);
BYTE session_is_visible_npc(int job);
BYTE session_is_wearing_item_id(struct Session *self, int itemId);
BYTE session_is_wearing_item_id_at(struct Session *self, int itemId, int min);
void session_set_inventory_move_pending(struct Session *self, int index);
void session_set_cart_move_pending(struct Session *self, int index);
void session_set_storage_move_pending(struct Session *self, int index);
void session_clear_inventory_move_pending(struct Session *self, int index);
void session_clear_cart_move_pending(struct Session *self, int index);
void session_clear_storage_move_pending(struct Session *self, int index);
void session_fill_buying_store_list(struct Session *self);
void session_fill_buying_store_list_priced(struct Session *self);
void session_set_store_setup_count_at(struct Session *self, int n, int count);
void session_set_store_setup_price_at(struct Session *self, int n, int price);

BYTE session_take_off_equip(struct Session *self, unsigned location, int index);
BYTE session_set_worn_costume(struct Session *self, const struct ItemInfo *item);
BYTE session_put_on_equip(struct Session *self, struct ItemInfo *item);
void session_set_costume_slot(struct Session *self, unsigned i, const struct ItemInfo *item);
void session_set_equip_slot(struct Session *self, unsigned i, const struct ItemInfo *item);
void session_set_arrow(struct Session *self, int index);
BYTE session_set_viewed_costume(struct Session *self, const struct ItemInfo *item);
void session_clear_worn_costumes(struct Session *self);
void session_clear_viewed_costumes(struct Session *self);
struct ItemInfo *session_get_worn_costume(struct Session *self, struct ItemInfo *out, int slot);
BYTE session_take_off_costume(struct Session *self, unsigned location);
void session_set_arrow_status(struct Session *self, int index);

void session_end_exchange(struct Session *self);
void session_end_exchange2(struct Session *self);
void session_clear_their_trade(struct Session *self);
void session_clear_my_trade(struct Session *self);
BYTE session_is_exist_index_in_my_change_list(struct Session *self, int index);
void session_set_my_exchange_money(struct Session *self, int money);
void session_set_other_exchange_money(struct Session *self, int money);
int session_is_exchange_a_full(struct Session *self);
BYTE session_add_my_trade_item(struct Session *self, struct ItemInfo item);
void session_set_their_trade_zeny(struct Session *self, int money);
void session_add_their_trade_item(struct Session *self, struct ItemInfo item);

void session_add_mail_item(struct Session *self, struct ItemInfo item);
void session_put_mail_item(struct Session *self, struct ItemInfo item);
void session_reset_mail_items(struct Session *self);
void session_clear_mail_item_counts(struct Session *self);
int session_count_mail_items(struct Session *self);
struct ItemInfo *session_get_mail_item(struct Session *self, struct ItemInfo *out, int index);

BYTE session_put_switch_costume(struct Session *self, const struct ItemInfo *item);
BYTE session_put_switch_equip(struct Session *self, const struct ItemInfo *item);
void session_clear_switch_costumes(struct Session *self);
void session_clear_switch_equips(struct Session *self);
struct ItemInfo *session_get_switch_costume(struct Session *self, struct ItemInfo *out, int slot);
struct ItemInfo *session_get_switch_equip(struct Session *self, struct ItemInfo *out, int slot);
struct ItemInfo *session_find_item_for_switch(struct Session *self, struct ItemInfo *out, int index,
                                                       unsigned location);
int session_is_in_switch_sets(struct Session *self, int index);
void session_take_out_switch_costume(struct Session *self, const struct ItemInfo *item);
void session_take_out_switch_equip(struct Session *self, const struct ItemInfo *item);

struct PartyMember *party_member_info_copy(struct PartyMember *self,
                                                    const struct PartyMember *src);
void names_resize(struct Vec *v, size_t count, const char *fill);

#endif
