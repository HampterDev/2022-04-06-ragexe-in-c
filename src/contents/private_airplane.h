#ifndef CONTENTS_PRIVATE_AIRPLANE_H
#define CONTENTS_PRIVATE_AIRPLANE_H
#include "std/std.h"

struct PrivateAirplaneMgr {
    struct Tree startable_maps;
    struct Tree endable_maps;
    struct Tree need_items;
};
extern struct PrivateAirplaneMgr *g_airshipTickets;

char private_airplane_create(void);
void private_airplane_destroy(void);
int private_airplane_need_item(struct PrivateAirplaneMgr *self, int index);
char private_airplane_can_start(struct PrivateAirplaneMgr *self, struct String map);
char private_airplane_can_end(struct PrivateAirplaneMgr *self, struct String map);
#endif
