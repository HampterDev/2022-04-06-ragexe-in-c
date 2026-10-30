#include "session/snapshot_values.h"
#include "session/state.h"
#include "session/session_windows.h"
#include "ui/window_access.h"
#include "game/context.h"
#include "actor/player_state.h"
#include <string.h>

enum ValueKind { VALUE_RAW, VALUE_TEXT, VALUE_CSTRING, VALUE_LOW32, VALUE_WEAPON };
struct SessionValueField { uint16_t tag; size_t offset, size; enum ValueKind kind; int write; };
#define SV_RAW(tag, member) {tag, offsetof(struct Session, member), sizeof(((struct Session *)0)->member), VALUE_RAW, 1}
#define SV_TEXT(tag, member) {tag, offsetof(struct Session, member), 0, VALUE_TEXT, 1}
#define SV_CSTRING(tag, member) {tag, offsetof(struct Session, member), 0, VALUE_CSTRING, 1}
#define SV_LOW32(tag, member) {tag, offsetof(struct Session, member), 4, VALUE_LOW32, 1}
#define SV_BYTES(tag, member, width) {tag, offsetof(struct Session, member), width, VALUE_RAW, 1}
#define SV_ALIAS(tag, member) {tag, offsetof(struct Session, member), sizeof(((struct Session *)0)->member), VALUE_RAW, 0}

static const struct SessionValueField identity_fields[] = {
    SV_RAW(0x3f2, ownAid),
    SV_RAW(0x3f3, ownGid),
    SV_RAW(0x3f4, identity_extra),
    SV_RAW(0x3f5, decoy_sqrt50),
    SV_RAW(0x3f6, job),
    SV_RAW(0x3f7, baseExp),
    SV_RAW(0x3f8, charBaseLevel),
    SV_RAW(0x3f9, statusPoints),
    SV_RAW(0x3fa, nextBaseExp),
    SV_RAW(0x3fb, jobLevel),
    SV_RAW(0x3fc, skillPoints),
    SV_RAW(0x3fd, guildSkillPoints),
    SV_RAW(0x3fe, homunSkillPoints),
    SV_RAW(0x3ff, merSkillPoints),
    SV_RAW(0x400, stats[0]),
    SV_RAW(0x401, stats[1]),
    SV_RAW(0x402, stats[2]),
    SV_RAW(0x403, stats[3]),
    SV_RAW(0x404, stats[4]),
    SV_RAW(0x405, stats[5]),
    SV_RAW(0x406, statCosts[0]),
    SV_RAW(0x407, statCosts[1]),
    SV_RAW(0x408, statCosts[2]),
    SV_RAW(0x409, statCosts[3]),
    SV_RAW(0x40a, statCosts[4]),
    SV_RAW(0x40b, statCosts[5]),
    SV_RAW(0x40c, aspd),
    SV_RAW(0x40d, atk),
    SV_RAW(0x40e, mdef),
    SV_RAW(0x40f, plusAspd),
    SV_RAW(0x410, def),
    SV_RAW(0x411, plusDef),
    SV_RAW(0x412, refineAtk),
    SV_RAW(0x413, matkMax),
    SV_RAW(0x414, matkMin),
    SV_RAW(0x415, plusMdef),
    SV_RAW(0x416, hit),
    SV_RAW(0x417, flee),
    SV_RAW(0x418, critical),
    SV_RAW(0x419, plusFlee),
    SV_RAW(0x41a, wornArrowIndex),
    SV_RAW(0x41b, zeny),
    SV_RAW(0x41c, walkSpeed),
    SV_RAW(0x41d, manner),
    SV_LOW32(0x41e, nextJobExp),
    SV_LOW32(0x41f, jobExp),
    SV_RAW(0x420, karma),
    SV_RAW(0x421, monsterSnapInEffect),
    SV_RAW(0x422, timeStampOffset),
    SV_RAW(0x423, char_info_hair_color),
    SV_RAW(0x424, hairStyle),
    {0x425, 0, 8, VALUE_WEAPON, 1},
    SV_RAW(0x426, shield),
    SV_RAW(0x427, clothesColor),
    SV_RAW(0x428, hairColor),
    SV_RAW(0x429, headBottom),
    SV_RAW(0x42a, headTop),
    SV_RAW(0x42b, headMid),
    SV_RAW(0x42c, bodyState),
    SV_RAW(0x42d, healthState),
    SV_RAW(0x42e, effectState),
    SV_RAW(0x42f, robe),
    SV_RAW(0x430, viewedLook[0]),
    SV_RAW(0x431, viewedLook[1]),
    SV_RAW(0x432, viewedLook[2]),
    SV_RAW(0x433, viewedLook[3]),
    SV_RAW(0x434, viewedLook[4]),
    SV_RAW(0x435, viewedLook[5]),
    SV_RAW(0x436, viewedLook[6]),
    SV_RAW(0x437, viewedLook[7]),
    SV_RAW(0x438, viewedLook[8]),
    SV_RAW(0x439, viewedName),
    SV_RAW(0x43a, battlegroundCamp),
    SV_RAW(0x43b, battlegroundPoints[0]),
    SV_RAW(0x43c, battlegroundPoints[1]),
    SV_RAW(0x43d, font),
    SV_RAW(0x43e, cartCount),
    SV_RAW(0x43f, cartCountMax),
    SV_RAW(0x440, cartWeight),
    SV_RAW(0x441, cartWeightMax),
    SV_RAW(0x442, vendingSlots),
    SV_RAW(0x443, buyingStoreSlots),
    SV_RAW(0x444, chatTarget),
    SV_RAW(0x445, chat_target_extra),
    SV_RAW(0x446, partyExpOption),
    SV_RAW(0x447, partyItemPickupRule),
    SV_RAW(0x448, partyItemDivisionRule),
    SV_TEXT(0x449, shopTitle),
    SV_RAW(0x44a, showGuildEmblems),
    SV_RAW(0x44b, married),
    SV_RAW(0x44c, body),
    SV_RAW(0x44d, inventoryExtension),
};

static const struct SessionValueField character_fields[] = {
    SV_RAW(0x7da, statBonus[0]),
    SV_RAW(0x7db, statBonus[1]),
    SV_RAW(0x7dc, statBonus[2]),
    SV_RAW(0x7dd, statBonus[3]),
    SV_RAW(0x7de, statBonus[4]),
    SV_RAW(0x7df, statBonus[5]),
    SV_RAW(0x7e0, attackRange),
    SV_RAW(0x7e1, weight),
    SV_RAW(0x7e2, maxWeight),
    SV_RAW(0x7e3, rerollValues[0]),
    SV_RAW(0x7e4, rerollValues[1]),
    SV_RAW(0x7e5, rerollValues[2]),
    SV_RAW(0x7e6, rerollValues[3]),
    SV_RAW(0x7e7, rerollValues[4]),
    SV_RAW(0x7e8, rerollZeros),
    SV_BYTES(0x7e9, vital_slot_tables, 48),
    SV_BYTES(0x7ea, vital_slot_tables[48], 48),
    SV_BYTES(0x7eb, vital_slot_tables[96], 48),
    SV_BYTES(0x7ec, vital_slot_tables[144], 48),
    SV_RAW(0x7ed, hp),
    SV_RAW(0x7ee, maxHp),
    SV_RAW(0x7ef, sp),
    SV_RAW(0x7f0, maxSp),
    SV_RAW(0x7f1, prohibitTarget),
    SV_RAW(0x7f2, battle_mode),
    SV_RAW(0x7f3, battleChat),
    SV_RAW(0x7f4, battle_mode_suspended),
    SV_RAW(0x7f5, battle_extra_flag),
    SV_TEXT(0x7f6, worldName),
    SV_RAW(0x804, madogear),
};

static const struct SessionValueField companions_fields[] = {
    SV_RAW(0x13ed, homunAid),
    SV_RAW(0x13ee, homunName),
    SV_RAW(0x13ef, homunStats[0]),
    SV_RAW(0x13f0, homunStats[1]),
    SV_RAW(0x13f1, homunStats[2]),
    SV_RAW(0x13f2, homunStats[3]),
    SV_RAW(0x13f3, homunStats[4]),
    SV_RAW(0x13f4, homunStats[5]),
    SV_RAW(0x13f5, homunStats[6]),
    SV_RAW(0x13f6, homunStats[7]),
    SV_RAW(0x13f7, homunJob),
    SV_RAW(0x13f8, homunLevel),
    SV_RAW(0x13f9, homunHp),
    SV_RAW(0x13fa, homunMaxHp),
    SV_RAW(0x13fb, homunSp),
    SV_RAW(0x13fc, homunMaxSp),
    SV_RAW(0x13fd, homunIntimacy),
    SV_RAW(0x13fe, homunItem),
    SV_RAW(0x13ff, homunExp),
    SV_RAW(0x1400, homunMaxExp),
    SV_RAW(0x1401, homunFullness),
    SV_RAW(0x1402, homunMaxFullness),
    SV_RAW(0x1403, homunRenamed),
    SV_RAW(0x1404, homun_command[0]),
    SV_RAW(0x1405, homun_command[1]),
    SV_RAW(0x1406, homun_command[2]),
    SV_RAW(0x1407, homun_command[3]),
    SV_RAW(0x1408, homun_command[4]),
    SV_RAW(0x1409, homun_command[5]),
    SV_RAW(0x140a, homun_ai_flags[0]),
    SV_RAW(0x140b, homun_ai_flags[1]),
    SV_RAW(0x140c, homun_ai_state[0]),
    SV_RAW(0x140d, homun_ai_state[1]),
    SV_RAW(0x140e, homun_ai_state[2]),
    SV_RAW(0x1451, mercAid),
    SV_RAW(0x1452, mercName),
    SV_RAW(0x1453, mercStats[0]),
    SV_RAW(0x1454, mercStats[1]),
    SV_RAW(0x1455, mercStats[2]),
    SV_RAW(0x1456, mercStats[3]),
    SV_RAW(0x1457, mercStats[4]),
    SV_RAW(0x1458, mercStats[5]),
    SV_RAW(0x1459, mercStats[6]),
    SV_RAW(0x145a, mercStats[7]),
    SV_RAW(0x145b, mercHp),
    SV_RAW(0x145c, mercMaxHp),
    SV_RAW(0x145d, mercJob),
    SV_RAW(0x145e, mercLevel),
    SV_RAW(0x145f, mercSp),
    SV_RAW(0x1460, mercMaxSp),
    SV_RAW(0x1461, merc_contract_stats[0]),
    SV_RAW(0x1462, merc_contract_stats[1]),
    SV_RAW(0x1463, merc_contract_stats[2]),
    SV_RAW(0x1464, merc_contract_stats[3]),
    SV_RAW(0x1465, merc_contract_stats[4]),
    SV_RAW(0x1466, merc_command[0]),
    SV_RAW(0x1467, merc_command[1]),
    SV_RAW(0x1468, merc_command[2]),
    SV_RAW(0x1469, merc_command[3]),
    SV_RAW(0x146a, merc_command[5]),
    SV_RAW(0x146b, merc_command[6]),
    SV_RAW(0x146c, mercPresent),
    SV_RAW(0x146d, mercOrder),
    SV_RAW(0x14b5, aid),
    SV_RAW(0x14b6, petModified),
    SV_RAW(0x14b7, petName),
    SV_RAW(0x14b8, petAccessory),
    SV_RAW(0x14b9, petJob),
    SV_RAW(0x14ba, petLevel),
    SV_RAW(0x14bb, petFullness),
    SV_RAW(0x14bc, petIntimacy),
    SV_RAW(0x14bd, hatchedEggIndex),
    SV_RAW(0x14be, petPrevFullness),
    SV_RAW(0x1519, elementalAid),
    SV_RAW(0x151a, elementalHp),
    SV_RAW(0x151b, elementalSp),
    SV_RAW(0x151c, elementalMaxHp),
    SV_RAW(0x151d, elementalMaxSp),
    SV_ALIAS(0x146e, homunSp),
    SV_ALIAS(0x146f, homun_ai_flags[1]),
};

static const struct SessionValueField dungeon_fields[] = {
    SV_RAW(0x1586, mdWndShown),
    SV_RAW(0x1587, mdWndX),
    SV_RAW(0x1588, mdWndY),
    SV_RAW(0x1589, mdState),
    SV_RAW(0x158a, md_extra_state),
    SV_CSTRING(0x158b, mdName),
    SV_RAW(0x158c, mdQueuePlace),
    SV_RAW(0x158d, mdDestroyTime),
    SV_RAW(0x158e, mdEnterLimit),
};

static const struct SessionValueField hotkeys_fields[] = {
    SV_RAW(0x1bbc, hotkeys0),
    SV_RAW(0x1bbd, quickSlots),
};

struct ValueSchema {
    uint16_t open, close, alternate_close;
    const struct SessionValueField *fields;
    size_t count;
};
#define SCHEMA(open, close, alternate, name) {open, close, alternate, name##_fields, sizeof name##_fields / sizeof name##_fields[0]}
static const struct ValueSchema schemas[] = {
    SCHEMA(0x3e8, 0x3e9, 0, identity),
    SCHEMA(0x7d0, 0x7d1, 0, character),
    SCHEMA(0x1388, 0x1389, 0x1470, companions),
    SCHEMA(0x157c, 0x157d, 0, dungeon),
    SCHEMA(0x1b58, 0x1b59, 0, hotkeys)
};
#undef SCHEMA
#undef SV_RAW
#undef SV_TEXT
#undef SV_CSTRING
#undef SV_LOW32
#undef SV_BYTES
#undef SV_ALIAS

static const struct ValueSchema *get_schema(enum SessionValueSnapshot kind)
{
    return (unsigned)kind < sizeof schemas / sizeof schemas[0] ? &schemas[kind] : NULL;
}

static const struct SessionValueField *find_field(const struct ValueSchema *schema, uint16_t tag)
{
    size_t i;
    for (i = 0; i < schema->count; ++i)
        if (schema->fields[i].tag == tag)
            return &schema->fields[i];
    return NULL;
}

int session_values_save(const struct Session *session, enum SessionValueSnapshot kind,
                        struct ReplayBuffer *out)
{
    const struct ValueSchema *schema = get_schema(kind);
    struct SnapshotWriter writer = {0};
    size_t i;
    if (!schema || !session)
        return 0;
    snapshot_write(&writer, schema->open, NULL, 0);
    for (i = 0; i < schema->count; ++i) {
        const struct SessionValueField *field = &schema->fields[i];
        const void *value = (const unsigned char *)session + field->offset;
        if (!field->write)
            continue;
        if (field->kind == VALUE_TEXT || field->kind == VALUE_CSTRING) {
            const struct String *text = value;
            snapshot_write(&writer, field->tag, string_cstr(text), text->size);
        } else if (field->kind == VALUE_WEAPON) {
            snapshot_write(&writer, field->tag, &session->weapon, sizeof session->weapon);
        } else if (field->kind == VALUE_LOW32) {
            uint64_t complete;
            uint32_t low;
            memcpy(&complete, value, sizeof complete);
            low = (uint32_t)complete;
            snapshot_write(&writer, field->tag, &low, sizeof low);
        } else {
            snapshot_write(&writer, field->tag, value, field->size);
        }
    }
    snapshot_write(&writer, schema->close, NULL, 0);
    return snapshot_finish(&writer, out);
}

int session_values_validate(enum SessionValueSnapshot kind, const void *data, size_t size)
{
    const struct ValueSchema *schema = get_schema(kind);
    struct SnapshotReader reader;
    struct SnapshotRecord record;
    if (!schema || !snapshot_open(&reader, data, size, schema->open))
        return -1;
    while (snapshot_next(&reader, &record) == 1) {
        const struct SessionValueField *field;
        if (record.tag == schema->close || (schema->alternate_close && record.tag == schema->alternate_close))
            return record.value.size ? -1 : (int)reader.offset;
        if (record.tag == schema->open)
            return -1;
        field = find_field(schema, record.tag);
        if (field && field->kind != VALUE_TEXT && field->kind != VALUE_CSTRING && record.value.size != field->size)
            return -1;
    }
    return -1;
}

int session_values_load(struct Session *session, enum SessionValueSnapshot kind,
                        const void *data, size_t size)
{
    const struct ValueSchema *schema = get_schema(kind);
    struct SnapshotReader reader;
    struct SnapshotRecord record;
    int used = session_values_validate(kind, data, size);
    if (!session || used < 0)
        return -1;
    snapshot_open(&reader, data, (size_t)used, schema->open);
    while (snapshot_next(&reader, &record) == 1) {
        const struct SessionValueField *field = find_field(schema, record.tag);
        void *value;
        if (!field)
            continue;
        value = (unsigned char *)session + field->offset;
        if (field->kind == VALUE_TEXT || field->kind == VALUE_CSTRING) {
            size_t length = record.value.size;
            if (field->kind == VALUE_CSTRING && length) {
                const unsigned char *zero = memchr(record.value.data, 0, length);
                if (zero) length = zero - record.value.data;
            }
            string_assign(value, (const char *)record.value.data, length);
        } else if (field->kind == VALUE_WEAPON) {
            memcpy(&session->weapon, record.value.data, sizeof session->weapon);
        } else if (field->kind == VALUE_LOW32) {
            uint64_t complete;
            uint32_t low;
            memcpy(&complete, value, sizeof complete);
            memcpy(&low, record.value.data, sizeof low);
            complete = (complete & UINT64_C(0xffffffff00000000)) | low;
            memcpy(value, &complete, sizeof complete);
        } else {
            memcpy(value, record.value.data, field->size);
        }
        if (kind == SESSION_VALUES_CHARACTER && record.tag == 0x804)
            player_restore_madogear(game_current_player(), session->madogear);
    }
    if (kind == SESSION_VALUES_CHARACTER)
        session_invalidate_windows(session);
    if (kind == SESSION_VALUES_DUNGEON && session->mdWndShown == 1)
        ui_window_open(0x89);
    return used;
}
