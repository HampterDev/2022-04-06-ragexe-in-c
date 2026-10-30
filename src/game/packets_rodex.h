#ifndef GAMEMODE_PACKETS_RODEX_H
#define GAMEMODE_PACKETS_RODEX_H

#include "game/packet_handlers.h"
#include "ui/window_manager_state.h"
#include "game/objects.h"
#include "game/item_info.h"
#include "game/rodex_mail.h"
#include "core/msg_str.h"
#include "ui/mail/rodex_ui.h"

#define RODEX_CHAT_COLOR 0x64ffff
#define RODEX_SENDER_MAX 24
enum {
    WID_MENU_ICON = 0x133,
};
enum { MM_RODEX_SENT = 0x125 };

struct PACKET_ZC_ACK_ALL_RODEX_LIST;
struct PACKET_ZC_ACK_RODEX_LIST;
struct PACKET_ZC_ACK_READ_RODEX2;
struct PACKET_ZC_ACK_ADD_ITEM_RODEX;
struct PACKET_ZC_ACK_ADD_ITEM_RODEX2;
struct PACKET_ZC_ACK_REMOVE_RODEX_ITEM;
struct PACKET_ZC_ACK_OPEN_WRITE_RODEX;
struct PACKET_ZC_ACK_SEND_RODEX;
struct PACKET_ZC_ACK_DELETE_RODEX;
struct PACKET_ZC_ACK_ZENY_FROM_RODEX;

void game_mode_rodex_open_mail(struct GameMode *self, unsigned char *found, struct MailPair **node,
                                        const char *text, unsigned item_count, unsigned open_type, int text_length,
                                        long long mail_id, long long zeny);

int item_options_count(const struct ItemOption *options);

void game_mode_zc_rodex_block_receive(struct GameMode *self, struct String name);

#endif
