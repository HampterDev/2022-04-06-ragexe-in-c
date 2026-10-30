#ifndef CONTENTS_TOWN_INFO_H
#define CONTENTS_TOWN_INFO_H

#include "std/std.h"

struct TownInfo {
    int kind;
    struct String name;
    int x;
    int y;
    struct String icon;
};

struct TownInfoMgr;
extern struct TownInfoMgr *g_townInfo;

char town_info_create(void);
void town_info_destroy(void);
char town_info_load(void);

void town_info_add(struct TownInfoMgr *self, struct String map, struct String name,
                   int x, int y, int kind);
struct TownInfo *town_info_get(struct TownInfoMgr *self, struct TownInfo *out,
                               struct String map, unsigned index);

#endif
