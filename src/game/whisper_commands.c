#include "contents/whisper_block.h"
#include "game/talk_type.h"
#include "game/client.h"
#include <string.h>
#include "core/thread_text.h"
#include "core/memory.h"
#include "core/fatal.h"
#include "render/math3d.h"
#include "base.h"

#define CZ_SETTING_WHISPER_PC 0xcf
#define CZ_SETTING_WHISPER_STATE 0xd0

#pragma pack(push, 1)
struct PacketWhisperPc {
    unsigned short id;
    char name[24];
    unsigned char unblock;
};
struct PacketWhisperState {
    unsigned short id;
    unsigned char unblock_all;
};
#pragma pack(pop)

void talk_cmd_whisper_block(struct GameMode *self, const struct String *name, int unblock)
{
    struct PacketWhisperPc packet;

    string_assign(&self->whisper_setting_name, string_cstr(name), name->size);
    if (self->whisper_setting_name.size > 24)
        return;
    if (unblock)
        whisper_block_remove(g_whisperBlockMgr, string_cstr(&self->whisper_setting_name));
    else if (!whisper_block_add(g_whisperBlockMgr, string_cstr(&self->whisper_setting_name)))
        return;
    packet.id = CZ_SETTING_WHISPER_PC;
    _snprintf_s(packet.name, sizeof packet.name, (size_t)-1, "%s", string_cstr(&self->whisper_setting_name));
    packet.unblock = (unsigned char)unblock;
    send_packet(packet.id, &packet);
}

void talk_cmd_whisper_block_all(struct GameMode *self, int unblock)
{
    struct PacketWhisperState packet;

    (void)self;
    if (unblock)
        whisper_block_clear(g_whisperBlockMgr);
    packet.unblock_all = (unsigned char)unblock;
    packet.id = CZ_SETTING_WHISPER_STATE;
    send_packet(CZ_SETTING_WHISPER_STATE, &packet);
}

void game_mode_reblock_whispers(struct GameMode *self)
{
    size_t i;
    for (i = 0; i < whisper_block_count(g_whisperBlockMgr); ++i) {
        struct String name;
        const char *blocked = whisper_block_name(g_whisperBlockMgr, i);
        string_init(&name);
        string_assign_cstr(&name, blocked);
        game_mode_process_talk_type(self, 0x12, &name);
        string_free(&name);
    }
}
