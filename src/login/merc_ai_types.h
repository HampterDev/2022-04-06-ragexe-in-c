#ifndef LOGINMODE_MERC_AI_TYPES_H
#define LOGINMODE_MERC_AI_TYPES_H

#include "platform/win_types.h"
#include "base.h"

struct lua_State;

struct MercenaryAI {
    struct lua_State *L;
    DWORD startTick;
    DWORD duration;
    int kind;
    DWORD lastRun;
};

#endif
