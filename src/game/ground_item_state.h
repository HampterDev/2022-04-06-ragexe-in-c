#ifndef GAMEMODE_GROUND_ITEM_STATE_H
#define GAMEMODE_GROUND_ITEM_STATE_H
#include "actor/types.h"

struct Item {
    struct AbleToMakeEffect base;
    struct String name;
    int item_id_short;
    unsigned char identified;
    int item_id;
    int aid;
    int falling;
    float velocity;
    float base_y;
    unsigned short type;
    unsigned short drop_effect_mode;
    int count;
};

struct Item *item_create(void);
struct Item *item_construct(struct Item *item);
#endif
