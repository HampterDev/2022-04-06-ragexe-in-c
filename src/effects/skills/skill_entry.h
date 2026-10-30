#ifndef EFFECTS_SKILLS_SKILL_ENTRY_H
#define EFFECTS_SKILLS_SKILL_ENTRY_H
#include <stdint.h>
struct SkillUnitEntry {
    int32_t aid;
    int32_t creator_aid;
    int32_t job;
    int32_t x, y;
    unsigned char visible;
    int32_t range;
    int32_t level;
};
#endif
