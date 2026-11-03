#include "navi/navi_ui_hooks.h"
#include "navi/navi_manager.h"
#include "navi/navigation.h"

void navi_link_navigate(const char *map, int kind, int flags, unsigned char silent, int x, int y, int id)
{
    navi_navigate(&g_navi, map, kind, flags, silent, x, y, id);
}

void navi_link_search(const char *text, size_t size, int filter, unsigned char silent)
{
    navi_open_search(&g_navi, text, size, filter, silent);
}

void navi_link_show_map(const char *map, int x, int y)
{
    navi_show_map(&g_navi, map, x, y);
}
