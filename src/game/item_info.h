#ifndef GAMEMODE_ITEM_INFO_H
#define GAMEMODE_ITEM_INFO_H

#include "iteminfo/item_list.h"
#include "session/state.h"

static __inline int item_option_count(const struct ItemOption *options)
{
    int n = 0, i;

    for (i = 0; i < ITEM_OPTION_SLOTS; i++)
        if (options[i].id != 0)
            n++;
    return n;
}

struct Session;

#endif
