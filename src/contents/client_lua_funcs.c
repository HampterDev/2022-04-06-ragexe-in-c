#include "skills/skill_bundle.h"
#include "session/skill_queries.h"
#include "skills/skill_info.h"
#include "session/state.h"
#include "session/chat_tabs_lua.h"
#include "session/talk_options_lua.h"
#include "world/mode_manager.h"
#include "game/context.h"
#include "ui/window_access.h"
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "contents/client_lua.h"
#include "contents/client_res.h"
#include "contents/game_session.h"
#include "contents/lua_data.h"
#include "std/std.h"
#include "contents/ui_window_mgr.h"
#include <lauxlib.h>

#include "ui/worldmap/kafra_services.h"
#include "contents/lapine_box.h"
#include "contents/lapine_upgrade.h"
#include "core/msg_str.h"

#define LAPINE_BOX_WINDOW       0x122
#define LAPINE_UPGRADE_WINDOW   0x12e

int lua_trace_value(lua_State *L);
int lua_get_msg_string(lua_State *L);
int lua_req_emotion(lua_State *L);
int lua_add_skill_list(lua_State *L);
int lua_add_need_skill_list(lua_State *L);
int lua_rgb(lua_State *L);
int lua_add_ingredient(lua_State *L);
int lua_add_ingredient_lapine_upgrade_box(lua_State *L);

static const struct LuaDataFunction script_functions[] = {
    { "TraceValue", lua_trace_value },
    { "c_GetMsgString", lua_get_msg_string },
    { "SetSubChatWndList", lua_set_sub_chat_wnd_list },
    { "SetSubChatWndOption", lua_set_sub_chat_wnd_option },
    { "c_ReqEmotion", lua_req_emotion },
    { "c_SetCmdOnOffList", lua_set_cmd_on_off_list },
    { "c_GetCmdOnOffValue", lua_get_cmd_on_off_value },
    { "c_AddSkillList", lua_add_skill_list },
    { "c_AddNeedSkillList", lua_add_need_skill_list },
    { "RGB", lua_rgb },
    { "SetEmitterInfo", effect_lua_set_emitter_info },
    { "SetMagicFloorInfo", effect_lua_set_magic_floor_info },
    { "SetEvilsPawInfo", effect_lua_set_evils_paw_info },
    { "SetOtherEffectInfo", effect_lua_set_other_effect_info },
    { "SetAnimatedEmitterInfo", effect_lua_set_animated_emitter_info },
    { "SetEZ2STRInfo", effect_lua_set_ez2str_info },
    { "SetWindEffectInfo", effect_lua_set_wind_effect_info },
};
static const struct LuaDataFunction late_script_functions[] = {
    { "add_ingredient", lua_add_ingredient },
    { "add_ingredient_lapine_upgrade_box", lua_add_ingredient_lapine_upgrade_box },
};

static void register_all(lua_State *L, const struct LuaDataFunction *functions, size_t count)
{
    size_t i;

    for (i = 0; i < count; i++) {
        lua_pushcclosure(L, functions[i].function, 0);
        lua_setfield(L, LUA_GLOBALSINDEX, functions[i].name);
    }
}

void client_lua_register_functions(struct ClientLua *self)
{
    register_all(self->L, script_functions, sizeof script_functions / sizeof *script_functions);
    map_list_register_lua_functions(self->L);
    register_all(self->L, late_script_functions, sizeof late_script_functions / sizeof *late_script_functions);
}

static void check_string(struct String *out, lua_State *L, int arg)
{
    const char *text = luaL_checklstring(L, arg, NULL);

    string_init(out);
    string_assign_cstr(out, text);
}

int lua_trace_value(lua_State *L)
{
    struct String text;

    check_string(&text, L, 1);
    MessageBoxA(0, string_cstr(&text), "Lua error", 0);
    string_free(&text);
    return 1;
}

int lua_get_msg_string(lua_State *L)
{
    int id = (int)luaL_checknumber(L, 1);
    struct String text;
    const char *msg;

    string_init(&text);
    if (id >= 0) {
        msg = msg_str(id);
        string_assign_cstr(&text, msg);
    }
    lua_pushstring(L, string_cstr(&text));
    string_free(&text);
    return 1;
}

int lua_req_emotion(lua_State *L)
{
    int emotion = (int)luaL_checknumber(L, 1);
    int pick;

    /* Legacy Lua catalogs may still register Yut command aliases. */
    if (emotion >= 0x51 && emotion <= 0x57)
        emotion = -1;
    if (emotion >= 0) {
        if (emotion == 0x3a) {
            pick = rand() % 6;
            emotion = 0x3a + (pick < 0 ? 0 : pick);
        }
        MODE_SEND_MSG((void *)client_current_mode(), 0x1f, emotion, 0, 0, 0);
    }
    lua_pushnumber(L, (double)emotion);
    return 1;
}

int lua_add_skill_list(lua_State *L)
{
    struct SkillInfo info;
    double key, skill_id, c;
    const char *name;
    int a, b;

    skill_info_construct_unknown(&info);
    key = luaL_checknumber(L, 1);
    skill_id = luaL_checknumber(L, 2);
    name = luaL_checklstring(L, 3, NULL);
    a = (int)luaL_checknumber(L, 4);
    b = (int)luaL_checknumber(L, 5);
    c = luaL_checknumber(L, 6);
    if (strcmp(name, "") != 0 && a != -1 && b != -1) {
        info.level = 0;
        info.sp_cost = 0;
        info.range = 0;
        info.upgradable = 0;
        info.found = 1;
        info.id_name = name;
        info.cell_index = a;
        info.max_level = b;
        info.id = (int)skill_id;
        info.level_limit = (int)c;
        info.ap_amount = 0;
        player_skill_bundle_add_tree_skill(&g_session1.skills, (int)key, &info);
    }
    skill_info_destruct(&info);
    return 1;
}

int lua_add_need_skill_list(lua_State *L)
{
    int skill_id = (int)luaL_checknumber(L, 1);
    int a = (int)luaL_checknumber(L, 2);
    int b = (int)luaL_checknumber(L, 3);
    struct SkillInfo *skill;
    struct SkillRequirement need;
    struct Vec *needs;

    if (skill_id == -1 || a == -1 || b == -1)
        return 1;
    skill = session_find_skill_info(&g_session1, skill_id);
    if (!skill || !skill->found)
        return 1;
    needs = &skill->requirements;
    need.skill_id = a;
    need.level = b;
    vec_push(needs, &need, sizeof need);
    return 1;
}

int lua_rgb(lua_State *L)
{
    double r = luaL_checknumber(L, 1);
    int g = (int)luaL_checknumber(L, 2);
    int b = (int)luaL_checknumber(L, 3);
    unsigned color = ((unsigned)(b & 0xff) << 16) | ((unsigned)(g & 0xff) << 8) | ((unsigned)(int)r & 0xff);

    lua_pushnumber(L, (double)color);
    return 1;
}

int lua_add_ingredient(lua_State *L)
{
    int a = (int)luaL_checknumber(L, 1);
    int b = (int)luaL_checknumber(L, 2);
    void *wnd = ui_window_find(LAPINE_BOX_WINDOW);

    if (wnd)
        lapine_box_add_ingredient(wnd, a, b);
    return 0;
}

int lua_add_ingredient_lapine_upgrade_box(lua_State *L)
{
    int a = (int)luaL_checknumber(L, 1);
    void *wnd = ui_window_find(LAPINE_UPGRADE_WINDOW);

    if (wnd)
        lapine_upgrade_add_resource(wnd, a);
    return 0;
}
