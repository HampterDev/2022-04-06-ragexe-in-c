#include "core/memory.h"
#include "base.h"
#include "ui/ui_std.h"
#include "ui/net/packet_crypt.h"

struct PacketKeys {
    BYTE valid;
    unsigned state;
    unsigned mul;
    unsigned add;
};

struct PacketKeys *g_packetKeys;

struct PacketKeys *packet_keys_construct(struct PacketKeys *self)
{
    self->state = 0;
    self->mul = 0;
    self->add = 0;
    self->valid = 1;
    return self;
}

BYTE packet_keys_create(void)
{
    struct PacketKeys *keys;

    if (g_packetKeys)
        return 0;
    keys = mem_try_alloc(sizeof *keys);
    if (keys)
        keys = packet_keys_construct(keys);
    g_packetKeys = keys;
    if (!keys)
        return 0;
    if (!keys->valid) {
        mem_free(keys);
        g_packetKeys = NULL;
        return 0;
    }
    return 1;
}

void packet_keys_release(void)
{
    if (g_packetKeys) {
        mem_free(g_packetKeys);
        g_packetKeys = NULL;
    }
}

int packet_keys_next(struct PacketKeys *self, int mode)
{
    if (mode == 0) {
        self->state = self->mul * self->state + self->add;
        return (self->state >> 16) & 0x7fff;
    }
    self->add = 0;
    self->state = 0;
    self->mul = 0;
    return 0;
}

struct ReassemblyPacketMgr {
    BYTE ok;
};

struct ReassemblyPacketMgr *g_reassemblyPacketMgr;

struct ReassemblyPacketMgr *reassembly_packet_mgr_construct(struct ReassemblyPacketMgr *self)
{
    self->ok = 0;
    if (packet_keys_create())
        self->ok = 1;
    return self;
}

BYTE reassembly_packet_mgr_create(void)
{
    struct ReassemblyPacketMgr *mgr;

    if (g_reassemblyPacketMgr)
        return 0;
    mgr = mem_try_alloc(sizeof *mgr);
    if (mgr)
        mgr = reassembly_packet_mgr_construct(mgr);
    g_reassemblyPacketMgr = mgr;
    if (!mgr)
        return 0;
    if (!mgr->ok) {
        packet_keys_release();
        mem_free(mgr);
        g_reassemblyPacketMgr = NULL;
        return 0;
    }
    return 1;
}

void reassembly_packet_mgr_release(void)
{
    struct ReassemblyPacketMgr *mgr = g_reassemblyPacketMgr;

    if (mgr) {
        packet_keys_release();
        mem_free(mgr);
        g_reassemblyPacketMgr = NULL;
    }
}
