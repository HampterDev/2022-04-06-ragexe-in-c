#include "game/packet_handlers.h"
#include "ui/chat/markup_links.h"
#include "platform/service.h"
#include "game/whisper_log.h"
#include "contents/manner_filter.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "world/actor_lookup.h"
#include "contents/connection_api.h"
#include <windows.h>
#include <string.h>
#include "game/chat.h"
#include "game/client.h"
#include "platform/language.h"
#include "platform/clientinfo.h"
#include "session/session_state.h"

static struct String *chat_line(struct UIWindowMgr *mgr)
{
    return &mgr->chat_line;
}

void window_mgr_clear_chat_line(struct UIWindowMgr *mgr)
{
    string_clear(chat_line(mgr));
}

const char *window_mgr_get_chat_line(struct UIWindowMgr *mgr)
{
    return string_cstr(chat_line(mgr));
}

#define CZ_REQ_GIVE_MANNER_POINT 0x149

#pragma pack(push, 1)
struct PacketMannerPoint {
    unsigned short id;
    int account_id;
    unsigned char type;
    short points;
};
#pragma pack(pop)

void game_mode_mute_self_for_flooding(void)
{
    struct PacketMannerPoint packet;

    if (clientinfo_is_admin_aid(g_session1.ownAid) || g_serverType == 3)
        return;
    packet.id = CZ_REQ_GIVE_MANNER_POINT;
    packet.account_id = g_session1.ownAid;
    packet.type = 2;
    packet.points = 10;
    send_packet(CZ_REQ_GIVE_MANNER_POINT, &packet);
}

struct File g_whisperLog;

void game_mode_log_whisper(struct GameMode *self, const char *line)
{
    struct File old;
    unsigned size;
    char *contents;

    if (!session_is_admin_option(&g_session1, 7))
        return;
    self->whispers_logged++;
    file_construct(&old);
    file_open(&old, g_whisperLogPath, 0, 1, 0);
    size = old.size;
    contents = mem_alloc(size);
    file_read(&old, contents, size);
    file_close(&old);
    file_open(&g_whisperLog, g_whisperLogPath, FILE_MODE_WRITE, 1, 0);
    file_write(&g_whisperLog, contents, size);
    file_write(&g_whisperLog, line, strlen(line) + 1);
    file_write(&g_whisperLog, "\r\n", 2);
    file_close(&g_whisperLog);
    mem_free(contents);
    file_destruct(&old);
}

void game_mode_close_chat_log(void)
{
    if (g_session1.autoSaveChat != 0 && session_is_admin_option(&g_session1, 7))
        file_close(&g_whisperLog);
}

#define CZ_INPUT_EDITDLGSTR 0x1d5

struct GameActor;

#pragma pack(push, 1)
struct PacketInputText {
    unsigned short id;
    unsigned short len;
    int npc_id;
    char text[0x16a];
};
#pragma pack(pop)

void game_mode_send_npc_input_text(struct GameMode *self, int npc_id, const char *text,
                                          const char *override)
{
    char typed[0x16a];
    struct PacketInputText packet;
    size_t len;

    if (npc_id && !world_get_game_actor_by_aid(self->world, npc_id))
        return;
    memset(typed, 0, sizeof typed);
    _snprintf_s(typed, sizeof typed, _TRUNCATE, "%s", override ? override : text);
    if (chat_filter_is_blocked(text_view_cstr(typed)))
        return;
    len = strlen(typed);
    packet.len = (unsigned short)(len + 8);
    packet.id = CZ_INPUT_EDITDLGSTR;
    packet.npc_id = npc_id;
    memcpy(packet.text, typed, len);
    rag_connection_send_packet(rag_connection_instance(), packet.len, &packet);
}

#define CHAT_LINE_MAX           0x169
#define UIM_CHAT_LINE_ALT       5
#define ACTOR_MSG_SAY           7

#pragma pack(push, 1)
struct PACKET_ZC_NOTIFY_PLAYERCHAT {
    unsigned short id;
    unsigned short length;
    char text[];
};
#pragma pack(pop)

void game_mode_zc_notify_player_chat(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_NOTIFY_PLAYERCHAT *packet = (const struct PACKET_ZC_NOTIFY_PLAYERCHAT *)pkt;
    size_t length = packet->length - 4;
    struct Vec text;
    char *chars;

    if (length - 1 > CHAT_LINE_MAX - 1)
        return;
    vec_init(&text);
    vec_resize(&text, length + 1, 1);
    memcpy_s(text.first, (char *)text.last - (char *)text.first, packet->text, length);
    if (word_filter_contains(&g_nameFilter, text.first) == 1) {
        word_filter_censor(&g_nameFilter, text.first);
    }
    chars = text.first == text.last ? 0 : text.first;
    if (g_windowMgr.chat_room_wnd != 0) {
        ui_window_mgr_send_msg_(&g_windowMgr, UIM_CHAT_LINE_ALT, (int)chars, 0x7800, 0, 0);
    } else {
        ui_window_mgr_send_msg_(&g_windowMgr, UIM_CHAT_LINE, (int)chars, 0xff00, 1, 0);
        chars = text.first == text.last ? 0 : text.first;
        render_object_send((struct GameActor *)self->world->player, NULL, ACTOR_MSG_SAY, MSG_ARG_PTR(chars), 0, 0, 0);
    }
    if (g_session1.autoSaveChat != 0)
        game_mode_log_whisper(self, text.first == text.last ? 0 : text.first);
    vec_free(&text);
}

#define GUILD_CHAT_LENGTH_MAX   0x16d
#define GUILD_CHAT_COLOR        0xb4ffb4
#define CHAT_CHANNEL_GUILD      4

#pragma pack(push, 1)
struct PACKET_ZC_GUILD_CHAT {
    unsigned short id;
    unsigned short length;
    char text[];
};
#pragma pack(pop)

void game_mode_zc_guild_chat(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_GUILD_CHAT *packet = (const struct PACKET_ZC_GUILD_CHAT *)pkt;
    int length = packet->length;
    struct Vec text;

    if (length - 4 <= 0 || length > GUILD_CHAT_LENGTH_MAX)
        return;
    vec_init(&text);
    vec_resize(&text, length - 3, 1);
    memcpy_s(text.first, (char *)text.last - (char *)text.first, packet->text, length - 4);
    if (word_filter_contains(&g_nameFilter, text.first) == 1) {
        word_filter_censor(&g_nameFilter, text.first);
    }
    if (g_session1.hideChatAndGuild == 0) {
        ui_window_mgr_send_msg_(&g_windowMgr, UIM_CHAT_LINE, (int)text.first, GUILD_CHAT_COLOR, CHAT_CHANNEL_GUILD, 0);
        if (g_session1.autoSaveChat != 0)
            game_mode_log_whisper(self, text.first);
    }
    vec_free(&text);
}
