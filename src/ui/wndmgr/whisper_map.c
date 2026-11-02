#include "ui/window_access.h"
#include "session/character_name.h"
#include "contents/sound.h"
#include "session/state.h"
#include <string.h>
#include "ui/wndmgr/wndmgr.h"
#include "ui/chat/chat.h"
#include "core/msg_str.h"
#include "session/session_party.h"

int wm_whisper_name_less(const void *a, const void *b)
{
    return string_compare(a, b) < 0;
}

void wm_whisper_wnd_destroy(void *value)
{
    string_free(&((struct WmWhisperWnd *)value)->name);
}

void ui_window_mgr_forget_whisper_wnd(struct UIWindowMgr *self, struct String name)
{
    tree_erase_key(&self->whisper_wnds, &name, wm_whisper_name_less, wm_whisper_wnd_destroy);
    string_free(&name);
}

void ui_window_mgr_notify_whisper_wnd(struct UIWindowMgr *self, const char *name)
{
    struct String key;
    struct WmWhisperWnd *entry;

    string_init(&key);
    string_assign_cstr(&key, name);
    entry = tree_find(&self->whisper_wnds, &key, wm_whisper_name_less);
    string_free(&key);
    if (entry)
        ui_window_send_msg(entry->wnd, NULL, 0x30, 0, 0, 0, 0);
}

#define MSG_WHISPER_FRIEND 0x66
#define MSG_NOT_IN_FRIEND_LIST 0x395
#define MSG_NOT_IN_GUILD 0x397
#define WHISPER_WARNING_COLOUR 0xffff
#define UIWM_MSG_WHISPER 14
#define WHISPER_MSG_ADD_LINE 0x25
#define WHISPER_LINE_RECEIVED 2
#define SOUND_BUTTON "버튼소리.wav"

const char *g_whisperSound = "..\\..\\BGM\\Msg_NewMsg.wav";

int ui_window_mgr_add_whisper_chat_to_whisper_wnd(struct UIWindowMgr *self, const char *name, const char *line,
                                                       unsigned colour, int cid)
{
    int is_friend = session_has_friend_named(&g_session1, name) != 0;
    struct UIWindow *opened = NULL;
    struct String key;
    struct WmWhisperWnd *entry;

    if ((g_session1.showWhisperWnd && !is_friend) || (g_session1.showWhisperWndFriend && is_friend)) {
        const char *note = NULL;
        struct String code;

        if (is_friend) {
            note = msg_str(MSG_WHISPER_FRIEND);
        } else if (session_is_guild_member_char_id(&g_session1, cid)) {
            note = "GuildMember";
        } else {
            int warning = 0;

            if (session_looks_like_a_party_member(&g_session1, name))
                warning = MSG_NOT_IN_FRIEND_LIST;
            else if (session_looks_like_a_friend(&g_session1, name))
                warning = MSG_NOT_IN_GUILD;
            if (warning)
                ui_window_mgr_send_msg_(&g_windowMgr, UIWM_MSG_CHAT_LINE, (intptr_t)msg_str(warning),
                                      WHISPER_WARNING_COLOUR, 0, 0);
        }
        session_scramble_number(&g_session1, &code, cid);
        opened = (struct UIWindow *)(INT_PTR)ui_window_mgr_send_msg_(self, UIWM_MSG_WHISPER, (intptr_t)name,
                                                                   (intptr_t)note, (intptr_t)string_cstr(&code), 0);
        string_free(&code);
        if (opened) {
            if (res_mgr_is_available(g_resMgr(), g_whisperSound))
                sound_play_wave(g_whisperSound, 0.0f, 0.0f, 0.0f, 0xfa, 0x28, 1.0f);
            else if (g_session1.playWhisperOpenSound)
                sound_play_wave(SOUND_BUTTON, 0.0f, 0.0f, 0.0f, 0xfa, 0x28, 1.0f);
        }
    }

    if (strcmp(name, session_character_name(&g_session1)) == 0)
        return 0;
    if (g_session1.showWhisperWnd && opened)
        ui_window_mgr_add_whisper_name(&g_windowMgr, name);
    string_init(&key);
    string_assign_cstr(&key, name);
    entry = tree_find(&self->whisper_wnds, &key, wm_whisper_name_less);
    string_free(&key);
    if (!entry)
        return 0;
    ui_window_send_msg(entry->wnd, NULL, WHISPER_MSG_ADD_LINE, (intptr_t)line, (int)colour, WHISPER_LINE_RECEIVED, 0);
    ui_window_mgr_raise_window(self, entry->wnd);
    return 1;
}

void ui_window_mgr_set_chat_line(struct UIWindowMgr *self, const char *text)
{
    string_assign_cstr(&self->chat_line, text);
}
