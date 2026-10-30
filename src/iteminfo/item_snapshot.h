#ifndef ITEMINFO_ITEM_SNAPSHOT_H
#define ITEMINFO_ITEM_SNAPSHOT_H
#include "replay/snapshot.h"
struct ItemInfo;

void item_snapshot_write(struct SnapshotWriter *writer, const struct ItemInfo *item);
int item_snapshot_validate(const void *data, size_t size);
int item_snapshot_load(struct ItemInfo *item, const void *data, size_t size);
#endif
