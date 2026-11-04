#ifndef LOGINMODE_ZONE_H
#define LOGINMODE_ZONE_H

#include "platform/win_types.h"

#include "base.h"

extern BYTE g_trackPing;

#pragma pack(push, 1)
struct PACKET_ZC_ACCEPT_ENTER {
    unsigned short packet_type;
    unsigned start_time;
    unsigned char pos_dir[3];
    unsigned char x_size;
    unsigned char y_size;
};
struct PACKET_ZC_ACCEPT_ENTER2 {
    unsigned short packet_type;
    unsigned start_time;
    unsigned char pos_dir[3];
    unsigned char x_size;
    unsigned char y_size;
    unsigned char font;
    unsigned char font_high;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_ACCEPT_ENTER, 11);
SIZE_ASSERT(PACKET_ZC_ACCEPT_ENTER2, 13);

#endif
