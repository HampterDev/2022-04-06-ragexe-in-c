#include "session/csession.h"
#include "session/session_items.h"
#include "session/session_party.h"
#include "session/session_state.h"

void session_clear_items_and_equipment(struct Session *self)
{
    int i;

    list_clear(&self->itemList, item_info_destroy);
    list_clear(&self->pingSamples, NULL);
    for (i = 0; i < EQUIP_SLOT_COUNT; i++)
        item_info_clear(&self->equipSlots[i]);
}

void session_clear_character_lists(struct Session *self)
{
    int i;

    list_clear(&self->purchaseItems, item_info_destroy);
    list_clear(&self->sellItems, item_info_destroy);
    list_clear(&self->shopItems, item_info_destroy);
    session_clear_my_trade(self);
    self->partnerTradeZeny = 0;
    for (i = 0; i < 10; i++)
        self->partnerTradeItems[i].count = 0;
    list_clear(&self->storageItems, item_info_destroy);
    list_clear(&g_session1.skills.skills, skill_info_destroy);
    session_clear_friends(self);
    session_clear_party_members(self);
    session_clear_mail_item_counts(self);
}
