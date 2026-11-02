#ifndef UI_SHOPS_SHOPS_STD_H
#define UI_SHOPS_SHOPS_STD_H

#include "ui/ui_std.h"
#include <stdint.h>

struct String;

struct MerchantLogEntry {
    struct String name;
    struct String item;
    int32_t time;
    int char_id;
};
void merchant_log_entry_destruct(struct MerchantLogEntry *e);

#endif
