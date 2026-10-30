#ifndef SESSION_SNAPSHOT_PARTY_H
#define SESSION_SNAPSHOT_PARTY_H
#include "replay/snapshot.h"
struct Session;

int session_party_save(const struct Session *session, struct ReplayBuffer *out);
int session_party_load(struct Session *session, const void *data, size_t size);
#endif
