#ifndef GAMEMODE_USER_CONFIG_H
#define GAMEMODE_USER_CONFIG_H

#include "base.h"

#pragma pack(push, 1)
struct UserConfigEntry {
    int type;
    int value;
};
#pragma pack(pop)
SIZE_ASSERT(UserConfigEntry, 8);

#endif
