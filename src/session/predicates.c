#include "actor/actor_ops.h"
#include "game/context.h"
#include "session/csession.h"
#include "session/session_achievements.h"
#include "session/session_jobs.h"
#include "session/session_names.h"
#include "session/session_party.h"
#include "actor/job.h"

struct GameActor;

int session_is_abracadabra_skill(struct Session *self, int id)
{
    if ((unsigned)(id - 0x123) > 0xc)
        return 0;
    return id != 0x128;
}

int session_is_attack(struct Session *self, int state)
{
    return state == 0 || state == 4;
}

BYTE session_is_baby_job(struct Session *self, int id)
{
    if (id >= 0xfb7 && id <= 0xfbd)
        return 1;
    if (id == 0xfcd || id == 0x105f)
        return 1;
    if ((unsigned)(id - 0xfbe) <= 0xe || (unsigned)(id - 0x1000) <= 0x10 || (unsigned)(id - 0x103e) <= 0x18)
        return 1;
    if (id >= 0x1062 && id <= 0x1064)
        return 1;
    if (id == 0x1061 || (unsigned)(id - 0x106d) <= 5 || id == 0x107c || (unsigned)(id - 0x107e) <= 0x10)
        return 1;
    if (id == 0x1091 || id == 0x1092 || id == 0x1094 || id == 0x1097 || id == 0x1098)
        return 1;
    return 0;
}

BYTE session_is_cash_item_type(struct Session *self, int id)
{
    return id == 0x12;
}

BYTE session_is_costume_or_shadow_location(struct Session *self, int flag)
{
    switch (flag) {
    case 0x400: case 0x800: case 0xc00: case 0x1000: case 0x1400: case 0x1800: case 0x1c00: case 0x2000:
    case 0x4000: case 0x10000: case 0x20000: case 0x40000: case 0x80000: case 0x100000: case 0x200000:
        return 1;
    default:
        return 0;
    }
}

BYTE session_is_equip_item_type(struct Session *self, int id)
{
    return id == 4 || id == 0xb || id == 0xc || id == 0xd || id == 0xe || id == 5 || id == 8 || id == 9 ||
           id == 0xf;
}

int session_is_guardian(struct Session *self, int job)
{
    return job == 0x505 || job == 0x506 || job == 0x507;
}

int session_is_homunculus_job(struct Session *self, int id)
{
    return (unsigned)(id - 0x1771) <= 0xf || (unsigned)(id - 0x17a0) <= 4;
}

int session_is_homunculus_s_job(struct Session *self, int id)
{
    return (unsigned)(id - 0x17a0) <= 4;
}

int session_is_homunculus_skill(struct Session *self, int id)
{
    return (unsigned)(id - 0x1f40) <= 0x3c;
}

int session_is_mercenary_or_homun_s_job(struct Session *self, int id)
{
    return (unsigned)(id - 0x1782) <= 0x22;
}

int session_is_homun_or_merc_job(struct Session *self, int id)
{
    return (unsigned)(id - 0x1771) <= 0x33;
}

int session_is_merc_or_elemental_skill(struct Session *self, int id)
{
    return (unsigned)(id - 0x2008) <= 0x29 || (unsigned)(id - 0x20d0) <= 0x39;
}

int session_is_pet_egg_id(struct Session *self, int id)
{
    return (unsigned)(id - 0x2329) <= 0x1f2;
}

BYTE session_is_space(struct Session *self, int id)
{
    return id == 0x7f7 || id == 0x9a0;
}

BYTE session_is_expanded_super_novice_job(struct Session *self, int id)
{
    return id == 0x105e || id == 0x105f || id == 0x1060 || id == 0x1061;
}

BYTE session_is_super_novice_job(struct Session *self, int id)
{
    return id == 0x17 || id == 0xfcd || id == 0x1020 || id == 0x104c;
}

BYTE session_is_any_super_novice_job(struct Session *self, int id)
{
    return id == 0x17 || id == 0xfcd || id == 0x1020 || id == 0x104c || id == 0x105e || id == 0x105f ||
           id == 0x1060 || id == 0x1061;
}

BYTE session_is_expanded_job(struct Session *self, int id)
{
    switch (id) {
    case 0x1073: case 0x1074: case 0x1075: case 0x1076: case 0x1077: case 0x1078:
    case 0x107f: case 0x1080:
    case 0x1085:
    case 0x1087: case 0x1088:
    case 0x108d:
    case 0x108f: case 0x1090: case 0x1091: case 0x1092: case 0x1093: case 0x1094: case 0x1095: case 0x1096:
    case 0x1097: case 0x1098:
        return 1;
    default:
        return 0;
    }
}

int session_is_monster_actor(struct Session *self, struct GameActor *actor)
{
    int job;

    if (!actor)
        return 0;
    job = game_actor_get_job(actor);
    if (job > 0x3e8 && job < 0xf9f)
        return 1;
    if ((unsigned)(job - 0x4e35) <= 0x2eca)
        return 1;
    if (game_actor_get_job(actor) >= 0x842)
        game_actor_get_job(actor);
    return 0;
}

int session_is_player_job(struct Session *self, int id)
{
    return session_is_pc_job(self, id);
}

static int has_status(int id)
{
    return own_states_find(id) != NULL;
}

int session_has_trick_dead(struct Session *self)
{
    return has_status(0x1d);
}

int session_has_status(struct Session *self, int id)
{
    return has_status(id);
}

int session_has_status_effect(struct Session *self, int id)
{
    return has_status(id);
}

int session_is_homunculus_aid(struct Session *self, int aid)
{
    return aid != -1 && aid == self->homunAid;
}

int session_is_mercenary_aid(struct Session *self, int aid)
{
    return aid != -1 && aid == self->mercAid;
}

int session_is_own_aid(struct Session *self, int aid)
{
    return aid != -1 && aid == self->aid;
}

int session_is_own_unit_aid(struct Session *self, int aid)
{
    long long key = (unsigned)aid;
    struct Int64Pair *hp;

    if (aid == -1)
        return 0;
    if (aid == self->homunAid || aid == self->mercAid || aid == self->aid || aid == self->elementalAid)
        return 1;
    hp = tree_find(&self->abrHp, &key, tree_less_int64);
    if (!hp)
        return 0;
    return hp->value != -1;
}

int session_is_party_member_aid(struct Session *self, int aid, BYTE onlyIfClear)
{
    struct ListNode *head = self->partyMembers.head;
    struct ListNode *node;

    for (node = head->next; node != head; node = node->next) {
        const struct PartyMember *member = LIST_VALUE(node, struct PartyMember);

        if ((int)member->aid == aid) {
            if (onlyIfClear && member->state != 0)
                return 0;
            return 1;
        }
    }
    return 0;
}

int session_is_own_guild_id(struct Session *self, int id)
{
    void *object;

    if (id == 0)
        return 0;
    object = game_current_player();
    if (!object)
        return 0;
    if (id != game_actor_get_guild_id(object))
        return 0;
    return 1;
}

int session_is_siege_mode(struct Session *self)
{
    return game_siege_state();
}

BYTE session_is_tracked_achievement(struct Session *self, int id)
{
    struct TrackedAchievement *it = (struct TrackedAchievement *)self->trackedAchievements.first;
    struct TrackedAchievement *end = (struct TrackedAchievement *)self->trackedAchievements.last;

    for (; it != end; it++)
        if (it->id == id)
            break;
    return (struct TrackedAchievement *)self->trackedAchievements.last != it;
}

BYTE session_has_tracked_achievements(struct Session *self)
{
    return VEC_COUNT(&self->trackedAchievements, struct TrackedAchievement) != 0;
}

int session_map_is_gvg(struct Session *self) { return map_property_mgr_get(g_mapFlags, self->mapType, 8); }
int session_map_is_pvp(struct Session *self) { return map_property_mgr_get(g_mapFlags, self->mapType, 7); }
int session_map_is_battlefield(struct Session *self) { return map_property_mgr_get(g_mapFlags, self->mapType, 0x13); }
int session_map_counts_pk(struct Session *self) { return map_property_mgr_get(g_mapFlags, self->mapType, 0x11); }
int session_map_disables_lock_on(struct Session *self) { return map_property_mgr_get(g_mapFlags, self->mapType, 0x10); }
int session_map_locks_party(struct Session *self) { return map_property_mgr_get(g_mapFlags, self->mapType, 0x12); }
int session_map_is_siege(struct Session *self) { return map_property_mgr_get(g_mapFlags, self->mapType, 9); }
int session_map_uses_simple_effect(struct Session *self) { return map_property_mgr_get(g_mapFlags, self->mapType, 0xf); }

BYTE session_is_doram_job(struct Session *self, int job)
{
    switch (job) {
    case 0x1079: case 0x107a: case 0x107b: case 0x107c: case 0x107d:
        return 1;
    default:
        return 0;
    }
}

BYTE session_is_head_replacing_view(struct Session *self, int id)
{
    return id == 0x14b || id == 0x78b || id == 0x87b;
}

BYTE session_is_npc(struct Session *self, int aid, int job)
{
    struct GameActor *actor = session_get_actor_by_aid(self, aid);

    if (actor) {
        BYTE kind = actor->object_type;

        if (kind == 1 || kind == 6 || kind == 12)
            return 1;
    }
    return session_get_pet_acc_act_name(self, job);
}

BYTE session_can_request_adoption(struct Session *self, int aid)
{
    struct GameActor *actor;
    struct String name;
    struct PartyMember member;
    DWORD first;
    BYTE ok;

    if (g_session1.charBaseLevel < 0x46)
        return 0;
    if (session_is_baby_job(&g_session1, session_get_drawn_job(&g_session1)) == 1)
        return 0;
    if (g_session1.married == 0)
        return 0;
    actor = session_get_actor_by_aid(&g_session1, aid);
    if (!actor)
        return 0;
    game_mode_get_actor_name(game_current_mode(), &name, aid);
    first = session_get_party_member_by_name(&g_session1, &member, string_cstr(&name))->found;
    string_free(&member.map_name);
    string_free(&member.name);
    if (first != 0 && (short)actor->level <= 0x32 && session_is_baby_job(&g_session1, aid) != 1
        && (unsigned)(aid - 0xfa1) > 0x14)
        ok = 1;
    else
        ok = 0;
    string_free(&name);
    return ok;
}
