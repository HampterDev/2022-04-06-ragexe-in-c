#include "contents/whisper_block.h"
#include "contents/connection_api.h"
#include "game/talk_type.h"
#include <assert.h>
#include <string.h>

extern int attempts, fail_at, live;
static int packets, calls;
static unsigned char last_packet[27];
static const char *reblocked[8];

struct RagConnection *rag_connection_instance(void) { return (struct RagConnection *)&packets; }
int rag_connection_get_packet_length(struct RagConnection *connection, int id)
{
    assert(connection == rag_connection_instance());
    assert(id == 0xcf || id == 0xd0);
    return id == 0xcf ? 27 : 3;
}
char rag_connection_send_packet(struct RagConnection *connection, int length, const void *packet)
{
    assert(connection == rag_connection_instance());
    assert(length == 27 || length == 3);
    ++packets; memcpy(last_packet, packet, length); return 1;
}
void game_mode_process_talk_type(struct GameMode *mode, int type, struct String *args)
{
    assert(type == TALK_WHISPER_BLOCK && calls < 8);
    assert(!strcmp(string_cstr(args), reblocked[calls++]));
    talk_cmd_whisper_block(mode, args, 0);
}
void test_whisper_commands(void)
{
    static struct GameMode mode;
    struct WhisperBlockMgr manager = {0};
    struct String name;
    int before;
    g_whisperBlockMgr = &manager;
    string_init(&mode.whisper_setting_name); string_init(&name);
    string_assign_cstr(&name, "Bob");
    attempts = 0; fail_at = 1;
    talk_cmd_whisper_block(&mode, &name, 0);
    assert(!packets && !whisper_block_count(&manager));
    fail_at = 0;
    talk_cmd_whisper_block(&mode, &name, 0);
    assert(packets == 1 && last_packet[0] == 0xcf && !last_packet[1]);
    assert(!strcmp((const char *)last_packet + 2, "Bob") && !last_packet[26]);
    assert(whisper_block_contains(&manager, "Bob"));
    string_assign_cstr(&name, "Alice"); talk_cmd_whisper_block(&mode, &name, 0);
    reblocked[0] = "Alice"; reblocked[1] = "Bob";
    game_mode_reblock_whispers(&mode);
    assert(calls == 2 && packets == 4);
    talk_cmd_whisper_block(&mode, &name, 1);
    assert(!whisper_block_contains(&manager, "Alice") && last_packet[26] == 1);
    before = packets;
    string_assign_cstr(&name, "1234567890123456789012345");
    talk_cmd_whisper_block(&mode, &name, 0);
    assert(packets == before && mode.whisper_setting_name.size == 25);
    string_assign_cstr(&name, "123456789012345678901234");
    talk_cmd_whisper_block(&mode, &name, 0);
    assert(packets == before + 1 && whisper_block_contains(&manager, "123456789012345678901234"));
    assert(!memcmp(last_packet + 2, "12345678901234567890123\0", 24));
    talk_cmd_whisper_block_all(&mode, 0); assert(whisper_block_count(&manager) == 2);
    talk_cmd_whisper_block_all(&mode, 1);
    assert(!whisper_block_count(&manager) && last_packet[0] == 0xd0 && last_packet[2] == 1);
    whisper_block_reset(&manager);
    string_free(&name); string_free(&mode.whisper_setting_name);
    g_whisperBlockMgr = NULL; assert(!live);
}
