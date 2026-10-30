#ifndef ASYNCWORK_NAME_CARD_SNAPSHOT_H
#define ASYNCWORK_NAME_CARD_SNAPSHOT_H
#include <stddef.h>
struct ActorNameCardHolder;
struct ReplayBuffer;

int name_cards_snapshot_save(const struct ActorNameCardHolder *holder, struct ReplayBuffer *out);
int name_cards_snapshot_valid(const void *data, size_t size);
int name_cards_snapshot_load(struct ActorNameCardHolder *holder, const void *data, size_t size);
#endif
