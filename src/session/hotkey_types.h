#ifndef SESSION_HOTKEY_TYPES_H
#define SESSION_HOTKEY_TYPES_H

#include "base.h"

#pragma pack(push, 1)
struct HotkeySlot {
    unsigned char is_skill;
    int id;
    short level;
};
#pragma pack(pop)
SIZE_ASSERT(HotkeySlot, 7);
#define HOTKEY_SLOTS 36

#endif
