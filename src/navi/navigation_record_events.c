#include "navi/replay_events.h"
#include "replay/replay.h"
#include <string.h>
SIZE_ASSERT(NavigationRouteEvent, 54);
SIZE_ASSERT(NavigationMapEvent, 44);

static int map_name_valid(struct ReplayRecorder *recorder, const char *name, size_t size)
{
    if (!name || size > sizeof ((struct NavigationRouteEvent *)0)->map_name) {
        recorder->writeFailed = 1;
        return 0;
    }
    return 1;
}

int navi_record_route(const char *name, size_t size, int kind, int flags,
        unsigned char silent, int x, int y, int id)
{
    struct ReplayRecorder *recorder = replay_recorder_instance();
    struct NavigationRouteEvent event = {{0x8ae, 0x38e, sizeof event}};
    if (recorder->state != 2) return 0;
    if (!map_name_valid(recorder, name, size)) return -1;
    memcpy(event.map_name, name, size);
    event.kind = kind; event.flags = (uint8_t)flags; event.silent = silent;
    event.x = x; event.y = y; event.id = id;
    return replay_recorder_write_packet(recorder, sizeof event, &event, REPLAY_TIME_ZERO, 0) ? (int)sizeof event : -1;
}

int navi_record_map(const char *name, size_t size, int x, int y)
{
    struct ReplayRecorder *recorder = replay_recorder_instance();
    struct NavigationMapEvent event = {{0x8ae, 0x390, sizeof event}};
    if (recorder->state != 2) return 0;
    if (!map_name_valid(recorder, name, size)) return -1;
    memcpy(event.map_name, name, size);
    event.x = x; event.y = y;
    return replay_recorder_write_packet(recorder, sizeof event, &event, REPLAY_TIME_ZERO, 0) ? (int)sizeof event : -1;
}
