#include "game/packet_handlers.h"
#include "ui/window_access.h"
#include "ui/chat/balloon.h"
#include "ui/chat/readable_links.h"
#include "ui/chat/navi_text.h"
#include "core/text_array.h"
#include "contents/whisper_block.h"
#include "contents/manner_filter.h"
#include "game/broadcast_text.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#include <string.h>
#include "game/client.h"
#include "game/gamemode.h"
#include "game/item_info.h"
#include "core/msg_str.h"
#include "core/text_util.h"
#include "ui/draw.h"


#define UIM_LOG_LINE 0x13
#define TEXT_WIDTH 0x48
#define NOTICE_Y 0x33

static void make_text_window(struct UIWindow **slot)
{
    struct UIWindow *wnd = mem_alloc(sizeof(struct UITransBalloonText));

    if (wnd != 0)
        wnd = ui_trans_balloon_text_construct(wnd);
    *slot = wnd;
    ui_window_create(wnd, 8, 8);
    ui_window_mgr_add_to_window_list(&g_windowMgr, *slot);
}

static int centred_x(const struct UIWindow *wnd)
{
    return (g_renderer->width - 640) / 2 - wnd->width / 2 + 320;
}

static void chat(int msg, const char *text, unsigned colour, int channel, const char *sender)
{
    ui_window_mgr_send_msg_(&g_windowMgr, msg, (int)text, colour, channel, (int)sender);
}

static int filter_text(char *text)
{
    if (word_filter_contains(&g_nameFilter, text) != 1)
        return 0;

    word_filter_censor(&g_nameFilter, text);

    return 1;
}

static void readable_links(struct String *text)
{
    ui_new_chat_wnd_readable_links(g_windowMgr.new_chat_wnd, text, string_view(text));
    navi_links_readable_links(text, string_view(text));
}

static void set_lines(struct UIWindow *wnd, const struct String *text, unsigned colour)
{
    struct TextArray lines = {0};
    unsigned i;

    text_split_lines(string_cstr(text), &lines, TEXT_WIDTH);
    if (lines.size != 0)
        ui_trans_balloon_text_set_first_line((struct UITransBalloonText *)wnd, text_array_at(&lines, 0).data, colour);
    for (i = 1; i < lines.size; i++) {
        ui_trans_balloon_text_add_colored_line((struct UITransBalloonText *)wnd, text_array_at(&lines, i).data, colour);
    }
    text_array_clear(&lines);
}

#pragma pack(push, 1)
struct PACKET_ZC_BROADCAST2 {
    unsigned short id;
    unsigned short length;
    BYTE red;
    BYTE green;
    BYTE blue;
    BYTE color_high;
    short font_type;
    short font_size;
    short align;
    short y;
    char text[];
};
#pragma pack(pop)

void game_mode_broadcast2(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_BROADCAST2 *packet = (const struct PACKET_ZC_BROADCAST2 *)pkt;
    char text[256];
    size_t bytes = packet->length;
    struct String line = {0};
    struct String shown = {0};
    struct String readable = {0};
    struct UIBalloonText *notice;
    unsigned colour;

    if (bytes < 16)
        return;
    bytes -= 16;
    if (bytes >= sizeof text)
        bytes = sizeof text - 1;
    memcpy(text, packet->text, bytes);
    text[bytes] = 0;
    filter_text(text);
    broadcast_text_strip_annotation(text, sizeof text);
    string_assign_cstr(&line, text);
    self->notice_shown_at = timeGetTime();
    if (self->notice_wnd == 0)
        make_text_window(&self->notice_wnd);
    colour = (unsigned)packet->red << 16 | (unsigned)packet->green << 8 | packet->blue;
    notice = (struct UIBalloonText *)self->notice_wnd;
    notice->outline = 0;
    notice->colour = colour;
    navi_links_readable_links(&readable, text_view(string_cstr(&line), line.size));
    string_assign(&line, string_cstr(&readable), readable.size);
    string_free(&readable);
    ui_new_chat_wnd_readable_links(g_windowMgr.new_chat_wnd, &shown, string_view(&line));
    ui_balloon_text_set_text((struct UIBalloonText *)self->notice_wnd, string_cstr(&shown), TEXT_WIDTH);
    ui_balloon_text_set_font_height((struct UIBalloonText *)self->notice_wnd, packet->font_size);
    ui_window_move_safe(self->notice_wnd, centred_x(self->notice_wnd), NOTICE_Y);
    chat(UIM_REFRESH, 0, 0, 0, 0);
    chat(UIM_CHAT_LINE, string_cstr(&line), colour, BROADCAST_CHANNEL, 0);
    chat(UIM_LOG_LINE, string_cstr(&line), colour, BROADCAST_CHANNEL, 0);
    string_free(&shown);
    string_free(&line);
}

static int sender_blocked(const char *sender)
{
    return whisper_block_contains(g_whisperBlockMgr, sender);
}

#pragma pack(push, 1)
struct PACKET_ZC_BROADCAST {
    unsigned short id;
    unsigned short length;
    char text[];
};
#pragma pack(pop)

void game_mode_broadcast(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_BROADCAST *packet = (const struct PACKET_ZC_BROADCAST *)pkt;
    struct BroadcastText parsed;
    struct String line = {0};
    struct UIWindow *balloon, *notice;
    size_t bytes = packet->length;

    if (bytes < 4 || !broadcast_text_parse(&parsed, packet->text, bytes - 4))
        return;
    if (parsed.channel == 0) {
        if (sender_blocked(parsed.sender))
            return;
        filter_text(parsed.text);
    }
    string_assign_cstr(&line, parsed.text);

    chat(UIM_REFRESH, 0, 0, 0, 0);
    chat(UIM_CHAT_LINE, string_cstr(&line), parsed.colour, parsed.channel, parsed.sender);
    if (parsed.log_line)
        chat(UIM_LOG_LINE, string_cstr(&line), parsed.colour, parsed.channel, parsed.sender);

    if (parsed.balloon) {
        self->admin_whisper_time = timeGetTime();
        if (self->admin_whisper_balloon == 0)
            make_text_window(&self->admin_whisper_balloon);
        readable_links(&line);
        set_lines(self->admin_whisper_balloon, &line, parsed.colour);
        balloon = self->admin_whisper_balloon;
        ui_window_move_safe(balloon, centred_x(balloon), NOTICE_Y);
        notice = self->notice_wnd;
        if (notice != 0)
            ui_window_move_safe(notice, notice->x, balloon->y + balloon->height + 0x11);
    } else {
        self->notice_shown_at = timeGetTime();
        if (self->notice_wnd == 0)
            make_text_window(&self->notice_wnd);
        readable_links(&line);
        set_lines(self->notice_wnd, &line, parsed.colour);
        notice = self->notice_wnd;
        balloon = self->admin_whisper_balloon;
        if (balloon != 0)
            ui_window_move_safe(notice, centred_x(notice), balloon->y + balloon->height + 0x11);
        else
            ui_window_move_safe(notice, centred_x(notice), NOTICE_Y);
    }
    string_free(&line);
}

#pragma pack(push, 1)
struct PACKET_ZC_BROADCAST4 {
    unsigned short id;
    unsigned short length;
    BYTE channel;
    unsigned colour;
    char text[];
};
#pragma pack(pop)

void game_mode_broadcast4(const unsigned char *pkt)
{
    const struct PACKET_ZC_BROADCAST4 *packet = (const struct PACKET_ZC_BROADCAST4 *)pkt;
    struct String text;
    size_t bytes = packet->length;

    if (bytes < 9)
        return;
    string_init(&text);
    string_assign(&text, packet->text, bytes - 9);
    if (text.size != 0)
        chat(UIM_CHAT_LINE, string_cstr(&text), packet->colour, packet->channel, 0);
    string_free(&text);
}

enum { OBTAIN_FROM_BOX = 0, OBTAIN_FROM_MONSTER = 1, OBTAIN_PLAIN = 2 };

#pragma pack(push, 1)
struct PACKET_ZC_BROADCASTING_SPECIAL_ITEM_OBTAIN_SOURCE {
    signed char monster_name_len;
    union {
        int box_id;
        char monster_name[1];
    };
};
struct PACKET_ZC_BROADCASTING_SPECIAL_ITEM_OBTAIN {
    unsigned short id;
    unsigned short length;
    BYTE type;
    int item_id;
    signed char name_len;
    char name[];
};
#pragma pack(pop)

void game_mode_broadcast_special_item_obtain(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_BROADCASTING_SPECIAL_ITEM_OBTAIN *packet = (const struct PACKET_ZC_BROADCASTING_SPECIAL_ITEM_OBTAIN *)pkt;
    struct String player, source, line, formatted;

    struct String item_name = {0};
    struct ItemInfo box, item;
    struct UIBalloonText *notice;
    const struct PACKET_ZC_BROADCASTING_SPECIAL_ITEM_OBTAIN_SOURCE *from =
        (const struct PACKET_ZC_BROADCASTING_SPECIAL_ITEM_OBTAIN_SOURCE *)(packet->name + packet->name_len);

    string_init(&player);
    string_init(&source);
    string_assign(&player, packet->name, packet->name_len);
    if (packet->type == OBTAIN_FROM_BOX) {
        item_info_construct(&box);
        item_info_set_item_id(&box, from->box_id);
        box.identified = 1;
        item_info_get_display_name(&box, &item_name);
        string_assign(&source, string_cstr(&item_name), item_name.size);
        string_free(&item_name);
        item_info_destruct(&box);
    } else if (packet->type == OBTAIN_FROM_MONSTER) {
        string_assign(&source, from->monster_name, from->monster_name_len);
    }
    item_info_construct(&item);
    item_info_set_item_id(&item, packet->item_id);
    string_init(&line);
    item.identified = 1;
    if (packet->type == OBTAIN_PLAIN) {
        item_info_get_display_name(&item, &item_name);
        string_init(&formatted);
        string_format(&formatted, msg_str(0x74c), string_cstr(&player), string_cstr(&item_name));
    } else {
        item_info_get_display_name(&item, &item_name);
        string_init(&formatted);
        string_format(&formatted, msg_str(0x65c), string_cstr(&player), string_cstr(&source),
                          string_cstr(&item_name));
    }
    if (&line != &formatted)
        string_assign(&line, string_cstr(&formatted), formatted.size);
    string_free(&formatted);
    string_free(&item_name);
    chat(UIM_CHAT_LINE, string_cstr(&line), 0xffff, 0, 0);
    self->notice_shown_at = timeGetTime();
    if (self->notice_wnd == 0)
        make_text_window(&self->notice_wnd);
    ui_balloon_text_set_text((struct UIBalloonText *)self->notice_wnd, string_cstr(&line), TEXT_WIDTH);
    ui_window_move_safe(self->notice_wnd, centred_x(self->notice_wnd), NOTICE_Y);
    notice = (struct UIBalloonText *)self->notice_wnd;
    notice->colour = 0x62b0ff;
    notice->outline = 0;
    string_free(&line);
    item_info_destruct(&item);
    string_free(&source);
    string_free(&player);
}
