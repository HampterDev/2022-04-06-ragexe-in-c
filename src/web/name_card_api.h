#ifndef ASYNCWORK_NAME_CARD_API_H
#define ASYNCWORK_NAME_CARD_API_H
#include "web/name_card_types.h"

void name_card_init(struct ActorNameCard *card);
void name_card_copy_init(struct ActorNameCard *out, const struct ActorNameCard *source);
void name_card_destroy(struct ActorNameCard *card);

void name_card_holder_init(struct ActorNameCardHolder *self);
void name_card_holder_destroy(struct ActorNameCardHolder *self);
void name_card_holder_clear(struct ActorNameCardHolder *self);
void name_card_holder_erase(struct ActorNameCardHolder *self, unsigned aid);
unsigned name_card_holder_find_id(struct ActorNameCardHolder *self, struct String name);

void name_card_holder_set_all(struct ActorNameCardHolder *self, unsigned aid, struct String name,
                              struct String party, struct String guild,
                              struct String position, struct String title,
                              struct String npc_title, int group_id);
void name_card_holder_set_name(struct ActorNameCardHolder *self, unsigned aid,
                               struct String name);
void name_card_holder_set_position(struct ActorNameCardHolder *self, unsigned aid,
                                   struct String position);
void name_card_holder_set_title(struct ActorNameCardHolder *self, unsigned aid,
                                struct String title);

struct ActorNameCard *name_card_holder_get(struct ActorNameCardHolder *self, unsigned aid);
struct String *name_card_holder_name(struct ActorNameCardHolder *self, struct String *out,
                                        unsigned aid);
const char *name_card_name_text(const struct ActorNameCardHolder *self, unsigned aid,
                                size_t *length);
char name_card_holder_has(struct ActorNameCardHolder *self, unsigned aid);
void name_card_holder_update(struct ActorNameCardHolder *self);
#endif
