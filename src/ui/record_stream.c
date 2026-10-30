#include "ui/record_stream.h"
#include "ui/window_info.h"

enum FieldKind { FIELD_FIXED, FIELD_TEXT, FIELD_INDEX_PACKET };
struct RecordField {
    uint16_t group, tag;
    enum FieldKind kind;
    size_t width;
};
struct RecordFormat {
    uint16_t begin, end, alternate_end, group;
};

#define FORMAT(begin, end) {begin, end, 0, begin}
#define PAIRED(begin, end, other_end, group) {begin, end, other_end, group}
static const struct RecordFormat formats[] = {
    FORMAT(REC_WINDOWS_BEGIN, REC_WINDOWS_END),
    FORMAT(REC_OPTIONS_BEGIN, REC_OPTIONS_END),
    FORMAT(REC_EMPTY_BEGIN, REC_EMPTY_END),
    FORMAT(CHAT_ROOM_REC_BEGIN, CHAT_ROOM_REC_END),
    FORMAT(REC_TRADE_BEGIN, REC_TRADE_END),
    FORMAT(REC_PURCHASE_BEGIN, REC_PURCHASE_END),
    FORMAT(REC_SELL_BEGIN, REC_SELL_END),
    FORMAT(REC_SELL2_BEGIN, REC_SELL2_END),
    FORMAT(CHOOSER_REC_BEGIN, CHOOSER_REC_END),
    FORMAT(STORE_REC_BEGIN, STORE_REC_END),
    PAIRED(REC_MY_SHOP_BEGIN, REC_MY_SHOP_END, REC_MY_SHOP_VARIANT_END, REC_MY_SHOP_BEGIN),
    PAIRED(REC_MY_SHOP_VARIANT_BEGIN, REC_MY_SHOP_VARIANT_END, REC_MY_SHOP_END, REC_MY_SHOP_BEGIN),
    PAIRED(REC_MERCHANT_PURCHASE_BEGIN, REC_MERCHANT_PURCHASE_END, REC_PURCHASE_VARIANT_END,
           REC_MERCHANT_PURCHASE_BEGIN),
    PAIRED(REC_PURCHASE_VARIANT_BEGIN, REC_PURCHASE_VARIANT_END, REC_MERCHANT_PURCHASE_END,
           REC_MERCHANT_PURCHASE_BEGIN),
    PAIRED(REC_SHOP_BEGIN, REC_SHOP_END, REC_SHOP_VARIANT_END, REC_SHOP_BEGIN),
    PAIRED(REC_SHOP_VARIANT_BEGIN, REC_SHOP_VARIANT_END, REC_SHOP_END, REC_SHOP_BEGIN),
    PAIRED(REC_VENDING_BEGIN, REC_VENDING_END, REC_BUYING_END, REC_VENDING_BEGIN),
    PAIRED(REC_BUYING_BEGIN, REC_BUYING_END, REC_VENDING_END, REC_VENDING_BEGIN),
    FORMAT(REC_MIRROR_BEGIN, REC_MIRROR_END),
    FORMAT(BABY_REC_BEGIN, BABY_REC_END),
    FORMAT(BABY2_REC_BEGIN, BABY2_REC_END),
    FORMAT(JOIN_FRIEND_REC_BEGIN, JOIN_FRIEND_REC_END),
    FORMAT(EXCHANGE_REC_BEGIN, EXCHANGE_REC_END),
    FORMAT(STAR_PLACE_REC_BEGIN, STAR_PLACE_REC_END),
    FORMAT(JOIN_PARTY_REC_BEGIN, JOIN_PARTY_REC_END),
    FORMAT(JOIN_GUILD_REC_BEGIN, JOIN_GUILD_REC_END),
    FORMAT(NPC_MENU_STATE_BEGIN, NPC_MENU_STATE_END),
    FORMAT(WARP_STATE_BEGIN, WARP_STATE_END),
    FORMAT(MT_PROCESS_REC_BEGIN, MT_PROCESS_REC_END),
    FORMAT(MT_LIST_REC_BEGIN, MT_LIST_REC_END),
    FORMAT(REC_MAKING_BEGIN, REC_MAKING_END),
    FORMAT(REC_REPAIR_BEGIN, REC_REPAIR_END),
    FORMAT(REC_REFINE_BEGIN, REC_REFINE_END),
    FORMAT(REC_IDENTIFY_BEGIN, REC_IDENTIFY_END),
    FORMAT(REC_COMPOSITION_BEGIN, REC_COMPOSITION_END),
    FORMAT(SAY_STATE_BEGIN, SAY_STATE_END),
};
#undef FORMAT
#undef PAIRED

#define FIXED(group, tag, width) {group, tag, FIELD_FIXED, width}
#define TEXT_FIELD(group, tag) {group, tag, FIELD_TEXT, UI_RECORD_TEXT_CAPACITY - 1}
static const struct RecordField fields[] = {
    FIXED(REC_OPTIONS_BEGIN, REC_DRAG_ALL, 4),
    FIXED(CHAT_ROOM_REC_BEGIN, CHAT_ROOM_REC_FRAME, sizeof(struct WindowInfo)),
    TEXT_FIELD(CHAT_ROOM_REC_BEGIN, CHAT_ROOM_REC_TITLE),
    TEXT_FIELD(CHAT_ROOM_REC_BEGIN, CHAT_ROOM_REC_PASSWORD),
    FIXED(CHAT_ROOM_REC_BEGIN, CHAT_ROOM_REC_TYPE, 4),
    FIXED(CHAT_ROOM_REC_BEGIN, CHAT_ROOM_REC_USERS, 4),
    FIXED(CHAT_ROOM_REC_BEGIN, CHAT_ROOM_REC_LIMIT, 4),
    FIXED(CHAT_ROOM_REC_BEGIN, CHAT_ROOM_REC_ROOM_ID, 4),
    TEXT_FIELD(CHAT_ROOM_REC_BEGIN, CHAT_ROOM_REC_MEMBER),
    TEXT_FIELD(CHAT_ROOM_REC_BEGIN, CHAT_ROOM_REC_MEMBER_SET),
    TEXT_FIELD(REC_TRADE_BEGIN, REC_TRADE_PARTNER_NAME),
    FIXED(REC_TRADE_BEGIN, REC_TRADE_PARTNER_CID, 4),
    FIXED(REC_TRADE_BEGIN, REC_TRADE_PARTNER_LEVEL, 2),
    FIXED(REC_TRADE_BEGIN, REC_TRADE_LOCKED_MINE, 4),
    FIXED(REC_TRADE_BEGIN, REC_TRADE_LOCKED_PARTNER, 4),
    FIXED(REC_TRADE_BEGIN, REC_TRADE_PRESSED_OK, 4),
    FIXED(REC_TRADE_BEGIN, REC_TRADE_PRESSED_EXCHANGE, 4),
    FIXED(REC_TRADE_BEGIN, REC_TRADE_PRESSED_CANCEL, 4),
    FIXED(REC_SELL2_BEGIN, REC_SELL2_DC, 4),
    FIXED(REC_MY_SHOP_BEGIN, REC_MY_SHOP_NOTICE, 4),
    FIXED(REC_SHOP_BEGIN, REC_SHOP_VARIANT_VALUE, 4),
    TEXT_FIELD(BABY_REC_BEGIN, BABY_REC_NAME),
    TEXT_FIELD(JOIN_FRIEND_REC_BEGIN, JOIN_FRIEND_REC_NAME),
    TEXT_FIELD(EXCHANGE_REC_BEGIN, EXCHANGE_REC_NAME),
    FIXED(STAR_PLACE_REC_BEGIN, STAR_PLACE_REC_KIND, 1),
    TEXT_FIELD(JOIN_PARTY_REC_BEGIN, JOIN_PARTY_REC_NAME),
    TEXT_FIELD(JOIN_GUILD_REC_BEGIN, JOIN_GUILD_REC_NAME),
    TEXT_FIELD(NPC_MENU_STATE_BEGIN, NPC_MENU_STATE_LINE),
    TEXT_FIELD(WARP_STATE_BEGIN, WARP_STATE_LINE),
    FIXED(MT_PROCESS_REC_BEGIN, MT_PROCESS_REC_ID, 4),
    FIXED(MT_LIST_REC_BEGIN, MT_LIST_REC_ID, 4),
    FIXED(REC_MAKING_BEGIN, REC_MAKING_ITEM, 4),
    FIXED(REC_MAKING_BEGIN, REC_MAKING_KIND, 2),
    FIXED(REC_REPAIR_BEGIN, REC_REPAIR_ITEM, 0x18),
    FIXED(REC_REFINE_BEGIN, REC_REFINE_ITEM, 8),
    {REC_IDENTIFY_BEGIN, REC_IDENTIFY_PACKET, FIELD_INDEX_PACKET, 0x177},
    {REC_COMPOSITION_BEGIN, REC_COMPOSITION_PACKET, FIELD_INDEX_PACKET, 0x17b},
    TEXT_FIELD(SAY_STATE_BEGIN, SAY_STATE_LINE),
    FIXED(SAY_STATE_BEGIN, SAY_STATE_CLEAR, 4),
};
#undef FIXED
#undef TEXT_FIELD

static unsigned read_short(const unsigned char *data) { return data[0] | (unsigned)data[1] << 8; }

static int valid_field(const struct RecordField *field, struct SnapshotSpan value)
{
    switch (field->kind) {
    case FIELD_FIXED:
        return value.size == field->width;
    case FIELD_TEXT:
        return value.size <= field->width;
    case FIELD_INDEX_PACKET:
        return value.size >= 4 && value.size <= UINT16_MAX && !(value.size & 1) &&
               read_short(value.data) == field->width && read_short(value.data + 2) == value.size;
    }
    return 0;
}

static int validate_stream(const void *data, size_t size, int allow_window_list)
{
    struct SnapshotReader reader = {{data, size}, 0};
    struct SnapshotRecord record;
    const struct RecordFormat *format = NULL;
    size_t i;
    if (!size)
        return 0;
    if (snapshot_next(&reader, &record) != 1 || record.value.size)
        return -1;
    for (i = 0; i < sizeof formats / sizeof formats[0]; ++i)
        if (formats[i].begin == record.tag) {
            format = &formats[i];
            break;
        }
    if (!format || (format->begin == REC_WINDOWS_BEGIN && !allow_window_list))
        return -1;
    while (snapshot_next(&reader, &record) == 1) {
        if (record.tag == format->end || (format->alternate_end && record.tag == format->alternate_end))
            return record.value.size ? -1 : (int)reader.offset;
        if (record.tag == format->begin)
            return -1;
        if (format->begin == REC_WINDOWS_BEGIN && (unsigned)(record.tag - REC_WINDOW_BASE) < REC_WINDOW_IDS) {
            int used = validate_stream(record.value.data, record.value.size, 0);
            if (used < 0 || (size_t)used != record.value.size)
                return -1;
        }
        for (i = 0; i < sizeof fields / sizeof fields[0]; ++i)
            if (fields[i].group == format->group && fields[i].tag == record.tag &&
                !valid_field(&fields[i], record.value))
                return -1;
    }
    return -1;
}

int ui_window_records_validate(const void *data, size_t size) { return validate_stream(data, size, 1); }

int ui_window_records_open(struct SnapshotReader *reader, const void *data, size_t size, uint16_t begin)
{
    int used = ui_window_records_validate(data, size);
    if (used <= 0 || !snapshot_open(reader, data, (size_t)used, begin))
        return -1;
    return used;
}
