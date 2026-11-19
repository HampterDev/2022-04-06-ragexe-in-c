#include "world/own_swing.h"

void own_swing_reset(struct OwnSwing *self)
{
    self->target = 0;
    self->at_tick = 0;
    self->lands = 0;
    self->shown = 0;
}

int own_swing_is_shown(const struct OwnSwing *self, uint32_t target, uint32_t at_tick)
{
    int32_t off = (int32_t)(at_tick - self->at_tick);

    return self->shown && target == self->target && off >= -OWN_SWING_MATCH_MS && off <= OWN_SWING_MATCH_MS;
}

int own_swing_follow(struct OwnSwing *self, int has, uint32_t target, uint32_t at_tick, uint32_t lands,
                     uint32_t own_now)
{
    int running = self->shown && (int32_t)(own_now - self->lands) < 0;
    int open = running && self->shown == OWN_SWING_AHEAD;

    if (has && own_swing_is_shown(self, target, at_tick))
        return OWN_SWING_KEEP;
    /* the swing the server told is the one the world has, whenever the world has it begin */
    if (has && running && self->shown == OWN_SWING_TOLD && target == self->target)
        return OWN_SWING_KEEP;
    if (!has) {
        if (!open)
            return OWN_SWING_KEEP;
        self->shown = 0;
        return OWN_SWING_TAKE_BACK;
    }
    if ((int32_t)(own_now - lands) >= 0) {
        /* the world's swing has landed: nothing to show, and what is shown and has not landed
           is not the world's */
        if (!open)
            return OWN_SWING_KEEP;
        self->shown = 0;
        return OWN_SWING_TAKE_BACK;
    }
    self->target = target;
    self->at_tick = at_tick;
    self->lands = lands;
    self->shown = OWN_SWING_AHEAD;
    return open ? OWN_SWING_REPLACE : OWN_SWING_SHOW;
}

void own_swing_shown(const struct OwnSwing *self, uint32_t gid, unsigned motion_ms, struct ActRecord *record)
{
    record->gid = gid;
    record->at_tick = self->at_tick;
    record->end_tick = self->at_tick + motion_ms;
    record->mark_tick = self->lands;
    record->target = self->target;
    record->x = 0;
    record->y = 0;
    record->skill_id = 0;
    record->skill_lv = 0;
    record->motion_ms = motion_ms;
    record->element = 0;
    record->kind = ACT_SWING;
    record->disposable = 0;
}

int own_swing_record(struct OwnSwing *self, const struct ActRecord *record)
{
    if (own_swing_is_shown(self, record->target, record->at_tick))
        return 1;
    self->target = record->target;
    self->at_tick = record->at_tick;
    self->lands = record->mark_tick;
    self->shown = OWN_SWING_TOLD;
    return 0;
}
