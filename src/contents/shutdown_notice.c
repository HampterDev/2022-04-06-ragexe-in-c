#include <stdio.h>
#include <string.h>
#include <time.h>
#include "contents/shutdown_notice.h"
#include "contents/schedule.h"
#include "contents/client_res.h"
#include "ui/window_messages.h"
#include "ui/window_manager_state.h"
#include "core/msg_str.h"

int shutdown_notice_receive(const void *packet, size_t size)
{
    const unsigned char *bytes = packet;
    char text[0x400];
    __time32_t end, now;
    struct tm local;
    struct PartialShutDownSchedule *job;
    if (!bytes || size < 10) return 0;
    memcpy(&end, bytes + 2, sizeof end);
    memcpy(&now, bytes + 6, sizeof now);
    if (_localtime32_s(&local, &end) != 0) return 0;
    snprintf(text, sizeof text, msg_str(0x92c), local.tm_year + 1900, local.tm_mon + 1,
             local.tm_mday, local.tm_hour, local.tm_min, local.tm_sec);
    ui_window_mgr_message_box(&g_windowMgr, text, 0, 1, 1, 0, "메시지", 0x118, 0x78, 0);
    job = partial_shutdown_create(now, 3600, SCHEDULE_REPEAT, end);
    if (job && !time_scheduler_add(g_timeScheduler, &job->base)) schedule_free(&job->base);
    return 1;
}
