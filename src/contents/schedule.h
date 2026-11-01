#ifndef CONTENTS_SCHEDULE_H
#define CONTENTS_SCHEDULE_H
#include <stddef.h>
#include "base.h"
#include "contents/schedule_ids.h"

struct Schedule;
struct ScheduleOps {
    void (*run)(struct Schedule *job, int now);
    void (*destroy)(struct Schedule *job);
};
enum ScheduleKind { SCHEDULE_ONCE, SCHEDULE_REPEAT };

struct Schedule {
    const struct ScheduleOps *ops;
    int id, last_run, interval, kind;
    char alive, paused;
    int tag;
};
OFFSET_ASSERT(Schedule, interval, 0x0c);
OFFSET_ASSERT(Schedule, alive, 0x14);

struct ScheduleNode;
struct TimeScheduler {
    struct ScheduleNode *first, *last;
    size_t count;
    char paused, clearing, ticking, clear_requested;
};
extern struct TimeScheduler *g_timeScheduler;
struct Schedule *schedule_construct(struct Schedule *job, int now, int interval, int kind, int id, int tag);
struct Schedule *schedule_create(int now, int interval, int kind, const struct ScheduleOps *ops);
void schedule_free(struct Schedule *job);
void schedule_destroy(struct Schedule *job);
char schedule_is_due(struct Schedule *job, int now);
void schedule_run(struct Schedule *job, int now);

struct TimeScheduler *time_scheduler_construct(struct TimeScheduler *scheduler);
void time_scheduler_destruct(struct TimeScheduler *scheduler);
char time_scheduler_create(void);
void time_scheduler_destroy(void);
void time_scheduler_clear(struct TimeScheduler *scheduler);
struct Schedule *time_scheduler_find(struct TimeScheduler *scheduler, int id);
struct Schedule *time_scheduler_add(struct TimeScheduler *scheduler, struct Schedule *job);
void time_scheduler_tick(struct TimeScheduler *scheduler);

#endif
