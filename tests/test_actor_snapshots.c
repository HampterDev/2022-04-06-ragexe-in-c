#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "game/gamemode.h"
#include <windows.h>
#include <mmsystem.h>
#include "contents/guild_emblem_api.h"
#include "replay/replay.h"
#include "replay/snapshot.h"
#ifndef ACTOR_SNAPSHOT_ORACLE
#include "actor/actor_ops.h"
#include "game/actor_packets.h"
#include "game/replay_actor_events.h"
#include "game/packets_entry.h"
#include "core/object_type.h"
#include "ui/actor_labels.h"
#endif
static struct Block
{
    unsigned char *base;
    size_t size;
} blocks[256];
static int allocations, attempts, fail_at;
static int block_index(void *p)
{
    int i;
    for (i = 0; i < 256; ++i)
        if (blocks[i].base && blocks[i].base + 8 == p)
            return i;
    assert(0);
    return -1;
}
static void check_guards(void)
{
    int i, j;
    for (i = 0; i < 256; ++i)
        if (blocks[i].base)
            for (j = 0; j < 8; ++j)
            {
                assert(blocks[i].base[j] == 0xa7);
                assert(blocks[i].base[8 + blocks[i].size + j] == 0xa7);
            }
}
static void *allocate(size_t size)
{
    int i;
    if (++attempts == fail_at)
        return NULL;
    for (i = 0; i < 256 && blocks[i].base; ++i)
    {
    }
    assert(i < 256 && size <= SIZE_MAX - 16);
    blocks[i].base = malloc(size + 16);
    assert(blocks[i].base);
    blocks[i].size = size;
    memset(blocks[i].base, 0xa7, size + 16);
    ++allocations;
    return blocks[i].base + 8;
}
static void release(void *p)
{
    int i;
    if (!p)
        return;
    check_guards();
    i = block_index(p);
    free(blocks[i].base);
    blocks[i].base = NULL;
    --allocations;
}
static void *resize(void *p, size_t size)
{
    void *result = allocate(size);
    if (result && p)
    {
        size_t old_size = blocks[block_index(p)].size;
        memcpy(result, p, size < old_size ? size : old_size);
        release(p);
    }
    return result;
}

static struct GameMode mode;
static struct World world;
static struct Pc actors[2];
static struct { struct ListNode links; void *value; } nodes[3];
static struct ReplayRecorder recorder;
static struct ReplayBuffer recorded;
static int handled, read_ok = 1, assistant_ok = 1;
#ifdef ACTOR_SNAPSHOT_ORACLE
static const void *pc_table[55], *npc_table[55];
#define PC_TABLE pc_table
#define NPC_TABLE npc_table
typedef void SnapshotActor;
typedef void SnapshotRender;
#else
static struct GameActorOps pc_table, npc_table;
#define PC_TABLE (&pc_table.base.base)
#define NPC_TABLE (&npc_table.base.base)
typedef struct GameActor SnapshotActor;
typedef struct RenderObject SnapshotRender;
#endif
static float path_x = 37.25f, path_y = 61.875f;
static char actor_name[128], shop_name[128], room_name[128];
static size_t actor_name_size;
static struct RoomFixture { unsigned char base[0x7c]; void *text; struct ChatRoomInfo room; int font; } room;
struct StaticFixture { unsigned char display[0xc0]; struct String tooltip; };
static struct ShopFixture { unsigned char base[0x7c]; struct StaticFixture *title; int owner, font, buying; } shop;
static struct StaticFixture shop_text;

#define malloc allocate
#define realloc resize
#define free release
#define timeGetTime() 100u
#ifdef ACTOR_SNAPSHOT_ORACLE
#else
#include "../src/replay/snapshot.c"
#include "../src/game/replay_actors.c"
#include "../src/game/replay_actor_events.c"
#endif
#undef timeGetTime
#undef malloc
#undef realloc
#undef free

void *mem_alloc(size_t size) { void *p = allocate(size); assert(p); return p; }
void mem_free(void *p)
{  release(p); }
void fatal_error(const char *message) { (void)message; abort(); }
struct GuildMgr *g_guildMgr;
int guild_mgr_emblem_version(struct GuildMgr *manager, unsigned guild)
{ (void)manager; return guild ? 42 : 0; }
int actor_path_position_at(struct GameActor *actor, uint32_t time, float *x, float *y, int *direction)
{ (void)actor; (void)time; *x = path_x; *y = path_y; *direction = 3; return 1; }
#ifdef ACTOR_SNAPSHOT_ORACLE
int game_actor_get_job(void *actor) { return ((struct GameActor *)actor)->job; }
int game_actor_get_gage_hp(void *actor) { (void)actor; return 123; }
int game_actor_get_gage_max_hp(void *actor) { (void)actor; return 456; }
#else
int game_actor_get_job(struct GameActor *actor) { return actor->job; }
int game_actor_get_gage_hp(struct GameActor *actor) { (void)actor; return 123; }
int game_actor_get_gage_max_hp(struct GameActor *actor) { (void)actor; return 456; }
void *object_as_type(void *object, enum ObjectType type)
{ assert(type == OBJECT_PC); return object && ((struct GameActor *)object)->ops == PC_TABLE ? object : NULL; }
const char *name_card_name_text(const struct ActorNameCardHolder *names, unsigned aid, size_t *length)
{ (void)names; (void)aid; if (length) *length = actor_name_size; return actor_name; }
const struct ChatRoomInfo *ui_chat_room_title_info(const struct UIWindow *window)
{ assert((const void *)window == &room); return &room.room; }
const char *ui_merchant_shop_title_text(const struct UIWindow *window, size_t *length)
{ assert((const void *)window == &shop); *length = shop_text.tooltip.size; return string_cstr(&shop_text.tooltip); }
#endif
struct ReplayRecorder *replay_recorder_instance(void) { return &recorder; }
BYTE replay_recorder_write_packet(struct ReplayRecorder *r, unsigned short size, const void *packet, int time_mode, DWORD time_at)
{
    (void)r; assert(time_mode == REPLAY_TIME_NOW && !time_at);
    release(recorded.data); recorded.data = allocate(size); assert(recorded.data);
    memcpy(recorded.data, packet, size); recorded.size = size; return 1;
}
int replay_read_chunk(struct ReplayRecorder *r, int slot, struct ReplayBuffer *out)
{
    (void)r; assert(slot == 0xe);
    if (!read_ok) return 0;
    if (recorded.size) {
        out->data = allocate(recorded.size);
        if (!out->data) return 0;
        memcpy(out->data, recorded.data, recorded.size);
    }
    out->size = recorded.size; return 1;
}
static int effect(SnapshotRender *p) { return ((struct GameActor *)p)->effect_state; }
static int guild(SnapshotActor *p) { return ((struct GameActor *)p)->guild_id; }
static int char_id(SnapshotActor *p) { return ((struct Pc *)p)->char_id; }
static int count(SnapshotActor *p) { return ((struct Pc *)p)->madogear_type; }
static int zero(SnapshotActor *p) { (void)p; return 0; }
static long long weapon(SnapshotActor *p) { return ((struct Pc *)p)->weapon; }
#ifndef ACTOR_SNAPSHOT_ORACLE
int game_actor_get_guild_id(struct GameActor *self) { return guild(self); }
#endif

static void fixture(int kind)
{
    struct Pc *pc = &actors[0];
    memset(&mode, 0, sizeof mode); memset(&world, 0, sizeof world);
    memset(actors, 0, sizeof actors); memset(nodes, 0, sizeof nodes);
    memset(&room, 0, sizeof room); memset(&shop, 0, sizeof shop); memset(&shop_text, 0, sizeof shop_text);
    memset(actor_name, 0, sizeof actor_name); memcpy(actor_name, "actor", 5); actor_name_size = 5;
    memset(shop_name, 0, sizeof shop_name); memcpy(shop_name, "items for sale", 14);
    memset(room_name, 0, sizeof room_name); memcpy(room_name, "chat", 4);
    string_init(&room.room.title); string_init(&shop_text.tooltip);
    room.room.title.data = room_name; room.room.title.size = 4; room.room.title.capacity = 4;
    shop_text.tooltip.data = shop_name; shop_text.tooltip.capacity = 127; shop_text.tooltip.size = 14;
    room.room.type = 1; room.room.users = 3; room.room.max_users = 12; room.room.room_id = 765;
    shop.title = &shop_text;
#ifdef ACTOR_SNAPSHOT_ORACLE
    pc_table[13] = effect; pc_table[49] = guild; pc_table[42] = char_id; pc_table[54] = count; pc_table[35] = weapon;
    npc_table[13] = effect; npc_table[49] = guild; npc_table[42] = zero; npc_table[54] = zero;
#else
    pc_table.base.get_effect_state = effect;
    pc_table.get_char_id = char_id; pc_table.get_sprite_variant = count; pc_table.get_weapon = weapon;
    npc_table.base.get_effect_state = effect;
    npc_table.get_char_id = zero; npc_table.get_sprite_variant = zero;
#endif
    mode.world = &world; world.actors.head = &nodes[0].links; world.actors.size = 1;
    nodes[0].links.next = nodes[0].links.prev = &nodes[1].links; nodes[1].links.next = nodes[1].links.prev = &nodes[0].links; nodes[1].value = pc;
    pc->ops = PC_TABLE; pc->aid = 12345; pc->job = 4011; pc->walk_speed = 123;
    pc->body_state = 0x1234; pc->health_state = 0x4321; pc->effect_state = 0x12345678;
    pc->guild_id = 99; pc->pk_mode = 2; pc->sex = 1; pc->xsize = 3; pc->ysize = 4;
    pc->level = 234; pc->balloon_font = 5; pc->body_palette = 6; pc->is_boss = 1;
    pc->honor = 7; pc->virtue = 0xf2345678; pc->head_dir = 2; pc->head = 8;
    pc->weapon = 0x1234567800112233LL; pc->accessory[0] = 9; pc->accessory[1] = 10; pc->accessory[2] = 11;
    pc->head_palette = 12; pc->robe = 13; pc->char_id = 0x33445566; pc->body_style = 14;
    pc->cell_x = 341; pc->cell_y = 683; pc->roty = 90; pc->path_start = 0x12345678;
#ifdef ACTOR_SNAPSHOT_ORACLE
    { int head = 15; memcpy(pc->entry_head, &head, sizeof head); }
#else
    pc->entry_head = 15;
#endif
    if (kind == 1) pc->state = 1;
    if (kind == 2) pc->state = 3;
    if (kind == 3) pc->state = 6;
    if (kind == 4) { pc->object_type = 5; pc->ops = NPC_TABLE; pc->state = 1; }
    if (kind == 5 || kind == 6) {
#ifdef ACTOR_SNAPSHOT_ORACLE
        pc->is_store_assistant = 1;
#else
        pc->is_store_assistant = 1;
#endif
        pc->shop_title_wnd = (void *)&shop; pc->buy_shop_title_wnd = (void *)&shop;
        pc->chatroom_title_wnd = (void *)&room; pc->recharge_gage_wnd = (void *)&room;
        pc->cast_start_tick = 0xfffff000u; pc->cast_end_tick = 2345; pc->madogear_type = 5;
    }
    if (kind == 6) {
        actors[1] = actors[0]; actors[1].aid = 54321;
        actors[0].chatroom_title_wnd = NULL;
        world.actors.size = 2; nodes[1].links.next = &nodes[2].links; nodes[2].links.next = &nodes[0].links;
        nodes[2].links.prev = &nodes[1].links; nodes[0].links.prev = &nodes[2].links; nodes[2].value = &actors[1];
    }
}

#ifdef ACTOR_SNAPSHOT_ORACLE
#else
static void delivered(const void *packet) { assert(packet); ++handled; }
void game_mode_zc_room_newentry(struct GameMode *self, const struct PacketRoomEntry *packet) { (void)self; delivered(packet); }
void game_mode_show_actor_text(struct GameMode *self, const struct PACKET_ZC_TALKBOX_CHATCONTENTS *packet) { (void)self; delivered(packet); }
void game_mode_player_progress(struct GameMode *self, const struct PacketActorProgress *packet) { (void)self; delivered(packet); }
void game_mode_buying_store_entry(struct GameMode *self, const struct PACKET_ZC_TALKBOX_CHATCONTENTS *packet) { (void)self; delivered(packet); }
void game_mode_on_efst_set_enter2(struct GameMode *self, const struct PacketEfstSetEnter2 *packet) { (void)self; delivered(packet); }
void game_mode_zc_notify_moveentry6(struct GameMode *self, const struct PacketMoveEntry6 *packet) { (void)self; delivered(packet); }
void game_mode_zc_notify_standentry5(struct GameMode *self, const struct PacketStandEntry5 *packet) { (void)self; delivered(packet); }
void game_mode_zc_notify_moveentry9(struct GameMode *self, const struct PacketMoveEntry9 *packet) { (void)self; delivered(packet); }
void game_mode_zc_notify_moveentry10(struct GameMode *self, const struct PacketMoveEntry10 *packet) { (void)self; delivered(packet); }
void game_mode_zc_notify_moveentry11(struct GameMode *self, const struct PacketMoveEntry11 *packet) { (void)self; delivered(packet); }
void game_mode_zc_notify_standentry8(struct GameMode *self, const struct PacketStandEntry8 *packet) { (void)self; delivered(packet); }
void game_mode_zc_notify_standentry10(struct GameMode *self, const struct PacketStandEntry10 *packet) { (void)self; delivered(packet); }
void game_mode_zc_notify_standentry11(struct GameMode *self, const struct PacketStandEntry11 *packet) { (void)self; delivered(packet); }
int game_store_assistant_restore(struct GameMode *self, const struct PacketStoreAssistantEntryV2 *packet)
{ (void)self; delivered(packet); return assistant_ok; }

#include "fixtures/actor_snapshots_wire.h"
static void same_wire(int kind, const struct ReplayBuffer *out)
{
    struct SnapshotReader reader; struct SnapshotRecord record;
    unsigned char expected[1024];
    assert(out->size == actor_wire[kind].size && out->size <= sizeof expected);
    memcpy(expected, actor_wire[kind].data, out->size);
    assert(snapshot_open(&reader, expected, out->size, ACTORS_BEGIN));
    while (snapshot_next(&reader, &record) == 1)
        if (record.tag == ACTOR_ROOM) {
            size_t offset = (size_t)(record.value.data - expected);
            expected[offset] = 0xd7; expected[offset + 1] = 0;
        } else if (record.tag == ACTOR_ASSISTANT) {
            struct PacketStoreAssistantEntryV2 *packet = (void *)record.value.data;
            char *end = memchr(packet->name, 0, sizeof packet->name);
            if (end)
                memset(end, 0, sizeof packet->name - (size_t)(end - packet->name));
        }
    assert(!memcmp(expected, out->data, out->size));
}
static void snapshots(void)
{
    struct ReplayBuffer out = {0};
    unsigned char saved[1024], damaged[1024];
    int kind, failures, i, alive;
    for (kind = 0; kind < 7; ++kind) {
        fixture(kind); assert(game_actors_save(&mode, &out)); same_wire(kind, &out);
        assert(out.size < sizeof saved); memcpy(saved, out.data, out.size);
        handled = 0; assert(game_actors_load(&mode, out.data, out.size) == (int)out.size); assert(handled);
        assert(!memcmp(out.data, saved, out.size));
        for (i = 0; i < (int)out.size; ++i) {
            handled = 0; assert(game_actors_load(&mode, out.data, i) == -1); assert(!handled);
        }
        memcpy(damaged, out.data, out.size); damaged[8] = 0xff; damaged[9] = 0xff;
        handled = 0; assert(game_actors_load(&mode, damaged, out.size) == -1 && !handled);
        attempts = 0; fail_at = 0; assert(game_actors_save(&mode, &out)); failures = attempts; alive = allocations;
        for (i = 1; i <= failures; ++i) {
            unsigned char *before = out.data; size_t size = out.size;
            attempts = 0; fail_at = i;
            assert(!game_actors_save(&mode, &out)); assert(out.data == before && out.size == size);
            assert(!memcmp(out.data, saved, size) && allocations == alive); check_guards();
        }
        attempts = 0; fail_at = 1; handled = 0;
        assert(game_actors_load(&mode, out.data, out.size) == -1 && !handled && allocations == alive);
        fail_at = 0;
    }
    replay_buffer_destroy(&out);
}
static void validation(void)
{
    struct SnapshotWriter writer = {0}; struct ReplayBuffer out = {0};
    unsigned char packet[256] = {0}; unsigned tags[] = {ACTOR_ROOM,ACTOR_SHOP,ACTOR_PROGRESS,ACTOR_BUY_SHOP,
        ACTOR_MOVE6,ACTOR_STAND5,ACTOR_MOVE9,ACTOR_MOVE10,ACTOR_MOVE11,ACTOR_STAND8,ACTOR_STAND10,ACTOR_STAND11,ACTOR_ASSISTANT,ACTOR_TALKBOX};
    size_t i, size; int variable, delta;
    fixture(0);
    for (i = 0; i < sizeof tags / sizeof tags[0]; ++i) {
        size_t required = actor_packet_size(tags[i], &variable);
        for (delta = -1; delta <= 1; ++delta) {
            size = required + delta; memset(packet, 0, sizeof packet); packet[2] = (unsigned char)size;
            snapshot_write(&writer, ACTORS_BEGIN, NULL, 0); snapshot_write(&writer, (uint16_t)tags[i], packet, size);
            snapshot_write(&writer, ACTORS_END, NULL, 0); assert(snapshot_finish(&writer, &out));
            handled = 0;
            if (delta == 0 || (variable && delta == 1)) assert(game_actors_load(&mode, out.data, out.size) == (int)out.size && handled == 1);
            else assert(game_actors_load(&mode, out.data, out.size) == -1 && !handled);
            if (variable && delta >= 0) {
                out.data[14] ^= 1; handled = 0;
                assert(game_actors_load(&mode, out.data, out.size) == -1 && !handled);
            }
        }
    }
    snapshot_write(&writer, ACTORS_BEGIN, NULL, 0); snapshot_write(&writer, 0x5555, "unknown", 7);
    snapshot_write(&writer, ACTORS_END, NULL, 0); assert(snapshot_finish(&writer, &out));
    handled = 0; assert(game_actors_load(&mode, out.data, out.size) == (int)out.size && !handled);
    out.data[out.size - 6] = 0x10; assert(game_actors_load(&mode, out.data, out.size) == -1);
    replay_buffer_destroy(&out);
    fixture(0); actor_name_size = INT16_MAX;
    assert(!game_actors_save(&mode, &out) && !out.data);
    fixture(0); actors[0].ops = NPC_TABLE;
    assert(!game_actors_save(&mode, &out) && !out.data);
    fixture(1); path_x = (float)HUGE_VAL;
    assert(!game_actors_save(&mode, &out) && !out.data); path_x = 37.25f;
    fixture(0); actors[0].roty = (float)HUGE_VAL;
    assert(!game_actors_save(&mode, &out) && !out.data);
    fixture(0); assert(game_actors_save(&mode, &recorded));
    handled = 0; assert(game_actors_restore(&mode, &recorder) && handled == 1);
    read_ok = 0; handled = 0; assert(!game_actors_restore(&mode, &recorder) && !handled); read_ok = 1;
    replay_buffer_destroy(&recorded); assert(game_actors_restore(&mode, &recorder));
}
static void events(void)
{
    struct ReplayWhisper *whisper; struct ReplayOwnRoom *header;
    char long_title[501];
    fixture(0); memset(&recorder, 0, sizeof recorder);
    string_init(&mode.whisper_target); string_init(&mode.whisper_text);
    game_mode_record_whisper(&mode, "a", "b"); assert(!recorded.size);
    recorder.state = REPLAY_STATE_RECORDING;
    game_mode_record_whisper(&mode, "a", "b"); assert(!recorded.size);
    recorder.info.options[6] = 1;
    game_mode_record_whisper(&mode, "a", "b"); assert(recorded.size == 254);
    whisper = (void *)recorded.data; assert(!strcmp(whisper->receiver, "a") && !strcmp(whisper->text, "b"));
    assert(game_replay_whisper(&mode, recorded.data, recorded.size));
    assert(!strcmp(string_cstr(&mode.whisper_target), "a"));
    memset(whisper->receiver, 'x', sizeof whisper->receiver); memset(whisper->text, 'y', sizeof whisper->text);
    assert(game_replay_whisper(&mode, recorded.data, recorded.size));
    assert(mode.whisper_target.size == 25 && mode.whisper_text.size == 223);
    assert(!game_replay_whisper(&mode, recorded.data, recorded.size - 1));
    string_free(&mode.whisper_target); string_free(&mode.whisper_text);
    assert(!game_mode_record_mode_msg(&mode, 0x45, 99, 2, 3));
    assert(game_mode_record_mode_msg(&mode, 0x45, 10000, 2, 3) && recorded.size == 22);
    memset(long_title, 't', 500); long_title[500] = 0;
    string_init(&mode.own_chat_room.title); string_init(&mode.own_chat_room.password);
    string_assign(&mode.own_chat_room.title, long_title, 500);
    string_assign_cstr(&mode.own_chat_room.password, "pw");
    game_mode_record_own_chat_room(&mode); assert(recorded.size == 532);
    header = (void *)recorded.data; assert(header->title_size == 500 && header->password_size == 2);
    assert(game_replay_chat_room(&mode, recorded.data, recorded.size));
    assert(mode.own_chat_room.title.size == 500 && !strcmp(string_cstr(&mode.own_chat_room.password), "pw"));
    header->password_size = 3; assert(!game_replay_chat_room(&mode, recorded.data, recorded.size));
    assert(mode.own_chat_room.password.size == 2); header->password_size = 2;
    attempts = 0; fail_at = 1; recorder.writeFailed = 0;
    game_mode_record_own_chat_room(&mode); assert(recorder.writeFailed); fail_at = 0;
    string_free(&mode.own_chat_room.title); string_free(&mode.own_chat_room.password);
    replay_buffer_destroy(&recorded);
}
int main(void)
{
    snapshots(); validation(); events(); assert(!allocations);
    puts("Actor snapshots: archived bytes, named packets, bounded text, every truncation, allocation rollback, immutable input and checked restore passed");
    return 0;
}
#endif
