#include "session/character_name.h"
#include "contents/manner_filter.h"
#include "game/context.h"
#include <string.h>
#include "game/mode_msg.h"
#include "session/session_items.h"

#define CZ_REQ_OPEN_WRITE_MAIL      0xa08
#define CZ_REQ_ADD_ITEM_TO_MAIL     0xa04
#define CZ_REQ_REMOVE_ITEM_MAIL     0xa06
#define CZ_REQ_CANCEL_WRITE_MAIL    0xa03
#define CZ_REQ_WRITE_MAIL           0xa6e
#define CZ_RETURN_MAIL              0x273

#define WID_WRITE_MAIL 0x108

struct RodexMail {
    int receiver_char_id;
    struct String receiver;
    struct String title;
    struct String body;
    int zeny;
};

#pragma pack(push, 1)
struct RodexWriteHeader {
    unsigned short id;
    unsigned short len;
    char receiver[24];
    char sender[24];
    __int64 zeny;
    unsigned short title_len;
    unsigned short body_len;
    int receiver_char_id;
};
#pragma pack(pop)

int game_mode_msg_send_rodex(const struct RodexMail *mail)
{
    struct RodexWriteHeader header;
    struct Vec packet;

    if (word_filter_contains(&g_nameFilter, string_cstr(&mail->title)) == 1
            || word_filter_contains(&g_nameFilter, string_cstr(&mail->body)) == 1) {
        message_box_msg(0xbd4);
        return 0;
    }
    session_reset_mail_items(&g_session1);
    session_clear_mail_item_counts(&g_session1);
    header.id = CZ_REQ_WRITE_MAIL;
    header.receiver_char_id = mail->receiver_char_id;
    header.zeny = mail->zeny;
    strcpy_s(header.receiver, 0x18, string_cstr(&mail->receiver));
    strcpy_s(header.sender, 0x18, session_character_name(&g_session1));
    header.title_len = (unsigned short)(mail->title.size + 1);
    header.body_len = (unsigned short)(mail->body.size + 1);
    packet.first = packet.last = packet.end = 0;
    vec_insert_range(&packet, packet.last, &header, &header + 1, 1);
    {
        const char *end = string_cstr(&mail->title) + (short)header.title_len;
        vec_insert_range(&packet, packet.last, string_cstr(&mail->title), end, 1);
    }
    {
        const char *end = string_cstr(&mail->body) + (short)header.body_len;
        vec_insert_range(&packet, packet.last, string_cstr(&mail->body), end, 1);
    }
    ((struct RodexWriteHeader *)packet.first)->len = (unsigned short)((char *)packet.last - (char *)packet.first);
    send_raw((short)((char *)packet.last - (char *)packet.first), packet.first);
    close_window(WID_WRITE_MAIL);
    vec_free(&packet);
    return 0;
}

int game_mode_msg_cancel_write_rodex(void)
{
    send_id(CZ_REQ_CANCEL_WRITE_MAIL);
    session_reset_mail_items(&g_session1);
    session_clear_mail_item_counts(&g_session1);
    return 0;
}

int game_mode_msg_rodex_item(int add, int index, int count)
{
#pragma pack(push, 1)
    struct {
        unsigned short id;
        unsigned short index;
        unsigned short count;
    } packet;
#pragma pack(pop)

    if (game_mode_is_blocking_window_open() == 1)
        return 0;
    packet.id = add ? CZ_REQ_ADD_ITEM_TO_MAIL : CZ_REQ_REMOVE_ITEM_MAIL;
    packet.index = (unsigned short)index;
    packet.count = (unsigned short)count;
    if (add)
        g_session1.mailItemCount = (short)count;
    send_packet(packet.id, &packet);
    return 0;
}

int game_mode_msg_rodex_mail(unsigned short id, int mail_id, int type)
{
#pragma pack(push, 1)
    struct {
        unsigned short id;
        unsigned char type;
        __int64 mail_id;
    } packet;
#pragma pack(pop)

    packet.type = (unsigned char)type;
    packet.id = id;
    packet.mail_id = mail_id;
    send_packet(id, &packet);
    return 0;
}

int game_mode_msg_return_mail(int mail_id, const char *name)
{
#pragma pack(push, 1)
    struct {
        unsigned short id;
        int mail_id;
        char name[24];
    } packet;
#pragma pack(pop)

    packet.id = CZ_RETURN_MAIL;
    packet.mail_id = mail_id;
    memcpy(packet.name, name, 24);
    send_packet(CZ_RETURN_MAIL, &packet);
    return 0;
}

int game_mode_msg_open_write_rodex(const char *receiver)
{
#pragma pack(push, 1)
    struct {
        unsigned short id;
        char receiver[24];
    } packet;
#pragma pack(pop)
    struct String name;

    memset(packet.receiver, 0, sizeof(packet.receiver));
    packet.id = CZ_REQ_OPEN_WRITE_MAIL;
    if (receiver) {
        string_init(&name);
        string_assign_cstr(&name, receiver);
        if (name.size >= 0x18) {
            message_box_msg(0x213);
            string_free(&name);
            return 0;
        }
        memcpy_s(packet.receiver, 0x18, string_cstr(&name), 0x18);
        string_free(&name);
    }
    send_packet(packet.id, &packet);
    return 0;
}
