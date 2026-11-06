#include "ui/chat/markup_links.h"
#include "std/string.h"
#include "session/character_name.h"
#include "game/replay_actor_events.h"
#include "contents/manner_filter.h"
#include "contents/guild_info.h"
#include "ui/window_manager_state.h"
#include <windows.h>
#include <mmsystem.h>
#include <string.h>
#include "game/mode_msg.h"
#include "game/chat.h"
#include "contents/clan_info.h"
#include "platform/language.h"
#include "contents/repute_wnd.h"
#include "session/session_jobs.h"
#include "session/session_names.h"

#define CZ_WHISPER              0x96
#define CZ_REQUEST_CHAT_PARTY   0x108
#define CZ_GUILD_CHAT           0x17e
#define CZ_CHOPOKGI             0x1ed
#define CZ_BATTLEFIELD_CHAT     0x2db
#define CZ_CLAN_CHAT            0x98d
#define CZ_REQUEST_CHAT         0xf3

#define CHAT_LINE_MAX 0x169
#define COLOR_TOO_FAST 0x6e96ff

#pragma pack(push, 1)
struct ChatPacket {
    unsigned short id;
    unsigned short len;
    char text[0x400 - 4];
};
#pragma pack(pop)

static const char *chat_line(void)
{
    return window_mgr_get_chat_line(&g_windowMgr);
}

static void filter_bad_words(char *line)
{
    if (!word_filter_contains(&g_nameFilter, line))
        return;

    word_filter_censor(&g_nameFilter, line);

    string_assign_cstr(&g_windowMgr.chat_line, line);
}

static void prepare_line(char *line)
{
    strcpy(line, chat_line());
    filter_bad_words(line);
}

static int take_chat_line(char *line)
{
    if (!strlen(chat_line()))
        return 0;
    if (session_has_berserk(&g_session1) == 1)
        return 0;
    prepare_line(line);
    return 1;
}

static int send_chat_line(int packet_id)
{
    char text[CHAT_LINE_MAX];
    const char *line, *charset;
    struct ChatPacket packet;
    size_t len;

    if (chat_filter_is_blocked(text_view_cstr(chat_line())))
        return 0;
    memset(text, 0, sizeof text);
    line = chat_line();
    charset = language_charset_tag(g_language, 0);
    _snprintf_s(text, sizeof text, (size_t)-1, "%s : %s%s", session_character_name(&g_session1), charset, line);
    window_mgr_clear_chat_line(&g_windowMgr);
    len = strlen(text);
    packet.id = (unsigned short)packet_id;
    packet.len = (unsigned short)(len + 4);
    memcpy_s(packet.text, len, text, len);
    send_raw(packet.len, &packet);
    return 0;
}

static int check_history_full(struct GameMode *self)
{
    char phrase1[0x100], phrase2[0x100], phrase3[0x100];
    const char *(*msg)(int);
    const char *m317, *m318, *name;
    int lines, k;

    if (timeGetTime() - self->chat_history_time[0] < 10000 && timeGetTime() - self->chat_history_time[0] != 0) {
        game_mode_mute_self_for_flooding();
        self->flood_count = 0;
        return 1;
    }
    if (timeGetTime() - self->chat_history_time[0] < 30000 && timeGetTime() - self->chat_history_time[0] != 0) {
        lines = self->chat_history_count;
        for (k = 0; k < lines; k++) {
            if (self->chat_history[k].size < 20)
                break;
            if (k == lines - 1) {
                chat_print("TOO FAST CHATTING!", COLOR_TOO_FAST);
                game_mode_mute_self_for_flooding();
                self->flood_count = 0;
                break;
            }
        }
    }
    if (!session_is_any_super_novice_job(&g_session1, session_get_drawn_job(&g_session1)))
        return 0;
    msg = g_serviceType == 0 ? msg_str : msg_str_to_compare;
    sprintf(phrase1, msg(0x316));
    m318 = msg(0x318);
    name = session_character_name(&g_session1);
    m317 = msg(0x317);
    sprintf(phrase2, "%s %s%s", m317, name, m318);
    sprintf(phrase3, msg(0x319));
    if (!string_equal_cstr(&self->chat_history[9], phrase3)
        || !string_equal_cstr(&self->chat_history[7], phrase1)
        || self->world->player->state == 3)
        return 0;
    if ((session_is_super_novice_job(&g_session1, session_get_drawn_job(&g_session1))
         && g_session1.charBaseLevel >= g_session1.baseLevel)
        || (session_is_expanded_super_novice_job(&g_session1, session_get_drawn_job(&g_session1))
            && g_session1.charBaseLevel >= g_session1.baseLevelExtend2)) {
        chat_print_msg(0x7df, CHAT_COLOR_RED);
        return 1;
    }
    send_id(CZ_CHOPOKGI);
    return 0;
}

static void count_flood_burst(struct GameMode *self)
{
    int bursts;

    if (self->flood_count == 3) {
        self->flood_time[0] = self->flood_time[1];
        self->flood_time[1] = self->flood_time[2];
        self->flood_count = 2;
    }
    self->flood_time[self->flood_count] = timeGetTime();
    bursts = self->flood_count;
    if (bursts >= 2 && self->flood_time[2] - self->flood_time[0] < 90000) {
        game_mode_mute_self_for_flooding();
        self->flood_count = 0;
    } else if (bursts < 3) {
        self->flood_count = bursts + 1;
    }
}

int game_mode_msg_chat(struct GameMode *self)
{
    char line[0x16a];
    char dup2[0x16a], dup1[0x16a], cut1[0x16a], cut2[0x16a];
    struct String *seen;
    int n, i, count, hits;

    if (!take_chat_line(line))
        return 0;

    memcpy(dup2, line, sizeof dup2);
    n = (int)strlen(dup2);
    if (n > 6) {
        if (n + 3 < 0x16a) {
            dup2[n + 2] = dup2[n];
            dup2[n + 3] = dup2[n + 1];
            dup2[n] = dup2[n - 2];
            dup2[n + 1] = dup2[n - 1];
        }
        memcpy(dup1, line, sizeof dup1);
        n = (int)strlen(dup1);
        if (n + 1 < 0x16a) {
            dup1[n + 1] = dup1[n];
            dup1[n] = dup1[n - 1];
        }
        memcpy(cut1, line, sizeof cut1);
        n = (int)strlen(cut1);
        if (n + 1 < 0x16a) {
            cut1[n - 1] = cut1[n];
            cut1[n] = cut1[n + 1];
        }
        memcpy(cut2, line, sizeof cut2);
        n = (int)strlen(cut2);
        if (n + 1 < 0x16a) {
            cut2[n - 2] = cut2[n];
            cut2[n - 1] = cut2[n + 1];
        }
    }

    if (self->chat_history_count == 10) {
        for (i = 0; i < 10; i++) {
            string_assign(&self->chat_history[i], string_cstr(&self->chat_history[i + 1]), self->chat_history[i + 1].size);
            self->chat_history_time[i] = self->chat_history_time[i + 1];
        }
    }
    string_assign_cstr(&self->chat_history[self->chat_history_count], line);
    self->chat_history_time[self->chat_history_count] = timeGetTime();
    count = ++self->chat_history_count;
    if (count > 10)
        self->chat_history_count = 10;

    hits = 0;
    for (i = 0; i <= self->chat_history_count; i++) {
        if (i == 10 && check_history_full(self))
            break;
        if (strlen(line) <= 6)
            continue;
        seen = &self->chat_history[i];
        if (!string_equal_cstr(seen, line) && !string_equal_cstr(seen, dup2)
            && !string_equal_cstr(seen, dup1) && !string_equal_cstr(seen, cut1)
            && !string_equal_cstr(seen, cut2))
            continue;
        if (timeGetTime() - self->chat_history_time[i] >= 30000 || timeGetTime() - self->chat_history_time[0] == 0)
            continue;
        if (++hits >= 3) {
            count_flood_burst(self);
            break;
        }
    }
    return send_chat_line(CZ_REQUEST_CHAT);
}

int game_mode_msg_guild_chat(struct GameMode *self)
{
    char line[0x16a];

    if (!strlen(chat_line()))
        return 0;
    if (!g_guildInfo.guild_id)
        return 0;
    if (session_has_berserk(&g_session1) == 1)
        return 0;
    prepare_line(line);
    return send_chat_line(CZ_GUILD_CHAT);
}

int game_mode_msg_party_chat(struct GameMode *self)
{
    char line[0x16a];

    if (!take_chat_line(line))
        return 0;
    return send_chat_line(CZ_REQUEST_CHAT_PARTY);
}

int game_mode_msg_clan_chat(struct GameMode *self)
{
    char line[0x16a];
    char text[0x2d8];
    const char *typed, *charset;
    struct ChatPacket packet;
    unsigned len;
    unsigned short packet_len;

    if (!strlen(chat_line()))
        return 0;
    if (!g_clanInfo->in_clan)
        return 0;
    if (session_has_berserk(&g_session1) == 1)
        return 0;
    strcpy_s(line, sizeof line, chat_line());
    filter_bad_words(line);
    typed = chat_line();
    charset = language_charset_tag(g_language, 0);
    sprintf(text, "%s : %s%s", session_character_name(&g_session1), charset, typed);
    len = strlen(text);
    packet_len = (unsigned short)(len + 5);
    window_mgr_clear_chat_line(&g_windowMgr);
    if (len > 0xdc)
        return 0;
    memset(packet.text, 0, sizeof packet.text);
    packet.id = CZ_CLAN_CHAT;
    packet.len = packet_len;
    memcpy(packet.text, text, len + 1);
    send_raw((short)packet_len, &packet);
    return 0;
}

int game_mode_msg_battlefield_chat(struct GameMode *self)
{
    char line[0x16a];
    char text[0x2d8];
    const char *typed, *charset;
    struct ChatPacket packet;
    unsigned len;

    if (!take_chat_line(line))
        return 0;
    typed = chat_line();
    charset = language_charset_tag(g_language, 0);
    sprintf(text, "%s : %s%s", session_character_name(&g_session1), charset, typed);
    len = strlen(text);
    window_mgr_clear_chat_line(&g_windowMgr);
    packet.id = CZ_BATTLEFIELD_CHAT;
    packet.len = (unsigned short)(len + 5);
    memcpy(packet.text, text, len + 1);
    send_raw(packet.len, &packet);
    return 0;
}

int game_mode_msg_whisper(struct GameMode *self, const char *target)
{
    char line[0x152];
    char name[0x19];
    char text[0x152];
    struct ChatPacket packet;
    const char *own;
    size_t len;

    if (!strlen(chat_line()))
        return 0;
    if (session_has_berserk(&g_session1) == 1)
        return 0;
    strncpy(line, chat_line(), 0x151);
    line[0x151] = 0;
    filter_bad_words(line);

    memset(name, 0, sizeof name);
    strcpy(name, target);
    string_assign_cstr(&self->whisper_target, name);
    own = session_character_name(&g_session1);
    if (strcmp(string_cstr(&self->whisper_target), own) == 0) {
        window_mgr_clear_chat_line(&g_windowMgr);
        return 0;
    }
    if (g_useCharsetTag) {
        strcpy(text, language_charset_tag(g_language, 0));
        strncpy(text + 3, chat_line(), 0x14e);
    } else {
        strncpy(text, chat_line(), 0x151);
    }
    text[0x151] = 0;
    string_assign_cstr(&self->whisper_text, text);
    len = strlen(text);
    if (len - 1 > 0x14f) {
        window_mgr_clear_chat_line(&g_windowMgr);
        return 0;
    }
    window_mgr_clear_chat_line(&g_windowMgr);
    game_mode_record_whisper(self, name, text);
    packet.id = CZ_WHISPER;
    packet.len = (unsigned short)(len + 0x1c);
    memcpy_s(packet.text, 0x18, name, 0x18);
    memcpy_s(packet.text + 0x18, len, text, len);
    send_raw(packet.len, &packet);
    return 0;
}
