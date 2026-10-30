#include "core/text_markup.h"
#include <string.h>
#include "contents/renew_quest.h"
#include <lauxlib.h>
#include "ui/ui_std.h"

static unsigned check_unsigned(lua_State *L, int arg)
{
    return (unsigned)luaL_checknumber(L, arg);
}

static void assign_arg(struct String *s, lua_State *L, int arg)
{
    const char *text = luaL_checklstring(L, arg, NULL);

    string_assign_cstr(s, text);
}

int renew_quest_lua_add_ongoing_description(lua_State *L)
{
    unsigned quest_id = check_unsigned(L, 1);
    struct String line;
    struct RenewQuestB *rec;

    string_init(&line);
    assign_arg(&line, L, 2);
    line.size = text_unescape(line.data, line.size);
    rec = renew_quest_b_at(&g_questTable->quests_b, &quest_id);
    string_vec_push_copy(&rec->description, &line);
    string_free(&line);
    return 0;
}

int renew_quest_lua_add_ongoing_reward_info(lua_State *L)
{
    unsigned quest_id = check_unsigned(L, 1);
    struct RenewQuestReward reward;
    struct RenewQuestB *rec;

    reward.item_id = check_unsigned(L, 2);
    reward.count = check_unsigned(L, 3);
    rec = renew_quest_b_at(&g_questTable->quests_b, &quest_id);
    vec_push(&rec->rewards, &reward, sizeof reward);
    return 0;
}

int renew_quest_lua_add_recommended_quest_info(lua_State *L)
{
    unsigned quest_id = check_unsigned(L, 1);
    lua_Number box = luaL_checknumber(L, 2);
    struct String line;
    struct RenewQuestA *rec;

    string_init(&line);
    assign_arg(&line, L, 3);
    line.size = text_unescape(line.data, line.size);
    rec = renew_quest_a_at(&g_questTable->quests_a, quest_id);
    string_vec_push_copy(&rec->lists[(int)box], &line);
    string_free(&line);
    return 0;
}

static const struct {
    const char *icon;
    const char *tip;
} icon_tips[] = {
    { "ico_ep", "EPISODE QUEST" },
    { "ico_dq", "DAILY QUEST" },
    { "ico_lq", "LOCAL QUEST" },
    { "ico_jq", "JOB QUEST" },
    { "ico_gq", "GUIDE QUEST" },
    { "ico_ev", "EVENT QUEST" },
    { "ico_mq", "MAIN QUEST" },
};

int renew_quest_lua_insert_recommended_quest_info(lua_State *L)
{
    struct RenewQuestA rec;
    unsigned quest_id;
    const char *tip = "NORMAL QUEST";
    size_t i;

    renew_quest_a_construct(&rec);
    quest_id = check_unsigned(L, 1);
    rec.base.active = 1;
    assign_arg(&rec.base.text[RENEW_QUEST_TITLE], L, 2);
    assign_arg(&rec.base.text[RENEW_QUEST_ICON], L, 3);
    assign_arg(&rec.base.text[RENEW_QUEST_SUMMARY], L, 4);
    assign_arg(&rec.image, L, 5);
    assign_arg(&rec.base.text[RENEW_QUEST_NPC_SPRITE], L, 6);
    assign_arg(&rec.base.text[RENEW_QUEST_NPC_MAP], L, 7);
    rec.base.npc_x = (int)luaL_checknumber(L, 8);
    rec.base.npc_y = (int)luaL_checknumber(L, 9);
    for (i = 0; i < sizeof icon_tips / sizeof icon_tips[0]; i++) {
        if (string_find(&rec.base.text[RENEW_QUEST_ICON], icon_tips[i].icon, 0) != STRING_NPOS) {
            tip = icon_tips[i].tip;
            break;
        }
    }
    string_assign_cstr(&rec.base.text[RENEW_QUEST_ICON_TIP], tip);
    renew_quest_put_recommended(g_questTable, quest_id, &rec);
    renew_quest_a_destruct(&rec);
    return 0;
}

int renew_quest_lua_set_recommended_quest_active(lua_State *L)
{
    unsigned quest_id = check_unsigned(L, 1);
    struct RecommendedQuestEntry *entry = tree_find(&g_questTable->quests_a, &quest_id, tree_less_uint);

    if (entry)
        entry->quest.base.active = 0;
    return 0;
}
