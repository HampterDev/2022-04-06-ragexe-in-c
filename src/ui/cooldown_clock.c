#include "ui/cooldown.h"
#include <windows.h>
#include <mmsystem.h>

unsigned cooldown_clock_now(void)
{
    return timeGetTime();
}

int skill_cooldown_running(const struct SkillCooldown *cooldown)
{
    return cooldown_time_pending(cooldown->end_time, cooldown_clock_now());
}
