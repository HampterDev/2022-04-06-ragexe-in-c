#include "ui/cooldown.h"
#include "platform/win_types.h"
#include "platform/clock.h"

unsigned cooldown_clock_now(void)
{
    return platform_ticks_ms();
}

int skill_cooldown_running(const struct SkillCooldown *cooldown)
{
    return cooldown_time_pending(cooldown->end_time, cooldown_clock_now());
}
