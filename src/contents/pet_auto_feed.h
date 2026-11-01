#ifndef CONTENTS_PET_AUTO_FEED_H
#define CONTENTS_PET_AUTO_FEED_H
#include "std/std.h"

struct PetAutoFeedMgr {
    struct Vec auto_feeding;
};
extern struct PetAutoFeedMgr *g_petAutoFeed;

char pet_auto_feed_create(void);
void pet_auto_feed_destroy(void);
char pet_auto_feed_is_allowed(struct PetAutoFeedMgr *self, int egg_id);
#endif
