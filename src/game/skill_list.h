#ifndef GAMEMODE_SKILL_LIST_H
#define GAMEMODE_SKILL_LIST_H

#include "game/packet_handlers.h"
#include "game/gamemode.h"
#include "skills/skill_info.h"

#pragma pack(push, 1)
struct SkillListEntry {
    unsigned short id;
    int type;
    short level;
    short sp;
    short range;
    unsigned char upgradable;
    unsigned short level2;
};
#pragma pack(pop)
SIZE_ASSERT(SkillListEntry, 15);

#pragma pack(push, 1)
struct SkillListEntryV1 {
    unsigned short id;
    int type;
    unsigned short level;
    unsigned short sp;
    unsigned short range;
    char name[24];
    unsigned char upgradable;
};
struct PACKET_ZC_SKILLINFO_LIST {
    short id;
    unsigned short length;
    struct SkillListEntryV1 skills[];
};
#pragma pack(pop)
SIZE_ASSERT(SkillListEntryV1, 37);
SIZE_ASSERT(PACKET_ZC_SKILLINFO_LIST, 4);

enum {
    HEADER_ZC_SKILLINFO_LIST = 0x10f,
    HEADER_ZC_HOSKILLINFO_LIST = 0x235,
    HEADER_ZC_MER_SKILLINFO_LIST = 0x29d,
    HEADER_ZC_SKILLINFO_LIST3 = 0xb32,
};

void game_mode_skill_list(int packet_id, const struct Vec *entries);

struct PACKET_ZC_SKILLINFO_UPDATE;
struct PACKET_ZC_SKILLINFO_UPDATE2;
struct PACKET_ZC_SKILL_SELECT_REQUEST;

#endif
