#include "game/actor_marks.h"
#include "actor/actor_ops.h"
#include "session/character_name.h"
#include "actor/render_object.h"
#include "contents/sound.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include <windows.h>
#include "core/fatal.h"
#include <mmsystem.h>
#include "game/packets_entry.h"
#include "game/packet_data.h"
#include "actor/actor_common.h"
#include "actor/game_actor.h"
#include "effects/rag/rag_effect.h"
#include "platform/clientinfo.h"
#include "session/session_jobs.h"
#include "session/session_skills.h"
#include "session/session_state.h"
#include "session/session_talk.h"
#include "world/world.h"

struct ActorEntryInfo *actor_entry_info_construct(struct ActorEntryInfo *info)
{
    memset(&info->job, 0, offsetof(struct ActorEntryInfo, head) - offsetof(struct ActorEntryInfo, job));
    memset(info->weapon, 0, sizeof *info - offsetof(struct ActorEntryInfo, weapon));
    return info;
}

#define EFFECT_PC_ENTRY        0x158
#define EFFECT_RANKING_CROWN    0x9f
#define ACTOR_MSG_DROP_EFFECT   0x6d
#define ACTOR_MSG_RANKING       0x72
#define EFFECT_MSG_RANK         0x2c

enum {
    JOB_BARD = 19, JOB_DANCER = 20,
    JOB_CLOWN = 4020, JOB_GYPSY = 4021,
    JOB_BABY_BARD = 4042, JOB_BABY_DANCER = 4043,
    JOB_MINSTREL = 4068, JOB_WANDERER = 4069,
    JOB_MINSTREL_H = 4075, JOB_WANDERER_H = 4076,
    JOB_BABY_MINSTREL = 4104, JOB_BABY_WANDERER = 4105,
    JOB_TROUBADOUR = 4263, JOB_TROUVERE = 4264,
    SEX_FEMALE = 0, SEX_MALE = 1,
};

int game_mode_job_sex(struct GameMode *self, int job, int sex)
{
    (void)self;
    switch (job) {
    case JOB_BARD: case JOB_CLOWN: case JOB_BABY_BARD: case JOB_MINSTREL: case JOB_MINSTREL_H:
    case JOB_BABY_MINSTREL: case JOB_TROUBADOUR:
        return SEX_MALE;
    case JOB_DANCER: case JOB_GYPSY: case JOB_BABY_DANCER: case JOB_WANDERER: case JOB_WANDERER_H:
    case JOB_BABY_WANDERER: case JOB_TROUVERE:
        return SEX_FEMALE;
    default:
        return sex;
    }
}

void entry_set_pos(struct ActorEntryInfo *info, const struct PacketPosDir *pos)
{
    info->x = (pos->b[0] << 2) | (pos->b[1] >> 6);
    info->y = ((pos->b[1] & 0x3f) << 4) | (pos->b[2] >> 4);
    info->dir = pos->b[2] & 0xf;
}

static int job_is_unit(int job, int lowest)
{
    return (job >= lowest && job < 1000) || (job > 1000 && job < 3999)
        || (unsigned)(job - 20021) <= 11978;
}

static void entry_note_pet(struct GameActor *actor, unsigned aid, int accessory)
{
    if ((unsigned)g_session1.aid == aid)
        g_session1.petAccessory = accessory;
    if (accessory != 0)
        render_object_load_act_res((struct RenderObject *)actor, session_get_pet_accessory_act(&g_session1, accessory));
}

void entry_siege_emblem(struct GameMode *self, struct GameActor *actor, int is_pet,
                        unsigned guild_id, int emblem_version)
{
    if (!session_is_siege_mode(&g_session1) || is_pet || actor->siege_wnd != 0)
        return;
    if (session_is_player_job(&g_session1, game_actor_get_job(actor))
        || session_is_guardian(&g_session1, game_actor_get_job(actor)))
        game_mode_make_emblem_wnd(self, actor, (int)guild_id);
}

int entry_aid_listed(struct GameMode *self, const unsigned *aid)
{
    if (tree_find(&self->square_marks, aid, tree_less_uint))
        return 1;
    return tree_find(&self->triangle_marks, aid, tree_less_uint) != NULL;
}

void entry_unit_value(struct GameActor *actor, short accessory2, short accessory3, int shield,
                      int keep_existing)
{
    int value = (int)(((unsigned)(int)accessory2 << 16) | (unsigned)(int)accessory3);

    if (value == 0)
        return;
    if (keep_existing && game_actor_get_guild_id(actor))
        return;
    game_actor_set_guild(actor, value, shield);
}

void world_add_actor(struct GameMode *self, struct GameActor *actor)
{
    list_push_back(&self->world->gameObjects, &actor, sizeof actor);
    actor_list_push_back(&self->world->actors, actor);
}

static struct GameActor *entry_make_actor(struct GameMode *self, short job, unsigned aid,
                                           int forbid_pc, int *sex, int *made_by_new)
{
    *made_by_new = 0;
    if (!job_is_unit(job, 45) && !session_is_homun_or_merc_job(&g_session1, job) && !forbid_pc) {
        void *mem;

        *made_by_new = 1;
        mem = mem_alloc(sizeof(struct Pc));
        return mem ? (struct GameActor *)pc_construct(mem) : 0;
    }
    if (session_is_homunculus_aid(&g_session1, aid))
        g_session1.homun_ai_flags[0] = 1;
    if (session_is_mercenary_aid(&g_session1, aid))
        g_session1.mercPresent = 1;
    {
        struct GameActor *actor = game_mode_new_actor_for_job(self, job);
        *sex = 0;
        return actor;
    }
}

static void entry_new_pc_effect(struct GameActor *actor, unsigned aid, int effect_state)
{
    if (!clientinfo_is_admin_aid(aid) && !effect_state_bit6(effect_state))
        game_actor_launch_effect(actor, EFFECT_PC_ENTRY, (Vec3){ 0.0f, 0.0f, 0.0f }, 0.0f);
}

void game_mode_zc_notify_newentry(struct GameMode *self, const struct PacketNewEntry *pkt)
{
    struct ActorEntryInfo info;
    struct GameActor *actor;
    int made_by_new;

    actor_entry_info_construct(&info);
    info.job = pkt->job;
    info.sex = game_mode_job_sex(self, pkt->job, pkt->sex);
    info.speed = pkt->speed;
    info.x_size = pkt->x_size;
    info.y_size = pkt->y_size;
    info.head = pkt->head;
    info.weapon[0] = pkt->weapon;
    info.weapon[1] = pkt->weapon < 0 ? -1 : 0;
    info.accessory = pkt->accessory;
    info.health_state = pkt->health_state;
    info.body_state = pkt->body_state;
    info.effect_state = pkt->effect_state;
    info.is_pk_mode = pkt->is_pk_mode;
    info.shield = pkt->shield;
    info.accessory2 = pkt->accessory2;
    info.accessory3 = pkt->accessory3;
    info.head_palette = pkt->head_palette;
    info.body_palette = pkt->body_palette;
    info.head_dir = pkt->head_dir;
    info.guild_id = pkt->guild_id;
    info.emblem_version = pkt->emblem_version;
    info.honor = pkt->honor;
    info.virtue = pkt->virtue;
    entry_set_pos(&info, &pkt->pos);
    info.aid = pkt->aid;
    info.state = 0;

    actor = world_get_game_actor_by_aid(self->world, pkt->aid);
    if (actor) {
        if (job_is_unit(game_actor_get_job(actor), 46))
            entry_unit_value(actor, pkt->accessory2, pkt->accessory3, pkt->shield, 1);
        if (info.head == ENTRY_HEAD_PET)
            entry_note_pet(actor, pkt->aid, info.accessory);
        actor_send(actor, ACTOR_MSG_ENTRY_OLD, (int)&info, 0);
        entry_siege_emblem(self, actor, info.head == ENTRY_HEAD_PET, pkt->guild_id, pkt->emblem_version);
    } else {
        actor = entry_make_actor(self, pkt->job, pkt->aid, 0, &info.sex, &made_by_new);
        if (actor && info.head == ENTRY_HEAD_PET)
            entry_note_pet(actor, pkt->aid, info.accessory);
        world_add_actor(self, actor);
        actor_send(actor, ACTOR_MSG_NEW_ENTRY_OLD, (int)&info, 2);
        if (job_is_unit(game_actor_get_job(actor), 45))
            entry_unit_value(actor, pkt->accessory2, pkt->accessory3, pkt->shield, 0);
        if (made_by_new)
            entry_new_pc_effect(actor, pkt->aid, pkt->effect_state);
        entry_siege_emblem(self, actor, info.head == ENTRY_HEAD_PET, pkt->guild_id, pkt->emblem_version);
    }
    game_mode_game_mode_show_efst(self, pkt->aid, pkt->effect_state, pkt->clevel, pkt->virtue);
}

void game_mode_zc_notify_newentry2(struct GameMode *self, const struct PacketNewEntry2 *pkt)
{
    struct ActorEntryInfo info;
    struct GameActor *actor;
    int made_by_new;

    actor_entry_info_construct(&info);
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
    info.emblem_version = pkt->emblem_version;
    info.honor = pkt->honor;
    info.virtue = pkt->virtue;
    entry_set_pos(&info, &pkt->pos);
    info.aid = pkt->aid;
    info.state = 0;

    actor = world_get_game_actor_by_aid(self->world, pkt->aid);
    if (actor) {
        if (info.head == ENTRY_HEAD_PET)
            entry_note_pet(actor, pkt->aid, info.accessory);
        actor_send(actor, ACTOR_MSG_ENTRY, (int)&info, 0);
        entry_siege_emblem(self, actor, info.head == ENTRY_HEAD_PET, pkt->guild_id, pkt->emblem_version);
    } else {
        actor = entry_make_actor(self, pkt->job, pkt->aid, 0, &info.sex, &made_by_new);
        if (actor && info.head == ENTRY_HEAD_PET)
            entry_note_pet(actor, pkt->aid, info.accessory);
        world_add_actor(self, actor);
        actor_send(actor, ACTOR_MSG_NEW_ENTRY, (int)&info, 2);
        if (made_by_new)
            entry_new_pc_effect(actor, pkt->aid, pkt->effect_state);
        entry_siege_emblem(self, actor, info.head == ENTRY_HEAD_PET, pkt->guild_id, pkt->emblem_version);
        if (entry_aid_listed(self, &pkt->aid))
            game_actor_set_mark_listed(actor, 1);
    }
    game_mode_game_mode_show_efst(self, pkt->aid, pkt->effect_state, pkt->clevel, pkt->virtue);
}

static void newentry4_info(struct GameMode *self, const struct PacketNewEntry4 *pkt, struct ActorEntryInfo *info)
{
    actor_entry_info_construct(info);
    info->job = pkt->job;
    info->sex = game_mode_job_sex(self, pkt->job, pkt->sex);
    info->speed = pkt->speed;
    info->x_size = pkt->x_size;
    info->y_size = pkt->y_size;
    info->head = pkt->head;
    info->weapon[0] = pkt->weapon;
    info->weapon[1] = pkt->shield;
    info->accessory = pkt->accessory;
    info->health_state = pkt->health_state;
    info->body_state = pkt->body_state;
    info->effect_state = pkt->effect_state;
    info->is_pk_mode = pkt->is_pk_mode;
    info->accessory2 = pkt->accessory2;
    info->accessory3 = pkt->accessory3;
    info->head_palette = pkt->head_palette;
    info->body_palette = pkt->body_palette;
    info->head_dir = pkt->head_dir;
    info->guild_id = pkt->guild_id;
    info->emblem_version = pkt->emblem_version;
    info->honor = pkt->honor;
    info->virtue = pkt->virtue;
    entry_set_pos(info, &pkt->pos);
    info->aid = pkt->aid;
    info->state = 0;
}

static void newentry4(struct GameMode *self, const struct PacketNewEntry4 *pkt, int with_font)
{
    struct ActorEntryInfo info;
    struct GameActor *actor;
    int made_by_new;

    newentry4_info(self, pkt, &info);
    actor = world_get_game_actor_by_aid(self->world, pkt->aid);
    if (actor) {
        if (with_font) {
            actor->balloon_font = pkt->font;
            actor->level = pkt->clevel;
        }
        if (info.head == ENTRY_HEAD_PET)
            entry_note_pet(actor, pkt->aid, info.accessory);
        actor_send(actor, ACTOR_MSG_ENTRY, (int)&info, 0);
        if (!with_font)
            actor->level = pkt->clevel;
        entry_siege_emblem(self, actor, info.head == ENTRY_HEAD_PET, pkt->guild_id, pkt->emblem_version);
    } else {
        actor = entry_make_actor(self, pkt->job, pkt->aid, 0, &info.sex, &made_by_new);
        if (actor && info.head == ENTRY_HEAD_PET)
            entry_note_pet(actor, pkt->aid, info.accessory);
        world_add_actor(self, actor);
        actor_send(actor, ACTOR_MSG_NEW_ENTRY, (int)&info, 2);
        actor->level = pkt->clevel;
        if (made_by_new) {
            if (with_font)
                actor->balloon_font = pkt->font;
            entry_new_pc_effect(actor, pkt->aid, pkt->effect_state);
        }
        entry_siege_emblem(self, actor, info.head == ENTRY_HEAD_PET, pkt->guild_id, pkt->emblem_version);
        if (entry_aid_listed(self, &pkt->aid))
            game_actor_set_mark_listed(actor, 1);
    }
    game_mode_game_mode_show_efst(self, pkt->aid, pkt->effect_state, pkt->clevel, pkt->virtue);
}

void game_mode_zc_notify_newentry3(struct GameMode *self, const struct PacketNewEntry4 *pkt)
{
    newentry4(self, pkt, 0);
}

void game_mode_zc_notify_newentry4(struct GameMode *self, const struct PacketNewEntry4 *pkt)
{
    newentry4(self, pkt, 1);
}

void game_mode_zc_notify_standentry(struct GameMode *self, const struct PacketStandEntry *pkt)
{
    struct ActorEntryInfo info;
    struct GameActor *actor;
    int made_by_new;

    actor_entry_info_construct(&info);
    info.job = pkt->job;
    info.sex = game_mode_job_sex(self, pkt->job, pkt->sex);
    info.speed = pkt->speed;
    info.x_size = pkt->x_size;
    info.y_size = pkt->y_size;
    info.head = pkt->head;
    info.weapon[0] = pkt->weapon;
    info.weapon[1] = pkt->weapon < 0 ? -1 : 0;
    info.accessory = pkt->accessory;
    info.state = pkt->state;
    info.health_state = pkt->health_state;
    info.body_state = pkt->body_state;
    info.effect_state = pkt->effect_state;
    info.is_pk_mode = pkt->is_pk_mode;
    info.shield = pkt->shield;
    info.accessory2 = pkt->accessory2;
    info.accessory3 = pkt->accessory3;
    info.head_palette = pkt->head_palette;
    info.body_palette = pkt->body_palette;
    info.head_dir = pkt->head_dir;
    info.guild_id = pkt->guild_id;
    info.emblem_version = pkt->emblem_version;
    info.honor = pkt->honor;
    info.virtue = pkt->virtue;
    entry_set_pos(&info, &pkt->pos);
    info.aid = pkt->aid;

    actor = world_get_game_actor_by_aid(self->world, pkt->aid);
    if (actor) {
        if (job_is_unit(game_actor_get_job(actor), 46))
            entry_unit_value(actor, pkt->accessory2, pkt->accessory3, pkt->shield, 1);
        if (pkt->object_type == ENTRY_OBJ_PET)
            entry_note_pet(actor, pkt->aid, info.accessory);
        actor_send(actor, ACTOR_MSG_ENTRY_OLD, (int)&info, 0);
        entry_siege_emblem(self, actor, pkt->object_type == ENTRY_OBJ_PET, pkt->guild_id, pkt->emblem_version);
    } else {
        actor = entry_make_actor(self, pkt->job, pkt->aid, pkt->object_type == ENTRY_OBJ_MERC, &info.sex,
                                 &made_by_new);
        if (actor && pkt->object_type == ENTRY_OBJ_PET)
            entry_note_pet(actor, pkt->aid, info.accessory);
        actor->object_type = pkt->object_type;
        world_add_actor(self, actor);
        actor_send(actor, ACTOR_MSG_NEW_ENTRY_OLD, (int)&info, 0);
        entry_siege_emblem(self, actor, pkt->object_type == ENTRY_OBJ_PET, pkt->guild_id, pkt->emblem_version);
        if (job_is_unit(game_actor_get_job(actor), 46))
            entry_unit_value(actor, pkt->accessory2, pkt->accessory3, pkt->shield, 0);
    }
    game_mode_game_mode_show_efst(self, pkt->aid, pkt->effect_state, pkt->clevel, pkt->virtue);
}

void game_mode_zc_notify_standentry_npc(struct GameMode *self, const struct PacketStandEntryNpc *pkt)
{
    struct ActorEntryInfo info;
    struct GameActor *actor;

    actor_entry_info_construct(&info);
    info.job = pkt->job;
    info.sex = pkt->sex;
    info.speed = pkt->speed;
    info.x_size = pkt->x_size;
    info.y_size = pkt->y_size;
    info.head = pkt->head;
    info.weapon[0] = pkt->weapon;
    info.weapon[1] = pkt->weapon < 0 ? -1 : 0;
    info.accessory = pkt->accessory;
    info.health_state = pkt->health_state;
    info.body_state = pkt->body_state;
    info.effect_state = pkt->effect_state;
    info.is_pk_mode = pkt->is_pk_mode;
    info.shield = pkt->shield;
    info.accessory2 = pkt->accessory2;
    info.accessory3 = pkt->accessory3;
    info.head_palette = pkt->head_palette;
    info.body_palette = pkt->body_palette;
    info.head_dir = pkt->head_dir;
    entry_set_pos(&info, &pkt->pos);
    info.aid = pkt->aid;

    actor = world_get_game_actor_by_aid(self->world, pkt->aid);
    if (actor) {
        actor_send(actor, ACTOR_MSG_ENTRY_OLD, (int)&info, 0);
        if (job_is_unit(game_actor_get_job(actor), 46))
            entry_unit_value(actor, pkt->accessory2, pkt->accessory3, pkt->shield, 1);
        actor->level = pkt->clevel;
        return;
    }
    actor = game_mode_new_actor_for_job(self, pkt->job);
    info.sex = 0;
    actor->object_type = pkt->object_type;
    world_add_actor(self, actor);
    actor_send(actor, ACTOR_MSG_NEW_ENTRY_OLD, (int)&info, 1);
    if (job_is_unit(game_actor_get_job(actor), 46))
        entry_unit_value(actor, pkt->accessory2, pkt->accessory3, pkt->shield, 0);
    actor->level = pkt->clevel;
}

void game_mode_zc_notify_standentry2(struct GameMode *self, const struct PacketStandEntry2 *pkt)
{
    struct ActorEntryInfo info;
    struct GameActor *actor;
    int made_by_new;

    actor_entry_info_construct(&info);
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
    info.emblem_version = pkt->emblem_version;
    info.honor = pkt->honor;
    info.virtue = pkt->virtue;
    entry_set_pos(&info, &pkt->pos);
    info.aid = pkt->aid;

    actor = world_get_game_actor_by_aid(self->world, pkt->aid);
    if (actor) {
        if (info.head == ENTRY_HEAD_PET)
            entry_note_pet(actor, pkt->aid, info.accessory);
        actor_send(actor, ACTOR_MSG_ENTRY, (int)&info, 0);
        entry_siege_emblem(self, actor, info.head == ENTRY_HEAD_PET, pkt->guild_id, pkt->emblem_version);
    } else {
        actor = entry_make_actor(self, pkt->job, pkt->aid, 0, &info.sex, &made_by_new);
        if (actor && info.head == ENTRY_HEAD_PET)
            entry_note_pet(actor, pkt->aid, info.accessory);
        world_add_actor(self, actor);
        if (actor) {
            actor_send(actor, ACTOR_MSG_NEW_ENTRY, (int)&info, 0);
            entry_siege_emblem(self, actor, info.head == ENTRY_HEAD_PET, pkt->guild_id, pkt->emblem_version);
            if (entry_aid_listed(self, &pkt->aid))
                game_actor_set_mark_listed(actor, 1);
        }
    }
    game_mode_game_mode_show_efst(self, pkt->aid, pkt->effect_state, pkt->clevel, pkt->virtue);
}

static void standentry4(struct GameMode *self, const struct PacketStandEntry4 *pkt, int with_font)
{
    struct ActorEntryInfo info;
    struct GameActor *actor;
    int made_by_new;

    actor_entry_info_construct(&info);
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
    info.emblem_version = pkt->emblem_version;
    info.honor = pkt->honor;
    info.virtue = pkt->virtue;
    entry_set_pos(&info, &pkt->pos);
    info.aid = pkt->aid;

    actor = world_get_game_actor_by_aid(self->world, pkt->aid);
    if (actor) {
        if (with_font)
            actor->balloon_font = pkt->font;
        if (info.head == ENTRY_HEAD_PET)
            entry_note_pet(actor, pkt->aid, info.accessory);
        actor_send(actor, ACTOR_MSG_ENTRY, (int)&info, 0);
        entry_siege_emblem(self, actor, info.head == ENTRY_HEAD_PET, pkt->guild_id, pkt->emblem_version);
    } else {
        actor = entry_make_actor(self, pkt->job, pkt->aid, 0, &info.sex, &made_by_new);
        if (actor && info.head == ENTRY_HEAD_PET)
            entry_note_pet(actor, pkt->aid, info.accessory);
        world_add_actor(self, actor);
        if (actor) {
            actor_send(actor, ACTOR_MSG_NEW_ENTRY, (int)&info, 0);
            entry_siege_emblem(self, actor, info.head == ENTRY_HEAD_PET, pkt->guild_id, pkt->emblem_version);
            if (with_font)
                actor->balloon_font = pkt->font;
            if (entry_aid_listed(self, &pkt->aid))
                game_actor_set_mark_listed(actor, 1);
        }
    }
    game_mode_game_mode_show_efst(self, pkt->aid, pkt->effect_state, pkt->clevel, pkt->virtue);
}

void game_mode_zc_notify_standentry3(struct GameMode *self, const struct PacketStandEntry4 *pkt)
{
    standentry4(self, pkt, 0);
}

void game_mode_zc_notify_standentry4(struct GameMode *self, const struct PacketStandEntry4 *pkt)
{
    standentry4(self, pkt, 1);
}

void game_mode_zc_notify_standentry5(struct GameMode *self, const struct PacketStandEntry5 *pkt)
{
    struct ActorEntryInfo info;
    struct GameActor *actor;
    int name_length, made;

    actor_entry_info_construct(&info);
    name_length = pkt->length - 0x45;
    info.robe = pkt->robe;
    if (name_length > 0) {
        struct String name;

        string_init(&name);
        string_assign(&name, pkt->name, name_length);
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
    info.emblem_version = pkt->emblem_version;
    info.honor = pkt->honor;
    info.virtue = pkt->virtue;
    if (pkt->object_type != ENTRY_OBJ_PC)
        info.shield = pkt->emblem_version;
    entry_set_pos(&info, &pkt->pos);
    info.aid = pkt->aid;

    made = 0;
    actor = world_get_game_actor_by_aid(self->world, pkt->aid);
    if (!actor) {
        made = 1;
        if (pkt->object_type == ENTRY_OBJ_PC) {
            void *mem = mem_alloc(sizeof(struct Pc));
            actor = mem ? (struct GameActor *)pc_construct(mem) : 0;
        } else {
            actor = game_mode_new_actor_for_job(self, pkt->job);
            info.sex = 0;
        }
        actor->object_type = pkt->object_type;
    }

    if (pkt->object_type == ENTRY_OBJ_PC) {
        actor->balloon_font = pkt->font;
        actor->level = pkt->clevel;
        if (made) {
            actor_send(actor, ACTOR_MSG_NEW_ENTRY, (int)&info, 0);
            if (entry_aid_listed(self, &pkt->aid))
                game_actor_set_mark_listed(actor, 1);
        } else {
            actor_send(actor, ACTOR_MSG_ENTRY, (int)&info, 0);
        }
        if (session_is_siege_mode(&g_session1) && actor->siege_wnd == 0)
            game_mode_make_emblem_wnd(self, actor, (int)pkt->guild_id);
    } else {
        if (pkt->object_type == ENTRY_OBJ_PET) {
            entry_note_pet(actor, pkt->aid, info.accessory);
        } else if (pkt->object_type == ENTRY_OBJ_HOMUN && session_is_homunculus_aid(&g_session1, pkt->aid)) {
            g_session1.homun_ai_flags[0] = 1;
        } else if (pkt->object_type == ENTRY_OBJ_MERC) {
            if (session_is_mercenary_aid(&g_session1, pkt->aid))
                g_session1.mercPresent = 1;
        }
        actor_send(actor, made ? ACTOR_MSG_NEW_ENTRY_OLD : ACTOR_MSG_ENTRY_OLD, (int)&info, 0);
        if (session_is_guardian(&g_session1, pkt->job) && session_is_siege_mode(&g_session1)
            && actor->siege_wnd == 0)
            game_mode_make_emblem_wnd(self, actor, (int)pkt->guild_id);
        entry_unit_value(actor, pkt->accessory2, pkt->accessory3, pkt->emblem_version, !made);
    }
    if (made)
        world_add_actor(self, actor);
    game_mode_game_mode_show_efst(self, pkt->aid, pkt->effect_state, pkt->clevel, pkt->virtue);
}

static void entry_store_name(struct GameMode *self, unsigned aid, const char *name, int length)
{
    struct String text;

    if (length <= 0)
        return;
    string_init(&text);
    string_assign(&text, name, length);
    name_card_holder_set_name(&self->actor_names, aid, text);
}

void game_mode_zc_notify_newentry5(struct GameMode *self, const struct PacketNewEntry5 *pkt)
{
    struct ActorEntryInfo info;
    struct GameActor *actor;
    int made;

    actor_entry_info_construct(&info);
    info.robe = pkt->robe;
    entry_store_name(self, pkt->aid, pkt->name, pkt->length - 0x44);
    info.job = pkt->job;
    info.sex = game_mode_job_sex(self, pkt->job, pkt->sex);
    info.speed = pkt->speed;
    info.x_size = pkt->x_size;
    info.y_size = pkt->y_size;
    info.head = pkt->head;
    info.weapon[0] = pkt->weapon;
    info.weapon[1] = pkt->shield;
    info.accessory = pkt->accessory;
    info.body_state = pkt->body_state;
    info.health_state = pkt->health_state;
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
    info.aid = pkt->aid;
    if (pkt->object_type == ENTRY_OBJ_PC)
        info.state = 0;
    else
        info.shield = pkt->emblem_version;
    entry_set_pos(&info, &pkt->pos);

    made = 0;
    actor = world_get_game_actor_by_aid(self->world, pkt->aid);
    if (!actor) {
        made = 1;
        if (pkt->object_type == ENTRY_OBJ_PC) {
            void *mem = mem_alloc(sizeof(struct Pc));
            actor = mem ? (struct GameActor *)pc_construct(mem) : 0;
        } else {
            actor = game_mode_new_actor_for_job(self, pkt->job);
            info.sex = 0;
        }
        actor->object_type = pkt->object_type;
    }
    actor->level = pkt->clevel;

    if (pkt->object_type == ENTRY_OBJ_PC) {
        actor->balloon_font = pkt->font;
        if (made) {
            actor_send(actor, ACTOR_MSG_NEW_ENTRY, (int)&info, 2);
            if (entry_aid_listed(self, &pkt->aid))
                game_actor_set_mark_listed(actor, 1);
            list_push_back(&self->world->gameObjects, &actor, sizeof actor);
            actor_list_push_back(&self->world->actors, actor);
        } else {
            actor_send(actor, ACTOR_MSG_ENTRY, (int)&info, 0);
        }
        if (session_is_siege_mode(&g_session1) && actor->siege_wnd == 0)
            game_mode_make_emblem_wnd(self, actor, pkt->guild_id);
    } else {
        if (pkt->object_type == ENTRY_OBJ_PET) {
            entry_note_pet(actor, pkt->aid, info.accessory);
        } else if (pkt->object_type == ENTRY_OBJ_HOMUN && session_is_homunculus_aid(&g_session1, pkt->aid)) {
            g_session1.homun_ai_flags[0] = 1;
        } else if (pkt->object_type == ENTRY_OBJ_MERC) {
            if (session_is_mercenary_aid(&g_session1, pkt->aid))
                g_session1.mercPresent = 1;
        }
        if (made) {
            list_push_back(&self->world->gameObjects, &actor, sizeof actor);
            actor_list_push_back(&self->world->actors, actor);
            actor_send(actor, ACTOR_MSG_NEW_ENTRY_OLD, (int)&info, 1);
        } else {
            actor_send(actor, ACTOR_MSG_ENTRY_OLD, (int)&info, 0);
        }
        if (session_is_guardian(&g_session1, pkt->job) && session_is_siege_mode(&g_session1)
            && actor->siege_wnd == 0)
            game_mode_make_emblem_wnd(self, actor, pkt->guild_id);
        entry_unit_value(actor, pkt->accessory2, pkt->accessory3, pkt->emblem_version, !made);
    }
    game_mode_game_mode_show_efst(self, pkt->aid, pkt->effect_state, pkt->clevel, pkt->virtue);
}

static void entry_note_pet_existing(struct GameActor *actor, unsigned aid, int accessory)
{
    if ((unsigned)g_session1.aid == aid)
        g_session1.petAccessory = accessory;
    if (accessory != 0
        && !game_actor_get_guild_id(actor))
        render_object_load_act_res((struct RenderObject *)actor, session_get_pet_accessory_act(&g_session1, accessory));
}

static void entry_move_marks(struct GameMode *self, struct GameActor *actor, const struct ActorMoveEntryInfo *info)
{
    struct MemberMark *mark;
    unsigned key;

    if (actor->mark_listed == 0)
        return;
    key = actor->aid;
    mark = tree_find(&self->square_marks, &key, tree_less_uint);
    if (mark) {
        mark->x = info->dst_x;
        mark->y = info->dst_y;
    }
    mark = tree_find(&self->triangle_marks, &key, tree_less_uint);
    if (mark) {
        mark->x = info->dst_x;
        mark->y = info->dst_y;
    }
}

static void entry_decode_move(struct ActorMoveEntryInfo *info, const unsigned char *move_data)
{
    packet_decode_move_data(move_data, &info->src_x, &info->src_y, &info->dst_x, &info->dst_y,
                          &info->sub_x, &info->sub_y);
    session_sync_server_time(&g_session1, info->move_start_time);
}

static int actor_is_unit(struct GameActor *actor)
{
    return job_is_unit(game_actor_get_job(actor), 46);
}

void game_mode_zc_notify_moveentry(struct GameMode *self, const struct PacketMoveEntry *pkt)
{
    struct ActorMoveEntryInfo info;
    struct GameActor *actor;
    int made_by_new;

    info.body = 0;
    info.aid = pkt->aid;
    info.move_start_time = pkt->move_start_time;
    info.job = pkt->job;
    info.sex = game_mode_job_sex(self, pkt->job, pkt->sex);
    info.speed = pkt->speed;
    info.head = pkt->head;
    info.weapon[0] = pkt->weapon;
    info.weapon[1] = pkt->weapon < 0 ? -1 : 0;
    info.accessory = pkt->accessory;
    info.health_state = pkt->health_state;
    info.body_state = pkt->body_state;
    info.effect_state = pkt->effect_state;
    info.is_pk_mode = pkt->is_pk_mode;
    info.shield = pkt->shield;
    info.accessory2 = pkt->accessory2;
    info.accessory3 = pkt->accessory3;
    info.head_palette = pkt->head_palette;
    info.body_palette = pkt->body_palette;
    info.head_dir = pkt->head_dir;
    info.guild_id = pkt->guild_id;
    info.emblem_version = pkt->emblem_version;
    info.honor = pkt->honor;
    info.virtue = pkt->virtue;
    entry_decode_move(&info, pkt->move_data);

    actor = world_get_game_actor_by_aid(self->world, pkt->aid);
    if (actor) {
        actor_send(actor, ACTOR_MSG_MOVE_ENTRY_OLD, (int)&info, 0);
        if (actor_is_unit(actor))
            entry_unit_value(actor, pkt->accessory2, pkt->accessory3, pkt->shield, 0);
        if (info.head == ENTRY_HEAD_PET)
            entry_note_pet_existing(actor, pkt->aid, info.accessory);
    } else {
        actor = entry_make_actor(self, pkt->job, pkt->aid, 0, &info.sex, &made_by_new);
        if (actor && info.head == ENTRY_HEAD_PET)
            entry_note_pet(actor, pkt->aid, info.accessory);
        world_add_actor(self, actor);
        actor_send(actor, ACTOR_MSG_NEW_MOVE_ENTRY_OLD, (int)&info, 0);
        if (actor_is_unit(actor))
            entry_unit_value(actor, pkt->accessory2, pkt->accessory3, pkt->shield, 1);
    }
    entry_siege_emblem(self, actor, info.head == ENTRY_HEAD_PET, pkt->guild_id, pkt->emblem_version);
    game_mode_game_mode_show_efst(self, pkt->aid, pkt->effect_state, pkt->clevel, pkt->virtue);
}

void game_mode_zc_notify_moveentry2(struct GameMode *self, const struct PacketMoveEntry2 *pkt)
{
    struct ActorMoveEntryInfo info;
    struct GameActor *actor;
    int made_by_new;

    info.body = 0;
    info.aid = pkt->aid;
    info.move_start_time = pkt->move_start_time;
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
    entry_decode_move(&info, pkt->move_data);

    actor = world_get_game_actor_by_aid(self->world, pkt->aid);
    if (actor) {
        actor_send(actor, ACTOR_MSG_MOVE_ENTRY, (int)&info, 0);
        if (info.head == ENTRY_HEAD_PET)
            entry_note_pet_existing(actor, pkt->aid, info.accessory);
        entry_siege_emblem(self, actor, info.head == ENTRY_HEAD_PET, pkt->guild_id, pkt->emblem_version);
        entry_move_marks(self, actor, &info);
    } else {
        actor = entry_make_actor(self, pkt->job, pkt->aid, 0, &info.sex, &made_by_new);
        if (actor && info.head == ENTRY_HEAD_PET)
            entry_note_pet(actor, pkt->aid, info.accessory);
        world_add_actor(self, actor);
        actor_send(actor, ACTOR_MSG_NEW_MOVE_ENTRY, (int)&info, 0);
        entry_siege_emblem(self, actor, info.head == ENTRY_HEAD_PET, pkt->guild_id, pkt->emblem_version);
        if (entry_aid_listed(self, &info.aid))
            game_actor_set_mark_listed(actor, 1);
    }
    game_mode_game_mode_show_efst(self, pkt->aid, pkt->effect_state, pkt->clevel, pkt->virtue);
}

void game_mode_zc_notify_moveentry3(struct GameMode *self, const struct PacketMoveEntry3 *pkt)
{
    struct ActorMoveEntryInfo info;
    struct GameActor *actor;
    int made_by_new;

    info.body = 0;
    info.aid = pkt->aid;
    info.move_start_time = pkt->move_start_time;
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
    entry_decode_move(&info, pkt->move_data);

    actor = world_get_game_actor_by_aid(self->world, pkt->aid);
    if (actor) {
        actor->level = pkt->clevel;
        actor_send(actor, ACTOR_MSG_MOVE_ENTRY, (int)&info, 0);
        if (pkt->object_type == ENTRY_OBJ_PET)
            entry_note_pet_existing(actor, pkt->aid, info.accessory);
        entry_siege_emblem(self, actor, pkt->object_type == ENTRY_OBJ_PET, pkt->guild_id, pkt->emblem_version);
        entry_move_marks(self, actor, &info);
    } else {
        actor = entry_make_actor(self, pkt->job, pkt->aid, pkt->object_type == ENTRY_OBJ_MERC, &info.sex,
                                 &made_by_new);
        if (actor && pkt->object_type == ENTRY_OBJ_PET)
            entry_note_pet(actor, pkt->aid, info.accessory);
        actor->level = pkt->clevel;
        actor->object_type = pkt->object_type;
        world_add_actor(self, actor);
        actor_send(actor, ACTOR_MSG_NEW_MOVE_ENTRY, (int)&info, 0);
        entry_siege_emblem(self, actor, pkt->object_type != ENTRY_OBJ_PET, pkt->guild_id, pkt->emblem_version);
        if (entry_aid_listed(self, &info.aid))
            game_actor_set_mark_listed(actor, 1);
    }
    game_mode_game_mode_show_efst(self, pkt->aid, pkt->effect_state, pkt->clevel, pkt->virtue);
}

void game_mode_zc_notify_moveentry6(struct GameMode *self, const struct PacketMoveEntry6 *pkt)
{
    struct ActorMoveEntryInfo info;
    struct GameActor *actor;
    int made_by_new;

    info.body = 0;
    info.robe = pkt->robe;
    entry_store_name(self, pkt->aid, pkt->name, pkt->length - 0x4b);
    info.aid = pkt->aid;
    info.move_start_time = pkt->move_start_time;
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
    entry_decode_move(&info, pkt->move_data);

    actor = world_get_game_actor_by_aid(self->world, pkt->aid);
    if (actor) {
        actor->level = pkt->clevel;
        actor->balloon_font = pkt->font;
        actor_send(actor, ACTOR_MSG_MOVE_ENTRY, (int)&info, 0);
        if (pkt->object_type == ENTRY_OBJ_PET)
            entry_note_pet_existing(actor, pkt->aid, info.accessory);
        entry_siege_emblem(self, actor, pkt->object_type != ENTRY_OBJ_PET, pkt->guild_id, pkt->emblem_version);
        entry_move_marks(self, actor, &info);
    } else {
        actor = entry_make_actor(self, pkt->job, pkt->aid, pkt->object_type == ENTRY_OBJ_MERC, &info.sex,
                                 &made_by_new);
        if (actor && pkt->object_type != ENTRY_OBJ_PC)
            entry_note_pet(actor, pkt->aid, info.accessory);
        actor->object_type = pkt->object_type;
        world_add_actor(self, actor);
        actor_send(actor, ACTOR_MSG_NEW_MOVE_ENTRY, (int)&info, 0);
        actor->balloon_font = pkt->font;
        actor->level = pkt->clevel;
        entry_siege_emblem(self, actor, pkt->object_type != ENTRY_OBJ_PET, pkt->guild_id, pkt->emblem_version);
        if (entry_aid_listed(self, &info.aid))
            game_actor_set_mark_listed(actor, 1);
    }
    game_mode_game_mode_show_efst(self, pkt->aid, pkt->effect_state, pkt->clevel, pkt->virtue);
}

void game_mode_zc_notify_ranking(struct GameMode *self, const struct PacketNotifyRanking *pkt)
{
    char text[256];
    struct GameActor *actor;

    if (g_session1.map_rule_flags[3] == 0 && self->world->is_pvp == 0)
        return;

    if (pkt->aid == ((unsigned)g_session1.ownAid) && self->world->player) {
        struct Player *player = self->world->player;

        if (pkt->rank == 1) {
            if (player->pvp_rank != 1)
                g_session1.pvpFirstPlaceTime = timeGetTime();
        } else {
            g_session1.pvpFirstPlaceTime = 0;
        }
        if (self->world->player->pvp_rank != pkt->rank) {
            int old_rank = self->world->player->pvp_rank, message = 0;

            if (old_rank > 10 && pkt->rank <= 10)
                message = 0x246;
            else if (old_rank <= 10 && pkt->rank > 10)
                message = 0x247;
            if (message) {
                const char *name = session_character_name(&g_session1);
                const char *format = msg_str(message);

                sprintf(text, format, name, pkt->rank);
                ui_window_mgr_send_msg_(&g_windowMgr, 1, (int)text, 0xffff, 0, 0);
            }
            player = self->world->player;
            sound_play_wave("effect\\number_change.wav", player->pos.x, 0.0f, player->pos.z, 0xfa, 0x28, 1.0f);
        }
        self->world->player->pvp_total = pkt->total;
        self->world->player->pvp_rank = pkt->rank;
        actor = (struct GameActor *)player;
    } else {
        int effect_state;

        actor = world_get_game_actor_by_aid(self->world, pkt->aid);
        effect_state = actor_get_effect_state(actor);
        if (is_effect_state_cloaking(effect_state) || is_effect_state_hiding(effect_state) || effect_state_bit6(effect_state))
            return;
    }

    if (clientinfo_is_admin_aid(actor->aid) && !session_is_admin_option(&g_session1, 7))
        return;
    actor_send(actor, ACTOR_MSG_RANKING, pkt->total, pkt->rank);
    actor_send(actor, ACTOR_MSG_DROP_EFFECT, EFFECT_RANKING_CROWN, 0);
    if (pkt->rank > 10)
        return;
    {
        struct RagEffect *effect = game_actor_launch_effect(actor, EFFECT_RANKING_CROWN, (Vec3){ 0.0f, 0.0f, 0.0f }, 0.0f);

        if (effect)
            rag_effect_send_msg(effect, NULL, EFFECT_MSG_RANK, pkt->rank, 0, 0);
    }
}
