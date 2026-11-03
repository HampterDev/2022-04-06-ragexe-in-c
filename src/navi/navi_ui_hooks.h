#ifndef NAVI_NAVI_UI_HOOKS_H
#define NAVI_NAVI_UI_HOOKS_H

#include <stddef.h>

/* The navigation module's side of g_uiHooks (ui/controls/ui_hooks.h): navi_navigate,
   navi_open_search and navi_show_map on the client's navigation manager g_navi. */
void navi_link_navigate(const char *map, int kind, int flags, unsigned char silent, int x, int y, int id);
void navi_link_search(const char *text, size_t size, int filter, unsigned char silent);
void navi_link_show_map(const char *map, int x, int y);

#endif
