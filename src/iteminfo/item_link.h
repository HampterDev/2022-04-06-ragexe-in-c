#ifndef ITEMINFO_ITEM_LINK_H
#define ITEMINFO_ITEM_LINK_H

#include "core/text_view.h"
struct ItemInfo;

int item_link_read(struct ItemInfo *item, struct TextView text);

struct ItemLinkSpan {
    size_t begin, end;
    struct TextView payload;
};

int item_link_next(struct TextView text, size_t *cursor, struct ItemLinkSpan *span);

#endif
