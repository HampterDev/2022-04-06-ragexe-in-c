#include "session/ranking.h"
#include "session/state.h"
#include "contents/ranking.h"
#include <time.h>

void session_on_ranking_packet(struct Session *self, int show, const struct RankPacket *packet)
{
    struct Schedule *running = time_scheduler_find(g_timeScheduler, self->rankingScheduleId);
    struct RankingSchedule *ranking;
    __time32_t now;

    if (show != 1) {
        if (running) running->alive = 0;
        return;
    }
    if (running || !packet) return;
    _time32(&now);
    ranking = ranking_create((int)now, 1, SCHEDULE_REPEAT);
    if (!ranking) return;
    ranking->timeout = 3;
    ranking_set(ranking, packet);
    ranking_run(ranking, (int)now);
    if (!time_scheduler_add(g_timeScheduler, &ranking->base)) {
        ranking_destroy(ranking);
        return;
    }
    self->rankingScheduleId = ranking->base.id;
}
