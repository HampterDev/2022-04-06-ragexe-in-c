#ifndef CONTENTS_TIPS_H
#define CONTENTS_TIPS_H

#include "base.h"

struct TipFile {
    char loaded;
    struct Vec tips;
};
OFFSET_ASSERT(TipFile, tips, 0x04);
SIZE_ASSERT(TipFile, 0x10);

extern struct TipFile g_tipOfTheDay;
extern struct TipFile g_guildTips;

int tip_of_the_day_read(struct TipFile *self);
struct Vec *tip_of_the_day_get(struct TipFile *self, int index);

int guild_tips_read(struct TipFile *self);
struct Vec *guild_tips_copy(struct TipFile *self, struct Vec *out);

void tip_list_free(struct Vec *tips);

#endif
