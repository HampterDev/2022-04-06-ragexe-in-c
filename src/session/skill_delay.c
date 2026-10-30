#include "session/session_state.h"
#include "ui/cooldown.h"

static const struct SkillCooldown *find_skill_delay(const struct Session *session, int skill_id)
{
    const struct SkillCooldownNode *node;

    for (node = session->skillDelays.head; node; node = node->next)
        if (node->value.skill_id == skill_id || node->value.skill_id == 0x241)
            return &node->value;
    return NULL;
}

BYTE session_is_skill_delay_over(struct Session *self, int id)
{
    const struct SkillCooldown *delay = find_skill_delay(self, id);
    unsigned now;

    if (!delay)
        return 1;
    now = cooldown_clock_now();
    return !cooldown_time_pending(delay->end_time, now) || delay->end_time - now <= 100;
}
