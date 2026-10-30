#include "iteminfo/item_snapshot.h"
#include "iteminfo/iteminfo.h"
#include <string.h>

enum { ITEM_BEGIN = 0x119, ITEM_END = 0x11a, ITEM_ID = 0x123, ITEM_NAME = 0x124 };
struct ItemField { uint16_t tag; size_t offset, size; };
#define FIELD(tag, member) {tag, offsetof(struct ItemInfo, member), sizeof(((struct ItemInfo *)0)->member)}
static const struct ItemField fields[] = {
    FIELD(0x11b, type), FIELD(0x11d, index), FIELD(0x11c, equip_location),
    FIELD(0x11e, worn_location), FIELD(0x11f, count), FIELD(0x120, price),
    FIELD(0x121, charged_price), FIELD(0x122, slot),
    {ITEM_ID, 0, sizeof(int)}, {ITEM_NAME, 0, 0},
    FIELD(0x125, identified), FIELD(0x126, damaged), FIELD(0x127, refine),
    FIELD(0x128, bind_on_equip_type), FIELD(0x129, expire_time), FIELD(0x12a, view)
};
#undef FIELD

static const struct ItemField *find_field(uint16_t tag)
{
    size_t i;
    for (i = 0; i < sizeof fields / sizeof fields[0]; ++i)
        if (fields[i].tag == tag) return &fields[i];
    return NULL;
}

void item_snapshot_write(struct SnapshotWriter *writer, const struct ItemInfo *item)
{
    size_t i;
    snapshot_write(writer, ITEM_BEGIN, NULL, 0);
    for (i = 0; i < sizeof fields / sizeof fields[0]; ++i) {
        const struct ItemField *field = &fields[i];
        if (field->tag == ITEM_ID) {
            int id = item_id(item);
            snapshot_write(writer, field->tag, &id, sizeof id);
        } else if (field->tag == ITEM_NAME) {
            snapshot_write(writer, field->tag, string_cstr(&item->name), item->name.size);
        } else {
            snapshot_write(writer, field->tag, (const unsigned char *)item + field->offset, field->size);
        }
    }
    snapshot_write(writer, ITEM_END, NULL, 0);
}

int item_snapshot_validate(const void *data, size_t size)
{
    struct SnapshotReader reader;
    struct SnapshotRecord record;
    if (!snapshot_open(&reader, data, size, ITEM_BEGIN)) return -1;
    while (snapshot_next(&reader, &record) == 1) {
        const struct ItemField *field;
        if (record.tag == ITEM_END)
            return record.value.size ? -1 : (int)reader.offset;
        if (record.tag == ITEM_BEGIN) return -1;
        field = find_field(record.tag);
        if (field && field->tag != ITEM_NAME && record.value.size != field->size)
            return -1;
    }
    return -1;
}

int item_snapshot_load(struct ItemInfo *item, const void *data, size_t size)
{
    struct SnapshotReader reader;
    struct SnapshotRecord record;
    int used = item_snapshot_validate(data, size);
    if (!item || used < 0) return -1;
    snapshot_open(&reader, data, (size_t)used, ITEM_BEGIN);
    while (snapshot_next(&reader, &record) == 1) {
        const struct ItemField *field = find_field(record.tag);
        if (!field) continue;
        if (field->tag == ITEM_ID) {
            int id;
            memcpy(&id, record.value.data, sizeof id);
            item_info_set_item_id(item, id);
        } else if (field->tag == ITEM_NAME) {
            const unsigned char *zero = memchr(record.value.data, 0, record.value.size);
            size_t length = zero ? (size_t)(zero - record.value.data) : record.value.size;
            string_assign(&item->name, (const char *)record.value.data, length);
        } else {
            memcpy((unsigned char *)item + field->offset, record.value.data, field->size);
        }
    }
    return used;
}
