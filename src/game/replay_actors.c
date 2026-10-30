#include "game/actor_snapshot.h"
#include "game/actor_packets.h"
#include "game/packets_entry.h"
#include "actor/actor_path.h"
#include "actor/gauge.h"
#include "actor/job.h"
#include "web/name_card_api.h"
#include "contents/guild_emblem_api.h"
#include "core/object_type.h"
#include "ui/actor_labels.h"
#include "replay/replay.h"
#include "replay/snapshot.h"
#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <mmsystem.h>

enum {
    ACTORS_BEGIN = 0x2710, ACTORS_END = 0x2711,
    ACTOR_ROOM = 0xc9, ACTOR_SHOP = 0xca, ACTOR_PROGRESS = 0xcb, ACTOR_BUY_SHOP = 0xcc,
    ACTOR_MOVE6 = 0xd0, ACTOR_STAND5 = 0xd2, ACTOR_MOVE9 = 0xd4,
    ACTOR_MOVE10 = 0xd5, ACTOR_MOVE11 = 0xd6, ACTOR_STAND8 = 0xd7,
    ACTOR_STAND10 = 0xd8, ACTOR_STAND11 = 0xd9, ACTOR_ASSISTANT = 0xda,
    ACTOR_TALKBOX = 0x191
};

static int cell_part(float value, int *cell, int *fraction)
{
    double rounded = (double)value + 0.5;
    if (!isfinite(value) || rounded > INT_MAX)
        return 0;
    if (value <= 0.0f) {
        *cell = *fraction = 0;
        return 1;
    }
    *cell = (int)rounded;
    *fraction = (int)((float)(rounded - *cell) * 16.0f);
    return 1;
}

static void append_named_packet(struct SnapshotWriter *writer, unsigned short tag,
                               const void *header, size_t header_size,
                               const char *name, size_t name_size)
{
    unsigned char *packet;
    if (writer->failed)
        return;
    if (header_size > INT16_MAX || name_size > INT16_MAX - header_size || (!name && name_size)) {
        writer->failed = 1;
        return;
    }
    packet = malloc(header_size + name_size);
    if (!packet) {
        writer->failed = 1;
        return;
    }
    memcpy(packet, header, header_size);
    if (name_size)
        memcpy(packet + header_size, name, name_size);
    snapshot_write(writer, tag, packet, header_size + name_size);
    free(packet);
}

static void append_actor_entry(struct GameMode *mode, struct SnapshotWriter *writer,
                               struct GameActor *actor, const struct Pc *pc)
{
    struct PacketStandEntry11 standing = {0};
    struct PacketStandEntry10 *packet = &standing.base;
    size_t name_size;
    const char *name = name_card_name_text(&mode->actor_names, actor->aid, &name_size);
    int moving = actor->state == 1;
    size_t header_size = moving ? sizeof(struct PacketMoveEntry11) : sizeof standing;
    if (name_size > INT16_MAX - header_size || (actor->object_type == ENTRY_OBJ_PC && !pc)) {
        writer->failed = 1;
        return;
    }
    packet->length = (short)(header_size + name_size);
    packet->object_type = actor->object_type;
    packet->aid = actor->aid;
    packet->speed = (short)actor->walk_speed;
    packet->body_state = (short)actor->body_state;
    packet->health_state = (short)actor->health_state;
    packet->effect_state = actor->effect_state;
    packet->job = (short)game_actor_get_job(actor);
    packet->head = (short)actor->entry_head;
    packet->guild_id = actor->guild_id;
    packet->emblem_version = (short)guild_mgr_emblem_version(g_guildMgr, actor->guild_id);
    packet->is_pk_mode = actor->pk_mode != 0;
    packet->sex = (unsigned char)actor->sex;
    packet->x_size = (unsigned char)actor->xsize;
    packet->y_size = (unsigned char)actor->ysize;
    packet->clevel = (short)actor->level;
    packet->font = actor->balloon_font;
    packet->body_palette = (short)actor->body_palette;
    packet->hp = game_actor_get_gage_hp(actor);
    packet->max_hp = game_actor_get_gage_max_hp(actor);
    packet->is_boss = actor->is_boss;
    if (actor->object_type == ENTRY_OBJ_PC) {
        packet->honor = (short)pc->honor;
        packet->virtue = (int)pc->virtue;
        packet->head_dir = (short)pc->head_dir;
        packet->weapon = (int)pc->virtue;
        packet->shield = (int)pc->virtue < 0 ? -1 : 0;
        packet->accessory = (short)pc->accessory[0];
        packet->accessory2 = (short)pc->accessory[1];
        packet->accessory3 = (short)pc->accessory[2];
        packet->head_palette = (short)pc->head_palette;
        packet->robe = (short)pc->robe;
        packet->char_id = pc->char_id;
        standing.body = (short)pc->body_style;
    }
    if (moving) {
        struct PacketMoveEntry11 movement = {0};
        struct PacketMoveEntry10 *move = &movement.base;
        float from_x = 0, from_y = 0;
        int height = 0, x, y, fx, fy;
        actor_path_position_at(actor, timeGetTime(), &from_x, &from_y, &height);
        if (!cell_part(from_x, &x, &fx) || !cell_part(from_y, &y, &fy)) {
            writer->failed = 1;
            return;
        }
        move->length = packet->length;
        move->object_type = packet->object_type;
        move->aid = packet->aid;
        move->char_id = packet->char_id;
        move->speed = packet->speed;
        move->body_state = packet->body_state;
        move->health_state = packet->health_state;
        move->effect_state = packet->effect_state;
        move->job = packet->job;
        move->head = packet->head;
        move->weapon = packet->weapon;
        move->shield = packet->shield;
        move->accessory = packet->accessory;
        move->accessory2 = packet->accessory2;
        move->accessory3 = packet->accessory3;
        move->head_palette = packet->head_palette;
        move->body_palette = packet->body_palette;
        move->head_dir = packet->head_dir;
        move->robe = packet->robe;
        move->guild_id = packet->guild_id;
        move->emblem_version = packet->emblem_version;
        move->honor = packet->honor;
        move->virtue = packet->virtue;
        move->is_pk_mode = packet->is_pk_mode;
        move->sex = packet->sex;
        move->x_size = packet->x_size;
        move->y_size = packet->y_size;
        move->clevel = packet->clevel;
        move->font = packet->font;
        move->max_hp = packet->max_hp;
        move->hp = packet->hp;
        move->is_boss = packet->is_boss;
        movement.body = standing.body;
        move->id = 0x9fd;
        move->move_tick = actor->path_start;
        move->move_data[0] = (unsigned char)((unsigned)x >> 2);
        move->move_data[1] = (unsigned char)(((unsigned)y >> 4 & 0x3f) | (unsigned)x << 6);
        move->move_data[2] = (unsigned char)(((unsigned)actor->cell_x >> 6 & 0xf) | (unsigned)y << 4);
        move->move_data[3] = (unsigned char)(((unsigned)actor->cell_y >> 8 & 3) | (unsigned)actor->cell_x << 2);
        move->move_data[4] = (unsigned char)actor->cell_y;
        move->move_data[5] = (unsigned char)(fx << 4 | (fy & 0xf));
        append_named_packet(writer, ACTOR_MOVE11, &movement, sizeof movement, name, name_size);
    } else {
        int direction;
        if (!isfinite(actor->roty) || (double)actor->roty > INT_MAX - 180 ||
            (double)actor->roty < INT_MIN) {
            writer->failed = 1;
            return;
        }
        direction = ((int)actor->roty + 180) / 45 % 8;
        packet->id = 0x9ff;
        packet->pos.b[0] = (unsigned char)((unsigned)actor->cell_x >> 2);
        packet->pos.b[1] = (unsigned char)(((unsigned)actor->cell_y >> 4 & 0x3f) | (unsigned)actor->cell_x << 6);
        packet->pos.b[2] = (unsigned char)((direction & 0xf) | (unsigned)actor->cell_y << 4);
        packet->state = actor->state == 3 ? 1 : actor->state == 6 ? 2 : 0;
        append_named_packet(writer, ACTOR_STAND11, &standing, sizeof standing, name, name_size);
    }
}

static void copy_text_field(char *out, size_t capacity, const char *text, size_t size)
{
    memset(out, 0, capacity);
    if (size > capacity)
        size = capacity;
    if (size)
        memcpy(out, text, size);
}

static void append_shop(struct SnapshotWriter *writer, struct GameActor *actor,
                         struct UIWindow *window, unsigned short tag, unsigned short id)
{
    struct PACKET_ZC_TALKBOX_CHATCONTENTS packet = {0};
    size_t length;
    const char *title = ui_merchant_shop_title_text(window, &length);
    packet.id = id;
    packet.aid = actor->aid;
    copy_text_field(packet.text, sizeof packet.text, title, length);
    snapshot_write(writer, tag, &packet, sizeof packet);
}

int game_actors_save(struct GameMode *mode, struct ReplayBuffer *out)
{
    struct SnapshotWriter writer = {0};
    struct ListNode *node, *head;
    if (!mode || !mode->world || !out)
        return 0;
    head = mode->world->actors.head;
    snapshot_write(&writer, ACTORS_BEGIN, NULL, 0);
    for (node = head ? head->next : NULL; node && node != head && !writer.failed; node = node->next) {
        struct GameActor *actor = *LIST_VALUE(node, struct GameActor *);
        struct Pc *pc = object_as_type(actor, OBJECT_PC);
        if (!actor) {
            writer.failed = 1;
            break;
        }
        if (!actor->is_store_assistant)
            append_actor_entry(mode, &writer, actor, pc);
        if (actor->chatroom_title_wnd) {
            const struct ChatRoomInfo *room = ui_chat_room_title_info(actor->chatroom_title_wnd);
            struct PacketRoomEntry packet = {0};
            if (room->title.size > INT16_MAX - sizeof packet) {
                writer.failed = 1;
                break;
            }
            packet.id = 0xd7;
            packet.length = (short)(sizeof packet + room->title.size);
            packet.aid = actor->aid;
            packet.room_id = room->room_id;
            packet.max_users = (short)room->max_users;
            packet.users = (short)room->users;
            packet.type = (unsigned char)room->type;
            append_named_packet(&writer, ACTOR_ROOM, &packet, sizeof packet,
                                string_cstr(&room->title), room->title.size);
        }
        if (actor->is_store_assistant == 1) {
            struct PacketStoreAssistantEntryV2 packet = {0};
            size_t length;
            const char *name = name_card_name_text(&mode->actor_names, actor->aid, &length);
            if (!pc) {
                writer.failed = 1;
                break;
            }
            packet.aid = actor->aid;
            packet.job = game_actor_get_job(actor);
            packet.x = (short)actor->cell_x;
            packet.y = (short)actor->cell_y;
            packet.sex = (unsigned char)actor->sex;
            packet.head = (short)pc->head;
            packet.weapon = (int)pc->weapon;
            packet.shield = (int)((uint64_t)pc->weapon >> 32);
            packet.accessory = (short)pc->accessory[0];
            packet.accessory2 = (short)pc->accessory[1];
            packet.accessory3 = (short)pc->accessory[2];
            packet.head_palette = (short)pc->head_palette;
            packet.body_palette = (short)actor->body_palette;
            packet.robe = (short)pc->robe;
            packet.body = (short)pc->body_style;
            copy_text_field(packet.name, sizeof packet.name, name, length);
            snapshot_write(&writer, ACTOR_ASSISTANT, &packet, sizeof packet);
        }
        if (actor->shop_title_wnd)
            append_shop(&writer, actor, actor->shop_title_wnd, ACTOR_SHOP, 0x131);
        if (actor->recharge_gage_wnd) {
            struct PacketActorProgress packet = {0};
            packet.id = 0x2f0;
            packet.seconds = (actor->cast_end_tick - actor->cast_start_tick) / 1000;
            snapshot_write(&writer, ACTOR_PROGRESS, &packet, sizeof packet);
        }
        if (actor->buy_shop_title_wnd)
            append_shop(&writer, actor, actor->buy_shop_title_wnd, ACTOR_BUY_SHOP, 0x814);
        if (pc && pc->madogear_type > 0) {
            struct PacketEfstSetEnter2 packet = {0};
            packet.aid = actor->aid;
            packet.index = 0x47d;
            packet.total_ms = packet.remain_ms = 9999;
            packet.val1 = pc->madogear_type;
            snapshot_write(&writer, ACTOR_TALKBOX, &packet, sizeof packet);
        }
    }
    snapshot_write(&writer, ACTORS_END, NULL, 0);
    return snapshot_finish(&writer, out);
}

static size_t actor_packet_size(unsigned tag, int *variable)
{
    *variable = 0;
    switch (tag) {
    case ACTOR_SHOP: case ACTOR_BUY_SHOP: return sizeof(struct PACKET_ZC_TALKBOX_CHATCONTENTS);
    case ACTOR_PROGRESS: return sizeof(struct PacketActorProgress);
    case ACTOR_ASSISTANT: return sizeof(struct PacketStoreAssistantEntryV2);
    case ACTOR_TALKBOX: return sizeof(struct PacketEfstSetEnter2);
    }
    *variable = 1;
    switch (tag) {
    case ACTOR_ROOM: return sizeof(struct PacketRoomEntry);
    case ACTOR_MOVE6: return offsetof(struct PacketMoveEntry6, name);
    case ACTOR_STAND5: return offsetof(struct PacketStandEntry5, name);
    case ACTOR_MOVE9: return offsetof(struct PacketMoveEntry9, name);
    case ACTOR_MOVE10: return sizeof(struct PacketMoveEntry10);
    case ACTOR_MOVE11: return sizeof(struct PacketMoveEntry11);
    case ACTOR_STAND8: return offsetof(struct PacketStandEntry8, name);
    case ACTOR_STAND10: return sizeof(struct PacketStandEntry10);
    case ACTOR_STAND11: return sizeof(struct PacketStandEntry11);
    }
    return 0;
}

static int validate_actors(const void *data, size_t size, size_t *largest)
{
    struct SnapshotReader reader;
    struct SnapshotRecord record;
    *largest = 0;
    if (size > INT_MAX || !snapshot_open(&reader, data, size, ACTORS_BEGIN))
        return 0;
    while (snapshot_next(&reader, &record) == 1) {
        size_t required;
        int variable;
        if (record.tag == ACTORS_END)
            return !record.value.size && reader.offset == size;
        if (record.tag == ACTORS_BEGIN)
            return 0;
        required = actor_packet_size(record.tag, &variable);
        if (!required)
            continue;
        if (variable) {
            const unsigned char *p = record.value.data;
            if (record.value.size < required || record.value.size > INT16_MAX ||
                ((unsigned)p[2] | (unsigned)p[3] << 8) != record.value.size)
                return 0;
        } else if (record.value.size != required) {
            return 0;
        }
        if (*largest < record.value.size)
            *largest = record.value.size;
    }
    return 0;
}

int game_actors_load(struct GameMode *mode, const void *data, size_t size)
{
    struct SnapshotReader reader;
    struct SnapshotRecord record;
    size_t largest;
    void *packet;
    int variable;
    if (!mode || !mode->world || !validate_actors(data, size, &largest))
        return -1;
    packet = largest ? malloc(largest) : NULL;
    if (largest && !packet)
        return -1;
    snapshot_open(&reader, data, size, ACTORS_BEGIN);
    while (snapshot_next(&reader, &record) == 1 && record.tag != ACTORS_END) {
        if (!actor_packet_size(record.tag, &variable))
            continue;
        memcpy(packet, record.value.data, record.value.size);
        switch (record.tag) {
        case ACTOR_ROOM: game_mode_zc_room_newentry(mode, packet); break;
        case ACTOR_SHOP: game_mode_show_actor_text(mode, packet); break;
        case ACTOR_PROGRESS: game_mode_player_progress(mode, packet); break;
        case ACTOR_BUY_SHOP: game_mode_buying_store_entry(mode, packet); break;
        case ACTOR_MOVE6: game_mode_zc_notify_moveentry6(mode, packet); break;
        case ACTOR_STAND5: game_mode_zc_notify_standentry5(mode, packet); break;
        case ACTOR_MOVE9: game_mode_zc_notify_moveentry9(mode, packet); break;
        case ACTOR_MOVE10: game_mode_zc_notify_moveentry10(mode, packet); break;
        case ACTOR_MOVE11: game_mode_zc_notify_moveentry11(mode, packet); break;
        case ACTOR_STAND8: game_mode_zc_notify_standentry8(mode, packet); break;
        case ACTOR_STAND10: game_mode_zc_notify_standentry10(mode, packet); break;
        case ACTOR_STAND11: game_mode_zc_notify_standentry11(mode, packet); break;
        case ACTOR_TALKBOX: game_mode_on_efst_set_enter2(mode, packet); break;
        case ACTOR_ASSISTANT:
            if (!game_store_assistant_restore(mode, packet)) {
                free(packet);
                return -1;
            }
            break;
        }
    }
    free(packet);
    return (int)size;
}

int game_actors_restore(struct GameMode *mode, struct ReplayRecorder *recorder)
{
    struct ReplayBuffer chunk = {0};
    int loaded;
    if (!mode || !mode->world || !recorder)
        return 0;
    loaded = replay_read_chunk(recorder, 0xe, &chunk) &&
        (!chunk.size || game_actors_load(mode, chunk.data, chunk.size) >= 0);
    replay_buffer_destroy(&chunk);
    return loaded;
}
