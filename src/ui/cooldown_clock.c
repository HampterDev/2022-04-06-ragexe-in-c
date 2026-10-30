#include "ui/cooldown.h"
#include "replay/replay.h"
#include <mmsystem.h>

unsigned cooldown_clock_now(void)
{
    return g_replayPlaying ? replay_recorder_instance()->elapsed : timeGetTime();
}

int skill_cooldown_running(const struct SkillCooldown *cooldown)
{
    return cooldown_time_pending(cooldown->end_time, cooldown_clock_now());
}
