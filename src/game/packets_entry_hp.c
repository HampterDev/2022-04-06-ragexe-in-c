#include "contents/champion_monster.h"
#include "actor/actor_ops.h"
#include "contents/champion_monster_effects.h"
#include "contents/player_names.h"
#include "session/state.h"
#include "platform/win_types.h"
#include "game/packets_entry.h"
#include "game/packet_data.h"
#include "actor/actor_common.h"
#include "actor/game_actor.h"
#include "effects/rag/rag_effect.h"
#include "session/session_jobs.h"
#include "session/session_state.h"
#include "session/session_talk.h"
#include "world/world.h"

#define ACTOR_MSG_HP            0x91
#define ENTRY_PAR_NEW_PC        2
#define ENTRY_PAR_NEW_UNIT      1
#define ENTRY_JOB_GUILD_FLAG           0x2d2

static struct GameActor *entry_new_actor(struct GameMode *self, unsigned char type, short job)
{
    if (type == ENTRY_OBJ_PC) {
        void *mem = mem_alloc(sizeof(struct Pc));

        return mem ? (struct GameActor *)pc_construct(mem) : 0;
    }
    return game_mode_new_actor_for_job(self, job);
}

static int entry_unit_keeps_sex(unsigned char type, short job, int third)
{
    return (type == 1 || type == 6 || type == third) && session_is_pc_job(&g_session1, job) == 1;
}

static void entry_mark_moved(struct GameMode *self, struct GameActor *actor, int x, int y)
{
    struct MemberMark *mark;
    unsigned key = actor->aid;

    mark = tree_find(&self->square_marks, &key, tree_less_uint);
    if (mark) {
        mark->x = x;
        mark->y = y;
    }
    mark = tree_find(&self->triangle_marks, &key, tree_less_uint);
    if (mark) {
        mark->x = x;
        mark->y = y;
    }
}

static void entry_hp_tail(struct GameMode *self, struct GameActor *actor, unsigned char type, unsigned aid,
                          int accessory, unsigned guild_id, short emblem_version, short clevel, short font,
                          int hp, int max_hp, unsigned char is_boss)
{
    long long boss = 0;

    if (is_boss && actor->is_boss == 0) {
        actor->is_boss = is_boss;
        boss = 1;
    }
    actor->level = clevel;
    actor->balloon_font = font;
    if (type == ENTRY_OBJ_PET) {
        if (session_is_own_aid(&g_session1, (int)aid))
            g_session1.petAccessory = accessory;
        if (accessory != 0 && !game_actor_get_guild_id(actor))
            game_actor_set_pet_accessory(actor, accessory);
    } else if (type == ENTRY_OBJ_HOMUN && session_is_homunculus_aid(&g_session1, aid)) {
        g_session1.homun_ai_flags[0] = 1;
    } else if (type == ENTRY_OBJ_MERC && session_is_mercenary_aid(&g_session1, aid)) {
        g_session1.mercPresent = 1;
    } else {
        entry_siege_emblem(self, actor, 0, guild_id, emblem_version);
    }
    switch (actor->object_type) {
    case 0: case 5: case 8: case 9: case 10:
        render_object_send(actor, NULL, ACTOR_MSG_HP, hp, max_hp, boss, 0);
        break;
    }
}

static void entry_hp_unit_value(struct GameActor *actor, unsigned char type, short accessory2, short accessory3,
                                short emblem_version)
{
    if (type != ENTRY_OBJ_PC)
        entry_unit_value(actor, accessory2, accessory3, emblem_version, 1);
}

#pragma pack(push, 1)
struct PacketVariableHeader {
    unsigned short packet_type;
    short length;
};
#pragma pack(pop)

static void entry_name(struct String *s, const void *pkt, const char *name)
{
    const char *end = (const char *)pkt + ((const struct PacketVariableHeader *)pkt)->length;

    string_init(s);
    if (name != end)
        string_assign(s, name, (size_t)(end - name));
}

static void entry_keep_names(struct GameMode *self, unsigned char type, unsigned aid, unsigned char_id,
                             const struct String *name, int even_empty)
{
    struct String copy;

    if (type == ENTRY_OBJ_PC && char_id != 0) {
        player_names_set(char_id, text_view(string_cstr(name), name->size));
    }
    if (even_empty || name->size != 0) {
        string_copy(&copy, name);
        name_card_holder_set_name(&self->actor_names, aid, copy);
    }
}

void game_mode_zc_notify_newentry7(struct GameMode *self, const struct PacketNewEntry7 *pkt)
{
    struct ActorEntryInfo info;
    struct GameActor *actor;

    actor_entry_info_construct(&info);
    if (pkt->length - 0x4d > 0) {
        struct String name;

        entry_name(&name, pkt, pkt->name);
        name_card_holder_set_name(&self->actor_names, pkt->aid, name);
    }
    info.job = pkt->job;
    info.sex = game_mode_job_sex(self, pkt->job, pkt->sex);
    info.speed = pkt->speed;
    info.x_size = pkt->x_size;
    info.y_size = pkt->y_size;
    info.head = pkt->head;
    info.weapon[0] = pkt->weapon;
    info.weapon[1] = pkt->shield;
    info.accessory = pkt->accessory;
    info.health_state = pkt->health_state;
    info.body_state = pkt->body_state;
    info.effect_state = pkt->effect_state;
    info.is_pk_mode = pkt->is_pk_mode;
    info.accessory2 = pkt->accessory2;
    info.accessory3 = pkt->accessory3;
    info.head_palette = pkt->head_palette;
    info.body_palette = pkt->body_palette;
    info.head_dir = pkt->head_dir;
    info.guild_id = pkt->guild_id;
    info.honor = pkt->honor;
    info.virtue = pkt->virtue;
    info.robe = pkt->robe;
    info.aid = pkt->aid;
    info.emblem_version = pkt->emblem_version;
    if (pkt->object_type == ENTRY_OBJ_PC)
        info.state = 0;
    else
        info.shield = pkt->emblem_version;
    entry_set_pos(&info, &pkt->pos);

    actor = world_get_game_actor_by_aid(self->world, pkt->aid);
    if (actor) {
        actor_send(actor, pkt->object_type == ENTRY_OBJ_PC ? ACTOR_MSG_ENTRY : ACTOR_MSG_ENTRY_OLD, (intptr_t)&info, 0);
    } else {
        actor = entry_new_actor(self, pkt->object_type, pkt->job);
        if (!actor)
            return;
        actor->object_type = pkt->object_type;
        world_add_actor(self, actor);
        if (pkt->object_type == ENTRY_OBJ_PC) {
            if (entry_aid_listed(self, &pkt->aid))
                game_actor_set_mark_listed(actor, 1);
            actor_send(actor, ACTOR_MSG_NEW_ENTRY, (intptr_t)&info, ENTRY_PAR_NEW_PC);
        } else {
            info.sex = 0;
            actor_send(actor, ACTOR_MSG_NEW_ENTRY_OLD, (intptr_t)&info, ENTRY_PAR_NEW_UNIT);
        }
        champion_monster_apply_effects(g_championMonster, info.job, actor, (int)info.aid);
    }
    entry_hp_tail(self, actor, pkt->object_type, pkt->aid, info.accessory, pkt->guild_id, pkt->emblem_version,
                  pkt->clevel, pkt->font, pkt->hp, pkt->max_hp, pkt->is_boss);
    entry_hp_unit_value(actor, pkt->object_type, pkt->accessory2, pkt->accessory3, pkt->emblem_version);
    game_mode_game_mode_show_efst(self, pkt->aid, pkt->effect_state, pkt->clevel, pkt->virtue);
}

void game_mode_zc_notify_standentry8(struct GameMode *self, const struct PacketStandEntry8 *pkt)
{
    struct ActorEntryInfo info;
    struct GameActor *actor;

    actor_entry_info_construct(&info);
    if (pkt->length - 0x4e > 0) {
        struct String name;

        entry_name(&name, pkt, pkt->name);
        name_card_holder_set_name(&self->actor_names, pkt->aid, name);
    }
    info.job = pkt->job;
    info.sex = game_mode_job_sex(self, pkt->job, pkt->sex);
    info.speed = pkt->speed;
    info.x_size = pkt->x_size;
    info.y_size = pkt->y_size;
    info.head = pkt->head;
    info.weapon[0] = pkt->weapon;
    info.weapon[1] = pkt->shield;
    info.accessory = pkt->accessory;
    info.state = pkt->state;
    info.health_state = pkt->health_state;
    info.body_state = pkt->body_state;
    info.effect_state = pkt->effect_state;
    info.is_pk_mode = pkt->is_pk_mode;
    info.accessory2 = pkt->accessory2;
    info.accessory3 = pkt->accessory3;
    info.head_palette = pkt->head_palette;
    info.body_palette = pkt->body_palette;
    info.head_dir = pkt->head_dir;
    info.guild_id = pkt->guild_id;
    info.honor = pkt->honor;
    info.virtue = pkt->virtue;
    info.robe = pkt->robe;
    info.emblem_version = pkt->emblem_version;
    if (pkt->object_type != ENTRY_OBJ_PC)
        info.shield = pkt->emblem_version;
    info.aid = pkt->aid;
    entry_set_pos(&info, &pkt->pos);

    actor = world_get_game_actor_by_aid(self->world, pkt->aid);
    if (actor) {
        actor_send(actor, pkt->object_type == ENTRY_OBJ_PC ? ACTOR_MSG_ENTRY : ACTOR_MSG_ENTRY_OLD, (intptr_t)&info, 0);
    } else {
        actor = entry_new_actor(self, pkt->object_type, pkt->job);
        if (!actor)
            return;
        actor->object_type = pkt->object_type;
        world_add_actor(self, actor);
        if (pkt->object_type == ENTRY_OBJ_PC) {
            if (entry_aid_listed(self, &pkt->aid))
                game_actor_set_mark_listed(actor, 1);
            actor_send(actor, ACTOR_MSG_NEW_ENTRY, (intptr_t)&info, 0);
        } else {
            info.sex = 0;
            actor_send(actor, ACTOR_MSG_NEW_ENTRY_OLD, (intptr_t)&info, 0);
        }
        champion_monster_apply_effects(g_championMonster, info.job, actor, (int)info.aid);
    }
    entry_hp_tail(self, actor, pkt->object_type, pkt->aid, info.accessory, pkt->guild_id, pkt->emblem_version,
                  pkt->clevel, pkt->font, pkt->hp, pkt->max_hp, pkt->is_boss);
    entry_hp_unit_value(actor, pkt->object_type, pkt->accessory2, pkt->accessory3, pkt->emblem_version);
    game_mode_game_mode_show_efst(self, pkt->aid, pkt->effect_state, pkt->clevel, pkt->virtue);
}

void game_mode_zc_notify_moveentry9(struct GameMode *self, const struct PacketMoveEntry9 *pkt)
{
    struct ActorMoveEntryInfo info;
    struct GameActor *actor;

    if (pkt->length - 0x54 > 0) {
        struct String name;

        entry_name(&name, pkt, pkt->name);
        name_card_holder_set_name(&self->actor_names, pkt->aid, name);
    }
    info.body = 0;
    info.robe = pkt->robe;
    info.aid = pkt->aid;
    info.move_start_time = pkt->move_tick;
    info.job = pkt->job;
    info.sex = game_mode_job_sex(self, pkt->job, pkt->sex);
    info.speed = pkt->speed;
    info.head = pkt->head;
    info.weapon[0] = pkt->weapon;
    info.weapon[1] = pkt->shield;
    info.accessory = pkt->accessory;
    info.health_state = pkt->health_state;
    info.body_state = pkt->body_state;
    info.effect_state = pkt->effect_state;
    info.is_pk_mode = pkt->is_pk_mode;
    info.accessory2 = pkt->accessory2;
    info.accessory3 = pkt->accessory3;
    info.head_palette = pkt->head_palette;
    info.body_palette = pkt->body_palette;
    info.head_dir = pkt->head_dir;
    info.guild_id = pkt->guild_id;
    info.emblem_version = pkt->emblem_version;
    info.honor = pkt->honor;
    info.virtue = pkt->virtue;
    packet_decode_move_data(pkt->move_data, &info.src_x, &info.src_y, &info.dst_x, &info.dst_y, &info.sub_x,
                          &info.sub_y);
    session_sync_server_time(&g_session1, info.move_start_time);

    actor = world_get_game_actor_by_aid(self->world, pkt->aid);
    if (actor) {
        if (actor->mark_listed)
            entry_mark_moved(self, actor, info.dst_x, info.dst_y);
        actor_send(actor, ACTOR_MSG_MOVE_ENTRY, (intptr_t)&info, 0);
    } else {
        actor = entry_new_actor(self, pkt->object_type, pkt->job);
        if (!actor)
            return;
        actor->object_type = pkt->object_type;
        world_add_actor(self, actor);
        if (actor->object_type == ENTRY_OBJ_PC) {
            if (entry_aid_listed(self, &info.aid))
                game_actor_set_mark_listed(actor, 1);
        } else {
            info.sex = 0;
        }
        champion_monster_apply_effects(g_championMonster, info.job, actor, (int)info.aid);
        actor_send(actor, ACTOR_MSG_NEW_MOVE_ENTRY, (intptr_t)&info, 0);
    }
    entry_hp_tail(self, actor, pkt->object_type, pkt->aid, info.accessory, pkt->guild_id, pkt->emblem_version,
                  pkt->clevel, pkt->font, pkt->hp, pkt->max_hp, pkt->is_boss);
    game_mode_game_mode_show_efst(self, pkt->aid, pkt->effect_state, pkt->clevel, pkt->virtue);
}

static void moveentry10(struct GameMode *self, const struct PacketMoveEntry10 *pkt, short body,
                        struct String *name)
{
    struct ActorMoveEntryInfo info;
    struct GameActor *actor;
    unsigned char type = pkt->object_type;
    int msg;

    entry_keep_names(self, type, pkt->aid, pkt->char_id, name, 1);
    info.robe = pkt->robe;
    info.job = pkt->job;
    info.move_start_time = pkt->move_tick;
    info.sex = game_mode_job_sex(self, pkt->job, pkt->sex);
    info.aid = pkt->aid;
    info.speed = pkt->speed;
    info.head = pkt->head;
    info.weapon[0] = pkt->weapon;
    info.weapon[1] = pkt->shield;
    info.accessory = pkt->accessory;
    info.health_state = pkt->health_state;
    info.body_state = pkt->body_state;
    info.effect_state = pkt->effect_state;
    info.is_pk_mode = pkt->is_pk_mode;
    info.accessory2 = pkt->accessory2;
    info.accessory3 = pkt->accessory3;
    info.head_palette = pkt->head_palette;
    info.body_palette = pkt->body_palette;
    info.head_dir = pkt->head_dir;
    info.guild_id = pkt->guild_id;
    info.emblem_version = pkt->emblem_version;
    info.honor = pkt->honor;
    info.virtue = pkt->virtue;
    info.char_id = pkt->char_id;
    info.body = body;
    packet_decode_move_data(pkt->move_data, &info.src_x, &info.src_y, &info.dst_x, &info.dst_y, &info.sub_x,
                          &info.sub_y);
    session_sync_server_time(&g_session1, info.move_start_time);

    actor = world_get_game_actor_by_aid(self->world, pkt->aid);
    if (actor) {
        if (actor->mark_listed)
            entry_mark_moved(self, actor, info.dst_x, info.dst_y);
        msg = ACTOR_MSG_MOVE_ENTRY;
    } else {
        actor = entry_new_actor(self, type, pkt->job);
        if (!actor)
            goto done;
        actor->object_type = type;
        list_push_back(&self->world->gameObjects, &actor, sizeof actor);
        actor_list_push_back(&self->world->actors, actor);
        if (actor->object_type == ENTRY_OBJ_PC) {
            if (entry_aid_listed(self, &info.aid))
                game_actor_set_mark_listed(actor, 1);
        } else if (!entry_unit_keeps_sex(type, pkt->job, 12)) {
            info.sex = 0;
        }
        msg = ACTOR_MSG_NEW_MOVE_ENTRY;
    }
    actor_send(actor, msg, (intptr_t)&info, 0);
    champion_monster_apply_effects(g_championMonster, info.job, actor, (int)info.aid);
    entry_hp_tail(self, actor, type, pkt->aid, info.accessory, pkt->guild_id, pkt->emblem_version, pkt->clevel,
                  pkt->font, pkt->hp, pkt->max_hp, pkt->is_boss);
    game_mode_game_mode_show_efst(self, pkt->aid, pkt->effect_state, pkt->clevel, pkt->virtue);
    if (type == ENTRY_OBJ_PC) {
        float pos[2];

        pos[0] = actor->pos.x;
        pos[1] = actor->pos.z;
        world_place_id_nowhere(self->world, pkt->aid, pos);
    }
done:
    string_free(name);
}

#define ENTRY_WITH_NAME(body_fn, pkt, name_at, body)                         \
    do {                                                                     \
        struct String name, arg;                                          \
        entry_name(&name, (pkt), (const char *)(pkt) + (name_at));           \
        string_copy(&arg, &name);                                        \
        body_fn(self, (pkt), (body), &arg);                                  \
        string_free(&name);                                           \
    } while (0)

void game_mode_zc_notify_moveentry10(struct GameMode *self, const struct PacketMoveEntry10 *pkt)
{
    ENTRY_WITH_NAME(moveentry10, pkt, 0x58, 0);
}

void game_mode_zc_notify_moveentry11(struct GameMode *self, const struct PacketMoveEntry11 *pkt)
{
    ENTRY_WITH_NAME(moveentry10, &pkt->base, sizeof *pkt, pkt->body);
}

static void newentry10(struct GameMode *self, const struct PacketNewEntry10 *pkt, short body,
                       struct String *name)
{
    struct ActorEntryInfo info;
    struct GameActor *actor;
    unsigned char type = pkt->object_type;

    actor_entry_info_construct(&info);
    entry_keep_names(self, type, pkt->aid, pkt->char_id, name, 0);
    info.job = pkt->job;
    info.sex = game_mode_job_sex(self, pkt->job, pkt->sex);
    info.speed = pkt->speed;
    info.x_size = pkt->x_size;
    info.y_size = pkt->y_size;
    info.head = pkt->head;
    info.weapon[0] = pkt->weapon;
    info.weapon[1] = pkt->shield;
    info.accessory = pkt->accessory;
    info.health_state = pkt->health_state;
    info.body_state = pkt->body_state;
    info.effect_state = pkt->effect_state;
    info.is_pk_mode = pkt->is_pk_mode;
    info.accessory2 = pkt->accessory2;
    info.accessory3 = pkt->accessory3;
    info.head_palette = pkt->head_palette;
    info.body_palette = pkt->body_palette;
    info.head_dir = pkt->head_dir;
    info.guild_id = pkt->guild_id;
    info.honor = pkt->honor;
    info.virtue = pkt->virtue;
    info.robe = pkt->robe;
    info.aid = pkt->aid;
    info.emblem_version = pkt->emblem_version;
    if (type == ENTRY_OBJ_PC)
        info.state = 0;
    else
        info.shield = pkt->emblem_version;
    info.char_id = (int)pkt->char_id;
    info.body = body;
    entry_set_pos(&info, &pkt->pos);

    actor = world_get_game_actor_by_aid(self->world, pkt->aid);
    if (actor) {
        actor_send(actor, type == ENTRY_OBJ_PC ? ACTOR_MSG_ENTRY : ACTOR_MSG_ENTRY_OLD, (intptr_t)&info, 0);
    } else {
        unsigned aid = pkt->aid;

        actor = entry_new_actor(self, type, pkt->job);
        if (!actor)
            goto done;
        actor->object_type = type;
        world_add_actor(self, actor);
        if (type == ENTRY_OBJ_PC) {
            if (entry_aid_listed(self, &aid))
                game_actor_set_mark_listed(actor, 1);
            actor_send(actor, ACTOR_MSG_NEW_ENTRY, (intptr_t)&info, ENTRY_PAR_NEW_PC);
        } else {
            if (!entry_unit_keeps_sex(type, pkt->job, 5))
                info.sex = 0;
            actor_send(actor, ACTOR_MSG_NEW_ENTRY_OLD, (intptr_t)&info, ENTRY_PAR_NEW_UNIT);
        }
        champion_monster_apply_effects(g_championMonster, info.job, actor, (int)info.aid);
    }
    entry_hp_tail(self, actor, type, pkt->aid, info.accessory, pkt->guild_id, pkt->emblem_version, pkt->clevel,
                  pkt->font, pkt->hp, pkt->max_hp, pkt->is_boss);
    entry_hp_unit_value(actor, type, pkt->accessory2, pkt->accessory3, pkt->emblem_version);
    game_mode_game_mode_show_efst(self, pkt->aid, pkt->effect_state, pkt->clevel, pkt->virtue);
    if (type == ENTRY_OBJ_PC) {
        int cell[2];

        cell[0] = info.x;
        cell[1] = info.y;
        world_place_id_at_client_cell(self->world, pkt->aid, cell);
    }
done:
    string_free(name);
}

void game_mode_zc_notify_newentry10(struct GameMode *self, const struct PacketNewEntry10 *pkt)
{
    ENTRY_WITH_NAME(newentry10, pkt, 0x51, 0);
}

void game_mode_zc_notify_newentry11(struct GameMode *self, const struct PacketNewEntry11 *pkt)
{
    ENTRY_WITH_NAME(newentry10, &pkt->base, sizeof *pkt, pkt->body);
}

static void standentry10(struct GameMode *self, const struct PacketStandEntry10 *pkt, short body,
                         struct String *name)
{
    struct ActorEntryInfo info;
    struct GameActor *actor;
    unsigned char type = pkt->object_type;

    actor_entry_info_construct(&info);
    entry_keep_names(self, type, pkt->aid, pkt->char_id, name, 0);
    info.job = pkt->job;
    info.sex = game_mode_job_sex(self, pkt->job, pkt->sex);
    info.speed = pkt->speed;
    info.x_size = pkt->x_size;
    info.y_size = pkt->y_size;
    info.head = pkt->head;
    info.weapon[0] = pkt->weapon;
    info.weapon[1] = pkt->shield;
    info.accessory = pkt->accessory;
    info.state = pkt->state;
    info.health_state = pkt->health_state;
    info.body_state = pkt->body_state;
    info.effect_state = pkt->effect_state;
    info.is_pk_mode = pkt->is_pk_mode;
    info.accessory2 = pkt->accessory2;
    info.accessory3 = pkt->accessory3;
    info.head_palette = pkt->head_palette;
    info.body_palette = pkt->body_palette;
    info.head_dir = pkt->head_dir;
    info.guild_id = pkt->guild_id;
    info.honor = pkt->honor;
    info.virtue = pkt->virtue;
    info.robe = pkt->robe;
    if (type != ENTRY_OBJ_PC)
        info.shield = pkt->emblem_version;
    info.emblem_version = pkt->emblem_version;
    info.char_id = (int)pkt->char_id;
    info.body = body;
    info.aid = pkt->aid;
    entry_set_pos(&info, &pkt->pos);

    actor = world_get_game_actor_by_aid(self->world, pkt->aid);
    if (actor) {
        actor_send(actor, type == ENTRY_OBJ_PC ? ACTOR_MSG_ENTRY : ACTOR_MSG_ENTRY_OLD, (intptr_t)&info, 0);
    } else {
        unsigned aid = pkt->aid;

        actor = entry_new_actor(self, type, pkt->job);
        if (!actor)
            goto done;
        actor->object_type = type;
        world_add_actor(self, actor);
        if (type == ENTRY_OBJ_PC) {
            if (entry_aid_listed(self, &aid))
                game_actor_set_mark_listed(actor, 1);
            actor_send(actor, ACTOR_MSG_NEW_ENTRY, (intptr_t)&info, 0);
        } else {
            if (!entry_unit_keeps_sex(type, pkt->job, 12))
                info.sex = 0;
            actor_send(actor, ACTOR_MSG_NEW_ENTRY_OLD, (intptr_t)&info, 0);
        }
        champion_monster_apply_effects(g_championMonster, info.job, actor, (int)info.aid);
    }
    entry_hp_tail(self, actor, type, pkt->aid, info.accessory, pkt->guild_id, pkt->emblem_version, pkt->clevel,
                  pkt->font, pkt->hp, pkt->max_hp, pkt->is_boss);
    if (type != ENTRY_OBJ_PC && pkt->job == ENTRY_JOB_GUILD_FLAG) {
        short emblem_version = pkt->emblem_version;
        int value = game_actor_get_guild_id(actor);

        game_actor_set_guild(actor, value, emblem_version);
    }
    game_mode_game_mode_show_efst(self, pkt->aid, pkt->effect_state, pkt->clevel, pkt->virtue);
    if (type == ENTRY_OBJ_PC) {
        int cell[2];

        cell[0] = info.x;
        cell[1] = info.y;
        world_place_id_at_client_cell(self->world, pkt->aid, cell);
    }
done:
    string_free(name);
}

void game_mode_zc_notify_standentry10(struct GameMode *self, const struct PacketStandEntry10 *pkt)
{
    ENTRY_WITH_NAME(standentry10, pkt, 0x52, 0);
}

void game_mode_zc_notify_standentry11(struct GameMode *self, const struct PacketStandEntry11 *pkt)
{
    ENTRY_WITH_NAME(standentry10, &pkt->base, sizeof *pkt, pkt->body);
}
