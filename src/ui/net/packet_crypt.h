#ifndef UI_NET_PACKET_CRYPT_H
#define UI_NET_PACKET_CRYPT_H

#include <windows.h>

struct PacketKeys;

BYTE reassembly_packet_mgr_create(void);
void reassembly_packet_mgr_release(void);
int packet_keys_next(struct PacketKeys *self, int mode);

extern struct PacketKeys *g_packetKeys;

#endif
