#ifndef SKILLS_SKILL_BUNDLE_TYPES_H
#define SKILLS_SKILL_BUNDLE_TYPES_H

#include "skills/skill_info.h"
#include "std/list.h"
#include "std/container_types.h"

struct SkillLevelChange {
    int level_before;
    int level_after;
};

struct PlayerSkillBundle {
    struct List skills;
    struct Tree tree_tabs;
    struct List extra_skills;
    struct Vec extra_skill_ids;
    struct Tree level_changes;
    struct Tree level_change_flags;
    struct Vec level_change_drops;
    struct Vec chosen_levels;
    struct List autorun_skills;
};

#endif
