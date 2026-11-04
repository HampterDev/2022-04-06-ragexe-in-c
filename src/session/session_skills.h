#ifndef SESSION_SESSION_SKILLS_H
#define SESSION_SESSION_SKILLS_H

#include "session/skill_commands.h"
#include "skills/skill_info.h"
#include "session/skill_queries.h"
#include "session/csession.h"

struct GameActor;

BYTE session_put_homun_skill(struct Session *self, const struct SkillInfo *skill, BYTE notify);
BYTE session_put_mer_skill(struct Session *self, const struct SkillInfo *skill, BYTE notify);
BYTE session_put_skill4(struct Session *self, const struct SkillInfo *skill);
BYTE session_put_skill(struct Session *self, const struct SkillInfo *skill, BYTE notify,
                                  BYTE keepType);
BYTE session_put_skill_keep_type(struct Session *self, const struct SkillInfo *skill, BYTE notify);
struct SkillInfo *session_get_skill_at(struct Session *self, struct SkillInfo *out, int index);
struct SkillInfo *session_get_skill4_by_id(struct Session *self, struct SkillInfo *out, int id);
struct SkillInfo *session_get_homun_skill_at(struct Session *self, struct SkillInfo *out, int index);
struct SkillInfo *session_get_mer_skill_at(struct Session *self, struct SkillInfo *out, int index);
int session_get_skill_level(int id);
int session_get_begin_effect_id(int id);

void session_get_cast_effect(struct Session *self, int skill, int *effect, int *action, int level,
                                       int job);

void session_get_use_effect(struct Session *self, int skill, int *effect, int *action);

const char *session_get_efst_image(struct Session *self, int efst, int pass);

BYTE effect_state_bit0(DWORD state);
BYTE effect_state_bit0_changed(DWORD a, DWORD b);
BYTE is_effect_state_hiding(DWORD state);
BYTE is_effect_state_cloaking(DWORD state);
BYTE effect_state_none(DWORD state);
void effect_state_set_bit6(DWORD *state);
void effect_state_clear_bit6(DWORD *state);
BYTE effect_state_bit1_changed(DWORD a, DWORD b);
BYTE effect_state_bit2_changed(DWORD a, DWORD b);
BYTE effect_state_bit4(DWORD state);
BYTE effect_state_bit4_changed(DWORD a, DWORD b);
BYTE effect_state_bit5(DWORD state);
BYTE effect_state_bit5_changed(DWORD a, DWORD b);
BYTE effect_state_bit6(DWORD state);
BYTE effect_state_bit6_changed(DWORD a, DWORD b);
BYTE effect_state_bit11(DWORD state);
BYTE effect_state_bit12(DWORD state);
BYTE effect_state_bit13(DWORD state);
BYTE effect_state_bit13_changed(DWORD a, DWORD b);
BYTE effect_state_bit14(DWORD state);
BYTE effect_state_bit15(DWORD state);
BYTE effect_state_bit15_changed(DWORD a, DWORD b);
BYTE effect_state_bit16(DWORD state);
BYTE effect_state_bit18(DWORD state);
BYTE effect_state_bit19(DWORD state);
BYTE effect_state_bit19_changed(DWORD a, DWORD b);
BYTE effect_state_bit20(DWORD state);
BYTE effect_state_bit20_changed(DWORD a, DWORD b);
BYTE effect_state_bit21(DWORD state);
BYTE effect_state_bit21_changed(DWORD a, DWORD b);
BYTE effect_state_bit22(DWORD state);
BYTE effect_state_bit23(DWORD state);
BYTE effect_state_bit23_changed(DWORD a, DWORD b);
BYTE effect_state_bit24(DWORD state);
BYTE effect_state_bit24_changed(DWORD a, DWORD b);
BYTE effect_state_bit25(DWORD state);
BYTE effect_state_bit25_changed(DWORD a, DWORD b);
BYTE effect_state_bit26(DWORD state);
BYTE effect_state_bit26_changed(DWORD a, DWORD b);
BYTE effect_state_bit27(DWORD state);
BYTE effect_state_bit28(DWORD state);

BYTE session_play_ground_skill_effect(struct Session *self, int skill, struct Vec3 pos);
BYTE skill_effect_on_caster(int skill, struct GameActor *caster, struct GameActor *target);
BYTE skill_effect_on_target(int skill, struct GameActor *target);

#endif
