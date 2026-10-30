#include <string.h>
#include "contents/achievement_internal.h"
#include "contents/lua_data.h"
#include "core/text_util.h"

static struct LuaDataResult *fail(struct LuaDataResult *out, const char *format, int id)
{
    struct String text;

    string_init(&text);
    string_format(&text, format, id);
    out->ok = 0;
    string_copy(&out->message, &text);
    string_free(&text);
    return out;
}

static struct LuaDataResult *insert_info(struct AchievementMgr *self, struct LuaDataResult *out, int id,
                                         const char *text0, const char *text1, const char *text2,
                                         int info_number)
{
    struct AchievementInfo info;
    char inserted;

    if (tree_find(&self->achievements, &id, tree_less_int))
        return fail(out, "duplicate achievement info [%d]", id);
    achievement_info_init(&info);
    string_init(&info.text[0]);
    string_assign_cstr(&info.text[0], text0);
    string_init(&info.text[1]);
    string_assign_cstr(&info.text[1], text1);
    string_init(&info.text[2]);
    string_assign_cstr(&info.text[2], text2);
    info.info_number = info_number;
    inserted = achievement_add(&self->achievements, id, &info);
    achievement_info_destruct(&info);
    if (inserted)
        return lua_data_result(out, 1, "good");
    return fail(out, "achievement info [%d] insert fail", id);
}

static struct LuaDataResult *insert_resource(struct AchievementMgr *self, struct LuaDataResult *out, int id,
                                             const char *text, int target, int goto_id)
{
    struct AchievementInfoEntry *entry = tree_find(&self->achievements, &id, tree_less_int);
    struct Vec *resources;
    const struct AchievementResource *have;
    struct AchievementResource added;

    if (!entry)
        return fail(out, "achievement info [%d] cannot be find", id);
    resources = &entry->info.resources;
    for (have = resources->first; have != resources->last; have++)
        if (string_equal_cstr(&have->text, text))
            return fail(out, "achievement info [%d] - resource info already exist", id);

    string_init(&added.text);
    string_assign_cstr(&added.text, text);
    added.target = target;
    added.goto_id = goto_id;
    vec_push(resources, &added, sizeof added);
    return lua_data_result(out, 1, "good");
}

enum AchievementReward { ACHIEVEMENT_REWARD_ITEM, ACHIEVEMENT_REWARD_TITLE, ACHIEVEMENT_REWARD_BUFF };

static struct LuaDataResult *set_reward(struct AchievementMgr *self, struct LuaDataResult *out, int id,
                                        enum AchievementReward reward, int value)
{
    struct AchievementInfoEntry *entry = tree_find(&self->achievements, &id, tree_less_int);

    if (!entry)
        return fail(out, "achievement info [%d] cannot be find", id);
    switch (reward) {
    case ACHIEVEMENT_REWARD_ITEM: entry->info.reward_item = value; break;
    case ACHIEVEMENT_REWARD_TITLE: entry->info.reward_title = value; break;
    case ACHIEVEMENT_REWARD_BUFF: entry->info.reward_buff = value; break;
    }
    return lua_data_result(out, 1, "good");
}

static struct LuaDataResult *set_ui_type(struct AchievementMgr *self, struct LuaDataResult *out, int id,
                                         int ui_type, const char *text)
{
    struct AchievementInfoEntry *entry = tree_find(&self->achievements, &id, tree_less_int);

    if (!entry)
        return fail(out, "achievement info [%d] cannot be find", id);
    entry->info.ui_type = ui_type;
    string_assign_cstr(&entry->info.ui_text, text);
    return lua_data_result(out, 1, "good");
}

static struct LuaDataResult *set_tab(struct AchievementMgr *self, struct LuaDataResult *out, int id,
                                     int major, int minor)
{
    struct Vec *ids = achievement_tab_at(&self->tabs, self->tab_stride * major + minor);
    const int *have;

    for (have = ids->first; have != ids->last; have++)
        if (*have == id)
            return fail(out, "duplicate achievement tab number info [%d]", id);
    vec_push(ids, &id, sizeof id);
    return lua_data_result(out, 1, "good");
}

static int lua_insert_info(lua_State *L)
{
    struct LuaDataResult result;
    const char *text0, *text1, *text2;
    int id, info_number;

    if (!lua_isnumber(L, -5))
        return lua_data_reply(L, 0, "argment 1nd is must number");
    if (!lua_isstring(L, -4))
        return lua_data_reply(L, 0, "argment 2th is must string");
    if (!lua_isstring(L, -3))
        return lua_data_reply(L, 0, "argment 3th is must string");
    if (!lua_isstring(L, -2))
        return lua_data_reply(L, 0, "argment 4th is must string");
    if (!lua_isnumber(L, -1))
        return lua_data_reply(L, 0, "argment 5th is must number");
    id = (int)lua_tonumber(L, -5);
    text0 = lua_tolstring(L, -4, 0);
    text1 = lua_tolstring(L, -3, 0);
    text2 = lua_tolstring(L, -2, 0);
    info_number = (int)lua_tonumber(L, -1);
    insert_info(g_achievementDb, &result, id, text0, text1, text2, info_number);
    return lua_data_reply_result(L, &result, "InsertAchieveInfo : %s");
}

static int lua_insert_resource(lua_State *L)
{
    struct LuaDataResult result;
    const char *text;
    int id, target, goto_id;

    if (!lua_isnumber(L, -4))
        return lua_data_reply(L, 0, "argment 1th is must number");
    if (!lua_isstring(L, -3))
        return lua_data_reply(L, 0, "argment 2th is must string");
    if (!lua_isnumber(L, -2))
        return lua_data_reply(L, 0, "argment 3th is must number");
    if (!lua_isnumber(L, -1))
        return lua_data_reply(L, 0, "argment 4th is must number");
    id = (int)lua_tonumber(L, -4);
    text = lua_tolstring(L, -3, 0);
    target = (int)lua_tonumber(L, -2);
    goto_id = (int)lua_tonumber(L, -1);
    insert_resource(g_achievementDb, &result, id, text, target, goto_id);
    return lua_data_reply_result(L, &result, "InsertAchieveResource : %s");
}

static int lua_set_reward(lua_State *L, enum AchievementReward reward, const char *fail_format)
{
    struct LuaDataResult result;
    int id, value;

    if (!lua_isnumber(L, -2))
        return lua_data_reply(L, 0, "argment 1th is must number");
    if (!lua_isnumber(L, -1))
        return lua_data_reply(L, 0, "argment 2th is must number");
    id = (int)lua_tonumber(L, -2);
    value = (int)lua_tonumber(L, -1);
    set_reward(g_achievementDb, &result, id, reward, value);
    return lua_data_reply_result(L, &result, fail_format);
}

static int lua_insert_reward_item(lua_State *L)
{
    return lua_set_reward(L, ACHIEVEMENT_REWARD_ITEM, "InsertAchieveRewardItem : %s");
}

static int lua_insert_reward_title(lua_State *L)
{
    return lua_set_reward(L, ACHIEVEMENT_REWARD_TITLE, "InsertAchieveRewardTitle : %s");
}

static int lua_insert_reward_buff(lua_State *L)
{
    return lua_set_reward(L, ACHIEVEMENT_REWARD_BUFF, "InsertAchieveRewardBuff : %s");
}

static int lua_insert_ui_type(lua_State *L)
{
    struct LuaDataResult result;
    const char *text;
    int id, ui_type;

    if (!lua_isnumber(L, -3))
        return lua_data_reply(L, 0, "argment 1th is must number");
    if (!lua_isnumber(L, -2))
        return lua_data_reply(L, 0, "argment 2th is must number");
    if (!lua_isstring(L, -1))
        return lua_data_reply(L, 0, "argment 3th is must string");
    id = (int)lua_tonumber(L, -3);
    ui_type = (int)lua_tonumber(L, -2);
    text = lua_tolstring(L, -1, 0);
    set_ui_type(g_achievementDb, &result, id, ui_type, text);
    return lua_data_reply_result(L, &result, "InsertAchieveUIType : %s");
}

static int lua_set_id_by_tab(lua_State *L)
{
    struct LuaDataResult result;
    int id, major, minor;

    if (!lua_isnumber(L, -3))
        return lua_data_reply(L, 0, "argment 1nd is must number");
    if (!lua_isnumber(L, -2))
        return lua_data_reply(L, 0, "argment 2th is must number");
    if (!lua_isnumber(L, -1))
        return lua_data_reply(L, 0, "argment 3th is must number");
    id = (int)lua_tonumber(L, -3);
    major = (int)lua_tonumber(L, -2);
    minor = (int)lua_tonumber(L, -1);
    set_tab(g_achievementDb, &result, id, major, minor);
    return lua_data_reply_result(L, &result, "SetAchieveTab : %s");
}

static const struct LuaDataFunction achievement_functions[] = {
    { "InsertAchieveInfo", lua_insert_info },
    { "InsertAchieveResource", lua_insert_resource },
    { "InsertAchieveRewardItem", lua_insert_reward_item },
    { "InsertAchieveRewardTitle", lua_insert_reward_title },
    { "InsertAchieveRewardBuff", lua_insert_reward_buff },
    { "InsertAchieveUIType", lua_insert_ui_type },
    { "SetAchieveIDByTab", lua_set_id_by_tab },
};

char achievement_load(void)
{
    return lua_data_run("system\\Achievement_list.lub", "CAchievementMgr", achievement_functions,
                        sizeof achievement_functions / sizeof achievement_functions[0],
                        LUA_DATA_ERROR_FORMAT);
}
