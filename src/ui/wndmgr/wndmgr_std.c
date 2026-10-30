#include "ui/wndmgr/wndmgr.h"

static int window_before(const void *a, const void *b)
{
    return (*(struct UIWindow *const *)a)->list_rank > (*(struct UIWindow *const *)b)->list_rank;
}

void wm_window_list_sort(struct List *windows)
{
    list_sort(windows, window_before);
}
