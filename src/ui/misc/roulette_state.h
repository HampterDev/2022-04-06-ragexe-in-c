#ifndef UI_MISC_ROULETTE_STATE_H
#define UI_MISC_ROULETTE_STATE_H

#include <windows.h>
#include "base.h"
#include "iteminfo/item_types.h"
#include "render/primitive_types.h"
#include "std/container_types.h"

enum RouletteCoin {
    ROULETTE_GOLD = 0,
    ROULETTE_SILVER = 1,
    ROULETTE_BRONZE = 2,
};

struct RoulettePrize {
    unsigned short row;
    unsigned short position;
    int item_id;
    unsigned short count;
    unsigned short zero;
};

struct PrizeEntry {
    struct RoulettePrize prize;
    struct ItemInfo item;
};

struct Roulette {
    BYTE ok;
    int serial;
    int frames;
    int rounds;
    BYTE spinning;
    int step;
    int spin_column;
    struct TexDrawVertex marker[6];
    struct TexDrawVertex start[6];
    struct TexDrawVertex end[6];
    DWORD step_time;
    DWORD interval;
    int slot;
    int addition_item_id;
    int coins[3];
    BYTE has_slot;
    BYTE slot_flag;
    struct Tree prizes;
    struct ItemInfo slot_item;
    struct ItemInfo addition_item;
};
OFFSET_ASSERT(Roulette, marker, 0x1c);
OFFSET_ASSERT(Roulette, step_time, 0x25c);
OFFSET_ASSERT(Roulette, prizes, 0x27c);

unsigned char roulette_create_instance(void);
void roulette_render(struct Roulette *self);
void run_faq_web_page(void);

#endif
