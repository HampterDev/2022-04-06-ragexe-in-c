#include "game/skill_cooldowns.h"
#include "ui/cooldown.h"
#include "session/state.h"
#include <string.h>

static unsigned short read_u16(const unsigned char *bytes)
{
    unsigned short value;
    memcpy(&value, bytes, sizeof value);
    return value;
}

static int read_i32(const unsigned char *bytes)
{
    int value;
    memcpy(&value, bytes, sizeof value);
    return value;
}

static int append_timer(struct CooldownTimerList *timers, const struct CooldownSlot *slot,
    int total_ms, unsigned start, unsigned elapsed)
{
    struct CooldownTimer timer;
    cooldown_timer_init(&timer, slot->slot, slot->x, slot->y, (unsigned)total_ms, start, elapsed, 0);
    return cooldown_timers_append(timers, &timer) != NULL;
}

static int add_timers(struct CooldownTimerList *timers, const struct CooldownSlotList *slots,
    int skill_id, int remain_ms, int total_ms, unsigned now)
{
    const struct CooldownSlotNode *slot;
    for (slot = slots->head; slot; slot = slot->next)
        if (slot->value.skill_id == skill_id &&
            !append_timer(timers, &slot->value, total_ms, now, (unsigned)total_ms - (unsigned)remain_ms))
            return 0;
    return 1;
}

static int add_skill_timers(struct SkillCooldown *cooldown, int remain_ms, unsigned now)
{
    return add_timers(&cooldown->timers_bar, &g_session1.cooldownSlotsBar,
        cooldown->skill_id, remain_ms, cooldown->total_ms, now) &&
        add_timers(&cooldown->timers_quick, &g_session1.cooldownSlotsQuick,
        cooldown->skill_id, remain_ms, cooldown->total_ms, now);
}

int skill_cooldowns_receive_delay(const void *packet, size_t size)
{
    const unsigned char *bytes = packet;
    struct SkillCooldownList changed = {0};
    struct SkillCooldown cooldown = {0};
    const struct CooldownSlotNode *slot;
    unsigned now;
    int delay;
    if (!bytes || size < 8) return 0;
    delay = read_i32(bytes + 4);
    if (delay < 0) return 0;
    cooldown.skill_id = read_u16(bytes + 2);
    now = cooldown_clock_now();
    cooldown.end_time = now + (unsigned)delay;
    cooldown.total_ms = delay;
    if (!skill_cooldowns_clone(&changed, &g_session1.skillDelays)) return 0;
    for (slot = g_session1.cooldownSlotsBar.head; slot; slot = slot->next) {
        struct SkillCooldownNode *listed;
        if (slot->value.skill_id != cooldown.skill_id) continue;
        listed = skill_cooldowns_find(&changed, cooldown.skill_id);
        if (listed) {
            if (!cooldown_timers_find(&listed->value.timers_bar, slot->value.slot)) {
                if (!append_timer(&listed->value.timers_bar, &slot->value, delay, now, 0)) goto fail;
                continue;
            }
            skill_cooldowns_remove(&changed, listed);
        }
        if (!append_timer(&cooldown.timers_bar, &slot->value, delay, now, 0) ||
            !skill_cooldowns_append(&changed, &cooldown)) goto fail;
    }
    for (slot = g_session1.cooldownSlotsQuick.head; slot; slot = slot->next) {
        if (slot->value.skill_id != cooldown.skill_id || skill_cooldowns_find(&changed, cooldown.skill_id)) continue;
        if (!append_timer(&cooldown.timers_quick, &slot->value, delay, now, 0) ||
            !skill_cooldowns_append(&changed, &cooldown)) goto fail;
    }
    skill_cooldowns_clear(&g_session1.skillDelays);
    g_session1.skillDelays = changed;
    skill_cooldown_clear(&cooldown);
    return 1;
fail:
    skill_cooldown_clear(&cooldown);
    skill_cooldowns_clear(&changed);
    return 0;
}

int skill_cooldowns_receive_delay2(const void *packet, size_t size)
{
    const unsigned char *bytes = packet;
    struct SkillCooldown cooldown = {0};
    unsigned now;
    int remain, result;
    if (!bytes || size < 12) return 0;
    cooldown.skill_id = read_u16(bytes + 2);
    remain = read_i32(bytes + 4);
    cooldown.total_ms = read_i32(bytes + 8);
    if (remain < 0 || cooldown.total_ms < 0) return 0;
    if (skill_cooldowns_find(&g_session1.skillDelays, cooldown.skill_id)) return 1;
    now = cooldown_clock_now();
    cooldown.end_time = now + (unsigned)remain;
    result = add_skill_timers(&cooldown, remain, now) &&
        skill_cooldowns_append(&g_session1.skillDelays, &cooldown) != NULL;
    skill_cooldown_clear(&cooldown);
    return result;
}

static int receive_list(const unsigned char *bytes, size_t size, int with_total)
{
    struct SkillCooldownList pending = {0};
    size_t length, offset, width = with_total ? 10 : 6;
    unsigned now;
    if (!bytes || size < 4) return 0;
    length = read_u16(bytes + 2);
    if (length < 4 || length > size || (length - 4) % width) return 0;
    now = cooldown_clock_now();
    for (offset = 4; offset < length; offset += width) {
        struct SkillCooldown cooldown = {0};
        int remain = read_i32(bytes + offset + (with_total ? 6 : 2));
        cooldown.total_ms = with_total ? read_i32(bytes + offset + 2) : remain;
        if (remain < 0 || cooldown.total_ms < 0) goto fail;
        cooldown.skill_id = read_u16(bytes + offset);
        cooldown.end_time = now + (unsigned)remain;
        if ((with_total && !add_skill_timers(&cooldown, remain, now)) ||
            !skill_cooldowns_append(&pending, &cooldown)) {
            skill_cooldown_clear(&cooldown);
            goto fail;
        }
        skill_cooldown_clear(&cooldown);
    }
    if (with_total) {
        skill_cooldowns_clear(&g_session1.skillDelays);
        g_session1.skillDelays = pending;
    } else if (!skill_cooldowns_splice(&g_session1.skillDelays, &pending)) goto fail;
    return 1;
fail:
    skill_cooldowns_clear(&pending);
    return 0;
}

int skill_cooldowns_receive_list(const void *packet, size_t size)
{
    return receive_list(packet, size, 0);
}

int skill_cooldowns_receive_list2(const void *packet, size_t size)
{
    return receive_list(packet, size, 1);
}
