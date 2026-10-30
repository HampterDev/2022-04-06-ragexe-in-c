#ifndef CONTENTS_MCSTORE_BACKUP_TYPES_H
#define CONTENTS_MCSTORE_BACKUP_TYPES_H

#include <stdint.h>
#include "core/owned_text.h"

enum { MCSTORE_OPTION_COUNT = 5, MCSTORE_CARD_COUNT = 4 };
struct McStoreOption {
    uint16_t id;
    int16_t value;
    uint8_t param;
};

struct McStoreItem {
    int id, count, price, refine, identified;
    int slot[MCSTORE_CARD_COUNT];
    int option_count;
    struct McStoreOption options[MCSTORE_OPTION_COUNT];
};

struct McStoreBackup {
    int loaded;
    int saves_pending;
    int save_type;
    struct OwnedText save_url;
    struct OwnedText load_url;
    struct OwnedText title;
    struct McStoreItem *items;
    size_t item_count;
};

#endif
