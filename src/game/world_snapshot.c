#include "game/world_snapshot.h"
#include "replay/snapshot.h"
#include "replay/replay.h"
#include "game/gamemode.h"
#include "game/ground_item_state.h"
#include "game/ground_item_packets.h"
#include "game/packet_handlers.h"
#include "game/skill_unit_packets.h"
#include "effects/skills/skill_state.h"
#include "game/objects.h"
#include "game/cells.h"
#include "actor/job.h"
#include <limits.h>
#include <math.h>
#include <string.h>

struct WorldSnapshotFormat {
    uint16_t begin, end, first, second;
    size_t first_size, second_size;
};
static const struct WorldSnapshotFormat ground_format = {
    0x3a98, 0x3a99, 0xcf, 0, sizeof(struct PacketGroundItemEntry), 0
};
static const struct WorldSnapshotFormat skills_format = {
    0x4268, 0x4269, 0xcd, 0xce,
    sizeof(struct PACKET_ZC_SKILL_ENTRY), sizeof(struct PACKET_ZC_SKILL_ENTRY2)
};
static const struct WorldSnapshotFormat npc_format = {
    0x36b0, 0x36b1, 0x36bb, 0, sizeof(int32_t), 0
};

static int validate(const void *data, size_t size, const struct WorldSnapshotFormat *format)
{
    struct SnapshotReader reader;
    struct SnapshotRecord record;
    if (!snapshot_open(&reader, data, size, format->begin)) return -1;
    while (snapshot_next(&reader, &record) == 1) {
        if (record.tag == format->end)
            return record.value.size ? -1 : (int)reader.offset;
        if (record.tag == format->begin) return -1;
        if (record.tag == format->first && record.value.size != format->first_size) return -1;
        if (format->second && record.tag == format->second && record.value.size != format->second_size) return -1;
    }
    return -1;
}
static int valid_position(const struct C3dAttr *attr, float x, float z)
{
    float cell_x, cell_y;
    if (!attr || attr->width <= 0 || attr->height <= 0 || attr->zoom <= 0) return 0;
    cell_x = (float)(attr->width / 2) + x / (float)attr->zoom;
    cell_y = (float)(attr->height / 2) + z / (float)attr->zoom;
    return isfinite(cell_x) && isfinite(cell_y) &&
        cell_x >= (float)INT_MIN && cell_x < 2147483648.0f &&
        cell_y >= (float)INT_MIN && cell_y < 2147483648.0f;
}
int game_ground_items_save(struct GameMode *mode, struct ReplayBuffer *out)
{
    struct SnapshotWriter writer = {0};
    struct ListNode *head, *node;
    if (!mode || !mode->world || !out || !(head = mode->world->items.head)) return 0;
    snapshot_write(&writer, ground_format.begin, NULL, 0);
    for (node = head->next; node != head && !writer.failed; node = node->next) {
        const struct Item *item = *LIST_VALUE(node, const struct Item *);
        struct PacketGroundItemEntry entry = {0};
        int x, y, sub_x, sub_y;
        if (!item || !valid_position(mode->world->attr, item->base.pos.x, item->base.pos.z)) {
            writer.failed = 1;
            break;
        }
        game_mode_world_to_sub_cell(mode, item->base.pos.x, item->base.pos.z, &x, &y, &sub_x, &sub_y);
        entry.id = 0x9d;
        entry.aid = item->aid;
        entry.item_id = item->item_id_short;
        entry.identified = item->identified;
        entry.x = (int16_t)x;
        entry.y = (int16_t)y;
        entry.sub_x = (uint8_t)sub_x;
        entry.sub_y = (uint8_t)sub_y;
        entry.count = (int16_t)item->count;
        snapshot_write(&writer, ground_format.first, &entry, sizeof entry);
    }
    snapshot_write(&writer, ground_format.end, NULL, 0);
    return snapshot_finish(&writer, out);
}
int game_skill_units_save(struct GameMode *mode, struct ReplayBuffer *out)
{
    struct SnapshotWriter writer = {0};
    struct ListNode *head, *node;
    if (!mode || !mode->world || !out || !(head = mode->world->skills.head)) return 0;
    snapshot_write(&writer, skills_format.begin, NULL, 0);
    for (node = head->next; node != head && !writer.failed; node = node->next) {
        struct Skill *unit = *LIST_VALUE(node, struct Skill *);
        struct PACKET_ZC_SKILL_ENTRY entry = {0};
        int x, y, sub_x, sub_y;
        if (!unit || !valid_position(mode->world->attr, unit->base.pos.x, unit->base.pos.z)) {
            writer.failed = 1;
            break;
        }
        game_mode_world_to_sub_cell(mode, unit->base.pos.x, unit->base.pos.z, &x, &y, &sub_x, &sub_y);
        entry.id = 0x11f;
        entry.aid = (unsigned)unit->unit_aid;
        entry.creator_aid = (unsigned)unit->creator_aid;
        entry.job = (unsigned char)game_actor_get_job(&unit->base);
        entry.visible = 1;
        entry.x = (short)x;
        entry.y = (short)y;
        snapshot_write(&writer, skills_format.first, &entry, sizeof entry);
    }
    snapshot_write(&writer, skills_format.end, NULL, 0);
    return snapshot_finish(&writer, out);
}
int game_npc_dialog_save(struct GameMode *mode, struct ReplayBuffer *out)
{
    struct SnapshotWriter writer = {0};
    int32_t value;
    if (!mode || !out) return 0;
    value = mode->npc_dialog_open;
    snapshot_write(&writer, npc_format.begin, NULL, 0);
    snapshot_write(&writer, npc_format.first, &value, sizeof value);
    snapshot_write(&writer, npc_format.end, NULL, 0);
    return snapshot_finish(&writer, out);
}
int game_ground_items_load(struct GameMode *mode, const void *data, size_t size)
{
    struct SnapshotReader reader;
    struct SnapshotRecord record;
    int used = validate(data, size, &ground_format);
    if (!mode || !mode->world || used < 0) return -1;
    snapshot_open(&reader, data, (size_t)used, ground_format.begin);
    while (snapshot_next(&reader, &record) == 1 && record.tag != ground_format.end) {
        struct PacketGroundItemEntry entry;
        if (record.tag != ground_format.first) continue;
        memcpy(&entry, record.value.data, sizeof entry);
        game_mode_item_entry(mode, &entry);
    }
    return used;
}
int game_skill_units_load(struct GameMode *mode, const void *data, size_t size)
{
    struct SnapshotReader reader;
    struct SnapshotRecord record;
    int used = validate(data, size, &skills_format);
    if (!mode || !mode->world || used < 0) return -1;
    snapshot_open(&reader, data, (size_t)used, skills_format.begin);
    while (snapshot_next(&reader, &record) == 1 && record.tag != skills_format.end) {
        if (record.tag == skills_format.first) {
            struct PACKET_ZC_SKILL_ENTRY entry;
            memcpy(&entry, record.value.data, sizeof entry);
            game_mode_zc_skill_entry(mode, &entry);
        } else if (record.tag == skills_format.second) {
            struct PACKET_ZC_SKILL_ENTRY2 entry;
            memcpy(&entry, record.value.data, sizeof entry);
            game_mode_zc_skill_entry2(mode, &entry);
        }
    }
    return used;
}
int game_npc_dialog_load(struct GameMode *mode, const void *data, size_t size)
{
    struct SnapshotReader reader;
    struct SnapshotRecord record;
    int used = validate(data, size, &npc_format);
    if (!mode || used < 0) return -1;
    snapshot_open(&reader, data, (size_t)used, npc_format.begin);
    while (snapshot_next(&reader, &record) == 1 && record.tag != npc_format.end)
        if (record.tag == npc_format.first)
            memcpy(&mode->npc_dialog_open, record.value.data, sizeof(int32_t));
    return used;
}

typedef int (*WorldSnapshotLoader)(struct GameMode *, const void *, size_t);
static int restore_chunk(struct GameMode *mode, struct ReplayRecorder *recorder,
                         int slot, WorldSnapshotLoader load)
{
    struct ReplayBuffer chunk = {0};
    int loaded = replay_read_chunk(recorder, slot, &chunk) &&
                 (!chunk.size || load(mode, chunk.data, chunk.size) >= 0);
    replay_buffer_destroy(&chunk);
    return loaded;
}

int game_world_snapshot_restore(struct GameMode *mode, struct ReplayRecorder *recorder)
{
    return mode && mode->world && recorder &&
        restore_chunk(mode, recorder, 0xf, game_ground_items_load) &&
        restore_chunk(mode, recorder, 0x10, game_skill_units_load) &&
        restore_chunk(mode, recorder, 0x16, game_npc_dialog_load);
}
