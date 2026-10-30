#include "ui/cooldown.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "owned_storage_test_allocator.h"

#define malloc checked_malloc
#define free checked_free
#include "../src/ui/cooldown_lists.c"
#undef free
#undef malloc

static void validate(const struct SkillCooldownList *list)
{
    struct SkillCooldownNode *node, *previous = NULL;
    unsigned count = 0;
    for (node = list->head; node; node = node->next) {
        struct CooldownTimerList const *timers[2] = {&node->value.timers_bar, &node->value.timers_quick};
        unsigned i;
        assert(node->prev == previous); previous = node; ++count;
        for (i = 0; i < 2; ++i) {
            struct CooldownTimerNode *t, *prev = NULL;
            unsigned n = 0;
            for (t = timers[i]->head; t; t = t->next) {
                assert(t->prev == prev); prev = t; ++n;
            }
            assert(n == timers[i]->size);
        }
    }
    assert(count == list->size);
}

int main(void)
{
    struct CooldownSlotList slots = {0};
    struct SkillCooldownList skills = {0}, copy = {0};
    struct SkillCooldown value = {0};
    struct CooldownTimer timer;
    struct SkillCooldownNode *first;
    unsigned i, live, calls;
    for (i = 0; i < 127; ++i) assert(cooldown_slots_add(&slots, i % 3, i, i * 2, 4));
    assert(slots.size == 127 && slots.head->value.slot == 0 && slots.head->next->value.slot == 1);
    live = outstanding; fail_at = allocation_calls + 1;
    assert(!cooldown_slots_add(&slots, 9, 9, 9, 9)); fail_at = 0;
    assert(slots.size == 127 && outstanding == live);
    cooldown_slots_clear(&slots); cooldown_slots_clear(&slots); assert(!outstanding);
    cooldown_timer_init(&timer, 2, 10, 4, 8000, 100, 0, 0);
    cooldown_timer_layout(&timer, 1000, 0, 0, 12, 12, 6, 0, 0);
    assert(cooldown_timers_append(&value.timers_bar, &timer));
    assert(cooldown_timers_append(&value.timers_bar, &timer));
    assert(cooldown_timers_append(&value.timers_quick, &timer));
    value.skill_id = 17; value.end_time = 8100; value.total_ms = 8000;
    first = skill_cooldowns_append(&skills, &value); assert(first);
    assert(first->value.timers_bar.head != value.timers_bar.head);
    live = outstanding;
    for (i = 1; i <= 4; ++i) {
        fail_at = allocation_calls + i;
        assert(!skill_cooldowns_append(&skills, &value)); fail_at = 0;
        assert(skills.head == first && !first->next && skills.size == 1 && outstanding == live);
        validate(&skills);
    }
    assert(skill_cooldowns_append(&skills, &first->value));
    assert(skill_cooldowns_find(&skills, 17) == first && !skill_cooldowns_find(&skills, 18));
    value.skill_id = 99;
    assert(skill_cooldowns_append(&copy, &value));
    live = outstanding;
    for (i = 1; i <= 8; ++i) {
        struct SkillCooldownNode *old = copy.head;
        fail_at = allocation_calls + i;
        assert(!skill_cooldowns_clone(&copy, &skills)); fail_at = 0;
        assert(copy.head == old && copy.size == 1 && old->value.skill_id == 99 && outstanding == live);
    }
    assert(skill_cooldowns_clone(&copy, &skills));
    validate(&copy);
    assert(copy.head->value.timers_bar.head->value.edge_mask == 255);
    copy.head->value.timers_bar.head->value.edges[0].reach = 99;
    assert(first->value.timers_bar.head->value.edges[0].reach == 12);
    assert(skill_cooldowns_clone(&copy, &copy)); validate(&copy);
    assert(!skill_cooldowns_splice(&copy, &copy));
    assert(skill_cooldowns_splice(&skills, &copy) && !copy.head && !copy.size && skills.size == 4);
    validate(&skills);
    for (i = 0; i < 64; ++i) assert(skill_cooldowns_append(&skills, &value));
    validate(&skills);
    {
        struct CooldownTimerList *timers = &first->value.timers_bar;
        struct CooldownTimerNode *next = cooldown_timers_remove(timers, timers->head);
        assert(next == timers->head && next->prev == NULL);
        assert(!cooldown_timers_remove(timers, next) && !timers->size && !timers->head);
    }
    skill_cooldowns_remove(&skills, first->next); validate(&skills);
    skill_cooldowns_remove(&skills, first); validate(&skills);
    {
        struct SkillCooldownList full = {NULL, UINT_MAX};
        struct CooldownTimerList full_timers = {NULL, UINT_MAX};
        calls = allocation_calls;
        assert(!skill_cooldowns_append(&full, &value));
        assert(!cooldown_timers_append(&full_timers, &timer));
        assert(!skill_cooldowns_splice(&full, &skills));
        assert(allocation_calls == calls);
    }
    skill_cooldown_clear(&value);
    skill_cooldowns_clear(&skills); skill_cooldowns_clear(&skills);
    skill_cooldowns_clear(&copy);
    assert(!outstanding);
    assert(cooldown_time_pending(0x10, 0xfffffff0));
    assert(!cooldown_time_pending(0xfffffff0, 0x10));
    assert(!cooldown_time_pending(8, 8));
    assert(cooldown_time_pending(INT_MAX, 0) && !cooldown_time_pending(0x80000000, 0));
    puts("Cooldown storage: ordered C ownership, stable values, deep copy, all allocation failures, atomic clone/splice, erase links, overflow guards and repeated cleanup passed");
    return 0;
}
