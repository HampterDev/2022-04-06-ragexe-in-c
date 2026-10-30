#ifndef CONTENTS_RANKING_H
#define CONTENTS_RANKING_H

#include "base.h"
#include <stddef.h>
#include "contents/schedule.h"

#define RANK_ENTRIES 10

struct RankPacket {
    unsigned short packet_id;
    short rank_type;
    int char_ids[RANK_ENTRIES];
    int points[RANK_ENTRIES];
    int my_points;
};
OFFSET_ASSERT(RankPacket, points, 0x2c);
SIZE_ASSERT(RankPacket, 0x58);

struct RankingSchedule {
    struct Schedule base;
    struct RankPacket rank;
    int timeout;
    int *nameless;
    size_t nameless_count, nameless_capacity;
};

struct RankingSchedule *ranking_create(int now, int interval, int kind);
struct RankingSchedule *ranking_construct(struct RankingSchedule *self, int now, int interval,
                                                     int kind);
void ranking_destruct(struct RankingSchedule *self);
void ranking_destroy(struct RankingSchedule *self);
void ranking_run(struct RankingSchedule *self, int now);
void ranking_set(struct RankingSchedule *self, const struct RankPacket *packet);
void ranking_print(struct RankingSchedule *self);

#endif
