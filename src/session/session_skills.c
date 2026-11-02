#include "skills/skill_info.h"
#include "contents/client_lua_calls.h"
#include "session/state.h"
#include "session/csession.h"
#include "session/session_skills.h"
#include "session/session_state.h"
#include "session/session_windows.h"

static void copy_skill_fields(struct SkillInfo *to, const struct SkillInfo *from)
{
    to->level = from->level;
    to->sp_cost = from->sp_cost;
    to->range = from->range;
    to->upgradable = from->upgradable;
}

static void sync_table_level(struct Session *self, const struct SkillInfo *skill)
{
    int have;

    if (!skill_info_is_level_used(skill->id) || skill->level == 0)
        return;
    have = session_get_skill_table_value(self, skill->id);
    if (have <= 0 || have > skill->level)
        player_skill_bundle_set_chosen_level(&self->skills, skill->id, skill->level);
}

static BYTE list_put_skill(struct Session *self, struct List *list, const struct SkillInfo *skill,
                           BYTE notify, int copyType,
                           void (*refresh)(struct Session *))
{
    struct ListNode *node;
    struct SkillInfo copy;

    for (node = list->head->next; node != list->head; node = node->next) {
        struct SkillInfo *have = LIST_VALUE(node, struct SkillInfo);
        if (have->id == skill->id) {
            copy_skill_fields(have, skill);
            if (copyType)
                have->type = skill->type;
            if (notify)
                refresh(self);
            return 0;
        }
    }
    skill_info_copy(&copy, skill);
    list_push_back(list, &copy, sizeof copy);
    if (notify)
        refresh(self);
    return 1;
}

BYTE session_put_homun_skill(struct Session *self, const struct SkillInfo *skill,
                                       BYTE notify)
{
    return list_put_skill(self, &self->homunSkills, skill, notify, 1, session_refresh_homun_skill_item_wnd);
}

BYTE session_put_mer_skill(struct Session *self, const struct SkillInfo *skill,
                                     BYTE notify)
{
    return list_put_skill(self, &self->merSkills, skill, notify, 1, session_refresh_mer_skill_item_wnd);
}

BYTE session_put_skill4(struct Session *self, const struct SkillInfo *skill)
{
    return list_put_skill(self, &self->guildSkills, skill, 0, 0, NULL);
}

BYTE session_put_skill(struct Session *self, const struct SkillInfo *skill,
                                  BYTE notify, BYTE keepType)
{
    struct SkillInfo copy, *have;

    skill_info_copy(&copy, skill);
    have = player_skill_bundle_find_tree_skill(&self->skills, copy.id);
    if (have) {
        copy_skill_fields(have, &copy);
        have->type = copy.type;
        have->level2 = copy.level2;
        sync_table_level(self, &copy);
    } else if (copy.found != 0) {
        have = player_skill_bundle_find_extra_skill(&self->skills, copy.id);
        if (have) {
            copy_skill_fields(have, &copy);
            have->type = copy.type;
        } else if (copy.found != 0) {
            copy.cell_index = -1;
            copy.level_limit = 0;
            player_skill_bundle_add_extra_skill(&self->skills, &copy);
        }
        sync_table_level(self, &copy);
    }
    skill_info_destruct(&copy);

    have = player_skill_bundle_find_skill(&self->skills, skill->id);
    if (have) {
        copy_skill_fields(have, skill);
        have->type = keepType == 1 ? skill->type : have->type;
        have->level2 = skill->level2;
        if (notify == 1)
            session_refresh_skill_wnds(self);
        return 0;
    }
    player_skill_bundle_append_skill(&self->skills, skill);
    if (notify != 0)
        session_refresh_skill_wnds(self);
    return 1;
}

BYTE session_put_skill_keep_type(struct Session *self, const struct SkillInfo *skill,
                                          BYTE notify)
{
    return session_put_skill(self, skill, notify, 1);
}

#define IS_HOMUN_SKILL(id) ((unsigned)((id) - 8000) <= 0x3c)
#define IS_MER_SKILL(id)   ((unsigned)((id) - 8200) <= 0x29 || (unsigned)((id) - 8400) <= 0x39)
#define IS_GUILD_SKILL(id) ((unsigned)((id) - 10000) <= 0x13)

static struct SkillInfo *list_skill_at(struct List *list, struct SkillInfo *out, int index)
{
    if (list->size != 0) {
        struct ListNode *head = list->head, *node;
        int i = 0;
        for (node = head->next; node != head; node = node->next, i++)
            if (i == index)
                return skill_info_copy(out, LIST_VALUE(node, struct SkillInfo));
    }
    skill_info_construct_unknown(out);
    return out;
}

static struct SkillInfo *list_skill_by_id(struct List *list, struct SkillInfo *out, int id)
{
    if (list->size != 0) {
        struct ListNode *head = list->head, *node;
        for (node = head->next; node != head; node = node->next)
            if (LIST_VALUE(node, struct SkillInfo)->id == id)
                return skill_info_copy(out, LIST_VALUE(node, struct SkillInfo));
    }
    skill_info_construct_unknown(out);
    return out;
}

struct SkillInfo *session_get_skill_at(struct Session *self, struct SkillInfo *out,
                                                 int index)
{
    return list_skill_at(&self->guildSkills, out, index);
}

struct SkillInfo *session_get_skill4_by_id(struct Session *self, struct SkillInfo *out,
                                                    int id)
{
    return list_skill_by_id(&self->guildSkills, out, id);
}

struct SkillInfo *session_get_homun_skill_at(struct Session *self,
                                                      struct SkillInfo *out, int index)
{
    return list_skill_at(&self->homunSkills, out, index);
}

struct SkillInfo *session_get_homun_skill_by_id(struct Session *self,
                                                        struct SkillInfo *out, int id)
{
    return list_skill_by_id(&self->homunSkills, out, id);
}

struct SkillInfo *session_get_mer_skill_at(struct Session *self,
                                                    struct SkillInfo *out, int index)
{
    return list_skill_at(&self->merSkills, out, index);
}

struct SkillInfo *session_get_mer_skill_by_id(struct Session *self,
                                                      struct SkillInfo *out, int id)
{
    return list_skill_by_id(&self->merSkills, out, id);
}

struct SkillInfo *session_get_skill_info(struct SkillInfo *out,
                                                   int id)
{
    struct SkillInfo found;

    skill_info_construct_unknown(out);
    if (IS_HOMUN_SKILL(id))
        session_get_homun_skill_by_id(&g_session1, &found, id);
    else if (IS_MER_SKILL(id))
        session_get_mer_skill_by_id(&g_session1, &found, id);
    else if (IS_GUILD_SKILL(id))
        session_get_skill4_by_id(&g_session1, &found, id);
    else
        skill_table_find(&g_session1.skills, &found, id);
    skill_info_assign(out, &found);
    skill_info_destruct(&found);
    return out;
}

int session_get_skill_level(int id)
{
    if (IS_HOMUN_SKILL(id))
        return session_get_homun_skill_use_level(&g_session1, id);
    if (IS_MER_SKILL(id))
        return session_get_merc_skill_use_level(&g_session1, id);
    return session_get_skill_table_value(&g_session1, id);
}

void session_drop_skill(struct Session *self, int id)
{
    struct SkillInfo *have = player_skill_bundle_find_tree_skill(&self->skills, id);

    if (have) {
        struct SkillActionAnswer level;
        player_skill_bundle_level_before_change(&self->skills, &level, id);
        if (level.found == 1)
            have->level = level.value;
        session_refresh_skill_wnds(self);
        return;
    }
    player_skill_bundle_remove_skill(&self->skills, id);
    if (player_skill_bundle_remove_extra_skill(&self->skills, id) == 1)
        session_refresh_skill_wnds(self);
}

int session_get_begin_effect_id(int id)
{
    int effect = -1;
    BYTE have = 0;

    if (client_lua_call(g_session1.luaState, "HaveSkillEffectInfo", "d>b", id, &have) && have)
        client_lua_call(g_session1.luaState, "GetBeginEffectID", "d>d", id, &effect);
    return effect;
}

struct SkillInfo *session_find_skill_info(struct Session *self, int id)
{
    return player_skill_bundle_find_tree_skill(&self->skills, id);
}
