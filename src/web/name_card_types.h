#ifndef WEB_NAME_CARD_TYPES_H
#define WEB_NAME_CARD_TYPES_H
#include "base.h"
#include "std/container_types.h"

struct ActorNameCard {
    struct String name;
    struct String party;
    struct String guild;
    struct String position;
    struct String achievement_title;
    struct String npc_title;
    int group_id;
    char complete;
};

struct ActorNameCardHolder {
    struct Tree cards;
    struct Vec wanted;
    struct Vec asked;
};
#endif
