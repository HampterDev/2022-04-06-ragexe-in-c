#include "ui/cooldown.h"
#include <limits.h>
#include <stdlib.h>

int cooldown_slots_add(struct CooldownSlotList *list, int skill_id, int slot, int x, int y)
{
    struct CooldownSlotNode *node, **link = &list->head;
    if (list->size == UINT_MAX) return 0;
    node = malloc(sizeof *node);
    if (!node) return 0;
    node->next = NULL;
    node->value = (struct CooldownSlot){skill_id, slot, x, y};
    while (*link) link = &(*link)->next;
    *link = node;
    ++list->size;
    return 1;
}

void cooldown_slots_clear(struct CooldownSlotList *list)
{
    struct CooldownSlotNode *node = list->head;
    while (node) {
        struct CooldownSlotNode *next = node->next;
        free(node);
        node = next;
    }
    *list = (struct CooldownSlotList){0};
}

struct CooldownTimerNode *cooldown_timers_append(struct CooldownTimerList *list, const struct CooldownTimer *timer)
{
    struct CooldownTimerNode *node, *last = list->head;
    if (list->size == UINT_MAX) return NULL;
    node = malloc(sizeof *node);
    if (!node) return NULL;
    if (last) while (last->next) last = last->next;
    node->value = *timer;
    node->next = NULL;
    node->prev = last;
    if (last) last->next = node; else list->head = node;
    ++list->size;
    return node;
}

struct CooldownTimerNode *cooldown_timers_find(const struct CooldownTimerList *list, int slot)
{
    struct CooldownTimerNode *node;
    for (node = list->head; node; node = node->next)
        if (node->value.slot == slot) return node;
    return NULL;
}

struct CooldownTimerNode *cooldown_timers_remove(struct CooldownTimerList *list, struct CooldownTimerNode *node)
{
    struct CooldownTimerNode *next = node->next;
    if (node->prev) node->prev->next = next; else list->head = next;
    if (next) next->prev = node->prev;
    --list->size;
    free(node);
    return next;
}

void cooldown_timers_clear(struct CooldownTimerList *list)
{
    while (list->head) cooldown_timers_remove(list, list->head);
}

void skill_cooldown_clear(struct SkillCooldown *cooldown)
{
    cooldown_timers_clear(&cooldown->timers_quick);
    cooldown_timers_clear(&cooldown->timers_bar);
}

struct SkillCooldownNode *skill_cooldowns_find(const struct SkillCooldownList *list, int skill_id)
{
    struct SkillCooldownNode *node;
    for (node = list->head; node; node = node->next)
        if (node->value.skill_id == skill_id) return node;
    return NULL;
}

struct SkillCooldownNode *skill_cooldowns_append(struct SkillCooldownList *list, const struct SkillCooldown *value)
{
    struct SkillCooldownNode *node, *last = list->head;
    struct CooldownTimerNode *timer;
    if (list->size == UINT_MAX) return NULL;
    node = malloc(sizeof *node);
    if (!node) return NULL;
    node->value = *value;
    node->value.timers_bar = (struct CooldownTimerList){0};
    node->value.timers_quick = (struct CooldownTimerList){0};
    for (timer = value->timers_bar.head; timer; timer = timer->next)
        if (!cooldown_timers_append(&node->value.timers_bar, &timer->value)) goto fail;
    for (timer = value->timers_quick.head; timer; timer = timer->next)
        if (!cooldown_timers_append(&node->value.timers_quick, &timer->value)) goto fail;
    if (last) while (last->next) last = last->next;
    node->next = NULL;
    node->prev = last;
    if (last) last->next = node; else list->head = node;
    ++list->size;
    return node;
fail:
    skill_cooldown_clear(&node->value);
    free(node);
    return NULL;
}

struct SkillCooldownNode *skill_cooldowns_remove(struct SkillCooldownList *list, struct SkillCooldownNode *node)
{
    struct SkillCooldownNode *next = node->next;
    if (node->prev) node->prev->next = next; else list->head = next;
    if (next) next->prev = node->prev;
    --list->size;
    skill_cooldown_clear(&node->value);
    free(node);
    return next;
}

void skill_cooldowns_clear(struct SkillCooldownList *list)
{
    while (list->head) skill_cooldowns_remove(list, list->head);
}

int skill_cooldowns_clone(struct SkillCooldownList *to, const struct SkillCooldownList *from)
{
    struct SkillCooldownList copy = {0};
    struct SkillCooldownNode *node;
    for (node = from->head; node; node = node->next)
        if (!skill_cooldowns_append(&copy, &node->value)) {
            skill_cooldowns_clear(&copy);
            return 0;
        }
    skill_cooldowns_clear(to);
    *to = copy;
    return 1;
}

int skill_cooldowns_splice(struct SkillCooldownList *to, struct SkillCooldownList *from)
{
    struct SkillCooldownNode *last = to->head;
    if (to == from || from->size > UINT_MAX - to->size) return 0;
    if (!from->head) return 1;
    if (last) {
        while (last->next) last = last->next;
        last->next = from->head;
        from->head->prev = last;
    } else to->head = from->head;
    to->size += from->size;
    *from = (struct SkillCooldownList){0};
    return 1;
}

int cooldown_time_pending(unsigned end, unsigned now)
{
    unsigned remaining = end - now;
    return remaining && remaining <= INT_MAX;
}
