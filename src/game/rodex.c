#include "game/packet_handlers.h"
#include "ui/window_manager_state.h"
#include <string.h>
#include <time.h>
#include "game/gamemode.h"
#include "game/ui_bridge.h"
#include "game/rodex_mail.h"
#include "ui/mail/rodex_ui.h"

struct UIRodexInboxWnd;

#pragma pack(push, 1)
struct MailEntry {
    __int64 mail_id;
    unsigned char is_read;
    unsigned char type;
    char sender[24];
    int reg_date_time;
    int expire_seconds;
    short title_length;
    char title[1];
};
struct PacketLatestRodexList {
    unsigned short packet_type;
    short packet_length;
    unsigned char open_type;
    unsigned char count;
    unsigned char list_complete;
    unsigned char entries[1];
};
#pragma pack(pop)

static size_t c_strlen(const char *s)
{
    const char *p = s;
    while (*p)
        p++;
    return (size_t)(p - s);
}

void game_mode_on_latest_rodex_list(struct GameMode *self, const struct PacketLatestRodexList *packet)
{
    struct Rodex *rodex = g_rodex;
    const unsigned char *at = packet->entries;
    struct Tree *box;
    int i;

    (void)self;
    rodex->open_type = packet->open_type;
    g_rodex->list_complete = packet->list_complete;
    switch (g_rodex->open_type) {
    case 0: box = &g_rodex->tabs[0]; break;
    case 1: box = &g_rodex->tabs[1]; break;
    case 2: box = &g_rodex->tabs[2]; break;
    default: box = 0; break;
    }
    tree_clear(box, mail_pair_destroy);

    for (i = 0; i < packet->count; i++) {
        const struct MailEntry *e = (const struct MailEntry *)at;
        struct Mail mail;
        size_t sender_length;

        string_init(&mail.sender);
        string_init(&mail.title);
        string_init(&mail.text);
        mail.deleted = 0;
        mail.sender_trusted = -1;
        mail.is_read = e->is_read;
        mail.type = e->type;
        mail.expire_time = e->expire_seconds + (int)_time32(0);
        sender_length = c_strlen(e->sender);
        string_assign(&mail.sender, e->sender, sender_length > 24 ? 24 : sender_length);
        string_assign(&mail.title, e->title, (size_t)(int)e->title_length);

        if (!tree_find(box, &e->mail_id, mail_id_greater)) {
            struct MailPair pair;
            int added;

            pair.id = e->mail_id;
            pair.mail.is_read = mail.is_read;
            pair.mail.type = mail.type;
            string_copy(&pair.mail.sender, &mail.sender);
            pair.mail.sent_time = mail.sent_time;
            pair.mail.expire_time = mail.expire_time;
            string_copy(&pair.mail.title, &mail.title);
            string_copy(&pair.mail.text, &mail.text);
            pair.mail.item_count = mail.item_count;
            memcpy(pair.mail.items, mail.items, sizeof pair.mail.items);
            pair.mail.zeny = mail.zeny;
            pair.mail.deleted = mail.deleted;
            pair.mail.sender_trusted = mail.sender_trusted;
            tree_add(box, &pair, sizeof pair, mail_id_greater, &added);
            if (!added)
                mail_destroy(&pair.mail);
        }
        at += 0x2c + e->title_length;
        string_free(&mail.text);
        string_free(&mail.title);
        string_free(&mail.sender);
    }
    if (((struct UIRodexInboxWnd *)g_windowMgr.rodex_inbox_wnd)) {
        ui_rodex_inbox_wnd_reset_to_first_page(((struct UIRodexInboxWnd *)g_windowMgr.rodex_inbox_wnd));
        ui_rodex_inbox_wnd_set_buttons_disabled(((struct UIRodexInboxWnd *)g_windowMgr.rodex_inbox_wnd), 0);
    }
}

enum {
    WID_RODEX_ICON = 0x106,
    WID_MENU_ICON = 0x133,
    RODEX_ICON_MSG_PLACE = 6,
};

#pragma pack(push, 1)
struct PacketNotifyUnreadRodex {
    unsigned short packet_type;
    unsigned char has_unread;
};
#pragma pack(pop)

void game_mode_on_notify_unread_rodex(struct GameMode *self, const struct PacketNotifyUnreadRodex *packet)
{
    struct UIWindow *host;

    (void)self;
    if (packet->has_unread != 1) {
        ui_close_window(WID_RODEX_ICON);
        return;
    }
    if (g_windowMgr.open_mail_box_wnd == 0)
        ui_make_window(WID_RODEX_ICON);
    host = ui_get_window(WID_MENU_ICON);
    if (host)
        ui_window_send(host, NULL, RODEX_ICON_MSG_PLACE, 0x116, 0x1d4, 1, 0);
}
