#ifndef SESSION_SKILL_QUERIES_H
#define SESSION_SKILL_QUERIES_H

#include "skills/skill_info.h"
struct Session;

struct SkillInfo *session_get_skill_info(struct SkillInfo *out, int id);
struct SkillInfo *session_get_homun_skill_by_id(struct Session *session, struct SkillInfo *out, int id);
struct SkillInfo *session_get_mer_skill_by_id(struct Session *session, struct SkillInfo *out, int id);
struct SkillInfo *session_find_skill_info(struct Session *session, int id);

#endif
