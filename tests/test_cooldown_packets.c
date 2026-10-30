#include "session/state.h"
#include "game/skill_cooldowns.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "owned_storage_test_allocator.h"

struct Session g_session1;
static unsigned clock_value = 1000;
unsigned cooldown_clock_now(void) { return clock_value; }
#define malloc checked_malloc
#define free checked_free
#include "../src/ui/cooldown_lists.c"
#undef free
#undef malloc
#include "../src/game/skill_cooldowns.c"

static void u16(unsigned char *out, unsigned short value) { memcpy(out, &value, 2); }
static void i32(unsigned char *out, int value) { memcpy(out, &value, 4); }
static unsigned char packet[40];

static void make_delay(int skill, int remain, int total)
{
    memset(packet, 0, sizeof packet); u16(packet + 2, (unsigned short)skill);
    i32(packet + 4, remain); i32(packet + 8, total);
}

static void check_timers(const struct SkillCooldown *cd, unsigned bar, unsigned quick)
{
    const struct CooldownTimerNode *node;
    assert(cd->timers_bar.size == bar && cd->timers_quick.size == quick);
    for (node = cd->timers_bar.head; node; node = node->next) {
        assert(node->value.start == clock_value);
        assert(node->value.total_ms == (unsigned)cd->total_ms);
    }
}

static void failure_rollback(int (*receive)(const void *, size_t), size_t size)
{
    struct SkillCooldownNode *head = g_session1.skillDelays.head;
    unsigned old_size = g_session1.skillDelays.size, live = outstanding, failure;
    assert(head && head->value.skill_id == 7);
    for (failure = 1; ; ++failure) {
        int result;
        fail_at = allocation_calls + failure;
        result = receive(packet, size);
        fail_at = 0;
        if (result) break;
        assert(g_session1.skillDelays.head == head && g_session1.skillDelays.size == old_size);
        assert(head->value.skill_id == 7 && head->value.end_time == 1200 && outstanding == live);
        assert(failure < 100);
    }
    assert(failure > 1);
}

static void seed(void)
{
    struct SkillCooldown old = {0};
    skill_cooldowns_clear(&g_session1.skillDelays);
    old.skill_id = 7; old.end_time = 1200; old.total_ms = 200;
    assert(skill_cooldowns_append(&g_session1.skillDelays, &old));
}

int main(void)
{
    unsigned i, calls;
    struct SkillCooldownNode *node;
    struct SkillCooldownList *skills = &g_session1.skillDelays;
    assert(cooldown_slots_add(&g_session1.cooldownSlotsBar, 42, 1, 10, 4));
    assert(cooldown_slots_add(&g_session1.cooldownSlotsBar, 42, 2, 39, 4));
    assert(cooldown_slots_add(&g_session1.cooldownSlotsQuick, 42, 3, 68, 5));
    seed();
    make_delay(42, 8000, 8000);
    calls = allocation_calls;
    assert(!skill_cooldowns_receive_delay(NULL, 8));
    for (i = 0; i < 8; ++i) assert(!skill_cooldowns_receive_delay(packet, i));
    for (i = 0; i < 12; ++i) assert(!skill_cooldowns_receive_delay2(packet, i));
    assert(allocation_calls == calls && skills->size == 1);
    failure_rollback(skill_cooldowns_receive_delay, 8);
    assert(skills->size == 2);
    node = skill_cooldowns_find(skills, 42); assert(node && node->value.end_time == 9000);
    check_timers(&node->value, 2, 0);
    assert(node->value.timers_bar.head->value.slot == 1 && node->value.timers_bar.head->next->value.slot == 2);
    assert(skill_cooldowns_receive_delay(packet, 8));
    assert(skills->size == 2);
    assert(cooldown_slots_add(&g_session1.cooldownSlotsBar, 42, 1, 10, 4));
    seed();
    assert(skill_cooldowns_receive_delay(packet, 8));
    node = skill_cooldowns_find(skills, 42);
    check_timers(&node->value, 2, 0);
    assert(node->value.timers_bar.head->value.slot == 1 && node->value.timers_bar.head->next->value.slot == 1);
    seed();
    make_delay(42, 3000, 8000);
    failure_rollback(skill_cooldowns_receive_delay2, 12);
    node = skill_cooldowns_find(skills, 42);
    check_timers(&node->value, 3, 1);
    assert(node->value.timers_bar.head->value.elapsed == 5000);
    calls = allocation_calls;
    assert(skill_cooldowns_receive_delay2(packet, 12) && allocation_calls == calls);
    make_delay(42, -1, 8000);
    assert(!skill_cooldowns_receive_delay(packet, 8) && !skill_cooldowns_receive_delay2(packet, 12));
    seed(); memset(packet, 0, sizeof packet); u16(packet + 2, 24);
    u16(packet + 4, 42); i32(packet + 6, 8000); i32(packet + 10, 3000);
    u16(packet + 14, 42); i32(packet + 16, 7000); i32(packet + 20, 2000);
    calls = allocation_calls;
    for (i = 0; i < 24; ++i) assert(!skill_cooldowns_receive_list2(packet, i));
    assert(allocation_calls == calls);
    failure_rollback(skill_cooldowns_receive_list2, 24);
    assert(skills->size == 2 && skills->head->value.total_ms == 8000 && skills->head->next->value.total_ms == 7000);
    check_timers(&skills->head->value, 3, 1);
    seed();
    i32(packet + 20, -1);
    assert(!skill_cooldowns_receive_list2(packet, 24) && skills->size == 1 && skills->head->value.skill_id == 7);
    i32(packet + 20, 2000); u16(packet + 2, 23);
    assert(!skill_cooldowns_receive_list2(packet, sizeof packet));
    u16(packet + 2, 3); assert(!skill_cooldowns_receive_list2(packet, sizeof packet));
    memset(packet, 0, sizeof packet); u16(packet + 2, 16);
    u16(packet + 4, 42); i32(packet + 6, 5000);
    u16(packet + 10, 42); i32(packet + 12, 2000);
    failure_rollback(skill_cooldowns_receive_list, 16);
    assert(skills->size == 3 && skills->head->value.skill_id == 7);
    assert(skills->head->next->value.total_ms == 5000 && skills->head->next->next->value.total_ms == 2000);
    check_timers(&skills->head->next->value, 0, 0);
    skill_cooldowns_clear(skills);
    clock_value = 0xfffffff0;
    make_delay(99, 32, 32);
    assert(skill_cooldowns_receive_delay2(packet, 12) && skills->head->value.end_time == 16);
    assert(cooldown_time_pending(skills->head->value.end_time, clock_value));
    memset(packet, 0, sizeof packet); u16(packet + 2, 4);
    assert(skill_cooldowns_receive_list2(packet, 4) && !skills->head && !skills->size);
    cooldown_slots_clear(&g_session1.cooldownSlotsBar);
    cooldown_slots_clear(&g_session1.cooldownSlotsQuick);
    assert(!outstanding);
    puts("Cooldown packets: four real handlers, truncated spans, lengths, ordered duplicate slots, defined totals, wraparound and every allocation failure preserve atomic state");
    return 0;
}
