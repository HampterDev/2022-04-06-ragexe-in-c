#include "game/actor_packets.h"
#include "game/gamemode.h"
#include "game/ground_item_packets.h"
#include <string.h>
#include "game/packet_handlers.h"
#include "game/client.h"
#include "core/msg_str.h"
#include "session/session_windows.h"

#define COLOUR_RED 0xff
#define WND_BANK 0x113

#pragma pack(push, 1)
struct PacketBankingAck {
    unsigned short header;
    unsigned short result;
    long long balance;
    int zeny;
};
struct PacketBankingCheck {
    unsigned short header;
    long long balance;
    unsigned short reason;
};
#pragma pack(pop)
SIZE_ASSERT(PacketBankingAck, 16);
OFFSET_ASSERT(PacketBankingAck, balance, 4);
SIZE_ASSERT(PacketBankingCheck, 12);
OFFSET_ASSERT(PacketBankingCheck, balance, 2);

static void say_red(const char *text)
{
    ui_window_mgr_send_msg_(&g_windowMgr, UIM_CHAT_LINE, (int)text, COLOUR_RED, 0, 0);
}

static void store_balance(const struct PacketBankingAck *p)
{
    g_session1.bankZeny = p->balance;
    g_session1.zeny = p->zeny;
    session_refresh_basic_info(&g_session1);
    session_invalidate_windows(&g_session1);
}

static void say_failure(int msg_id)
{
    char text[0x100];

    memset(text, 0, 0xfa);
    sprintf(text, msg_str(msg_id));
    say_red(text);
}

void game_mode_ack_banking_deposit(struct GameMode *self, const struct PacketBankingAck *p)
{
    (void)self;
    if (p->result == 0) {
        store_balance(p);
        return;
    }
    if (p->result == 2)
        say_failure(0x998);
    else if (p->result == 3)
        say_failure(0x9a1);
    else if (p->result == 4)
        say_failure(0x9b9);
    else
        say_failure(0x996);
}

void game_mode_ack_banking_withdraw(struct GameMode *self, const struct PacketBankingAck *p)
{
    (void)self;
    if (p->result == 0) {
        store_balance(p);
        return;
    }
    if (p->result == 1)
        say_failure(0x997);
    else if (p->result == 3)
        say_failure(0x9b9);
    else
        say_failure(0x996);
}

void game_mode_banking_check(struct GameMode *self, const struct PacketBankingCheck *p)
{
    (void)self;
    if (p->reason == 0) {
        g_session1.bankZeny = p->balance;
        if (!ui_window_mgr_post_quit1(&g_windowMgr, WND_BANK))
            ui_window_open(WND_BANK);
        return;
    }
    if (p->reason == 2)
        say_red(msg_str(0xbd7));
    else
        say_failure(0x996);
}
