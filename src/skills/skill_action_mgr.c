#include "resources/attr_queries.h"
#include "skills/skill_action.h"
#include "game/context.h"
#include "world/world.h"

Vec3 *skill_cell_position(Vec3 *out, int cell_x, int cell_y)
{
    struct World *world = game_current_world();
    struct C3dAttr *attr = game_current_map_attr();
    float x = 0.0f, z = 0.0f, height;

    if (!world || !attr) {
        out->x = 0.0f;
        out->y = 0.0f;
        out->z = 0.0f;
        return out;
    }
    world_cell_to_world(world, cell_x, cell_y, &x, &z);
    height = c3d_attr_get_height(attr, x, z);
    out->x = x + 2.5f;
    out->y = height + 0.0f;
    out->z = z + 2.5f;
    return out;
}

unsigned char skill_action_mgr_is_good_for_actor(struct SkillActionMgr *self, int skill_id)
{
    const struct SkillAction *action = skill_action_mgr_find_skill(self, skill_id);
    unsigned char answer = 0;

    if (action)
        answer = action->is_good_for_actor;
    return answer;
}

struct SkillActionAnswer *skill_action_mgr_cast_action(struct SkillActionMgr *self,
                                                     struct SkillActionAnswer *out, int skill_id)
{
    const struct SkillAction *action = skill_action_mgr_find_skill(self, skill_id);

    if (action) {
        out->value = action->cast_action;
        out->found = 1;
    } else {
        out->found = 0;
        out->value = 8;
    }
    return out;
}

struct SkillActionAnswer *skill_action_mgr_use_action(struct SkillActionMgr *self,
                                                    struct SkillActionAnswer *out, int skill_id)
{
    const struct SkillAction *action = skill_action_mgr_find_skill(self, skill_id);

    if (action) {
        out->value = action->use_action;
        out->found = 1;
    } else {
        out->found = 0;
        out->value = 7;
    }
    return out;
}

unsigned char skill_action_mgr_skips_use_motion(struct SkillActionMgr *self, int skill_id)
{
    const struct SkillAction *action = skill_action_mgr_find_skill(self, skill_id);
    unsigned char answer = 0;

    if (action)
        answer = action->skips_use_motion;
    return answer;
}

unsigned char skill_action_mgr_skips_attack_delay(struct SkillActionMgr *self, int skill_id)
{
    const struct SkillAction *action = skill_action_mgr_find_skill(self, skill_id);
    unsigned char answer = 0;

    if (action)
        answer = action->skips_attack_delay;
    return answer;
}

unsigned char skill_action_mgr_cast(struct SkillActionMgr *self, int skill_id, unsigned cast_time,
                                   SkillActor *caster)
{
    const struct SkillAction *action = skill_action_mgr_find_skill(self, skill_id);

    if (!action)
        return 0;
    if (action->cast)
        action->cast(cast_time, caster);
    return 1;
}

unsigned char skill_action_mgr_cast_on_target(struct SkillActionMgr *self, int skill_id,
                                           SkillActor *actor)
{
    const struct SkillAction *action = skill_action_mgr_find_skill(self, skill_id);

    if (!action)
        return 0;
    if (action->cast_on_target)
        action->cast_on_target(actor);
    return 1;
}

unsigned char skill_action_mgr_cast_at_cell(struct SkillActionMgr *self, int skill_id,
                                         unsigned time_ms, short cell_x, short cell_y)
{
    const struct SkillAction *action = skill_action_mgr_find_skill(self, skill_id);
    Vec3 pos;

    if (!action)
        return 0;
    skill_cell_position(&pos, cell_x, cell_y);
    if (action->cast_at_cell)
        action->cast_at_cell(time_ms, pos);
    return 1;
}

unsigned char skill_action_mgr_use(struct SkillActionMgr *self, int skill_id, int level,
                                  SkillActor *caster, SkillActor *target)
{
    const struct SkillAction *action = skill_action_mgr_find_skill(self, skill_id);

    if (!action)
        return 0;
    if (action->use)
        action->use(level, caster, target);
    return 1;
}

unsigned char skill_action_mgr_use_on_ground(struct SkillActionMgr *self, int skill_id, int cell_x,
                                          int cell_y, int level, SkillActor *caster)
{
    const struct SkillAction *action = skill_action_mgr_find_skill(self, skill_id);
    Vec3 pos;

    if (!action)
        return 0;
    skill_cell_position(&pos, cell_x, cell_y);
    if (action->use_on_ground)
        action->use_on_ground(pos, level, caster);
    return 1;
}

unsigned char skill_action_mgr_holds_ground_pose(struct SkillActionMgr *self, int skill_id)
{
    const struct SkillAction *action = skill_action_mgr_find_skill(self, skill_id);
    unsigned char answer = 0;

    if (action)
        answer = action->holds_ground_pose;
    return answer;
}

int skill_action_mgr_get_skill_area(struct SkillActionMgr *self, int skill_id)
{
    const struct SkillAction *action = skill_action_mgr_find_skill(self, skill_id);
    int area = 0;

    if (action)
        area = action->area;
    return area;
}

unsigned char skill_action_mgr_hit(struct SkillActionMgr *self, int skill_id, SkillActor *caster,
                                  SkillActor *target, SkillActor *target_unit)
{
    const struct SkillAction *action = skill_action_mgr_find_skill(self, skill_id);

    if (!action)
        return 0;
    if (action->hit)
        action->hit(caster, target, target_unit);
    return 1;
}

unsigned char skill_action_mgr_damage_hit(struct SkillActionMgr *self, int skill_id,
                                        SkillActor *source, SkillActor *target)
{
    const struct SkillAction *action = skill_action_mgr_find_skill(self, skill_id);

    if (!action)
        return 0;
    if (action->damage_hit)
        action->damage_hit(source, target);
    return 1;
}

unsigned char skill_action_mgr_efst_on(struct SkillActionMgr *self, int efst, int value,
                                     SkillActor *actor)
{
    const struct StatusAction *action = skill_action_mgr_find_efst(self, efst);

    if (!action)
        return 0;
    if (action->on)
        action->on(efst, value, actor);
    return 1;
}

unsigned char skill_action_mgr_efst_off(struct SkillActionMgr *self, int efst, int value,
                                      SkillActor *actor)
{
    const struct StatusAction *action = skill_action_mgr_find_efst(self, efst);

    if (!action)
        return 0;
    if (action->off)
        action->off(efst, value, actor);
    return 1;
}

unsigned char skill_action_mgr_efst_reapply(struct SkillActionMgr *self, int efst, int value,
                                          SkillActor *actor)
{
    const struct StatusAction *action = skill_action_mgr_find_efst(self, efst);

    if (!action)
        return 0;
    if (action->reapply)
        action->reapply(efst, value, actor);
    return 1;
}

unsigned char skill_action_mgr_on_object_appear(struct SkillActionMgr *self, int type, int level,
                                              SkillActor *unit, SkillActor *owner)
{
    const struct SkillObjectAction *action = skill_action_mgr_find_object(self, type);

    if (!action)
        return 0;
    if (action->appear)
        action->appear(level, unit, owner);
    return 1;
}

unsigned char skill_action_mgr_on_object_leave(struct SkillActionMgr *self, int type,
                                             SkillActor *unit)
{
    const struct SkillObjectAction *action = skill_action_mgr_find_object(self, type);

    if (!action)
        return 0;
    if (action->leave)
        action->leave(unit);
    return 1;
}
