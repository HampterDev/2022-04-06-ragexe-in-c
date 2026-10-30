#ifndef NAVI_NAVIGATION_H
#define NAVI_NAVIGATION_H
#include <stddef.h>
struct NaviManager;
int navi_navigate(struct NaviManager *manager, const char *map_name,
        int kind, int flags, unsigned char silent, int x, int y, int id);
int navi_show_map(struct NaviManager *manager, const char *map_name, int x, int y);
int navi_open_search(struct NaviManager *manager, const char *text, size_t size,
        int filter, unsigned char silent);
#endif
