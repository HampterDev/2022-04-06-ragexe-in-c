#include <stdlib.h>
#include <stdint.h>
#include "contents/schedule_storage.h"
#include "platform/clock.h"

struct TimeScheduler *g_timeScheduler;

static const struct ScheduleOps plain_ops = {schedule_run, schedule_free};

struct Schedule *schedule_construct(struct Schedule *self, int now, int interval, int kind, int id, int tag)
{
    *self = (struct Schedule){0};
    self->ops = &plain_ops;
    self->id = id;
    self->last_run = now;
    self->interval = interval;
    self->kind = kind;
    self->tag = tag;
    self->alive = 1;
    return self;
}

struct Schedule *schedule_create(int now, int interval, int kind, const struct ScheduleOps *ops)
{
    struct Schedule *self = malloc(sizeof *self);
    if (!self) return NULL;
    schedule_construct(self, now, interval, kind, schedule_next_id(), 0);
    if (ops) self->ops = ops;
    return self;
}

void schedule_free(struct Schedule *self) { free(self); }
void schedule_destroy(struct Schedule *self) { if (self) self->ops->destroy(self); }

char schedule_is_due(struct Schedule *self, int now)
{
    return self->interval > 0 && (int64_t)self->last_run + self->interval < now;
}

void schedule_run(struct Schedule *self, int now)
{
    if (self->kind == SCHEDULE_ONCE) { self->alive = 0; return; }
    self->last_run = now;
}

struct TimeScheduler *time_scheduler_construct(struct TimeScheduler *self)
{
    *self = (struct TimeScheduler){0};
    return self;
}

void time_scheduler_clear(struct TimeScheduler *self)
{
    struct ScheduleNode *node, *next;
    if (!self || self->clearing) return;
    if (self->ticking) { self->clear_requested = 1; return; }
    self->clearing = 1;
    for (node = self->first; node; node = node->next) {
        schedule_destroy(node->job);
        node->job = NULL;
    }
    for (node = self->first; node; node = next) { next = node->next; free(node); }
    *self = (struct TimeScheduler){0};
}

void time_scheduler_destruct(struct TimeScheduler *self) { time_scheduler_clear(self); }

char time_scheduler_create(void)
{
    struct TimeScheduler *self;
    if (g_timeScheduler) return 0;
    self = malloc(sizeof *self);
    if (!self) return 0;
    g_timeScheduler = time_scheduler_construct(self);
    return 1;
}

void time_scheduler_destroy(void)
{
    if (!g_timeScheduler) return;
    time_scheduler_destruct(g_timeScheduler);
    free(g_timeScheduler);
    g_timeScheduler = NULL;
}

struct Schedule *time_scheduler_find(struct TimeScheduler *self, int id)
{
    struct ScheduleNode *node;
    if (!self) return NULL;
    for (node = self->first; node; node = node->next)
        if (node->job && node->job->id == id) return node->job;
    return NULL;
}

struct Schedule *time_scheduler_add(struct TimeScheduler *self, struct Schedule *job)
{
    struct ScheduleNode *node;
    int32_t now;
    if (!self || !job || self->clearing || self->clear_requested || self->count == (size_t)-1) return NULL;
    for (node = self->first; node; node = node->next)
        if (node->job == job) return NULL;
    node = malloc(sizeof *node);
    if (!node) return NULL;
    now = platform_time_now();
    job->last_run = now;
    node->job = job;
    node->next = NULL;
    node->prev = self->last;
    if (self->last) self->last->next = node;
    else self->first = node;
    self->last = node;
    ++self->count;
    return job;
}
