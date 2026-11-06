#include "game/packet_handlers.h"
#include "ui/window_manager_state.h"
#include "ui/window_access.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "game/packets_rodex.h"
#include "core/msg_str.h"
#include "core/text_util.h"
#include "session/session_items.h"
#include "session/session_party.h"
#include "ui/mail/rodex_compose.h"
#include "ui/mail/rodex_ui.h"

struct Rodex *g_rodex;

#pragma pack(push, 1)
struct PACKET_ZC_ACK_ALL_RODEX_LIST {
    unsigned short id;
    short length;
    unsigned char is_end;
    unsigned char mails[1];
};
struct AllRodexEntry {
    unsigned char open_type;
    long long mail_id;
    unsigned char is_read;
    unsigned char type;
    char sender[24];
    int expire_seconds;
    short title_length;
    char title[1];
};
struct PACKET_ZC_ACK_RODEX_LIST {
    unsigned short id;
    short length;
    unsigned char open_type;
    unsigned char count;
    unsigned char is_end;
    unsigned char mails[1];
};
struct RodexEntry {
    long long mail_id;
    unsigned char is_read;
    unsigned char type;
    char sender[24];
    int reg_date_time;
    int expire_seconds;
    short title_length;
    char title[1];
};
struct PACKET_ZC_ACK_READ_RODEX2 {
    unsigned short id;
    short length;
    unsigned char open_type;
    long long mail_id;
    unsigned short text_length;
    long long zeny;
    unsigned char item_count;
    char text[1];
};
struct RodexItem {
    unsigned short count;
    int item_id;
    unsigned char is_identified;
    unsigned char is_damaged;
    int cards[ITEM_CARD_SLOTS];
    int item_index;
    unsigned char type;
    unsigned short view_sprite;
    unsigned short bind_on_equip_type;
    struct ItemOption options[ITEM_OPTION_SLOTS];
    unsigned char refine;
    unsigned char grade;
};
struct PACKET_ZC_ACK_DELETE_RODEX {
    unsigned short id;
    unsigned char open_type;
    long long mail_id;
};
struct PACKET_ZC_ACK_ZENY_FROM_RODEX {
    unsigned short id;
    long long mail_id;
    unsigned char open_type;
    unsigned char result;
};
#pragma pack(pop)
OFFSET_ASSERT(AllRodexEntry, title, 0x29);
OFFSET_ASSERT(RodexEntry, expire_seconds, 0x26);
OFFSET_ASSERT(RodexEntry, title, 0x2c);
OFFSET_ASSERT(PACKET_ZC_ACK_READ_RODEX2, text, 0x18);
OFFSET_ASSERT(RodexItem, options, 0x21);
SIZE_ASSERT(RodexItem, MAIL_ITEM_SIZE);
OFFSET_ASSERT(PACKET_ZC_ACK_ZENY_FROM_RODEX, result, 0x0b);

static void mail_was_read(void)
{
    struct UIWindow *wnd;

    g_rodex->unread--;
    if (g_rodex->unread != 0)
        return;
    wnd = ui_window_find(WID_MENU_ICON);
    if (wnd != 0)
        ui_window_send(wnd, NULL, 6, 0x116, 0x1d4, 0, 0);
}

static struct Tree *tab_of(unsigned char open_type)
{
    switch (open_type) {
    case 0: return &g_rodex->tabs[0];
    case 1: return &g_rodex->tabs[1];
    case 2: return &g_rodex->tabs[2];
    }
    return 0;
}

static struct MailPair *find_mail(unsigned char open_type, const long long *mail_id)
{
    struct Tree *map = tab_of(open_type);

    if (map == 0)
        return 0;
    return tree_find(map, mail_id, mail_id_greater);
}

static void mail_init(struct Mail *mail)
{
    string_init(&mail->sender);
    string_init(&mail->title);
    string_init(&mail->text);
    mail->deleted = 0;
    mail->sender_trusted = -1;
}

static void set_sender(struct Mail *mail, const char *sender)
{
    size_t length = strlen(sender);

    if (length > RODEX_SENDER_MAX)
        length = RODEX_SENDER_MAX;
    string_assign(&mail->sender, sender, length);
}

static void check_sender(struct Mail *mail, unsigned char open_type)
{
    struct String copy;

    if (open_type == 0 && !(mail->type & MAIL_FROM_SERVER)) {
        string_copy(&copy, &mail->sender);
        if (!session_is_account_char_name(&g_session1, copy)) {
            string_copy(&copy, &mail->sender);
            mail->sender_trusted = session_is_known_name(&g_session1, copy);
            return;
        }
    }
    mail->sender_trusted = 1;
}

static void mail_pair_init(struct MailPair *pair, long long id, const struct Mail *mail)
{
    pair->id = id;
    pair->mail.is_read = mail->is_read;
    pair->mail.type = mail->type;
    string_copy(&pair->mail.sender, &mail->sender);
    pair->mail.sent_time = mail->sent_time;
    pair->mail.expire_time = mail->expire_time;
    string_copy(&pair->mail.title, &mail->title);
    string_copy(&pair->mail.text, &mail->text);
    pair->mail.item_count = mail->item_count;
    memcpy(pair->mail.items, mail->items, sizeof mail->items);
    pair->mail.zeny = mail->zeny;
    pair->mail.deleted = mail->deleted;
    pair->mail.sender_trusted = mail->sender_trusted;
}

static void insert_mail(struct Tree *map, struct MailPair *pair)
{
    int added;

    tree_add(map, pair, sizeof *pair, mail_id_greater, &added);
    if (!added)
        mail_destroy(&pair->mail);
}

void game_mode_zc_ack_all_rodex_list(struct GameMode *self, const struct PACKET_ZC_ACK_ALL_RODEX_LIST *packet)
{
    const unsigned char *base = (const unsigned char *)packet;
    const struct AllRodexEntry *e;
    struct Tree *map;
    struct Mail mail;
    struct MailPair pair;
    struct UIWindow *wnd;
    int offset;

    (void)self;
    for (offset = 5; packet->length > offset; offset += e->title_length + 0x29) {
        e = (const struct AllRodexEntry *)(base + offset);
        map = tab_of(e->open_type);
        if (map == 0)
            continue;
        mail_init(&mail);
        mail.is_read = e->is_read;
        mail.type = e->type;
        set_sender(&mail, e->sender);
        check_sender(&mail, e->open_type);
        mail.expire_time = e->expire_seconds + _time32(0);
        string_assign(&mail.title, e->title, (size_t)(int)e->title_length);
        if (!tree_find(map, &e->mail_id, mail_id_greater) && !mail.is_read)
            g_rodex->unread++;
        mail_pair_init(&pair, e->mail_id, &mail);
        insert_mail(map, &pair);
        mail_destroy(&mail);
    }
    if (packet->is_end == 1) {
        g_rodex->list_complete = 1;
        wnd = ui_window_find(WID_RODEX_INBOX);
        if (wnd != 0)
            ui_window_send(wnd, NULL, UIM_RODEX_REFRESH, 0, 0, 0, 0);
    }
}

void game_mode_zc_ack_failed_all_rodex_list(struct GameMode *self)
{
    (void)self;
}

void game_mode_zc_ack_rodex_list(struct GameMode *self, const struct PACKET_ZC_ACK_RODEX_LIST *packet)
{
    const struct RodexEntry *e = (const struct RodexEntry *)packet->mails;
    struct Tree *map;
    struct Mail mail;
    struct MailPair pair;
    int i;

    (void)self;
    g_rodex->open_type = packet->open_type;
    g_rodex->list_complete = packet->is_end;
    if (packet->count != 0) {
        for (i = 1; i < packet->count + 1; i++) {
            mail_init(&mail);
            mail.is_read = e->is_read;
            mail.type = e->type;
            mail.expire_time = e->expire_seconds + _time32(0);
            set_sender(&mail, e->sender);
            check_sender(&mail, packet->open_type);
            string_replace(&mail.title, mail.title.size, 0, e->title, e->title_length);
            map = tab_of(packet->open_type);
            if (map != 0 && !tree_find(map, &e->mail_id, mail_id_greater)) {
                mail_pair_construct(&pair, &e->mail_id, &mail);
                insert_mail(&g_rodex->tabs[packet->open_type], &pair);
            }
            e = (const struct RodexEntry *)((const unsigned char *)e + e->title_length + 0x2c);
            mail_destroy(&mail);
        }
    }
    if (g_windowMgr.rodex_inbox_wnd != 0) {
        ui_rodex_inbox_wnd_rebuild_lists((struct UIRodexInboxWnd *)g_windowMgr.rodex_inbox_wnd);
        ui_rodex_inbox_wnd_set_buttons_disabled((struct UIRodexInboxWnd *)g_windowMgr.rodex_inbox_wnd, 0);
        ui_rodex_inbox_wnd_next_page((struct UIRodexInboxWnd *)g_windowMgr.rodex_inbox_wnd);
    }
}

void game_mode_rodex_open_mail(struct GameMode *self, unsigned char *found, struct MailPair **node,
                                        const char *text, unsigned item_count, unsigned open_type, int text_length,
                                        long long mail_id, long long zeny)
{
    struct Tree *map;
    struct String *s;

    (void)self;
    if (*found != 0) {
        if (ui_window_mgr_post_quit1(&g_windowMgr, WID_RODEX_CONTENTS))
            return;
        ui_window_open(WID_RODEX_CONTENTS);
        ui_window_send(g_windowMgr.rodex_contents_wnd, NULL, UIM_RODEX_REFRESH, 0, 0, 0, 0);
        return;
    }
    g_rodex->open_mail_id = mail_id;
    g_rodex->open_type = (unsigned char)open_type;
    map = tab_of((unsigned char)open_type);
    if (map != 0) {
        *node = tree_find(map, &mail_id, mail_id_greater);
        if (*node)
            *found = 1;
    }
    if (*found != 1)
        return;
    if (!(*node)->mail.is_read)
        mail_was_read();
    s = &(*node)->mail.text;
    string_append(s, text, (size_t)(short)text_length);
    (*node)->mail.item_count = (unsigned char)item_count;
    (*node)->mail.zeny = zeny;
    (*node)->mail.is_read = 1;
}

static void item_copy_inline(struct ItemInfo *dst, const struct ItemInfo *src)
{
    memcpy(dst, src, offsetof(struct ItemInfo, id_text));
    string_copy(&dst->id_text, &src->id_text);
    string_copy(&dst->name, &src->name);
    memcpy(&dst->identified, &src->identified, sizeof *dst - offsetof(struct ItemInfo, identified));
}

void game_mode_zc_ack_read_rodex2(struct GameMode *self, const struct PACKET_ZC_ACK_READ_RODEX2 *packet)
{
    unsigned char found = 0;
    struct MailPair *node = 0;
    const struct RodexItem *a;
    const unsigned char *src;
    unsigned char *dst;
    struct ItemInfo item, arg;
    int i, k;

    session_clear_mail_item_counts(&g_session1);
    game_mode_rodex_open_mail(self, &found, &node, packet->text, packet->item_count, packet->open_type,
                            packet->text_length, packet->mail_id, packet->zeny);
    if (found != 1)
        return;
    for (i = 0; i < packet->item_count; i++) {
        dst = node->mail.items[i];
        src = (const unsigned char *)packet->text + (short)packet->text_length + i * MAIL_ITEM_SIZE;
        memcpy_s(dst, MAIL_ITEM_SIZE, src, MAIL_ITEM_SIZE);
        a = (const struct RodexItem *)node->mail.items[i];
        item_info_construct(&item);
        item.type = a->type;
        item.equip_location = a->item_index;
        item.count = a->count;
        item.slot[0] = a->cards[0];
        item.slot[1] = a->cards[1];
        item.slot[2] = a->cards[2];
        item.slot[3] = a->cards[3];
        item_info_set_item_id(&item, a->item_id);
        item.identified = a->is_identified;
        item.damaged = a->is_damaged;
        item.refine = a->refine;
        item.bind_on_equip_type = a->bind_on_equip_type;
        item.view = a->view_sprite;
        item.option_count = item_option_count(a->options);
        for (k = 0; k < item.option_count; k++)
            item.options[k] = a->options[k];
        item.grade = a->grade;
        item_copy_inline(&arg, &item);
        session_put_mail_item(&g_session1, arg);
        item_info_destruct(&item);
    }
    game_mode_rodex_open_mail(self, &found, 0, 0, 0, 0, 0, 0, 0);
}

void game_mode_zc_ack_delete_rodex(struct GameMode *self, const struct PACKET_ZC_ACK_DELETE_RODEX *packet)
{
    struct MailPair *node;

    (void)self;
    node = find_mail(packet->open_type, &packet->mail_id);
    if (node != 0) {
        node->mail.deleted = 1;
        if (!node->mail.is_read)
            mail_was_read();
    }
    if (g_windowMgr.rodex_inbox_wnd != 0)
        ui_window_refresh(g_windowMgr.rodex_inbox_wnd);
}

void game_mode_zc_ack_zeny_from_rodex(struct GameMode *self,
                                                const struct PACKET_ZC_ACK_ZENY_FROM_RODEX *packet)
{
    struct MailPair *node;

    (void)self;
    if (g_windowMgr.rodex_contents_wnd != 0)
        rodex_contents_wnd_set_zeny_flag((struct UIRodexContentsWnd *)g_windowMgr.rodex_contents_wnd, 0);
    switch (packet->result) {
    case 0:
        rodex_chat(0xa1f);
        node = find_mail(packet->open_type, &packet->mail_id);
        if (node != 0)
            node->mail.type -= MAIL_HAS_ZENY;
        if (g_windowMgr.rodex_contents_wnd != 0)
            rodex_contents_wnd_zeny_taken((struct UIRodexContentsWnd *)g_windowMgr.rodex_contents_wnd);
        break;
    case 1: rodex_chat(0xa20); break;
    case 2: rodex_chat(0xa21); break;
    }
}

void game_mode_zc_ack_item_from_rodex(struct GameMode *self,
                                                const struct PACKET_ZC_ACK_ZENY_FROM_RODEX *packet)
{
    struct MailPair *node;

    (void)self;
    if (g_windowMgr.rodex_contents_wnd != 0)
        rodex_contents_wnd_set_items_flag((struct UIRodexContentsWnd *)g_windowMgr.rodex_contents_wnd, 0);
    switch (packet->result) {
    case 0:
        rodex_chat(0xa1c);
        session_clear_mail_item_counts(&g_session1);
        node = find_mail(packet->open_type, &packet->mail_id);
        if (node != 0)
            node->mail.type -= MAIL_HAS_ITEMS;
        if (g_windowMgr.rodex_contents_wnd != 0) {
            ui_window_send(g_windowMgr.rodex_contents_wnd, NULL, UIM_RODEX_REFRESH, 0, 0, 0, 0);
            rodex_contents_wnd_items_taken((struct UIRodexContentsWnd *)g_windowMgr.rodex_contents_wnd);
        }
        if (g_windowMgr.rodex_inbox_wnd != 0)
            ui_window_send(g_windowMgr.rodex_inbox_wnd, NULL, UIM_RODEX_INBOX_REFRESH, 0, 0, 0, 0);
        break;
    case 1: rodex_chat(0xa1d); break;
    case 2: rodex_chat(0xa1e); break;
    }
}

void game_mode_zc_rodex_block_receive(struct GameMode *self, struct String name)
{
    struct String text;

    (void)self;
    string_init(&text);
    string_format(&text, msg_str(0xf18), string_cstr(&name));
    ui_window_mgr_send_msg_(&g_windowMgr, UIM_PUSH_INTO_CHAT, (int)string_cstr(&text), RODEX_CHAT_COLOR, 0, 0);
    string_free(&text);
    string_free(&name);
}

void game_mode_zc_rodex_return_ack(struct GameMode *self, int mail_id, int result)
{
    struct Tree *map;
    struct MailPair *it;
    long long id;
    int msg;

    (void)self;
    if (result != 0) {
        msg = 0xf1a;
    } else {
        if (g_windowMgr.rodex_contents_wnd != 0)
            ui_window_send(g_windowMgr.rodex_contents_wnd, NULL, 6, 0xc1, 0, 0, 0);
        map = &g_rodex->tabs[0];
        id = mail_id;
        it = tree_find(map, &id, mail_id_greater);
        if (it) {
            if (!it->mail.is_read)
                mail_was_read();
            tree_erase(map, TREE_NODE_OF(it), mail_pair_destroy);
            if (g_windowMgr.rodex_inbox_wnd != 0)
                rodex_inbox_wnd_remove((struct UIRodexInboxWnd *)g_windowMgr.rodex_inbox_wnd, mail_id);
        }
        msg = 0xf19;
    }
    ui_window_mgr_message_box(&g_windowMgr, msg_str(msg), 0, 1, 0, 0, "Message", 0x118, 0x78, 0);
    rodex_chat(msg);
}

void game_mode_zc_check_receive_character_name2(struct GameMode *self, int char_id,
                                                         unsigned short job, unsigned short level,
                                                         const char *name)
{
    (void)self;
    if (char_id == 0) {
        rodex_chat(0xa37);
        return;
    }
    if (g_windowMgr.rodex_compose_wnd != 0)
        ui_rodex_compose_wnd_set_receiver((struct UIRodexComposeWnd *)g_windowMgr.rodex_compose_wnd, char_id, job, level, name);
}

#pragma pack(push, 1)
struct PACKET_ZC_RODEX_BLOCK_RECEIVE {
    unsigned short packet_type;
    char name[24];
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_RODEX_BLOCK_RECEIVE, 26);

void game_mode_zc_rodex_block_receive_packet(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_RODEX_BLOCK_RECEIVE *packet = (const struct PACKET_ZC_RODEX_BLOCK_RECEIVE *)pkt;
    struct String name;

    string_init(&name);
    string_assign_cstr(&name, packet->name);
    game_mode_zc_rodex_block_receive(self, name);
}
