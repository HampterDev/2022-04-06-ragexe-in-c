#ifndef UI_SOCIAL_ENTRY_QUEUE_SNAPSHOT_H
#define UI_SOCIAL_ENTRY_QUEUE_SNAPSHOT_H
#include "replay/snapshot.h"
struct EntryQueueMgr;
int entry_queue_snapshot_save(const struct EntryQueueMgr *manager, struct ReplayBuffer *out);
int entry_queue_snapshot_load(struct EntryQueueMgr *manager, const void *data, size_t size);
#endif
