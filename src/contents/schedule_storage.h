#ifndef SCHEDULE_STORAGE_H
#define SCHEDULE_STORAGE_H
#include "contents/schedule.h"
struct ScheduleNode {
    struct ScheduleNode *next, *prev;
    struct Schedule *job;
};
#endif
