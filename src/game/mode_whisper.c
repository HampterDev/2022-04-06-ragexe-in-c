#include "game/packet_handlers.h"
#include "ui/chat/friends.h"
#include "ui/window_types.h"
#include "ui/window_access.h"
#include "ui/chat/readable_links.h"
#include "ui/chat/navi_text.h"
#include "session/number_codec.h"
#include "session/character_name.h"
#include "game/client_state.h"
#include "core/msg_str.h"
#include "game/chat.h"
#include "session/session_party.h"
#include "std/std.h"
#include "ui/chat/chat.h"
#include "ui/ui_items.h"
#include "ui/wndmgr/wndmgr.h"

signed char g_silentWhisperAcks;
#define WIN32_LEAN_AND_MEAN
#include "core/markup_tags.h"
#include <windows.h>
#include "render/renderer.h"
#include <mmsystem.h>
#include <string.h>
#include "game/whisper.h"
#include "game/client.h"
#include "ui/chat/balloon.h"

#define COLOUR_WHISPER   0xffff
#define COLOUR_WHISPER_WND 0x7800
#define CHAT_CHANNEL_WHISPER 2

static void show_sent_whisper(struct GameMode *self, int receiver_cid)
{
    struct String *target = &self->whisper_target;
    struct String line, tmp, tmp2;

    string_init(&line);
    if (ui_window_mgr_is_friend(&g_windowMgr, string_cstr(target)) == 0
        && session_is_guild_member_by_name(&g_session1, string_cstr(target)) == 0) {
        string_init(&tmp);
        string_assign_cstr(&tmp, "( To ");
        string_append(&tmp, string_cstr(target), target->size);
        string_append_cstr(&tmp, " (");
        string_move(&tmp2, &tmp);
        string_free(&line);
        string_move(&line, &tmp2);
        string_free(&tmp);
        session_scramble_number(&g_session1, &tmp, receiver_cid);
        string_append(&line, string_cstr(&tmp), tmp.size);
        string_free(&tmp);
        string_init(&tmp);
        string_assign_cstr(&tmp, ") : ");
        string_append(&tmp, string_cstr(&self->whisper_text), self->whisper_text.size);
        string_append(&line, string_cstr(&tmp), tmp.size);
    } else {
        string_init(&tmp2);
        string_assign_cstr(&tmp2, "[ To ");
        string_append(&tmp2, string_cstr(target), target->size);
        string_append_cstr(&tmp2, " (");
        string_move(&tmp, &tmp2);
        string_free(&line);
        string_move(&line, &tmp);
        string_free(&tmp2);
        session_scramble_number(&g_session1, &tmp, receiver_cid);
        string_append(&line, string_cstr(&tmp), tmp.size);
        string_free(&tmp);
        string_init(&tmp);
        string_assign_cstr(&tmp, ") ] : ");
        string_append(&tmp, string_cstr(&self->whisper_text), self->whisper_text.size);
        string_append(&line, string_cstr(&tmp), tmp.size);
    }
    string_free(&tmp);

    if (g_silentWhisperAcks > 0) {
        g_silentWhisperAcks--;
    } else {
        ui_window_mgr_send_msg_(&g_windowMgr, UIM_REFRESH, 0, 0, 0, 0);
        ui_window_mgr_send_msg_(&g_windowMgr, UIM_CHAT_LINE, (int)string_cstr(&line), COLOUR_WHISPER,
                              CHAT_CHANNEL_WHISPER, (int)string_cstr(target));
    }
    if (g_session1.autoSaveChat != 0)
        game_mode_log_whisper(self, string_cstr(&line));
    string_free(&line);
}

static void build_failure_line(struct GameMode *self, struct String *out, int msg_id)
{
    struct String open, closed;

    string_init(&open);
    string_assign_cstr(&open, "(");
    string_append(&open, string_cstr(&self->whisper_target), self->whisper_target.size);
    string_append_cstr(&open, ")");
    string_move(&closed, &open);
    string_append_cstr(&closed, msg_str(msg_id));
    string_move(out, &closed);
    string_free(&open);
}

static void show_failure_line(struct String *line)
{
    ui_window_mgr_send_msg_(&g_windowMgr, UIM_CHAT_LINE, (int)string_cstr(line), COLOUR_WHISPER,
                          CHAT_CHANNEL_WHISPER, 0);
}

void game_mode_ack_whisper(struct GameMode *self, unsigned char result, int receiver_cid)
{
    struct String line, part, moved;

    switch (result) {
    case WHISPER_ACK_SENT:
        string_init(&line);
        string_assign_cstr(&line, session_character_name(&g_session1));
        string_append_cstr(&line, " : ");
        string_append(&line, string_cstr(&self->whisper_text), self->whisper_text.size);
        if (ui_window_mgr_add_whisper_chat_to_whisper_wnd(&g_windowMgr, string_cstr(&self->whisper_target),
                                                    string_cstr(&line), COLOUR_WHISPER_WND, receiver_cid) == 0)
            show_sent_whisper(self, receiver_cid);
        string_free(&line);
        return;

    case WHISPER_ACK_NOT_THERE:
        build_failure_line(self, &line, 0x94);
        if (g_silentWhisperAcks > 0) {
            g_silentWhisperAcks--;
            string_free(&line);
            return;
        }
        show_failure_line(&line);
        string_free(&line);
        return;

    case WHISPER_ACK_IGNORED:
    case WHISPER_ACK_IGNORING_ALL:
        build_failure_line(self, &line, result == WHISPER_ACK_IGNORED ? 0x95 : 0x96);
        show_failure_line(&line);
        string_free(&line);
        return;

    default:
        string_init(&part);
        string_reserve(&part, self->whisper_target.size + 1);
        string_append_cstr(&part, "(");
        string_append(&part, string_cstr(&self->whisper_target), self->whisper_target.size);
        string_append_cstr(&part, ")");
        moved = part;
        string_init(&part);
        string_append_cstr(&moved, msg_str(0x94));
        string_move(&line, &moved);
        string_free(&part);
        show_failure_line(&line);
        if (g_session1.autoSaveChat != 0)
            game_mode_log_whisper(self, string_cstr(&line));
        string_free(&line);
        return;
    }
}

#define MSG_AWAY_PREFIX 0x3ad

#define MSG_NOT_IN_FRIEND_LIST 0x395
#define MSG_NOT_IN_GUILD 0x397
#define UIM_WHISPER_LOG_LINE 0x13
#define CHAT_CHANNEL_ADMIN_WHISPER 0x16

#define COLOUR_WHISPER_IN_WND 0x222222

static void answer_with_away_message(const char *text, const char *sender)
{
    const char *prefix = msg_str(MSG_AWAY_PREFIX);
    struct String reply;

    if (strncmp(text, prefix, strlen(prefix)) == 0)
        return;
    string_init(&reply);
    string_assign_cstr(&reply, msg_str(MSG_AWAY_PREFIX));
    string_append_cstr(&reply, string_cstr(&g_session1.autoMessage));
    string_assign_cstr(&g_windowMgr.chat_line, string_cstr(&reply));
    CMODE_SENDMSG(client_current_mode(), CMODE_MSG_WHISPER, (int)sender, 0, 0, 0);
    string_free(&reply);
}

static void open_admin_balloon(struct GameMode *self)
{
    struct UIWindow *w;

    self->admin_whisper_time = timeGetTime();
    if (self->admin_whisper_balloon == 0) {
        w = (struct UIWindow *)mem_alloc(sizeof(struct UITransBalloonText));
        w = w ? ui_trans_balloon_text_construct(w) : 0;
        self->admin_whisper_balloon = w;
        ui_window_create(w, 8, 8);
        ui_window_mgr_add_to_window_list(&g_windowMgr, self->admin_whisper_balloon);
    }
}

static void show_admin_balloon(struct GameMode *self, const char *line)
{
    struct UIWindow *w;

    ui_balloon_text_set_text((struct UIBalloonText *)self->admin_whisper_balloon, line, 0x48);
    w = self->admin_whisper_balloon;
    ui_window_move(w, (g_renderer->width - 640) / 2 - w->width / 2 + 320, 0x33);
    ((struct UIBalloonText *)self->admin_whisper_balloon)->colour = 0xffff;
    ((struct UIBalloonText *)self->admin_whisper_balloon)->outline = 0;
}

static void append_sender_tag(struct String *prefix, int sender_cid, const char *open, const char *close)
{
    struct String tag;
    session_scramble_number(&g_session1, &tag, sender_cid);
    string_append_cstr(prefix, open);
    string_append(prefix, string_cstr(&tag), tag.size);
    string_append_cstr(prefix, close);
    string_free(&tag);
}

static void finish_prefix(struct String *prefix, const char *tail, const char *text)
{
    string_append_cstr(prefix, tail);
    string_append_cstr(prefix, text);
}

void game_mode_whisper(struct GameMode *self, int sender_cid, struct TextView sender,
                      struct TextView text, unsigned char from_admin)
{
    struct String sender_text = {0}, msg = {0}, line = {0};
    struct String head = {0}, body = {0}, prefix = {0};
    const char *chars;
    string_assign(&sender_text, sender.data, sender.size);
    string_assign(&msg, text.data, text.size);
    string_assign(&line, string_cstr(&sender_text), sender_text.size);
    string_append_cstr(&line, " : ");
    string_append_cstr(&line, string_cstr(&msg));
    if (ui_window_mgr_add_whisper_chat_to_whisper_wnd(&g_windowMgr, string_cstr(&sender_text), string_cstr(&line),
                                                COLOUR_WHISPER_IN_WND, sender_cid) != 0) {
        if (g_session1.autoMessageOn)
            answer_with_away_message(string_cstr(&msg), string_cstr(&sender_text));
        chars = string_cstr(&msg);
        if (from_admin) {
            open_admin_balloon(self);
            string_assign_cstr(&head, "[ ");
            string_append(&head, string_cstr(&sender_text), sender_text.size);
            string_assign_cstr(&body, chars);
            navi_links_readable_links(&body, string_view(&body));
            if (strstr(string_cstr(&msg), string_cstr(&text_tag_ITEML))) {
                ui_new_chat_wnd_readable_links(g_windowMgr.new_chat_wnd, &body, string_view(&body));
                string_append_cstr(&head, " ] : ");
                string_append(&head, string_cstr(&body), body.size);
            } else {
                finish_prefix(&head, " ] : ", chars);
            }
            show_admin_balloon(self, string_cstr(&head));
        }
    } else {
        chars = string_cstr(&msg);
        string_clear(&prefix);
        if (ui_window_mgr_is_friend(&g_windowMgr, string_cstr(&sender_text)) != 0) {
            string_assign_cstr(&prefix, "[ Friend ");
            string_append(&prefix, string_cstr(&sender_text), sender_text.size);
            if (sender_cid != 0)
                append_sender_tag(&prefix, sender_cid, " (", ")");
            finish_prefix(&prefix, " ] : ", chars);
        } else if (from_admin) {
            string_assign_cstr(&prefix, "[ ");
            string_append(&prefix, string_cstr(&sender_text), sender_text.size);
            if (sender_cid != 0)
                append_sender_tag(&prefix, sender_cid, " (", ")");
            finish_prefix(&prefix, " ] : ", chars);
        } else if (session_is_guild_member_by_name(&g_session1, string_cstr(&sender_text)) != 0) {
            string_assign_cstr(&prefix, "[ Member ");
            string_append(&prefix, string_cstr(&sender_text), sender_text.size);
            if (sender_cid != 0)
                append_sender_tag(&prefix, sender_cid, " (", ")");
            finish_prefix(&prefix, " ] : ", chars);
        } else {
            int warning = 0;
            if (session_looks_like_a_party_member(&g_session1, string_cstr(&sender_text)) != 0)
                warning = MSG_NOT_IN_FRIEND_LIST;
            else if (session_looks_like_a_friend(&g_session1, string_cstr(&sender_text)) != 0)
                warning = MSG_NOT_IN_GUILD;
            if (warning != 0) {
                ui_window_mgr_send_msg_(&g_windowMgr, UIM_CHAT_LINE, (int)msg_str(warning), COLOUR_WHISPER, 0, 0);
                ui_window_mgr_send_msg_(&g_windowMgr, UIM_WHISPER_LOG_LINE, (int)msg_str(warning), COLOUR_WHISPER, 0, 0);
            }
            string_assign_cstr(&prefix, "( From ");
            string_append(&prefix, string_cstr(&sender_text), sender_text.size);
            if (sender_cid != 0)
                append_sender_tag(&prefix, sender_cid, " [", "]");
            finish_prefix(&prefix, " ) : ", chars);
        }

        if (strcmp(string_cstr(&sender_text), session_character_name(&g_session1)) != 0) {
            ui_window_mgr_send_msg_(&g_windowMgr, UIM_REFRESH, 0, 0, 0, 0);
            ui_window_mgr_send_msg_(&g_windowMgr, UIM_CHAT_LINE, (int)string_cstr(&prefix), COLOUR_WHISPER,
                                  from_admin ? CHAT_CHANNEL_ADMIN_WHISPER : CHAT_CHANNEL_WHISPER,
                                  (int)string_cstr(&sender_text));
            ui_window_mgr_add_whisper_name(&g_windowMgr, string_cstr(&sender_text));
            if (from_admin) {
                open_admin_balloon(self);
                if (string_find(&prefix, "<ITEML>", 0) != STRING_NPOS) {
                    ui_new_chat_wnd_readable_links(g_windowMgr.new_chat_wnd, &prefix, string_view(&prefix));
                }
                navi_links_readable_links(&prefix, string_view(&prefix));
                show_admin_balloon(self, string_cstr(&prefix));
            }
            if (g_session1.autoSaveChat != 0)
                game_mode_log_whisper(self, string_cstr(&prefix));
        }
        if (g_session1.autoMessageOn)
            answer_with_away_message(string_cstr(&msg), string_cstr(&sender_text));
    }
    string_free(&prefix);
    string_free(&body);
    string_free(&head);
    string_free(&line);
    string_free(&msg);
    string_free(&sender_text);
}

#pragma pack(push, 1)
struct PACKET_ZC_WHISPER02 {
    unsigned short id;
    unsigned short length;
    int char_id;
    char name[24];
    BYTE gm_flag;
    char text[];
};
#pragma pack(pop)

void game_mode_zc_whisper02(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_WHISPER02 *packet = (const struct PACKET_ZC_WHISPER02 *)pkt;
    unsigned short bytes;
    size_t name_size = 0;
    int cid;
    bytes = packet->length;
    if (bytes < 33) return;
    while (name_size < 23 && packet->name[name_size]) ++name_size;
    cid = packet->char_id;
    game_mode_whisper(self, cid, text_view(packet->name, name_size),
                     text_view(packet->text, bytes - 33), packet->gm_flag);
}

#pragma pack(push, 1)
struct PACKET_ZC_WHISPER {
    unsigned short id;
    unsigned short length;
    char name[24];
    int gm_flag;
    char text[];
};
#pragma pack(pop)

void game_mode_zc_whisper(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_WHISPER *packet = (const struct PACKET_ZC_WHISPER *)pkt;
    unsigned short bytes;
    size_t name_size = 0;
    int flag;
    bytes = packet->length;
    if (bytes < 32) return;
    while (name_size < 23 && packet->name[name_size]) ++name_size;
    flag = packet->gm_flag;
    game_mode_whisper(self, 0, text_view(packet->name, name_size),
                     text_view(packet->text, bytes - 32), flag != 0);
}
