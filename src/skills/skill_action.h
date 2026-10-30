#ifndef SKILLS_SKILL_ACTION_H
#define SKILLS_SKILL_ACTION_H

#include "render/vec3.h"
#include "skills/skill_answer.h"
#include "skills/skill_action_registry.h"

typedef struct RenderObject SkillActor;

struct SkillAction {
    unsigned char is_good_for_actor;
    int cast_action;
    int use_action;
    unsigned char skips_use_motion;
    unsigned char skips_attack_delay;
    void (*cast)(unsigned cast_time, SkillActor *caster);
    void (*cast_on_target)(SkillActor *target);
    void (*cast_at_cell)(unsigned time_ms, Vec3 position);
    void (*use)(int level, SkillActor *caster, SkillActor *target);
    void (*use_on_ground)(Vec3 position, int level, SkillActor *caster);
    unsigned char holds_ground_pose;
    int area;
    void (*hit)(SkillActor *caster, SkillActor *target, SkillActor *target_unit);
    void (*damage_hit)(SkillActor *source, SkillActor *target);
};

struct StatusAction {
    void (*on)(int status, int value, SkillActor *actor);
    void (*off)(int status, int value, SkillActor *actor);
    void (*reapply)(int status, int value, SkillActor *actor);
};

struct SkillObjectAction {
    void (*appear)(int level, SkillActor *unit, SkillActor *owner);
    void (*leave)(SkillActor *unit);
};

unsigned char skill_action_mgr_is_good_for_actor(struct SkillActionMgr *self, int skill_id);
struct SkillActionAnswer *skill_action_mgr_cast_action(struct SkillActionMgr *self,
                                                     struct SkillActionAnswer *out, int skill_id);
struct SkillActionAnswer *skill_action_mgr_use_action(struct SkillActionMgr *self,
                                                    struct SkillActionAnswer *out, int skill_id);
unsigned char skill_action_mgr_skips_use_motion(struct SkillActionMgr *self, int skill_id);
unsigned char skill_action_mgr_skips_attack_delay(struct SkillActionMgr *self, int skill_id);
unsigned char skill_action_mgr_cast(struct SkillActionMgr *self, int skill_id, unsigned cast_time,
                                   SkillActor *caster);
unsigned char skill_action_mgr_cast_on_target(struct SkillActionMgr *self, int skill_id,
                                           SkillActor *actor);
unsigned char skill_action_mgr_cast_at_cell(struct SkillActionMgr *self, int skill_id,
                                         unsigned time_ms, short cell_x, short cell_y);
unsigned char skill_action_mgr_use(struct SkillActionMgr *self, int skill_id, int level,
                                  SkillActor *caster, SkillActor *target);
unsigned char skill_action_mgr_use_on_ground(struct SkillActionMgr *self, int skill_id, int cell_x,
                                          int cell_y, int level, SkillActor *caster);
unsigned char skill_action_mgr_holds_ground_pose(struct SkillActionMgr *self, int skill_id);
int skill_action_mgr_get_skill_area(struct SkillActionMgr *self, int skill_id);
unsigned char skill_action_mgr_hit(struct SkillActionMgr *self, int skill_id, SkillActor *caster,
                                  SkillActor *target, SkillActor *target_unit);
unsigned char skill_action_mgr_damage_hit(struct SkillActionMgr *self, int skill_id,
                                        SkillActor *source, SkillActor *target);
unsigned char skill_action_mgr_efst_on(struct SkillActionMgr *self, int efst, int value,
                                     SkillActor *actor);
unsigned char skill_action_mgr_efst_off(struct SkillActionMgr *self, int efst, int value,
                                      SkillActor *actor);
unsigned char skill_action_mgr_efst_reapply(struct SkillActionMgr *self, int efst, int value,
                                          SkillActor *actor);
unsigned char skill_action_mgr_on_object_appear(struct SkillActionMgr *self, int type, int level,
                                              SkillActor *unit, SkillActor *owner);
unsigned char skill_action_mgr_on_object_leave(struct SkillActionMgr *self, int type,
                                             SkillActor *unit);

Vec3 *skill_cell_position(Vec3 *out, int cell_x, int cell_y);

void skill_actions_register(void);

#endif
