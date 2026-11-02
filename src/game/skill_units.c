#include "game/packet_handlers.h"
#include "game/cells.h"
#include "actor/job.h"
#include "actor/render_object.h"
#include "resources/attr_queries.h"
#include "session/state.h"
#include "core/memory.h"
#include "game/skill_unit_packets.h"
#include "effects/skills/skill_state.h"
#include "game/objects.h"
#include "game/actor_entry.h"
#include "actor/actor_common.h"
#include "effects/rag/rag_effect.h"
#include "effects/skills/skill.h"
#include "game/packets_entry.h"
#include "session/session_jobs.h"
#include "session/session_talk.h"
#include "ui/ui_std.h"
#include "ui/wndmgr/wndmgr.h"
#include "world/world.h"

enum {
    UNT_FIRST = 0x2d,
    UNT_GRAFFITI = 0xb0,
    UNT_DRAWN_AS_CREATOR = 0x126,
};
enum {
    OBJ_MSG_SET_AID = 0x16,
    OBJ_MSG_SKILL_ENTRY = 0x2d,
    OBJ_MSG_SET_TEXT = 0x75,
    OBJ_MSG_SKILL_UPDATE = 0x77,
};

#define SKILL_UNIT_SIZE sizeof(struct Skill)

struct AbleToMakeEffect;
void game_mode_spawn_unit_double(struct GameMode *self, unsigned aid, unsigned creator_aid,
                                         int x, int y);

static struct ListNode *find_unit_id(struct GameMode *self, unsigned aid)
{
    struct ListNode *head = self->shown_unit_ids.head, *node;

    for (node = head->next; node != head; node = node->next)
        if (*LIST_VALUE(node, unsigned) == aid)
            return node;
    return head;
}

static void note_unit_inline(struct GameMode *self, const unsigned *aid, unsigned creator_aid, char *visible)
{
    if (creator_aid != ((unsigned)g_session1.ownAid) && !*visible) {
        struct ListNode *node = find_unit_id(self, *aid);
        if (node != self->shown_unit_ids.head) {
            list_erase(&self->shown_unit_ids, node, NULL);
            *visible = 0;
        }
        return;
    }
    if (*visible)
        list_push_back(&self->shown_unit_ids, aid, sizeof *aid);
}

static void note_unit_call(struct GameMode *self, const unsigned *aid, unsigned creator_aid, char *visible)
{
    if (creator_aid != ((unsigned)g_session1.ownAid) && !*visible) {
        struct ListNode *node = find_unit_id(self, *aid);
        if (node != self->shown_unit_ids.head) {
            list_erase(&self->shown_unit_ids, node, NULL);
            *visible = 0;
        }
        return;
    }
    if (*visible)
        list_push_back(&self->shown_unit_ids, aid, sizeof *aid);
}

static void add_text_object(struct GameMode *self, unsigned aid, const char *text, short x, short y)
{
    struct AbleToMakeEffect *obj = (struct AbleToMakeEffect *)graffi_skill_new();

    list_push_back(&self->world->gameObjects, &obj, sizeof obj);
    list_push_back(&self->world->skills, &obj, sizeof obj);
    render_object_send(obj, NULL, OBJ_MSG_SET_AID, aid, 0, 0, 0);
    render_object_send(obj, NULL, OBJ_MSG_SET_TEXT, (long long)(int)text, x, y, 0);
}

static struct Skill *add_unit_inline(struct GameMode *self, void *block, unsigned aid)
{
    struct Skill *unit = 0;

    if (!block) return NULL;
    unit = skill_construct(block, (int)aid);
    list_push_back(&self->world->gameObjects, &unit, sizeof unit);
    list_push_back(&self->world->skills, &unit, sizeof unit);
    return unit;
}

static struct Skill *add_unit_call(struct GameMode *self, void *block, unsigned aid)
{
    struct Skill *unit = 0;

    if (!block) return NULL;
    unit = skill_construct(block, (int)aid);
    list_push_back(&self->world->gameObjects, &unit, sizeof unit);
    list_push_back(&self->world->skills, &unit, sizeof unit);
    return unit;
}

static int job_makes_unit(int job)
{
    return (job >= UNT_FIRST && job < 1000) || (job > 1000 && job < 3999)
        || (unsigned)(job - 20021) <= 11978;
}

static void send_entry(struct Skill *unit, struct SkillUnitEntry *entry)
{
    if (unit) render_object_send(unit, NULL, OBJ_MSG_SKILL_ENTRY, (intptr_t)entry, 0, 0, 0);
}

void game_mode_zc_skill_entry(struct GameMode *self, struct PACKET_ZC_SKILL_ENTRY *packet)
{
    struct SkillUnitEntry entry = {0};
    struct Skill *unit;

    entry.job = packet->job;
    entry.x = packet->x;
    entry.y = packet->y;
    packet->visible = 1;
    entry.aid = packet->aid;
    entry.visible = 1;
    entry.creator_aid = packet->creator_aid;
    entry.range = 0;
    unit = (struct Skill *)world_get_skill_by_aid(self->world, packet->aid);
    if (packet->creator_aid != ((unsigned)g_session1.ownAid) && !entry.visible) {
        struct ListNode *node = find_unit_id(self, packet->aid);
        if (node != self->shown_unit_ids.head) {
            list_erase(&self->shown_unit_ids, node, NULL);
            packet->visible = 0;
            entry.visible = 0;
        }
    } else if (entry.visible) {
        list_push_back(&self->shown_unit_ids, &packet->aid, sizeof packet->aid);
    }

    if (unit == 0) {
        if (packet->job < UNT_FIRST)
            return;
        unit = add_unit_inline(self, mem_alloc(SKILL_UNIT_SIZE), packet->aid);
    }
    if (!unit) return;
    unit->creator_aid = (int)packet->creator_aid;
    send_entry(unit, &entry);
}

void game_mode_zc_skill_entry2(struct GameMode *self, struct PACKET_ZC_SKILL_ENTRY2 *packet)
{
    struct SkillUnitEntry entry = {0};
    struct Skill *unit;

    entry.visible = 1;
    entry.range = 0;
    entry.job = packet->job;
    entry.x = packet->x;
    entry.y = packet->y;
    entry.aid = packet->aid;
    packet->visible = 1;
    entry.creator_aid = packet->creator_aid;
    if (packet->has_text == 1) {
        char text[sizeof packet->text + 1];
        memcpy(text, packet->text, sizeof packet->text);
        text[sizeof packet->text] = 0;
        add_text_object(self, packet->aid, text, packet->x, packet->y);
        return;
    }

    unit = (struct Skill *)world_get_skill_by_aid(self->world, packet->aid);
    if (entry.creator_aid != ((unsigned)g_session1.ownAid) && !entry.visible) {
        struct ListNode *node = find_unit_id(self, packet->aid);
        if (node != self->shown_unit_ids.head) {
            list_erase(&self->shown_unit_ids, node, NULL);
            packet->visible = 0;
            entry.visible = 0;
        }
    } else if (entry.visible) {
        list_push_back(&self->shown_unit_ids, &packet->aid, sizeof packet->aid);
    }

    if (unit == 0) {
        if (packet->job < UNT_FIRST)
            return;
        unit = add_unit_call(self, mem_alloc(SKILL_UNIT_SIZE), 0);
    }
    send_entry(unit, &entry);
}

void game_mode_zc_skill_entry3(struct GameMode *self, const struct PACKET_ZC_SKILL_ENTRY3 *packet)
{
    struct SkillUnitEntry entry = {0};
    struct Skill *unit;

    if (packet->job == UNT_GRAFFITI) {
        if (packet->length > 0x13)
            add_text_object(self, packet->aid, packet->text, packet->x, packet->y);
        return;
    }

    entry.job = packet->job;
    entry.x = packet->x;
    entry.y = packet->y;
    entry.aid = packet->aid;
    entry.visible = packet->visible;
    entry.creator_aid = packet->creator_aid;
    entry.range = packet->range;
    note_unit_inline(self, &packet->aid, packet->creator_aid, &entry.visible);

    unit = (struct Skill *)world_get_skill_by_aid(self->world, packet->aid);
    if (unit == 0) {
        if (packet->job < UNT_FIRST)
            return;
        unit = add_unit_inline(self, mem_alloc(SKILL_UNIT_SIZE), packet->aid);
        if (unit == 0)
            return;
    }
    unit->creator_aid = packet->creator_aid;
    send_entry(unit, &entry);
}

void game_mode_zc_skill_entry4(struct GameMode *self, const struct PACKET_ZC_SKILL_ENTRY4 *packet)
{
    struct SkillUnitEntry entry = {0};
    struct Skill *unit;

    if (packet->job == UNT_GRAFFITI) {
        if (packet->length > 0x16)
            add_text_object(self, packet->aid, packet->text, packet->x, packet->y);
        return;
    }

    entry.x = packet->x;
    entry.job = packet->job;
    entry.y = packet->y;
    entry.aid = packet->aid;
    entry.visible = packet->visible;
    entry.creator_aid = packet->creator_aid;
    entry.range = packet->range;
    note_unit_inline(self, &packet->aid, packet->creator_aid, &entry.visible);

    unit = (struct Skill *)world_get_skill_by_aid(self->world, packet->aid);
    if (unit == 0) {
        if (!job_makes_unit(packet->job))
            return;
        unit = add_unit_inline(self, mem_try_alloc(SKILL_UNIT_SIZE), packet->aid);
        if (unit == 0)
            return;
    }
    unit->creator_aid = packet->creator_aid;
    send_entry(unit, &entry);
}

void game_mode_zc_skill_entry5(struct GameMode *self, const struct PACKET_ZC_SKILL_ENTRY5 *packet)
{
    struct SkillUnitEntry entry = {0};
    struct Skill *unit;

    if (packet->job == UNT_GRAFFITI) {
        struct AbleToMakeEffect *obj;

        if (packet->length <= 0x17)
            return;
        obj = (struct AbleToMakeEffect *)graffi_skill_new();
        render_object_send(obj, NULL, OBJ_MSG_SET_AID, packet->aid, 0, 0, 0);
        render_object_send(obj, NULL, OBJ_MSG_SET_TEXT, (long long)(int)packet->text, (short)packet->x, (short)packet->y, 0);
        list_push_back(&self->world->gameObjects, &obj, sizeof obj);
        list_push_back(&self->world->skills, &obj, sizeof obj);
        return;
    }
    if (packet->job == UNT_DRAWN_AS_CREATOR) {
        game_mode_spawn_unit_double(self, packet->aid, packet->creator_aid, packet->x, packet->y);
        return;
    }

    entry.x = (short)packet->x;
    entry.y = (short)packet->y;
    entry.visible = packet->visible;
    entry.job = packet->job;
    entry.aid = packet->aid;
    entry.creator_aid = packet->creator_aid;
    entry.range = packet->range;
    entry.level = packet->level;
    note_unit_call(self, &packet->aid, packet->creator_aid, &entry.visible);

    unit = (struct Skill *)world_get_skill_by_aid(self->world, packet->aid);
    if (unit == 0) {
        if (!job_makes_unit(packet->job))
            return;
        unit = add_unit_call(self, mem_try_alloc(SKILL_UNIT_SIZE), packet->aid);
        if (unit == 0)
            return;
    }
    unit->creator_aid = packet->creator_aid;
    send_entry(unit, &entry);
}

void game_mode_zc_skill_update(struct GameMode *self, const struct PACKET_ZC_SKILL_UPDATE *packet)
{
    struct GameActor *unit = (struct GameActor *)world_get_skill_by_aid(self->world, packet->aid);

    if (unit == 0)
        return;
    if (game_actor_get_job(unit) > 0x8a && game_actor_get_job(unit) <= 0x99)
        render_object_send(unit, NULL, OBJ_MSG_SKILL_UPDATE, 0, 0, 0, 0);
}

void game_mode_spawn_unit_double(struct GameMode *self, unsigned aid, unsigned creator_aid,
                                         int x, int y)
{
    if (creator_aid == ((unsigned)g_session1.ownAid)) {
        struct GameActor *actor = world_get_game_actor_by_aid(self->world, aid);

        if (!actor) {
            struct ActorEntryInfo info;
            struct Vec *ids = &self->own_double_aids;
            void *block;

            actor_entry_info_construct(&info);
            info.x = (short)x;
            info.aid = aid;
            info.y = (short)y;
            info.job = session_get_drawn_job(&g_session1);
            info.sex = session_get_sex(&g_session1);
            info.head = g_session1.hairStyle;
            info.body = g_session1.body;
            info.head_palette = g_session1.hairColor;
            info.body_palette = g_session1.clothesColor;
            info.weapon[0] = (int)g_session1.weapon;
            info.weapon[1] = (int)(g_session1.weapon >> 32);
            info.shield = g_session1.shield;
            info.accessory = g_session1.headBottom;
            info.accessory2 = g_session1.headTop;
            info.accessory3 = g_session1.headMid;
            info.robe = g_session1.robe;
            info.dir = 0xb4;
            block = mem_alloc(sizeof(struct Pc));
            actor = block ? (struct GameActor *)pc_construct(block) : 0;
            render_object_send(actor, NULL, ACTOR_MSG_NEW_ENTRY, (long long)(int)&info, 0, 0, 0);
            actor->object_type = 7;
            game_actor_launch_effect(actor, 0x3c3, (Vec3){ 0.0f, 0.0f, 0.0f }, 0.0f);
            list_push_back(&self->world->gameObjects, &actor, sizeof actor);
            actor_list_push_back(&self->world->actors, actor);
            vec_push(ids, &aid, sizeof(unsigned));
        } else {
            game_mode_cell_to_world(self, (short)x, (short)y, &actor->pos.x, &actor->pos.z);
            actor->pos.y = c3d_attr_get_height(self->world->attr, actor->pos.x, actor->pos.z);
        }
    } else {
        struct SkillUnitEntry entry = {0};
        struct Skill *unit;

        if (world_get_skill_by_aid(self->world, aid))
            world_remove_skill(self->world, (int)aid);
        entry.x = (short)x;
        entry.aid = aid;
        entry.creator_aid = creator_aid;
        entry.job = UNT_DRAWN_AS_CREATOR;
        entry.y = (short)y;
        entry.visible = 1;
        unit = 0;
        {
            void *block = mem_alloc(SKILL_UNIT_SIZE);

            if (block)
                unit = skill_construct(block, 0);
        }
        send_entry(unit, &entry);
        list_push_back(&self->world->gameObjects, &unit, sizeof unit);
        list_push_back(&self->world->skills, &unit, sizeof unit);
    }
}
