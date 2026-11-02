#ifndef EFFECTS_EZ2STR_EFFECT_MGR_H
#define EFFECTS_EZ2STR_EFFECT_MGR_H

#include "base.h"
#include "std/container_types.h"

#define EZ2STR_EX_FIRST_ID 10000

#define EZ2STR_EX_INFINITE    0x1
#define EZ2STR_EX_FLOOR       0x2
#define EZ2STR_EX_SCREEN_FILL 0x4
#define EZ2STR_EX_SOUND_LOOP  0x8

struct EZ2STREntry {
    int index;
    struct String file_path;
    float pos_x, pos_y;
    unsigned flags;
    struct String sound_path;
    int quake_delay;
    int start_delay;
};
OFFSET_ASSERT(EZ2STREntry, file_path, 0x04);

struct EZ2STRRow {
    struct String name;
    struct EZ2STREntry entry;
};

struct Ez2StrEffectMgr {
    struct Tree rows;
};

struct Ez2StrEffectMgr *ez2str_effect_mgr_instance(void);
void ez2str_effect_mgr_destroy(void);

unsigned char ez2_str_effect_mgr_load(struct Ez2StrEffectMgr *self);
const struct EZ2STREntry *ez2_str_effect_mgr_find_by_effect_id(struct Ez2StrEffectMgr *self, int effect_id);
struct Vec *ez2_str_effect_mgr_get_effect_ids(struct Ez2StrEffectMgr *self, struct Vec *ids);

#endif
