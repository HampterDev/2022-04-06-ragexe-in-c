#include "contents/client_lua.h"
#include "platform/client_config.h"
#include "session/state.h"
#include "session/csession.h"
#include "session/session_items.h"
#include "session/session_lua.h"
#include "session/session_names.h"
#include "session/session_options.h"

#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include "core/window_state.h"
#include "platform/language.h"
#include "platform/window.h"

int session_get_real_weapon_id(int weapon)
{
    int real = 0;
    if (client_lua_call(g_session1.luaState, "GetRealWeaponId", "d>d", weapon, &real))
        weapon = real;
    return weapon;
}

struct ShadowFactor {
    int job;
    float factor;
};

float session_get_npc_location_radius(struct Session *self, int job, BYTE mapJob)
{
    const struct ShadowFactor *found;
    int radius;

    if (mapJob == 1)
        job = session_get_mounted_job(job);
    radius = 0;
    client_lua_call(g_session1.luaState, "GetNpcLocationRadius", "d>d", job, &radius);
    if (radius != 0)
        return (float)(radius * 2 + 1);

    found = tree_find(&self->shadowFactors, &job, tree_less_int);
    if (!found)
        return 1.0f;
    return found->factor;
}

const char *session_get_weapon_name_by_class(int itemId)
{
    const char *name = "";
    int classNum = item_table_get(itemId)->class_num;

    if (!client_lua_call(g_session1.luaState, "ReqWeaponNameByClassNum", "d>s", classNum, &name))
        return "";
    return name;
}

static BYTE run_lua(struct Session *self, const char *file)
{
    return client_lua_load_file(self->luaState, file, 1, 0);
}

static BYTE run_saved(struct Session *self, const char *file)
{
    return client_lua_load_file(self->luaState, file, 0, 1);
}

static const char *const navi_files[3][9] = {
    {   "Lua Files\\Navigation\\Navi_f_krpri",           "Lua Files\\Navigation\\Navi_Map_krpri",
        "Lua Files\\Navigation\\Navi_Npc_krpri",         "Lua Files\\Navigation\\Navi_Mob_krpri",
        "Lua Files\\Navigation\\Navi_Link_krpri",        "Lua Files\\Navigation\\Navi_LinkDistance_krpri",
        "Lua Files\\Navigation\\Navi_NpcDistance_krpri", "Lua Files\\Navigation\\Navi_Scroll_krpri",
        "Lua Files\\Navigation\\navi_picknpc_krpri" },
    {   "Lua Files\\Navigation\\Navi_f_krSak",           "Lua Files\\Navigation\\Navi_Map_krSak",
        "Lua Files\\Navigation\\Navi_Npc_krSak",         "Lua Files\\Navigation\\Navi_Mob_krSak",
        "Lua Files\\Navigation\\Navi_Link_krSak",        "Lua Files\\Navigation\\Navi_LinkDistance_krSak",
        "Lua Files\\Navigation\\Navi_NpcDistance_krSak", "Lua Files\\Navigation\\Navi_Scroll_krSak",
        "Lua Files\\Navigation\\navi_picknpc_krSak" },
    {   "Lua Files\\Navigation\\Navi_f_krLoc",           "Lua Files\\Navigation\\Navi_Map_krLoc",
        "Lua Files\\Navigation\\Navi_Npc_krLoc",         "Lua Files\\Navigation\\Navi_Mob_krLoc",
        "Lua Files\\Navigation\\Navi_Link_krLoc",        "Lua Files\\Navigation\\Navi_LinkDistance_krLoc",
        "Lua Files\\Navigation\\Navi_npcDistance_krLoc", "Lua Files\\Navigation\\Navi_Scroll_krLoc",
        "Lua Files\\Navigation\\navi_picknpc_krLoc" },
};

unsigned char (*g_sessionNaviTables)(struct NaviManager *navi, void *lua);

static void session_load_navigation_lua(struct Session *self)
{
    const char *const *files = navi_files[g_serverType == 0 ? 0 : g_serverType == 1 ? 1 : 2];
    int i;

    if (!run_lua(self, files[0]))
        return;
    for (i = 1; i < 9; i++)
        run_lua(self, files[i]);
    if (g_sessionNaviTables)
        g_sessionNaviTables(&g_navi, self->luaState);
}

/* "querytransparentiteminfo // %d 읽기 실패". An array, not a literal format: the compiler
   would check a literal format in the CP949 execution charset. */
static const char transparent_item_failed[] =
    "querytransparentiteminfo // %d \xec\x9d\xbd\xea\xb8\xb0 \xec\x8b\xa4\xed\x8c\xa8";

void session_load_transparent_items(struct Session *self)
{
    int id;

    for (id = 1; id < 1000; id++) {
        struct Pulse entry;
        int key = 0, lo = 0, hi = 0, period = 0;

        if (!client_lua_call(self->luaState, "querytransparentiteminfo", "d>dddd",
                        id, &key, &lo, &hi, &period)) {
            char text[0x104];
            memset(text, 0, sizeof(text));
            sprintf(text, transparent_item_failed, id);
            platform_message_box(g_hMainWnd, text, "Error", 0);
            return;
        }
        if (key == 0)
            break;
        entry.item = key;
        entry.lo = lo;
        entry.hi = hi;
        entry.period = period;
        tree_add(&self->transparentItemPulses, &entry, sizeof entry, tree_less_int, NULL);
    }
}

void session_init_lua_files(struct Session *self)
{
    int hatEffects;
    if (!self->luaState) {
        self->luaState = malloc(sizeof *self->luaState);
        if (!self->luaState) return;
        client_lua_construct(self->luaState);
    }
    if (!self->luaState->L) client_lua_open(self->luaState);
    if (!self->luaState->L) return;
    run_lua(self, "Lua Files\\MsgString_KR");
    run_lua(self, "Lua Files\\Common");
    run_lua(self, "Lua Files\\wndInfo");
    run_lua(self, "Lua Files\\HotKey");
    run_lua(self, "Lua Files\\Emotion\\EmotionList_F");
    run_lua(self, "Lua Files\\Emotion\\EmotionList");
    run_lua(self, "Lua Files\\DataInfo\\NPCIdentity");
    run_lua(self, "Lua Files\\DataInfo\\jobName_F");
    run_lua(self, "Lua Files\\DataInfo\\jobName");
    run_lua(self, "Lua Files\\DataInfo\\PetInfo");
    run_lua(self, "Lua Files\\DataInfo\\AccessoryId");
    run_lua(self, "Lua Files\\DataInfo\\accName_F");
    run_lua(self, "Lua Files\\DataInfo\\accName");

    if (!client_lua_load_local_file(self->luaState, "SaveData\\ChatWndInfo_U.lua")
        || !client_lua_has_global_table(self->luaState, "ChatSubWnd_1"))
        run_lua(self, "Lua Files\\ChatWndInfo\\ChatWndInfo");
    run_lua(self, "Lua Files\\ChatWndInfo\\ChatWndInfo_F");
    self->canDragWnd = client_lua_get_global_int(self->luaState, "CanDragWnd") == 1;

    run_lua(self, "Lua Files\\SkillInfoz\\jobinheritlist");
    run_lua(self, "Lua Files\\SkillInfoz\\SkillID");
    run_lua(self, "Lua Files\\SkillInfoz\\SkillInfoList");
    run_lua(self, "Lua Files\\SkillInfoz\\SkillDescript");
    run_lua(self, "Lua Files\\SkillInfoz\\SkillInfo_F");
    run_lua(self, "Lua Files\\SkillInfoz\\skilltreeview");
    run_lua(self, "Lua Files\\StateIcon\\EFSTIDs");
    run_lua(self, "Lua Files\\StateIcon\\StateIconInfo");
    run_lua(self, "Lua Files\\StateIcon\\StateIconInfo_F");
    run_lua(self, "Lua Files\\StateIcon\\StateIconImgInfo");
    run_saved(self, "System\\LuaFiles514\\OptionInfo");
    run_lua(self, "Lua Files\\OptionInfo\\CmdInfo");
    run_lua(self, "Lua Files\\OptionInfo\\OptionInfo_F");
    run_saved(self, "SaveData\\OptionInfo");
    run_lua(self, "Lua Files\\DataInfo\\SpriteRobeId");
    run_lua(self, "Lua Files\\DataInfo\\SpriteRobeName_F");
    run_lua(self, "Lua Files\\DataInfo\\SpriteRobeName");
    run_lua(self, "Lua Files\\SprEditInfo\\2DLayerDir_F");
    run_lua(self, "Lua Files\\SprEditInfo\\BigLayerDir_male");
    run_lua(self, "Lua Files\\SprEditInfo\\BigLayerDir_female");
    run_lua(self, "Lua Files\\SprEditInfo\\SmallLayerDir_male");
    run_lua(self, "Lua Files\\SprEditInfo\\SmallLayerDir_female");
    if (run_lua(self, "Lua Files\\SprEditInfo\\_New_2DLayerDir_F")) {
        run_lua(self, "Lua Files\\SprEditInfo\\_New_BigLayerDir_male");
        run_lua(self, "Lua Files\\SprEditInfo\\_New_BigLayerDir_female");
        run_lua(self, "Lua Files\\SprEditInfo\\_New_SmallLayerDir_male");
        run_lua(self, "Lua Files\\SprEditInfo\\_New_SmallLayerDir_female");
        self->new_layer_table_loaded = 1;
    } else {
        platform_message_box(NULL, "NO!! 2D_LAYER_NEW_TABLE", "\xec\x95\x8c\xeb\xa6\xbc" /* "알림" */, 0);
    }
    run_lua(self, "Lua Files\\DataInfo\\NpcLocationRadius");
    run_lua(self, "Lua Files\\DataInfo\\NpcLocationRadius_F");
    run_lua(self, "Lua Files\\EffectTool\\effectmsgid");
    run_lua(self, "Lua Files\\EffectTool\\effectfunc");
    run_lua(self, "Lua Files\\EffectTool\\EffectToolUtil_ver_2");
    run_lua(self, "Lua Files\\SkillEffectInfo\\EffectID");
    run_lua(self, "Lua Files\\SkillEffectInfo\\ActorState");
    run_lua(self, "Lua Files\\SkillEffectInfo\\SkillEffectInfoList");
    run_lua(self, "Lua Files\\SkillEffectInfo\\SkillEffectInfo_F");
    run_lua(self, "Lua Files\\DataInfo\\KafraMoveMapServiceList");
    run_lua(self, "Lua Files\\DataInfo\\KafraMoveMapServiceList_F");
    session_load_navigation_lua(self);
    run_lua(self, "Lua Files\\DataInfo\\HelpMsgStr");
    run_lua(self, "Lua Files\\DataInfo\\WeaponTable");
    run_lua(self, "Lua Files\\DataInfo\\WeaponTable_F");
    run_lua(self, "Lua Files\\DataInfo\\JobIdentity");
    run_lua(self, "Lua Files\\DataInfo\\ShadowTable");
    run_lua(self, "Lua Files\\DataInfo\\ShadowTable_F");
    world_view_load(&g_worldView, self->luaState);
    run_lua(self, "Lua Files\\DataInfo\\EnumVAR");
    run_lua(self, "Lua Files\\DataInfo\\AddRandomOptionNameTable");
    run_lua(self, "Lua Files\\DataInfo\\AddRandomOption_F");
    run_lua(self, "Lua Files\\DressRoom\\Dress_f");
    run_lua(self, "Lua Files\\DressRoom\\jobDressList");
    run_lua(self, "Lua Files\\DataInfo\\TitleTable");
    run_lua(self, "Lua Files\\HatEffectInfo\\HatEffectInfo");

    hatEffects = 0;
    client_lua_call(self->luaState, "GetTableSizeForC", "s>d", "hatEffectTable", &hatEffects);

    run_lua(self, "Lua Files\\StylingShop\\StylingShopInfo_F");
    run_lua(self, "Lua Files\\StylingShop\\StylingShopInfo");
    run_lua(self, "Lua Files\\EffectTool\\ForceRenderEffect");
    run_lua(self, "Lua Files\\OffsetItemPos\\OffsetItemPos_F");
    run_lua(self, "Lua Files\\OffsetItemPos\\OffsetItemPos");
    run_lua(self, "Lua Files\\transparentItem\\transparentItem");
    run_lua(self, "Lua Files\\transparentItem\\transparentItem_f");

    session_load_transparent_items(self);

    session_load_external_settings(self);
    run_lua(self, "Lua Files\\DataInfo\\TB_Layer_Priority");
    run_lua(self, "Lua Files\\Agit\\AgitConfig");
    run_lua(self, "Lua Files\\Agit\\AgitConfig_F");
}

void session_load_shadow_factors(struct Session *self)
{
    int job;

    tree_clear(&self->shadowFactors, NULL);
    for (job = 0; job < 0x7d01; job++) {
        double factor = 0.0;
        if (client_lua_call(self->luaState, "ReqshadowFactor", "d>l", job, &factor)
            && factor >= 0.0) {
            struct ShadowFactor pair;
            pair.job = job;
            pair.factor = (float)factor;
            tree_add(&self->shadowFactors, &pair, sizeof pair, tree_less_int, NULL);
        }
    }
}

void session_load_weapon_hit_waves(struct Session *self)
{
    struct Vec *v = &self->weaponHitWaves;
    int weapon;

    string_vec_clear(v);
    vec_reserve(v, 0x69, sizeof(struct String));
    for (weapon = 0; weapon < 0x69; weapon++)
        string_vec_push_text(v, "_hit_mace.wav", 0xd);

    for (weapon = 0; weapon < 0x69; weapon++) {
        const char *name = NULL;
        if (client_lua_call(self->luaState, "ReqWeaponHitWaveName", "d>s", weapon, &name)
            && strcmp(name, "") != 0)
            string_assign_cstr((struct String *)v->first + weapon, name);
    }
}

static void table_int(struct Session *self, const char *table, const char *key, int *out)
{
    client_lua_call(self->luaState, "GetTableIntValueForC", "ss>d", table, key, out);
}

static void check(int bad)
{
    if (bad)
        error_msg("Error ExternSettings Value", 0);
}

void session_load_external_settings(struct Session *self)
{
    const char *file = g_serverType == 1 ? "Lua Files\\service_eu\\ExternalSettings_eu_sak"
                                         : "Lua Files\\service_eu\\ExternalSettings_eu";

    client_lua_load_file(self->luaState, file, 1, 0);

    table_int(self, "MaxLevelTable", "BaseLevel", &self->baseLevel);
    table_int(self, "MaxLevelTable", "BaseLevelHomun", &self->baseLevelHomun);
    table_int(self, "MaxLevelTable", "JobLevelNovice", &self->jobLevelNovice);
    table_int(self, "MaxLevelTable", "JobLevelSuperNovice", &self->jobLevelSuperNovice);
    table_int(self, "MaxLevelTable", "JobLevelBase", &self->jobLevelBase);
    table_int(self, "MaxLevelTable", "JobLevel2nd", &self->jobLevel2nd);
    check(self->baseLevel == -1 || self->baseLevelHomun == -1 || self->jobLevelNovice == -1
          || self->jobLevelSuperNovice == -1 || self->jobLevelBase == -1 || self->jobLevel2nd == -1);

    table_int(self, "Level99AuraTable", "Default99LvAura", &self->auras[0]);
    table_int(self, "Level99AuraTable", "Default99LvAura_sub", &self->auras[1]);
    table_int(self, "Level99AuraTable", "Baby99LvAura", &self->auras[2]);
    table_int(self, "Level99AuraTable", "Baby99LvAura_sub", &self->auras[3]);
    table_int(self, "Level99AuraTable", "SecondHigh99LvAura", &self->auras[4]);
    table_int(self, "Level99AuraTable", "SecondHigh99LvAura_sub", &self->auras[5]);
    table_int(self, "Level99AuraTable", "Homun99LvAura", &self->auras[6]);
    table_int(self, "Level99AuraTable", "Homun99LvAura_sub", &self->auras[7]);
    table_int(self, "Level99AuraTable", "Boss99LvAura_sub", &self->auras[8]);
    table_int(self, "MaxLevelAuraTable", "Default150LvAura", &self->auras[9]);
    table_int(self, "MaxLevelAuraTable", "Default150LvAura_sub", &self->auras[10]);
    table_int(self, "MaxLevelAuraTable", "HomunMaxLvAura", &self->auras[11]);
    table_int(self, "MaxLevelAuraTable", "HomunMaxLvAura_sub", &self->auras[12]);
}
