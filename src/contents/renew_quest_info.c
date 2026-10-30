#include "contents/client_lua_calls.h"
#include "platform/service.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include <stdlib.h>
#include <string.h>
#include "contents/client_lua.h"
#include "contents/lua_data.h"
#include "contents/renew_quest.h"
#include "contents/ui_window.h"
#include "game/store_search_items.h"
#include "ui/ui_std.h"

struct RenewQuestB g_questEmptyOngoing;
INIT_ONCE g_questEmptyOngoingGuard;
struct RenewQuestA g_questEmptyRecommended;
INIT_ONCE g_questEmptyRecommendedGuard;
void renew_quest_empty_ongoing_atexit(void);
void renew_quest_empty_recommended_atexit(void);

#define SERVICE_SAKRAY 1

#define TITLE_WIDTH 0xfa
#define TITLE_FONT_HEIGHT 0xc

struct RenewQuestB *renew_quest_ongoing_info(struct RenewQuestMgr *self, unsigned quest_id)
{
    struct OngoingQuestEntry *entry = tree_find(&self->quests_b, &quest_id, tree_less_uint);

    if (entry)
        return &entry->quest;
    if (once_begin(&g_questEmptyOngoingGuard)) {
        quest_hunt_construct(&g_questEmptyOngoing);
        atexit(renew_quest_empty_ongoing_atexit);
        once_complete(&g_questEmptyOngoingGuard);
    }
    return &g_questEmptyOngoing;
}

struct RenewQuestA *renew_quest_recommended_info(struct RenewQuestMgr *self, unsigned quest_id)
{
    struct RecommendedQuestEntry *entry = tree_find(&self->quests_a, &quest_id, tree_less_uint);

    if (entry)
        return &entry->quest;
    if (once_begin(&g_questEmptyRecommendedGuard)) {
        renew_quest_a_construct(&g_questEmptyRecommended);
        atexit(renew_quest_empty_recommended_atexit);
        once_complete(&g_questEmptyRecommendedGuard);
    }
    return &g_questEmptyRecommended;
}

struct RenewQuestB *renew_quest_find_ongoing(struct RenewQuestMgr *self, unsigned quest_id)
{
    struct OngoingQuestEntry *entry = tree_find(&self->quests_b, &quest_id, tree_less_uint);

    return entry ? &entry->quest : 0;
}

struct RenewQuestA *renew_quest_find_recommended(struct RenewQuestMgr *self, unsigned quest_id)
{
    struct RecommendedQuestEntry *entry = tree_find(&self->quests_a, &quest_id, tree_less_uint);

    return entry ? &entry->quest : 0;
}

struct RenewQuestBase *renew_quest_find(struct RenewQuestMgr *self, char ongoing, unsigned quest_id)
{
    if (ongoing) {
        struct RenewQuestB *quest = renew_quest_find_ongoing(self, quest_id);

        return quest ? &quest->base : 0;
    } else {
        struct RenewQuestA *quest = renew_quest_find_recommended(self, quest_id);

        return quest ? &quest->base : 0;
    }
}

static void register_function(const char *name, lua_CFunction fn)
{
    lua_State *L = g_session1.luaState->L;

    lua_pushcclosure(L, fn, 0);
    lua_setfield(L, LUA_GLOBALSINDEX, name);
}

void renew_quest_load(void)
{
    struct ClientLua *lua;

    register_function("AddOngoingDescription", renew_quest_lua_add_ongoing_description);
    register_function("AddOngoingRewardInfo", renew_quest_lua_add_ongoing_reward_info);
    register_function("InsertRecommededQuestInfo", renew_quest_lua_insert_recommended_quest_info);
    register_function("AddRecommendedQuestInfo", renew_quest_lua_add_recommended_quest_info);
    register_function("SetRecommendedQuestActive", renew_quest_lua_set_recommended_quest_active);

    lua = g_session1.luaState;
    client_lua_load_file(lua, "Lua Files\\DataInfo\\QuestInfo_f", 1, 0);
    client_lua_load_file(g_session1.luaState, "savedata\\OngoingSimpleViewList", 0, 1);
    client_lua_load_file(g_session1.luaState, "savedata\\RecommendedActiveList", 0, 1);
    if (g_serverType == SERVICE_SAKRAY) {
        client_lua_load_file(g_session1.luaState, "system\\OngoingQuestInfoList_Sakray", 0, 1);
        client_lua_load_file(g_session1.luaState, "system\\RecommendedQuestInfoList_Sakray", 0, 1);
    } else {
        client_lua_load_file(g_session1.luaState, "system\\OngoingQuestInfoList_True", 0, 1);
        client_lua_load_file(g_session1.luaState, "system\\RecommendedQuestInfoList_True", 0, 1);
    }
    client_lua_call(g_session1.luaState, "RecommendedQuestInfoLoad", "");
    client_lua_call(g_session1.luaState, "RecommendedQuestActiveLoad", "");
}

static const struct {
    const char *icon;
    const char *tip;
} icon_tips[] = {
    { "ico_ep", "EPISODE QUEST" }, { "ico_dq", "DAILY QUEST" }, { "ico_lq", "LOCAL QUEST" },
    { "ico_jq", "JOB QUEST" },     { "ico_gq", "GUIDE QUEST" }, { "ico_ev", "EVENT QUEST" },
    { "ico_mq", "MAIN QUEST" },
};
#define ICON_TIP_DEFAULT "NORMAL QUEST"

static void assign_answer(struct String *s, const char *text)
{
    if (!text)
        text = "";
    string_assign_cstr(s, text);
}

static void fit_text(struct String *s, int bold)
{
    struct String fitted = {0};

    ui_window_fit_text(g_windowMgr.basic_info_wnd, &fitted, TITLE_WIDTH, string_cstr(s), 0,
                                           TITLE_FONT_HEIGHT, bold);
    string_assign(s, string_cstr(&fitted), fitted.size);
    string_free(&fitted);
}

static void ongoing_copy(struct RenewQuestB *dst, const struct RenewQuestB *src)
{
    renew_quest_base_copy(&dst->base, &src->base);
    dst->shown = src->shown;
    string_vec_copy(&dst->description, &src->description);
    dst->plain = src->plain;
    string_copy(&dst->reward_exp, &src->reward_exp);
    string_copy(&dst->reward_job_exp, &src->reward_job_exp);
    vec_copy(&dst->rewards, &src->rewards, sizeof(struct RenewQuestReward));
    dst->cool_time_quest = src->cool_time_quest;
    dst->cool_time_quest_high = src->cool_time_quest_high;
}

void renew_quest_read_ongoing(struct RenewQuestMgr *self, unsigned quest_id, struct RenewQuestB *info)
{
    char shown = 1;
    const char *title = 0, *icon = 0, *summary = 0, *npc_sprite = 0, *npc_map = 0;
    const char *reward_exp = 0, *reward_job_exp = 0;
    int npc_x = 0, npc_y = 0, kind = 0;
    struct OngoingQuestEntry entry;
    int added;
    size_t i;

    if (client_lua_call(g_session1.luaState, "GetOngoingQuestInfoByID", "d>bsssssssddd", quest_id, &shown, &title,
                &icon, &summary, &npc_sprite, &npc_map, &reward_exp, &reward_job_exp, &npc_x, &npc_y,
                &kind)) {
        const char *tip = ICON_TIP_DEFAULT;

        assign_answer(&info->base.text[RENEW_QUEST_TEXT_TITLE], title);
        assign_answer(&info->base.text[RENEW_QUEST_TEXT_ICON], icon);
        assign_answer(&info->base.text[RENEW_QUEST_TEXT_SUMMARY], summary);
        assign_answer(&info->base.text[RENEW_QUEST_TEXT_NPC_SPRITE], npc_sprite);
        assign_answer(&info->base.text[RENEW_QUEST_TEXT_NPC_MAP], npc_map);
        assign_answer(&info->reward_exp, reward_exp);
        assign_answer(&info->reward_job_exp, reward_job_exp);
        info->base.npc_x = npc_x;
        info->base.npc_y = npc_y;
        info->shown = shown;
        info->cool_time_quest = kind;
        info->cool_time_quest_high = kind < 0 ? -1 : 0;
        if (kind == 1)
            info->shown = 0;
        for (i = 0; i < sizeof icon_tips / sizeof icon_tips[0]; i++) {
            if (string_find(&info->base.text[RENEW_QUEST_TEXT_ICON], icon_tips[i].icon, 0) != STRING_NPOS) {
                tip = icon_tips[i].tip;
                break;
            }
        }
        string_assign_cstr(&info->base.text[RENEW_QUEST_TEXT_ICON_TIP], tip);
    }
    if (g_windowMgr.basic_info_wnd) {
        fit_text(&info->base.text[RENEW_QUEST_TEXT_TITLE], 1);
        fit_text(&info->base.text[RENEW_QUEST_TEXT_SUMMARY], 0);
    }

    tree_erase_key(&self->quests_b, &quest_id, tree_less_uint, ongoing_quest_entry_destroy);

    entry.id = quest_id;
    ongoing_copy(&entry.quest, info);
    tree_add(&self->quests_b, &entry, sizeof entry, tree_less_uint, &added);
    if (!added)
        quest_hunt_destroy(&entry.quest);

    client_lua_call(g_session1.luaState, "GetOngoingDescription", "d", quest_id);
    client_lua_call(g_session1.luaState, "GetOngoingRewardInfo", "d", quest_id);
}

void renew_quest_empty_ongoing_atexit(void)
{
    vec_free(&g_questEmptyOngoing.rewards);
    string_free(&g_questEmptyOngoing.reward_job_exp);
    string_free(&g_questEmptyOngoing.reward_exp);
    string_vec_free(&g_questEmptyOngoing.description);
    renew_quest_base_destruct(&g_questEmptyOngoing.base);
}

void renew_quest_empty_recommended_atexit(void)
{
    int i;

    for (i = 3; i-- > 0;)
        string_vec_free(&g_questEmptyRecommended.lists[i]);
    string_free(&g_questEmptyRecommended.image);
    renew_quest_base_destruct(&g_questEmptyRecommended.base);
}
