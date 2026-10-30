#include "actor/tetra_ball.h"
#include "actor/actor_common.h"
#include "core/object_type.h"
#include "effects/skills/skill_view.h"

static void *tetra_find(struct GameActor *self)
{
    void *effect = NULL;
    int type;

    for (type = 0x4ee; type <= 0x4f1; type++) {
        effect = object_as_type(effect_mgr_find_effect(g_effectMgr, self->aid, type), OBJECT_TETRA_BALL_EFFECT);
        if (effect)
            break;
    }
    return effect;
}

void game_actor_set_tetra_ball(struct GameActor *self, int slot, int on, int value)
{
    static const int types[4] = { 0x4ee, 0x4f0, 0x4ef, 0x4f1 };
    void *effect;

    if (slot < 0 || slot >= ACTOR_EFFECT_SLOT_COUNT)
        return;

    if (on == 1) {
        int type;

        if (!value || self->effect_slots[slot] == value)
            return;
        effect = tetra_find(self);
        type = types[value & 3];
        if (effect) {
            tetra_ball_effect_add_ball(effect, type, slot);
        } else {
            Vec3 at;

            at.x = 0.0f;
            at.y = 0.0f;
            at.z = 0.0f;
            effect = object_as_type(effect_mgr_spawn(g_effectMgr, self->aid, type, at, 0, -1, 0, 0), OBJECT_TETRA_BALL_EFFECT);
            if (effect)
                tetra_ball_effect_move_first_to(effect, slot);
        }
        self->effect_slots[slot] = value;
        return;
    }
    effect = tetra_find(self);
    if (effect) {
        if (tetra_ball_effect_count(effect) > 1)
            tetra_ball_effect_remove_ball(effect, slot);
        else
            effect_mgr_remove(g_effectMgr, effect, 1);
    }
    self->effect_slots[slot] = 0;
}
