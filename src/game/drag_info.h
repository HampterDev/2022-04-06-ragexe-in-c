#ifndef GAMEMODE_DRAG_INFO_H
#define GAMEMODE_DRAG_INFO_H

#include "base.h"
#include "iteminfo/item_types.h"
#include <stdint.h>

struct UIWindow;

struct DragInfo {
    int source;
    int index;
    int price;
    int charged_price;
    int drop_count;
    int count;
    int type;
    int refine;
    char identified;
    struct String display_name;
    struct String name;
    struct String name2;
    int level;
    int slots[4];
    char named;
    struct UIWindow *drop_window;
    int option_count;
    struct ItemOption options[5];
    int shop_index;
    char by_index;
    int equip_location;
    int64_t view;
};

#endif
