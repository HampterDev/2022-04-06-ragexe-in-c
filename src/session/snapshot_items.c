#include "session/snapshot_items.h"
#include "session/state.h"
#include "iteminfo/iteminfo.h"
#include "iteminfo/item_snapshot.h"
#include <string.h>

#define MEMBER(name) offsetof(struct Session, name)
static const size_t list_offsets[] = {
    MEMBER(itemList), MEMBER(shopItems), MEMBER(purchaseItems), MEMBER(snapshotOnlyItems),
    MEMBER(sellItems), MEMBER(storageItems), MEMBER(cartItems), MEMBER(otherStoreItems),
    MEMBER(storeCandidateItems), MEMBER(ownStoreItems), MEMBER(storePurchaseItems),
    MEMBER(storeSetupItems), MEMBER(storePurchaseSent)
};
static const size_t array_offsets[] = {
    MEMBER(equipSlots), MEMBER(viewedEquips), MEMBER(costumeSlots),
    MEMBER(viewedCostumes), MEMBER(exchangeA), MEMBER(partnerTradeItems)
};
#undef MEMBER
#define LIST_COUNT (sizeof list_offsets / sizeof list_offsets[0])
#define ARRAY_COUNT (sizeof array_offsets / sizeof array_offsets[0])
#define ITEM_LIST_MAX 0x1111110u
enum { ITEMS_BEGIN = 0x1194, ITEMS_END = 0x1195, ITEMS_END_OLD = 0x125f,
       FIRST_LIST = 0x119e, FIRST_ARRAY = 0x11f9, TRADE_OWN = 0x125d, TRADE_PARTNER = 0x125e };

int session_items_save(const struct Session *session, struct ReplayBuffer *out)
{
    struct SnapshotWriter writer = {0};
    size_t i, j, start;
    if (!session) return 0;
    snapshot_write(&writer, ITEMS_BEGIN, NULL, 0);
    snapshot_write(&writer, TRADE_OWN, &session->myTradeZeny, sizeof session->myTradeZeny);
    snapshot_write(&writer, TRADE_PARTNER, &session->partnerTradeZeny, sizeof session->partnerTradeZeny);
    for (i = 0; i < LIST_COUNT; ++i) {
        const struct List *list = (const void *)((const unsigned char *)session + list_offsets[i]);
        const struct ListNode *node;
        start = snapshot_begin_record(&writer, (uint16_t)(FIRST_LIST + i));
        if (list->head) {
            for (node = list->head->next; node != list->head; node = node->next)
                item_snapshot_write(&writer, LIST_VALUE(node, struct ItemInfo));
        }
        snapshot_end_record(&writer, start);
    }
    for (i = 0; i < ARRAY_COUNT; ++i) {
        const struct ItemInfo *items = (const void *)((const unsigned char *)session + array_offsets[i]);
        start = snapshot_begin_record(&writer, (uint16_t)(FIRST_ARRAY + i));
        for (j = 0; j < EQUIP_SLOT_COUNT; ++j)
            item_snapshot_write(&writer, &items[j]);
        snapshot_end_record(&writer, start);
    }
    snapshot_write(&writer, ITEMS_END, NULL, 0);
    return snapshot_finish(&writer, out);
}

static int validate_sequence(struct SnapshotSpan sequence, size_t maximum, size_t *count)
{
    size_t at = 0;
    while (at < sequence.size) {
        int used;
        if (*count >= maximum) return 0;
        used = item_snapshot_validate(sequence.data + at, sequence.size - at);
        if (used < 0) return 0;
        at += (size_t)used;
        ++*count;
    }
    return 1;
}

static int validate_items(const void *data, size_t size, size_t counts[LIST_COUNT])
{
    struct SnapshotReader reader;
    struct SnapshotRecord record;
    if (!snapshot_open(&reader, data, size, ITEMS_BEGIN)) return -1;
    while (snapshot_next(&reader, &record) == 1) {
        unsigned index;
        if (record.tag == ITEMS_END || record.tag == ITEMS_END_OLD)
            return record.value.size ? -1 : (int)reader.offset;
        if (record.tag == ITEMS_BEGIN) return -1;
        index = (unsigned)record.tag - FIRST_LIST;
        if (index < LIST_COUNT) {
            if (!validate_sequence(record.value, ITEM_LIST_MAX, &counts[index])) return -1;
        } else if ((index = (unsigned)record.tag - FIRST_ARRAY) < ARRAY_COUNT) {
            size_t count = 0;
            if (!validate_sequence(record.value, EQUIP_SLOT_COUNT, &count)) return -1;
        } else if ((record.tag == TRADE_OWN || record.tag == TRADE_PARTNER) && record.value.size != sizeof(int)) {
            return -1;
        }
    }
    return -1;
}

static void load_sequence(struct SnapshotSpan sequence, struct List *list, struct ItemInfo *slots)
{
    size_t at = 0, index = 0;
    while (at < sequence.size) {
        struct ItemInfo item = {0};
        int used;
        item_info_construct(&item);
        used = item_snapshot_load(&item, sequence.data + at, sequence.size - at);
        if (list) {
            item_info_copy(list_push_back(list, NULL, sizeof(struct ItemInfo)), &item);
        } else {
            item_info_assign(&slots[index++], &item);
        }
        item_info_destruct(&item);
        at += (size_t)used;
    }
}

int session_items_load(struct Session *session, const void *data, size_t size)
{
    size_t counts[LIST_COUNT] = {0}, i;
    struct SnapshotReader reader;
    struct SnapshotRecord record;
    int used = validate_items(data, size, counts);
    if (!session || used < 0) return -1;
    for (i = 0; i < LIST_COUNT; ++i) {
        const struct List *list = (const void *)((const unsigned char *)session + list_offsets[i]);
        if (counts[i] && (!list->head || list->size > ITEM_LIST_MAX || counts[i] > ITEM_LIST_MAX - list->size))
            return -1;
    }
    snapshot_open(&reader, data, (size_t)used, ITEMS_BEGIN);
    while (snapshot_next(&reader, &record) == 1) {
        unsigned index = (unsigned)record.tag - FIRST_LIST;
        if (index < LIST_COUNT) {
            struct List *list = (void *)((unsigned char *)session + list_offsets[index]);
            load_sequence(record.value, list, NULL);
        } else if ((index = (unsigned)record.tag - FIRST_ARRAY) < ARRAY_COUNT) {
            struct ItemInfo *slots = (void *)((unsigned char *)session + array_offsets[index]);
            load_sequence(record.value, NULL, slots);
        } else if (record.tag == TRADE_OWN) {
            memcpy(&session->myTradeZeny, record.value.data, sizeof session->myTradeZeny);
        } else if (record.tag == TRADE_PARTNER) {
            memcpy(&session->partnerTradeZeny, record.value.data, sizeof session->partnerTradeZeny);
        }
    }
    return used;
}
