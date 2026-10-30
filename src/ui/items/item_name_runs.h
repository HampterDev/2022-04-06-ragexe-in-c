#ifndef UI_ITEM_NAME_RUNS_H
#define UI_ITEM_NAME_RUNS_H

#include "std/string.h"
#include <stddef.h>

struct UIWindow;
struct ItemInfo;

struct ItemNameRuns {
    struct String text;
    size_t ends[8];
    size_t count;
    size_t marked;
    unsigned long mark_colour;
};

void item_name_runs_end(struct ItemNameRuns *runs);
void item_info_name_runs(struct UIWindow *window, const struct ItemInfo *item,
                      struct ItemNameRuns *runs, int twice_maker, int with_sockets);
int ui_window_draw_runs_shortened(struct UIWindow *window, int x, int y, int max_width,
                              const struct ItemNameRuns *runs, unsigned long colour,
                              int line_height);
#endif
