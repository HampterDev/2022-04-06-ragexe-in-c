#include "core/text_array.h"
#include "core/drive_state.h"
#include "ui/window_access.h"
#include <string.h>
#include "ui/wndmgr/wndmgr.h"
#include "ui/draw.h"
#include "ui/chat/chat.h"
#include "ui/wndmgr/wndmgr_handlers.h"

#define UIWM_MSG_CHAT_COMMAND 2
#define UIWM_MSG_SHOW_CHAT 3
#define UIWM_MSG_LOADING 4
#define UIWM_MSG_CHAT_ROOM_LINE 5
#define UIWM_MSG_SAVE_CHAT 6
#define UIWM_MSG_SAVE_CHAT_ROOM 7
#define UIWM_MSG_WHISPER 14
#define UIWM_MSG_CHAT_LINE_ALT 0x13
#define UIWM_MSG_TO_WHISPERS 0x9a

#define WM_CHAT_LINE_WIDTH 0x5e
#define WM_COLOUR_WHITE 0xffffff

static void keep_chat_line(struct UIWindowMgr *self, const char *text, COLORREF colour)
{
    struct TextArray lines = {0};
    size_t i, count;

    text_split_lines(text, &lines, WM_CHAT_LINE_WIDTH);
    count = lines.size;
    for (i = 0; i < count; i++) {
        struct WmChatLine *line = list_push_back(&self->pending_chat_lines, NULL, sizeof *line);

        string_init(&line->text);
        string_assign(&line->text, text_array_at(&lines, i).data, text_array_at(&lines, i).size);
        line->colour = colour;
    }
    text_array_clear(&lines);
}

static void chat_line(struct UIWindowMgr *self, int msg, const char *text, COLORREF colour, int val3, int val4)
{
    struct UIWindow *chat;

    chat = self->new_chat_wnd;
    if (colour == 0)
        colour = WM_COLOUR_WHITE;
    if (chat == NULL) {
        drive_state_note_chat(text);
        keep_chat_line(self, text, colour);
        return;
    }
    if (strcmp(text, "No Msg") == 0 || strcmp(text, "NO MSG") == 0)
        return;
    drive_state_note_chat(text);
    ui_window_send_msg(chat, NULL, msg == UIWM_MSG_CHAT_LINE_ALT ? 0x73 : 0x25, (intptr_t)text, (int)colour,
             val3, val4);
}

static void show_chat(struct UIWindowMgr *self)
{
    {
        self->chat_wnd_show = 1;
        if (self->new_chat_wnd == NULL)
            ui_window_open(0x1);
        ui_window_set_show(self->new_chat_wnd, 1);
        ui_window_mgr_raise_window(&g_windowMgr, self->new_chat_wnd);
    }
    if (self->new_chat_wnd)
        ui_window_send_msg(self->new_chat_wnd, NULL, 0x10, 0, 0, 0, 0);
}

intptr_t ui_window_mgr_send_msg_(struct UIWindowMgr *self, int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4)
{
    struct TreeNode *node;

    switch (msg) {
    case UIWM_MSG_CHAT_LINE:
    case UIWM_MSG_CHAT_LINE_ALT:
        chat_line(self, msg, (const char *)(INT_PTR)val1, (COLORREF)val2, val3, val4);
        break;
    case UIWM_MSG_CHAT_COMMAND:
        if (self->new_chat_wnd)
            ui_window_send_msg(self->new_chat_wnd, NULL, 0xf, val1, 0, 0, 0);
        break;
    case UIWM_MSG_SHOW_CHAT:
        show_chat(self);
        break;
    case UIWM_MSG_LOADING:
        if (self->loading_wnd)
            ui_window_send_msg(self->loading_wnd, NULL, 0x11, val1, 0, 0, 0);
        break;
    case UIWM_MSG_CHAT_ROOM_LINE:
        if (self->chat_room_wnd)
            ui_window_send_msg(self->chat_room_wnd, NULL, 0x25, val1, val2, 0, 0);
        break;
    case UIWM_MSG_SAVE_CHAT:
        if (self->new_chat_wnd && g_windowMgrHandlers.save_chat_log)
            g_windowMgrHandlers.save_chat_log((struct UINewChatWnd *)self->new_chat_wnd);
        break;
    case UIWM_MSG_SAVE_CHAT_ROOM:
        if (self->chat_room_wnd && g_windowMgrHandlers.save_chat_room_talk)
            g_windowMgrHandlers.save_chat_room_talk((struct UIChatRoomWnd *)self->chat_room_wnd);
        break;
    case UIWM_MSG_WHISPER:
        if (!g_windowMgrHandlers.open_whisper)
            return 0;
        return g_windowMgrHandlers.open_whisper(self, (const char *)(INT_PTR)val1, (const char *)(INT_PTR)val2,
                                                (const char *)(INT_PTR)val3);
    case UIWM_MSG_TO_WHISPERS:
        for (node = tree_first(&self->whisper_wnds); node != self->whisper_wnds.head; node = tree_next(node))
            ui_window_send_msg(TREE_VALUE(node, struct WmWhisperWnd)->wnd, NULL, 0x9a, val1, 0, 0, 0);
        break;
    }
    return 0;
}
