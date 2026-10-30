#ifndef NAVI_REPLAY_EVENTS_H
#define NAVI_REPLAY_EVENTS_H
#include "replay/event.h"
#include <stddef.h>
#pragma pack(push, 1)
struct NavigationRouteEvent {
    struct ReplayEventHeader header;
    char map_name[30];
    int32_t kind;
    uint8_t flags, silent;
    int32_t x, y, id;
};
struct NavigationMapEvent {
    struct ReplayEventHeader header;
    char map_name[30];
    int32_t x, y;
};
#pragma pack(pop)
int navi_record_route(const char *map_name, size_t size, int kind, int flags,
        unsigned char silent, int x, int y, int id);
int navi_record_map(const char *map_name, size_t size, int x, int y);
#endif
