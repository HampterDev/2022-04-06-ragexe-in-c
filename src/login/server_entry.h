#ifndef LOGIN_SERVER_ENTRY_H
#define LOGIN_SERVER_ENTRY_H
#include "base.h"
#pragma pack(push, 1)
struct LoginServerEntry {
    DWORD ip;
    WORD port;
    char name[20];
    WORD usercount;
    WORD state;
    WORD isNew;
    BYTE host[0x80];
};
SIZE_ASSERT(LoginServerEntry, 0xa0);
OFFSET_ASSERT(LoginServerEntry, usercount, 0x1a);
OFFSET_ASSERT(LoginServerEntry, host, 0x20);

#pragma pack(pop)
#endif
