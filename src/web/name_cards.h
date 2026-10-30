#ifndef ASYNCWORK_NAME_CARDS_H
#define ASYNCWORK_NAME_CARDS_H
#include "contents/connection_api.h"
#include "web/asyncwork.h"
#include "web/name_card_api.h"

struct NameCardEntry {
    unsigned aid;
    struct ActorNameCard card;
};

struct NameCardRequest {
    unsigned aid;
    unsigned sent;
};
SIZE_ASSERT(NameCardRequest, 8);

extern struct ActorNameCard g_emptyNameCard;
extern INIT_ONCE g_emptyNameCardGuard;
void name_card_empty_atexit(void);

#endif
