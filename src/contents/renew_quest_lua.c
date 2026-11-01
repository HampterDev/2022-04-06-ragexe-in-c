#include "core/text_markup.h"
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
