#include "game/skill_fx.h"
#include "skills/skill_action.h"
#include "skills/skill_action_registry.h"
#include "attack_records_session.h"

const char *session_get_job_name(struct Session *session, int job, int sex)
{ (void)session; (void)job; (void)sex; return "PORING"; }
int session_is_monster_actor(struct Session *session, struct GameActor *actor)
{ (void)session; (void)actor; return attack_test_monster_actor; }
void session_get_monster_skill_info(struct Session *session, const char *monster_name, int skill_id, int *action,
                                 int *loop_type)
{ (void)session; (void)monster_name; (void)skill_id; *action = -1; *loop_type = 0; }

struct SkillActionMgr *skill_action_mgr_instance(void) { return NULL; }
void skill_hit_caster_effects(struct GameMode *mode, int skill_id, struct GameActor *caster, struct GameActor *target)
{ (void)mode; (void)skill_id; (void)caster; (void)target; }
void skill_hit_target_effects(int skill_id, int level, struct GameActor *caster, struct GameActor *target)
{ (void)skill_id; (void)level; (void)caster; (void)target; }
