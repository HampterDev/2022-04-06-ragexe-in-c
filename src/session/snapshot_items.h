#ifndef SESSION_SNAPSHOT_ITEMS_H
#define SESSION_SNAPSHOT_ITEMS_H
#include "replay/snapshot.h"
struct Session;

int session_items_save(const struct Session *session, struct ReplayBuffer *out);
int session_items_load(struct Session *session, const void *data, size_t size);
#endif
