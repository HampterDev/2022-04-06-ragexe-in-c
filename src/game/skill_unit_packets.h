#ifndef GAMEMODE_SKILL_UNIT_PACKETS_H
#define GAMEMODE_SKILL_UNIT_PACKETS_H
#include "base.h"
#pragma pack(push, 1)
struct PACKET_ZC_SKILL_ENTRY {
    unsigned short id;
    unsigned aid;
    unsigned creator_aid;
    short x, y;
    unsigned char job;
    unsigned char visible;
};
struct PACKET_ZC_SKILL_ENTRY2 {
    unsigned short id;
    unsigned aid;
    unsigned creator_aid;
    short x, y;
    unsigned char job;
    unsigned char visible;
    unsigned char has_text;
    char text[80];
};
struct PACKET_ZC_SKILL_ENTRY3 {
    unsigned short id;
    short length;
    unsigned aid;
    unsigned creator_aid;
    short x, y;
    unsigned char job;
    signed char range;
    unsigned char visible;
    char text[1];
};
struct PACKET_ZC_SKILL_ENTRY4 {
    unsigned short id;
    short length;
    unsigned aid;
    unsigned creator_aid;
    short x, y;
    int job;
    signed char range;
    unsigned char visible;
    char text[1];
};
struct PACKET_ZC_SKILL_UPDATE {
    unsigned short id;
    unsigned aid;
};
struct PACKET_ZC_SKILL_ENTRY5 {
    unsigned short id;
    short length;
    unsigned aid;
    unsigned creator_aid;
    unsigned short x, y;
    int job;
    signed char range;
    unsigned char visible;
    unsigned char level;
    char text[1];
};
#pragma pack(pop)
OFFSET_ASSERT(PACKET_ZC_SKILL_ENTRY2, text, 0x11);
OFFSET_ASSERT(PACKET_ZC_SKILL_ENTRY3, text, 0x13);
OFFSET_ASSERT(PACKET_ZC_SKILL_ENTRY4, text, 0x16);
OFFSET_ASSERT(PACKET_ZC_SKILL_ENTRY5, text, 0x17);

SIZE_ASSERT(PACKET_ZC_SKILL_ENTRY, 16);
SIZE_ASSERT(PACKET_ZC_SKILL_ENTRY2, 97);
#endif
