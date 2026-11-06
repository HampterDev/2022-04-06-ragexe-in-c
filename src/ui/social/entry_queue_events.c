#include "contents/sound.h"
#include <stdlib.h>
#include <string.h>
#include "ui/social/social.h"
#include "ui/social/entry_queue_state.h"
#include "ui/social/entry_queue_windows.h"
#include "ui/ui_std.h"
#include "core/msg_str.h"

#define WAVE_REQUEST "se_btg_request.wav"
#define WAVE_READY "se_btg_ready.wav"
#define WAVE_FORWARD "se_btg_forward.wav"

#define ENTRY_QUEUE_CHAT_COLOUR 0x4763ff
#define ENTRY_QUEUE_CHAT_CANCEL_COLOUR 0xff

#define MSG_ENTRY_APPLIED 0x833
#define MSG_ENTRY_APPLY_FAIL_FIRST 0x834
#define MSG_ENTRY_TEAM_IN_BATTLE 0x83d
#define MSG_ENTRY_LEFT_QUEUE 0x83e
#define MSG_ENTRY_WRONG_NAME 0x83f
#define MSG_ENTRY_NOT_QUEUED 0x840
#define MSG_ENTRY_ARENA_UNAVAILABLE 0x841
#define MSG_ENTRY_CANCELLED 0x842
#define MSG_ENTRY_READY 0x843

static void entry_queue_box(int msg)
{
    ui_window_mgr_message_box(&g_windowMgr, msg_str(msg), UI_MSGBOX_OK, 0, 0, 0, "Message", 0x118, 0x78, 0);
}

static void entry_queue_chat(struct EntryQueueMgr *self, int msg, COLORREF colour)
{
    struct String text;
    const char *line;

    string_init(&text);
    string_assign_cstr(&text, "[");
    if (self->current)
        string_append_cstr(&text, self->current->title);
    string_append_cstr(&text, "]");
    line = msg_str(msg);
    string_append_cstr(&text, line);
    ui_chat_line(string_cstr(&text), colour);
    string_free(&text);
}

static void entry_queue_wave(const char *wave)
{
    sound_play_wave(wave, 0.0f, 0.0f, 0.0f, 0xfa, 0x28, 1.0f);
}

#pragma pack(push, 1)
struct PACKET_ZC_ACK_ENTRY_QUEUE_APPLY {
    WORD packet_type;
    BYTE result;
    char queue_name[24];
};
struct PACKET_ZC_NOTIFY_ENTRY_QUEUE_APPLY {
    WORD packet_type;
    char queue_name[24];
    int arg2;
};
struct PACKET_ZC_ACK_ENTRY_QUEUE_CANCEL {
    WORD packet_type;
    BYTE result;
};
struct PACKET_ZC_REPLY_ACK_ENTRY_QUEUE_ADMISSION {
    WORD packet_type;
    BYTE result;
};
struct PACKET_ZC_NOTIFY_LOBBY_ADMISSION {
    WORD packet_type;
    char queue_name[24];
    char lobby_name[24];
};
struct PACKET_ZC_REPLY_ACK_LOBBY_ADMISSION {
    WORD packet_type;
    BYTE result;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_ACK_ENTRY_QUEUE_APPLY, 27);
SIZE_ASSERT(PACKET_ZC_NOTIFY_ENTRY_QUEUE_APPLY, 30);
SIZE_ASSERT(PACKET_ZC_ACK_ENTRY_QUEUE_CANCEL, 3);
SIZE_ASSERT(PACKET_ZC_REPLY_ACK_ENTRY_QUEUE_ADMISSION, 3);
SIZE_ASSERT(PACKET_ZC_NOTIFY_LOBBY_ADMISSION, 50);
SIZE_ASSERT(PACKET_ZC_REPLY_ACK_LOBBY_ADMISSION, 3);

void entry_queue_mgr_on_apply_result(struct EntryQueueMgr *self, const BYTE *packet)
{
    const struct PACKET_ZC_ACK_ENTRY_QUEUE_APPLY *reply = (const struct PACKET_ZC_ACK_ENTRY_QUEUE_APPLY *)packet;
    if (!self || !packet) return;
    switch (reply->result) {
    case 1:
        if (!ui_close_window(ENTRY_QUEUE_STANDBY_WND_ID))
            ui_make_window(ENTRY_QUEUE_STANDBY_WND_ID);
        memcpy(self->queue_name, reply->queue_name, sizeof self->queue_name);
        entry_queue_wave(WAVE_REQUEST);
        entry_queue_chat(self, MSG_ENTRY_APPLIED, ENTRY_QUEUE_CHAT_COLOUR);
        self->in_queue = 1;
        break;
    case 2: case 3: case 4: case 5: case 6: case 7: case 8:
        entry_queue_box(MSG_ENTRY_APPLY_FAIL_FIRST + reply->result - 2);
        break;
    case 9:
        entry_queue_box(0x83c);
        break;
    case 10:
        entry_queue_box(0x83b);
        break;
    case 15:
        entry_queue_box(MSG_ENTRY_TEAM_IN_BATTLE);
        break;
    }
}

void entry_queue_mgr_on_cancel_result(struct EntryQueueMgr *self, const BYTE *packet)
{
    const struct PACKET_ZC_ACK_ENTRY_QUEUE_CANCEL *reply = (const struct PACKET_ZC_ACK_ENTRY_QUEUE_CANCEL *)packet;
    if (!self || !packet) return;
    switch (reply->result) {
    case 1:
    case 14:
        entry_queue_box(reply->result == 1 ? MSG_ENTRY_LEFT_QUEUE : MSG_ENTRY_ARENA_UNAVAILABLE);
        ui_close_window(ENTRY_QUEUE_STANDBY_WND_ID);
        self->in_queue = 0;
        break;
    case 3:
        entry_queue_box(MSG_ENTRY_WRONG_NAME);
        break;
    case 11:
        entry_queue_box(MSG_ENTRY_NOT_QUEUED);
        break;
    }
    entry_queue_chat(self, MSG_ENTRY_CANCELLED, ENTRY_QUEUE_CHAT_CANCEL_COLOUR);
    self->current = NULL;
}

void entry_queue_mgr_on_leave(struct EntryQueueMgr *self, const BYTE *packet)
{
    if (!self || !packet) return;
    self->current = NULL;
    self->in_queue = 0;
}

void entry_queue_mgr_on_ignored(struct EntryQueueMgr *self, const BYTE *packet)
{
    if (!self || !packet) return;
}

void entry_queue_mgr_on_status(struct EntryQueueMgr *self, const BYTE *packet)
{
    const struct PACKET_ZC_NOTIFY_ENTRY_QUEUE_APPLY *status = (const struct PACKET_ZC_NOTIFY_ENTRY_QUEUE_APPLY *)packet;
    if (!self || !packet) return;
    char name[25];
    int arg2;
    struct UIWindow *wnd;
    size_t i;

    memcpy(name, status->queue_name, 24); name[24] = 0;
    arg2 = status->arg2;

    self->in_queue = 1;
    wnd = ui_get_window(ENTRY_QUEUE_STANDBY_WND_ID);
    if (!wnd && !ui_close_window(ENTRY_QUEUE_STANDBY_WND_ID))
        wnd = ui_make_window(ENTRY_QUEUE_STANDBY_WND_ID);
    if (!self->current) {
        for (i = 0; i < entry_queue_count(); ++i) {
            struct EntryQueueInfo *queue = entry_queue_at(i);
            if (!strcmp(queue->name, name)) {
                self->current = queue;
                break;
            }
        }
    }
    if (wnd && self->current)
        ui_entry_queue_stand_by_wnd_set_status((struct UIEntryQueueStandByWnd *)wnd, name,
                                         self->current->enemy_limit + self->current->member_limit,
                                         arg2);
    ui_close_window(ENTRY_QUEUE_WND_ID);
}

void entry_queue_mgr_on_request(struct EntryQueueMgr *self, const BYTE *packet)
{
    const struct PACKET_ZC_NOTIFY_LOBBY_ADMISSION *request = (const struct PACKET_ZC_NOTIFY_LOBBY_ADMISSION *)packet;
    if (!self || !packet) return;
    memcpy(self->queue_name, request->queue_name, sizeof self->queue_name);
    memcpy(self->lobby_name, request->lobby_name, sizeof self->lobby_name);
    if (ui_close_window(ENTRY_QUEUE_REQUEST_WND_ID))
        return;
    ui_make_window(ENTRY_QUEUE_REQUEST_WND_ID);
    entry_queue_wave(WAVE_READY);
    entry_queue_chat(self, MSG_ENTRY_READY, ENTRY_QUEUE_CHAT_COLOUR);
}

void entry_queue_mgr_on_answer_result(struct EntryQueueMgr *self, const BYTE *packet)
{
    const struct PACKET_ZC_REPLY_ACK_ENTRY_QUEUE_ADMISSION *reply = (const struct PACKET_ZC_REPLY_ACK_ENTRY_QUEUE_ADMISSION *)packet;
    if (!self || !packet) return;
    if (reply->result == 3)
        entry_queue_box(MSG_ENTRY_WRONG_NAME);
    else if (reply->result == 11)
        entry_queue_box(MSG_ENTRY_NOT_QUEUED);
}

void entry_queue_mgr_on_forward(struct EntryQueueMgr *self, const BYTE *packet)
{
    const struct PACKET_ZC_REPLY_ACK_LOBBY_ADMISSION *reply = (const struct PACKET_ZC_REPLY_ACK_LOBBY_ADMISSION *)packet;
    if (!self || !packet) return;
    if (reply->result == 1)
        entry_queue_wave(WAVE_FORWARD);
}
