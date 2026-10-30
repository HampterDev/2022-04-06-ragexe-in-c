#ifndef LOGINMODE_MERC_AI_TYPES_H
#define LOGINMODE_MERC_AI_TYPES_H

#include <windows.h>
#include "base.h"

struct lua_State;

struct MercenaryAI {
    struct lua_State *L;
    DWORD startTick;
    DWORD duration;
    int kind;
    DWORD lastRun;
};
SIZE_ASSERT(MercenaryAI, 0x14);

#endif
