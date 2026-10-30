#if defined(SKILL_EFFECT_CALLS_REFERENCE) || defined(SKILL_ACTIONS_REFERENCE)
unsigned char CSkillAction_IsGoodForActor(struct CSkillAction *self)
{
    abort();
}
int CSkillAction_CastAction(struct CSkillAction *self)
{
    abort();
}
int CSkillAction_UseAction(struct CSkillAction *self)
{
    abort();
}
unsigned char CSkillAction_SkipsUseMotion(struct CSkillAction *self, int arg)
{
    abort();
}
unsigned char CSkillAction_SkipsAttackDelay(struct CSkillAction *self)
{
    abort();
}
void CSkillAction_Cast(struct CSkillAction *self, unsigned cast_time, SkillActor *caster)
{
    abort();
}
void CSkillAction_CastOnTarget(struct CSkillAction *self, int arg, SkillActor *actor)
{
    abort();
}
void CSkillAction_CastAtCell(struct CSkillAction *self, unsigned time_ms, int arg, Vec3 pos)
{
    abort();
}
void CSkillAction_Use(struct CSkillAction *self, int level, SkillActor *caster, SkillActor *target)
{
    abort();
}
void CSkillAction_UseOnGround(struct CSkillAction *self, Vec3 pos, int level, SkillActor *caster)
{
    abort();
}
unsigned char CSkillAction_HoldsGroundPose(struct CSkillAction *self)
{
    abort();
}
int CSkillAction_Area(struct CSkillAction *self)
{
    abort();
}
void CSkillAction_Hit(struct CSkillAction *self, SkillActor *caster, SkillActor *target,
                      SkillActor *target_unit)
{
    abort();
}
void CSkillAction_DamageHit(struct CSkillAction *self, SkillActor *source, SkillActor *target)
{
    abort();
}
void CSkillObjectAction_Leave(struct CSkillObjectAction *self, SkillActor *unit)
{
    abort();
}
void CEfstAction_On(struct CEfstAction *self, int efst, int value, SkillActor *actor)
{
    abort();
}
void CEfstAction_Off(struct CEfstAction *self, int efst, int value, SkillActor *actor)
{
    abort();
}
void CEfstAction_Reapply(struct CEfstAction *self, int efst, int value, SkillActor *actor)
{
    abort();
}
#endif
struct GameMode *game_current_mode(void)
{
    abort();
}
struct Effect *effect_mgr_find_effect(struct EffectMgr *self, int aid, int type)
{
    abort();
}
void effect_mgr_remove_type(struct EffectMgr *self, int aid, int type)
{
    abort();
}
struct RagEffect *game_actor_launch_effect(struct GameActor *self, int effect_id, Vec3 offset,
                                           float rotation)
{
    abort();
}
int skill_named_effect_id(const char *name)
{
    abort();
}
unsigned long long game_mode_launch_dummy_effect(struct GameMode *mode, const Vec3 *at,
                                                int effect_id, int frames, int value)
{
    abort();
}
unsigned char able_to_make_effect_is_effect_exist(void *actor, int effect_id)
{
    abort();
}
