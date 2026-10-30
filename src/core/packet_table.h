#ifndef CORE_PACKET_TABLE_H
#define CORE_PACKET_TABLE_H

#include "base.h"
#include "std/std.h"

#define PACKET_ID_LIMIT   10000
#define PACKET_VARIABLE   (-1)

struct PacketLength {
    int size;
    int min_size;
    int recorded;
};
SIZE_ASSERT(PacketLength, 0x0c);

struct PacketLengthEntry {
    int id;
    struct PacketLength length;
};

struct PacketTable {
    struct Tree lengths;
    const char *names[PACKET_ID_LIMIT];
};
OFFSET_ASSERT(PacketTable, names, 0x08);
SIZE_ASSERT(PacketTable, 0x9c48);

enum {
    PACKET_KIND_UNKNOWN = -1,
    PACKET_KIND_VARIABLE = 0,
    PACKET_KIND_FIXED = 1
};
struct PacketLengthInfo {
    int kind;
    int length;
};
SIZE_ASSERT(PacketLengthInfo, 8);

extern struct PacketTable g_packetLengthMap;

struct PacketTable *packet_table_construct(struct PacketTable *self);
void packet_table_destruct(struct PacketTable *self);
struct PacketLengthInfo *packet_table_get_length(struct PacketTable *self,
                                                          struct PacketLengthInfo *out, int id);
int packet_length_lookup(struct PacketTable *self, int id);

#pragma pack(push, 1)
struct PACKET_HEADER {
    unsigned short packet_type;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_HEADER, 2);

#endif
