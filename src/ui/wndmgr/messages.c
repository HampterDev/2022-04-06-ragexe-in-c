#include "core/text_array.h"
#include "core/drive_state.h"
#include "ui/window_access.h"
#include "contents/connection_api.h"
#include <string.h>
#include "ui/wndmgr/wndmgr.h"
#include "ui/draw.h"
#include "ui/chat/whisper.h"
#include "ui/chat/chat.h"
#include "ui/chat/chat_room_wnd.h"
#include "ui/chat/chat_window.h"

#define UIWM_MSG_CHAT_COMMAND 2
#define UIWM_MSG_SHOW_CHAT 3
#define UIWM_MSG_LOADING 4
#define UIWM_MSG_CHAT_ROOM_LINE 5
#define UIWM_MSG_SAVE_CHAT 6
#define UIWM_MSG_SAVE_CHAT_ROOM 7
#define UIWM_MSG_WHISPER 14
#define UIWM_MSG_CHAT_LINE_ALT 0x13
#define UIWM_MSG_TO_WHISPERS 0x9a

#define WHISPER_WND_ID 0x43
#define WHISPER_MSG_INFO 0x22
#define WHISPER_CASCADE 0x11
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
    ui_window_send_msg(chat, NULL, msg == UIWM_MSG_CHAT_LINE_ALT ? 0x73 : 0x25, (int)(INT_PTR)text, (int)colour,
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

static void place_whisper(struct UIWindowMgr *self, struct UIWindow *wnd)
{
    const struct WindowInfo *info = &self->whisper_info;
    int off = (int)self->whisper_wnds.size * WHISPER_CASCADE, x, y;
    int right = g_renderer->width - WHISPER_CASCADE, bottom = g_renderer->height - WHISPER_CASCADE;

    if (self->whisper_wnds.size == 0)
        return;
    x = info->x + off;
    y = info->y + off;
    if (x > 0 && info->width + x < right && y > 0 && info->height + y < bottom)
        ui_window_move(wnd, x, y);
    else if (off > 0 && info->width + off < right && info->height + off < bottom)
        ui_window_move(wnd, off, off);
}

static int open_whisper(struct UIWindowMgr *self, const char *name, const char *title, const char *code)
{
    struct String key, keyed, part1, part2, full, text;
    struct WmWhisperWnd *entry;
    struct UIWindow *wnd;
    void *mem;

    string_init(&key);
    string_assign_cstr(&key, name);
    if (tree_find(&self->whisper_wnds, &key, wm_whisper_name_less)) {
        string_free(&key);
        return 0;
    }
    mem = mem_alloc(sizeof(struct UIWhisperWnd));
    wnd = mem ? (struct UIWindow *)ui_whisper_wnd_construct(mem) : NULL;
    ui_window_create(wnd, 0x118, 0x78);
    ui_window_mgr_add_window(self, wnd);
    ui_whisper_wnd_send_msg(wnd, NULL, WHISPER_MSG_INFO, (int)(INT_PTR)&self->whisper_info, 0, 0, 0);
    ui_window_set_show_base(wnd, 1);
    string_assign(&((struct UIWhisperWnd *)wnd)->name, string_cstr(&key), key.size);
    wnd->id = WHISPER_WND_ID;
    if (title == NULL) {
        string_assign(&((struct UIWhisperWnd *)wnd)->title_name, string_cstr(&key), key.size);
    } else {
        string_copy(&part1, &key);
        string_append_cstr(&part1, " (");
        string_append_cstr(&part1, title);
        string_move(&part2, &part1);
        string_append_cstr(&part2, ") *^_^*");
        string_move(&full, &part2);
        string_free(&((struct UIWhisperWnd *)wnd)->title_name);
        string_move(&((struct UIWhisperWnd *)wnd)->title_name, &full);
        string_free(&part2);
        string_free(&part1);
    }
    if (code) {
        string_init(&text);
        string_assign_cstr(&text, code);
        string_free(&((struct UIWhisperWnd *)wnd)->code);
        ((struct UIWhisperWnd *)wnd)->code = text;
    }
    place_whisper(self, wnd);
    string_copy(&keyed, &key);
    entry = tree_find(&self->whisper_wnds, &keyed, wm_whisper_name_less);
    if (!entry) {
        struct WmWhisperWnd fresh;

        string_copy(&fresh.name, &keyed);
        fresh.wnd = NULL;
        entry = tree_add(&self->whisper_wnds, &fresh, sizeof fresh, wm_whisper_name_less, NULL);
    }
    entry->wnd = wnd;
    string_free(&keyed);
    ui_window_mgr_add_whisper_name(self, string_cstr(&key));
    string_free(&key);
    return 1;
}

int ui_window_mgr_send_msg_(struct UIWindowMgr *self, int msg, int val1, int val2, int val3, int val4)
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
        if (self->new_chat_wnd)
            ui_chat_wnd_write_chat_to_file((struct UINewChatWnd *)self->new_chat_wnd);
        break;
    case UIWM_MSG_SAVE_CHAT_ROOM:
        if (self->chat_room_wnd)
            ui_chat_room_wnd_save_talk((struct UIChatRoomWnd *)self->chat_room_wnd);
        break;
    case UIWM_MSG_WHISPER:
        return open_whisper(self, (const char *)(INT_PTR)val1, (const char *)(INT_PTR)val2,
                            (const char *)(INT_PTR)val3);
    case UIWM_MSG_TO_WHISPERS:
        for (node = tree_first(&self->whisper_wnds); node != self->whisper_wnds.head; node = tree_next(node))
            ui_window_send_msg(TREE_VALUE(node, struct WmWhisperWnd)->wnd, NULL, 0x9a, val1, 0, 0, 0);
        break;
    }
    return 0;
}

#define CZ_ADD_FRIENDS 0x202
#pragma pack(push, 1)
struct PacketAddFriends {
    short id;
    char name[24];
};
#pragma pack(pop)
SIZE_ASSERT(PacketAddFriends, 0x1a);

void ui_window_mgr_request_add_friend(struct UIWindowMgr *self, const char *name)
{
    struct PacketAddFriends packet;
    int len;

    (void)self;
    packet.id = CZ_ADD_FRIENDS;
    memcpy(packet.name, name, sizeof packet.name);
    len = rag_connection_get_packet_length(rag_connection_instance(), CZ_ADD_FRIENDS);
    rag_connection_send_packet(rag_connection_instance(), len, &packet);
}
