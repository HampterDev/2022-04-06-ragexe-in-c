#ifndef LOGINMODE_CHAR_INFO_H
#define LOGINMODE_CHAR_INFO_H

#include "platform/win_types.h"
#include "base.h"

#pragma pack(push, 1)
struct CharInfo {
    DWORD gid;
    long long exp;
    int zeny;
    long long job_exp;
    int job_level;
    int body_state;
    int health_state;
    int effect_state;
    int karma;
    int manner;
    short status_points;
    long long hp;
    long long max_hp;
    long long sp;
    long long max_sp;
    short walk_speed;
    short job;
    short head;
    short body;
    short weapon;
    short level;
    short skill_points;
    short accessory;
    short shield;
    short accessory2;
    short accessory3;
    short head_palette;
    short body_palette;
    char name[24];
    BYTE stats[6];
    BYTE slot;
    BYTE hair_color;
    short is_changed_char_name;
    char map_name[16];
    int delete_date;
    int robe;
    int slot_change_count;
    int rename_count;
    BYTE sex;
};
#pragma pack(pop)
OFFSET_ASSERT(CharInfo, job_exp, 0x10);
OFFSET_ASSERT(CharInfo, manner, 0x2c);
OFFSET_ASSERT(CharInfo, hp, 0x32);
OFFSET_ASSERT(CharInfo, job, 0x54);
OFFSET_ASSERT(CharInfo, name, 0x6c);
OFFSET_ASSERT(CharInfo, slot, 0x8a);
OFFSET_ASSERT(CharInfo, delete_date, 0x9e);
OFFSET_ASSERT(CharInfo, sex, 0xae);
SIZE_ASSERT(CharInfo, 0xaf);

struct NewCharLooks {
    int stats[6];
    int head_palette;
    int head;
};

extern struct CharInfo g_loginCharInfo;

#endif
