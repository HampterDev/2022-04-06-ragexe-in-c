#ifndef SKILLS_SKILL_INFO_H
#define SKILLS_SKILL_INFO_H

#include "base.h"

struct SkillRequirement {
    int skill_id;
    int level;
};
SIZE_ASSERT(SkillRequirement, 8);

struct SkillInfo {
    int found;
    int id;
    int type;
    int level;
    int sp_cost;
    int upgradable;
    int range;
    const char *id_name;
    int cell_index;
    int max_level;
    int level_limit;
    unsigned short level2;
    int ap_amount;
    struct Vec requirements;
};

struct SkillInfo *skill_info_construct(struct SkillInfo *skill);
struct SkillInfo *skill_info_construct_unknown(struct SkillInfo *skill);
struct SkillInfo *skill_info_copy(struct SkillInfo *dst, const struct SkillInfo *src);
struct SkillInfo *skill_info_assign(struct SkillInfo *dst, const struct SkillInfo *src);
void skill_info_destruct(struct SkillInfo *skill);
void skill_info_destroy(void *skill);
const char *skill_info_get_id_name(struct SkillInfo *skill);
const char *skill_info_get_name(struct SkillInfo *skill);
int skill_info_get_sp_amount(struct SkillInfo *skill, int level);
int skill_info_get_ap_amount(struct SkillInfo *skill, int level);
const char *skill_get_id_name(int skill_id);
const char *skill_name(int skill_id);
unsigned char skill_info_may_harm(int skill_id);
unsigned char skill_info_is_good_skill_for_actor(int skill_id);
unsigned char skill_info_shows_use_effect(int skill_id);
unsigned char skill_info_is_level_used(int skill_id);

#endif
