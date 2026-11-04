#include "platform/win_types.h"
#include "contents/schedule.h"
#include "contents/connection_api.h"
#include "session/state.h"
#include "ui/window_access.h"
#include "game/schedules.h"
#include "platform/clock.h"

static void ping_run(struct Schedule *self, int now)
{
    unsigned ticks;
    unsigned short packet = 0xb1c;
    self->last_run = now;
    if (!connection_is_open(rag_connection_transport(rag_connection_instance()))) {
        self->alive = 0;
        return;
    }
    ticks = platform_ticks_ms();
    if ((unsigned)(ticks - g_session1.lastPacketTime) > 60000u) {
        self->alive = 0;
        connection_disconnect(rag_connection_transport(rag_connection_instance()));
        ui_window_open(0x14c);
        return;
    }
    if ((unsigned)(ticks - g_session1.lastSendTime) > 10000u)
        rag_connection_send_packet(rag_connection_instance(), sizeof packet, &packet);
}
static const struct ScheduleOps ping_ops = {ping_run, schedule_free};

void game_mode_add_schedules(void)
{
    int32_t now;
    struct Schedule *job;
    now = platform_time_now();
    job = schedule_create(now, 5, SCHEDULE_REPEAT, &ping_ops);
    if (!job) return;
    if (!time_scheduler_add(g_timeScheduler, job)) { schedule_free(job); return; }
    g_session1.gameModeScheduleId = job->id;
}
