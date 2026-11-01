#include <windows.h>
#include <mmsystem.h>
#include <assert.h>
#include <stdio.h>
#include "session/session_state.h"
static unsigned ticks;
static DWORD fixture_clock(void) { return ticks; }
#define timeGetTime fixture_clock
#include "../src/ui/cooldown_clock.c"
#undef timeGetTime

static void test_session_delays(void)
{
    static struct Session session;
    struct SkillCooldownNode unrelated = {0}, exact = {0}, shared = {0};

    ticks = 1000;
    assert(session_is_skill_delay_over(&session, 7));
    unrelated.value.skill_id = 8;
    unrelated.value.end_time = 2000;
    exact.value.skill_id = 7;
    exact.value.end_time = 1101;
    shared.value.skill_id = 0x241;
    shared.value.end_time = 2000;
    session.skillDelays.head = &unrelated;
    unrelated.next = &exact;
    exact.next = &shared;
    assert(!session_is_skill_delay_over(&session, 7));
    ticks = 1001;
    assert(session_is_skill_delay_over(&session, 7));
    assert(!session_is_skill_delay_over(&session, 9));
    session.skillDelays.head = &shared;
    shared.next = &exact;
    exact.next = NULL;
    assert(!session_is_skill_delay_over(&session, 7));
    session.skillDelays.head = &exact;
    exact.value.end_time = 0x55;
    ticks = 0xfffffff0;
    assert(!session_is_skill_delay_over(&session, 7));
    ticks++;
    assert(session_is_skill_delay_over(&session, 7));
    ticks = 0x55;
    assert(session_is_skill_delay_over(&session, 7));
    ticks++;
    assert(session_is_skill_delay_over(&session, 7));
    session.skillDelays.head = NULL;
}

int main(void)
{
    struct SkillCooldown cd = {0};
    ticks = 0xfffffff0;
    cd.end_time = 0x10;
    assert(cooldown_clock_now() == ticks && skill_cooldown_running(&cd));
    ticks = 0x10;
    assert(!skill_cooldown_running(&cd));
    test_session_delays();
    puts("Cooldown clock: live clock, wraparound, session lookup, shared delay ordering and grace period passed");
    return 0;
}
