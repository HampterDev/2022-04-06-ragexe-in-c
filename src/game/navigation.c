#include "navi/navi_manager.h"
#include <string.h>
#include "game/navigation.h"

static unsigned char navigation_flags(int digits)
{
    unsigned char flags = digits >= 100;
    int rest = digits >= 100 ? digits - 100 : digits;

    if (rest >= 10) {
        flags += 2;
        rest -= 10;
    }
    return rest >= 1 ? (unsigned char)(flags + 4) : flags;
}

void game_mode_on_navigation_active(struct GameMode *self, const struct PacketNavigationActive *packet)
{
    char map_name[sizeof packet->map_name + 1];
    (void)self;
    if (!packet) return;
    memcpy(map_name, packet->map_name, sizeof packet->map_name);
    map_name[sizeof packet->map_name] = 0;
    navi_navigate(&g_navi, map_name, packet->type, navigation_flags(packet->flag), packet->hide_window,
                  packet->x, packet->y, packet->mob_id);
}
