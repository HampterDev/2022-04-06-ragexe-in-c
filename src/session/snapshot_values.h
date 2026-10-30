#ifndef SESSION_SNAPSHOT_VALUES_H
#define SESSION_SNAPSHOT_VALUES_H
#include "replay/snapshot.h"
struct Session;

enum SessionValueSnapshot {
    SESSION_VALUES_IDENTITY,
    SESSION_VALUES_CHARACTER,
    SESSION_VALUES_COMPANIONS,
    SESSION_VALUES_DUNGEON,
    SESSION_VALUES_HOTKEYS
};

int session_values_save(const struct Session *session, enum SessionValueSnapshot kind,
                        struct ReplayBuffer *out);
int session_values_validate(enum SessionValueSnapshot kind, const void *data, size_t size);
int session_values_load(struct Session *session, enum SessionValueSnapshot kind,
                        const void *data, size_t size);
#endif
