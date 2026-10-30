#ifndef CONTENTS_CLAN_INFO_H
#define CONTENTS_CLAN_INFO_H

#include "std/std.h"
#include "std/list.h"

struct ClanRelation {
    int relation;
    struct String name;
};

struct ClanInfo {
    char constructed;
    struct String name;
    struct String master;
    int level;
    int online_count;
    int member_count;
    struct String map_name;
    int emblem;
    char in_clan;
    struct List relations;
};
OFFSET_ASSERT(ClanInfo, name, 0x04);

extern struct ClanInfo *g_clanInfo;

char clan_info_create(void);
void clan_info_destroy(void);
struct ClanInfo *clan_info_construct(struct ClanInfo *self);
void clan_info_destruct(struct ClanInfo *self);
void clan_info_delete(struct ClanInfo *self);
void clan_info_clear(struct ClanInfo *self);

#endif
