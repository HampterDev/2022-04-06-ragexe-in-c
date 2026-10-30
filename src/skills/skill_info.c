#include "std/vec.h"
#include "skills/skill_info.h"
#include "contents/client_lua_calls.h"
#include "session/state.h"
#include "skills/skill_bundle.h"
#include "skills/skill_action.h"
#include "contents/lua_script.h"

static const char k_unknownSkillName[] = "Unknown-Skill";
static const char k_zeroSkillIdName[] = "Zero Skill";
static int skill_name_id(int id)
{
    if (id == 0x13b2)
        return 0x13a4;
    if (id == 0x13b3)
        return 0x13ac;
    return id;
}

struct SkillInfo *skill_info_construct_unknown(struct SkillInfo *skill)
{
    skill->found = 0;
    skill->id = 0;
    skill->level = 0;
    skill->id_name = k_unknownSkillName;
    skill->level2 = 0;
    skill->ap_amount = 0;
    memset(&skill->requirements, 0, sizeof skill->requirements);
    return skill;
}

struct SkillInfo *skill_info_construct(struct SkillInfo *skill)
{
    skill->found = 1;
    skill->id = 0;
    skill->level = 0;
    skill->id_name = NULL;
    skill->level2 = 0;
    skill->ap_amount = 0;
    memset(&skill->requirements, 0, sizeof skill->requirements);
    return skill;
}

static void skill_copy_fields(struct SkillInfo *dst, const struct SkillInfo *src)
{
    dst->found = src->found;
    dst->id = src->id;
    dst->type = src->type;
    dst->level = src->level;
    dst->sp_cost = src->sp_cost;
    dst->upgradable = src->upgradable;
    dst->range = src->range;
    dst->id_name = src->id_name;
    dst->cell_index = src->cell_index;
    dst->max_level = src->max_level;
    dst->level_limit = src->level_limit;
    dst->level2 = src->level2;
    dst->ap_amount = src->ap_amount;
}

struct SkillInfo *skill_info_copy(struct SkillInfo *dst, const struct SkillInfo *src)
{
    skill_copy_fields(dst, src);
    vec_copy(&dst->requirements, &src->requirements, sizeof(struct SkillRequirement));
    return dst;
}

struct SkillInfo *skill_info_assign(struct SkillInfo *dst, const struct SkillInfo *src)
{
    if (dst == src)
        return dst;
    skill_copy_fields(dst, src);
    if (src->requirements.first == src->requirements.last)
        dst->requirements.last = dst->requirements.first;
    else
        vec_assign(&dst->requirements, src->requirements.first, src->requirements.last, sizeof(struct SkillRequirement));
    return dst;
}

void skill_info_destruct(struct SkillInfo *skill)
{
    vec_free(&skill->requirements);
}

void skill_info_destroy(void *skill)
{
    skill_info_destruct(skill);
}

int skill_info_get_ap_amount(struct SkillInfo *skill, int level)
{
    int amount = 0;

    client_lua_call(g_session1.luaState, "GetLevelUseApAmount", "dd>d", skill->id, level, &amount);
    return amount;
}

int skill_info_get_sp_amount(struct SkillInfo *skill, int level)
{
    int amount = 0;

    client_lua_call(g_session1.luaState, "GetLevelUseSpAmount", "dd>d", skill->id, level, &amount);
    return amount;
}

const char *skill_info_get_name(struct SkillInfo *skill)
{
    const char *name = NULL;
    int id = skill_name_id(skill->id);

    if (client_lua_call(g_session1.luaState, "GetSkillName", "d>s", id, &name) == 1)
        return name;
    return k_unknownSkillName;
}

const char *skill_info_get_id_name(struct SkillInfo *skill)
{
    const char *name = NULL;

    if (client_lua_call(g_session1.luaState, "GetSkillIdName", "d>s", skill->id, &name))
        return name;
    return k_zeroSkillIdName;
}

const char *skill_get_id_name(int skill_id)
{
    const char *name = NULL;

    if (client_lua_call(g_session1.luaState, "GetSkillIdName", "d>s", skill_id, &name))
        return name;
    return k_zeroSkillIdName;
}

const char *skill_name(int skill_id)
{
    const char *name = NULL;
    int id = skill_name_id(skill_id);

    if (client_lua_call(g_session1.luaState, "GetSkillName", "d>s", id, &name) == 1)
        return name;
    return k_unknownSkillName;
}

void skill_info_nop(void)
{
}

static unsigned char skill_id_listed(int id, const int (*ranges)[2], size_t count)
{
    size_t i;

    for (i = 0; i < count; i++)
        if (id >= ranges[i][0] && id <= ranges[i][1])
            return 1;
    return 0;
}

static const int harmful_skill_ranges[][2] = {
    {0x1c, 0x1e},   {0x22, 0x23},   {0x35, 0x36},   {0x42, 0x44},   {0x47, 0x49},   {0x4d, 0x4e},
    {0x8a, 0x8b},   {0xe7, 0xe7},   {0xea, 0xed},   {0xff, 0xff},   {0x118, 0x11b}, {0x11e, 0x11e},
    {0x121, 0x121}, {0x176, 0x176}, {0x192, 0x192}, {0x1cf, 0x1d8}, {0x1df, 0x1df}, {0x3f7, 0x3f7},
    {0x7d1, 0x7d1}, {0x7e4, 0x7e4}, {0x7ea, 0x7eb}, {0x802, 0x805}, {0x925, 0x925}, {0x928, 0x92c},
    {0x993, 0x993}, {0x9d3, 0x9d3}, {0x201d, 0x201d},
};

unsigned char skill_info_may_harm(int skill_id)
{
    return skill_id_listed(skill_id, harmful_skill_ranges, sizeof harmful_skill_ranges / sizeof harmful_skill_ranges[0]);
}

static const int skill_list_good[][2] = {
    {0x4, 0x4},       {0x1c, 0x1d},     {0x22, 0x23},     {0x35, 0x36},     {0x42, 0x44},
    {0x47, 0x49},     {0x6c, 0x6c},     {0x8a, 0x8b},     {0xe7, 0xe7},     {0xea, 0xed},
    {0xff, 0x100},    {0x106, 0x106},   {0x118, 0x11b},   {0x169, 0x169},   {0x1bd, 0x1ca},
    {0x1cc, 0x1d1},   {0x1df, 0x1df},   {0x1ee, 0x1ee},   {0x234, 0x234},   {0x23c, 0x23f},
    {0x241, 0x241},   {0x251, 0x251},   {0x26d, 0x26e},   {0x272, 0x272},   {0x27c, 0x27d},
    {0x281, 0x283},   {0x285, 0x286},   {0x2b0, 0x2b3},   {0x2b6, 0x2b6},   {0x2d9, 0x2d9},
    {0x3f7, 0x3f7},   {0x7d1, 0x7d1},   {0x7dc, 0x7dc},   {0x7ea, 0x7ea},   {0x7f9, 0x7fb},
    {0x802, 0x805},   {0x8e3, 0x8e3},   {0x8ef, 0x8ef},   {0x912, 0x912},   {0x925, 0x925},
    {0x929, 0x929},   {0x92b, 0x92c},   {0x94f, 0x94f},   {0x975, 0x975},   {0x979, 0x979},
    {0x993, 0x993},   {0x9b0, 0x9b0},   {0x9b2, 0x9b2},   {0x9d3, 0x9d3},   {0x9e9, 0x9e9},
    {0xa24, 0xa27},   {0xa30, 0xa30},   {0xa34, 0xa34},   {0xbd1, 0xbd3},   {0xbd7, 0xbda},
    {0x13ab, 0x13ab}, {0x13ae, 0x13b1}, {0x13c8, 0x13c8}, {0x1494, 0x1495}, {0x1497, 0x1498},
    {0x149b, 0x149b},
    {0x14a0, 0x14a2}, {0x14b2, 0x14b3}, {0x14db, 0x14db}, {0x14f6, 0x14f6}, {0x14f8, 0x14f8},
    {0x1f55, 0x1f55}, {0x2022, 0x2027}, {0x202e, 0x2030},
};

unsigned char skill_info_is_good_skill_for_actor(int skill_id)
{
    if (skill_id_listed(skill_id, skill_list_good, sizeof skill_list_good / sizeof skill_list_good[0]))
        return 1;
    return skill_action_mgr_is_good_for_actor(skill_action_mgr_instance(), skill_id);
}

unsigned char skill_info_is_level_used(int skill_id)
{
    unsigned char uses = 0;

    if (client_lua_call(g_session1.luaState, "IsLevelUseSkill", "d>b", skill_id, &uses))
        return uses;
    return 0;
}

static const int no_use_effect_ranges[][2] = {
    {0xb, 0x15},   {0x1b, 0x1b},   {0x1d, 0x1e},   {0x20, 0x21},   {0x4d, 0x4d},   {0x56, 0x56},
    {0x154, 0x154}, {0x2b2, 0x2b2}, {0x2db, 0x2db}, {0x7d8, 0x7d8}, {0x9e5, 0x9e6}, {0x9e8, 0x9e8},
    {0x138c, 0x138c},
};

unsigned char skill_info_shows_use_effect(int skill_id)
{
    return !skill_id_listed(skill_id, no_use_effect_ranges, sizeof no_use_effect_ranges / sizeof no_use_effect_ranges[0]);
}
