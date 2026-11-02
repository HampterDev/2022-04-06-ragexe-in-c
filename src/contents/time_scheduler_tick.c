#include <stdlib.h>
#include "contents/schedule_storage.h"
#include "platform/clock.h"

void time_scheduler_tick(struct TimeScheduler *self)
{
    struct ScheduleNode *node, *next;
    if (!self || self->paused || self->ticking || self->clearing) return;
    self->ticking = 1;
    for (node = self->first; node && !self->clear_requested;) {
        struct Schedule *job = node->job;
        int32_t now;
        if (!job->alive) {
            next = node->next;
            if (node->prev) node->prev->next = next;
            else self->first = next;
            if (next) next->prev = node->prev;
            else self->last = node->prev;
            --self->count;
            free(node);
            schedule_destroy(job);
            node = next;
            continue;
        }
        now = platform_time_now();
        if (!job->paused && schedule_is_due(job, now)) job->ops->run(job, now);
        node = node->next;
    }
    self->ticking = 0;
    if (self->clear_requested) time_scheduler_clear(self);
}
