#include "session/state.h"
#include "contents/connection_api.h"
#include "ui/social/entry_queue_state.h"
#include <stdlib.h>
#include <string.h>

struct EntryQueueMgr *g_entryQueueMgr;

#define CZ_ENTRY_QUEUE_APPLY 0x8d7
#define CZ_ENTRY_QUEUE_CANCEL 0x8da
#define CZ_ENTRY_QUEUE_ANSWER 0x8e0
#define CZ_ENTRY_QUEUE_RANKING 0x90a

#pragma pack(push, 1)
struct PacketEntryQueueApply {
    short id;
    short type;
    char name[0x18];
};
struct PacketEntryQueueName {
    short id;
    char name[0x18];
};
struct PacketEntryQueueAnswer {
    short id;
    BYTE answer;
    char name[0x18];
    char lobby_name[0x18];
};
#pragma pack(pop)
SIZE_ASSERT(PacketEntryQueueApply, 0x1c);
SIZE_ASSERT(PacketEntryQueueName, 0x1a);
SIZE_ASSERT(PacketEntryQueueAnswer, 0x33);

unsigned char entry_queue_mgr_create(void)
{
    struct EntryQueueMgr *manager;
    if (g_entryQueueMgr) return 0;
    manager = calloc(1, sizeof *manager);
    if (!manager) return 0;
    entry_queue_catalog_load(g_session1.luaState);
    g_entryQueueMgr = manager;
    return 1;
}

void entry_queue_mgr_release(void)
{
    free(g_entryQueueMgr);
    g_entryQueueMgr = NULL;
    entry_queue_catalog_clear();
}

static void send_packet(const void *packet, int size)
{
    rag_connection_send_packet(rag_connection_instance(), size, packet);
}

static void packet_name(char out[24], const char *text)
{
    size_t size = strlen(text);
    if (size > 23) size = 23;
    memcpy(out, text, size);
}

void entry_queue_mgr_apply(struct EntryQueueMgr *self, short type, const char *name)
{
    struct PacketEntryQueueApply packet = {0};
    if (!self || !name) return;
    packet.id = CZ_ENTRY_QUEUE_APPLY;
    packet.type = type;
    packet_name(packet.name, name);
    send_packet(&packet, sizeof packet);
}

void entry_queue_mgr_cancel(struct EntryQueueMgr *self)
{
    struct PacketEntryQueueName packet = {0};
    if (!self || !self->current) return;
    packet.id = CZ_ENTRY_QUEUE_CANCEL;
    packet_name(packet.name, self->current->name);
    send_packet(&packet, sizeof packet);
}

void entry_queue_mgr_request_ranking(struct EntryQueueMgr *self)
{
    struct PacketEntryQueueName packet = {0};
    if (!self || !self->current) return;
    packet.id = CZ_ENTRY_QUEUE_RANKING;
    packet_name(packet.name, self->current->name);
    send_packet(&packet, sizeof packet);
}

void entry_queue_mgr_answer(struct EntryQueueMgr *self, int answer)
{
    struct PacketEntryQueueAnswer packet = {0};
    if (!self || !self->current) return;
    packet.id = CZ_ENTRY_QUEUE_ANSWER;
    packet.answer = (unsigned char)answer;
    packet_name(packet.name, self->current->name);
    memcpy(packet.lobby_name, self->lobby_name, sizeof packet.lobby_name);
    if (answer == ENTRY_ANSWER_DECLINE) self->in_queue = 0;
    send_packet(&packet, sizeof packet);
}
