#include "platform/language.h"
#include "platform/language_api.h"
#include "iteminfo/item_api.h"
#include "navi/navi_manager.h"
#ifndef UI_QUESTS_TAG_LIST_BOX_H
#define UI_QUESTS_TAG_LIST_BOX_H

#include "ui/quests/quests.h"
#include "ui/ui_std.h"

#define TAG_KIND_NAVI   0x1ae
#define TAG_KIND_ITEM   0x1c8
#define TAG_KIND_ITEML  0x1ed

struct TagPrototype {
    int id;
    struct String open;
    struct String close;
};

extern struct Vec g_tagPrototypes;

struct Vec *tag_prototypes_construct(struct Vec *table);

struct NaviManager;
#define WND_ITEM_DESCRIBE 0xc
#define ITEM_DESCRIBE_MSG_SHOW 0x18
struct Language;

typedef unsigned char (*StdSortLessFn)(const void *a, const void *b, void *pred);

void tag_prototypes_free(struct Vec *v);

#endif
