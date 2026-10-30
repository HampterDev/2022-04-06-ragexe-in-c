#include "game/replay_navigation.h"
#include "replay/event.h"
#include "navi/snapshot.h"
#include "navi/navi.h"
#include "navi/navigation.h"
#include "navi/replay_events.h"
#include "ui/navigation_view.h"
#include "ui/window_mgr.h"
#include <string.h>

int game_replay_navigation_results(const void *data, size_t size)
{
    struct ReplayEventHeader header;
    if (!data || size < sizeof header) return 0;
    memcpy(&header, data, sizeof header);
    if (header.id != 0x8ae || header.kind != 0x38f || header.size != size ||
        !navi_results_load(&g_navi, (const unsigned char *)data + sizeof header, size - sizeof header)) return 0;
    ui_window_mgr_post_quit1(&g_windowMgr, 0x1a);
    ui_navigation_refresh_results();
    return 1;
}

int game_replay_navigation_route(const void *data, size_t size)
{
    struct NavigationRouteEvent event;
    char map_name[sizeof event.map_name + 1];
    if (!data || size != sizeof event) return 0;
    memcpy(&event, data, sizeof event);
    if (event.header.id != 0x8ae || event.header.kind != 0x38e || event.header.size != size) return 0;
    memcpy(map_name, event.map_name, sizeof event.map_name);
    map_name[sizeof event.map_name] = 0;
    return navi_navigate(&g_navi, map_name, event.kind, event.flags, event.silent, event.x, event.y, event.id);
}

int game_replay_navigation_map(const void *data, size_t size)
{
    struct NavigationMapEvent event;
    char map_name[sizeof event.map_name + 1];
    if (!data || size != sizeof event) return 0;
    memcpy(&event, data, sizeof event);
    if (event.header.id != 0x8ae || event.header.kind != 0x390 || event.header.size != size) return 0;
    memcpy(map_name, event.map_name, sizeof event.map_name);
    map_name[sizeof event.map_name] = 0;
    return navi_show_map(&g_navi, map_name, event.x, event.y);
}
