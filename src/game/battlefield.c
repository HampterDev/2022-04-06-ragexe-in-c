#include "game/barter_packets.h"
#include "game/gamemode.h"
#include "game/item_list_packets.h"
#include "game/packet_handlers.h"
#include "game/quest_packets.h"
#include "game/actor_marks.h"
#include "ui/window_access.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include <string.h>
#include "game/packets.h"
#include "game/packet_dispatch.h"
#include "game/chat.h"
#include "platform/clientinfo.h"
#include "session/session_jobs.h"

#define ACTOR_MSG_SAY           7
#define ACTOR_MSG_HP            0x22
#define MM_BATTLEGROUND_MEMBER  0x3f
#define BATTLEGROUND_CHAT_COLOR 0x9b00ff
#define BATTLEGROUND_TEXT_MAX   0x100

#pragma pack(push, 1)
struct PACKET_ZC_BATTLEFIELD_NOTIFY_CAMPINFO {
    unsigned short id;
    unsigned aid;
    char name[24];
    BYTE camp;
    BYTE camp_high;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_BATTLEFIELD_NOTIFY_CAMPINFO, 32);

void game_mode_zc_battlefield_notify_camp_info(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_BATTLEFIELD_NOTIFY_CAMPINFO *packet = (const struct PACKET_ZC_BATTLEFIELD_NOTIFY_CAMPINFO *)pkt;
    unsigned aid = packet->aid;
    struct GameActor *actor;

    if (aid == ((unsigned)g_session1.ownAid)) {
        g_session1.battlegroundCamp = packet->camp;
        return;
    }
    actor = world_get_game_actor_by_aid(self->world, aid);
    if (!actor)
        return;
    actor->camp = packet->camp;
    if (actor->camp != g_session1.battlegroundCamp) {
        game_mode_make_word_display_wnd(self, actor);
    } else if (actor->word_display_wnd) {
        ui_window_mgr_post_quit2(&g_windowMgr, actor->word_display_wnd);
        actor->word_display_wnd = 0;
    }
}

#pragma pack(push, 1)
struct PACKET_ZC_BATTLEFIELD_NOTIFY_POINT {
    unsigned short id;
    short points_a;
    short points_b;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_BATTLEFIELD_NOTIFY_POINT, 6);

void game_mode_zc_battlefield_notify_point(const unsigned char *pkt)
{
    const struct PACKET_ZC_BATTLEFIELD_NOTIFY_POINT *packet = (const struct PACKET_ZC_BATTLEFIELD_NOTIFY_POINT *)pkt;

    g_session1.battlegroundPoints[0] = packet->points_a;
    g_session1.battlegroundPoints[1] = packet->points_b;
}

#pragma pack(push, 1)
struct PACKET_ZC_BATTLEFIELD_NOTIFY_POSITION {
    unsigned short id;
    unsigned aid;
    char name[24];
    short job;
    short x;
    short y;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_BATTLEFIELD_NOTIFY_POSITION, 36);

void game_mode_zc_battlefield_notify_position(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_BATTLEFIELD_NOTIFY_POSITION *packet = (const struct PACKET_ZC_BATTLEFIELD_NOTIFY_POSITION *)pkt;
    unsigned aid = packet->aid;

    if (aid != ((unsigned)g_session1.ownAid))
        mode_send_msg(self, MM_BATTLEGROUND_MEMBER, aid, packet->x, packet->y,
                     0);
}

static void battleground_hp(struct GameMode *self, unsigned aid, int hp, int max_hp)
{
    struct GameActor *actor;

    if (session_is_own_unit_aid(&g_session1, aid))
        return;
    actor = world_get_game_actor_by_aid(self->world, aid);
    if (actor)
        actor_send(actor, ACTOR_MSG_HP, hp, max_hp);
}

#pragma pack(push, 1)
struct PACKET_ZC_BATTLEFIELD_NOTIFY_HP {
    unsigned short id;
    unsigned aid;
    char name[24];
    short hp;
    short max_hp;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_BATTLEFIELD_NOTIFY_HP, 34);

void game_mode_zc_battlefield_notify_hp(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_BATTLEFIELD_NOTIFY_HP *packet = (const struct PACKET_ZC_BATTLEFIELD_NOTIFY_HP *)pkt;

    battleground_hp(self, packet->aid, packet->hp, packet->max_hp);
}

#pragma pack(push, 1)
struct PACKET_ZC_BATTLEFIELD_NOTIFY_HP2 {
    unsigned short id;
    unsigned aid;
    int hp;
    int max_hp;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_BATTLEFIELD_NOTIFY_HP2, 14);

void game_mode_zc_battlefield_notify_hp2(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_BATTLEFIELD_NOTIFY_HP2 *packet = (const struct PACKET_ZC_BATTLEFIELD_NOTIFY_HP2 *)pkt;

    battleground_hp(self, packet->aid, packet->hp, packet->max_hp);
}

#pragma pack(push, 1)
struct PACKET_ZC_BATTLEFIELD_CHAT {
    unsigned short id;
    unsigned short length;
    unsigned aid;
    char name[24];
    char text[];
};
#pragma pack(pop)

void game_mode_zc_battlefield_chat(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_BATTLEFIELD_CHAT *packet = (const struct PACKET_ZC_BATTLEFIELD_CHAT *)pkt;
    size_t length = packet->length - 0x20;
    unsigned aid = packet->aid;
    struct Vec text;
    struct GameActor *speaker;

    if (length - 1 > BATTLEGROUND_TEXT_MAX - 1)
        return;
    vec_init(&text);
    vec_resize(&text, length, 1);
    memcpy(text.first, packet->text, (char *)text.last - (char *)text.first);
    if (g_session1.hideChat != 1 && g_session1.hideChatAndGuild != 1)
        ui_window_mgr_send_msg_(&g_windowMgr, 1, (intptr_t)text.first, BATTLEGROUND_CHAT_COLOR, 1, 0);
    speaker = world_get_game_actor_by_aid(self->world, aid);
    if (speaker)
        actor_send(speaker, ACTOR_MSG_SAY, (int)text.first, 0);
    if (g_session1.autoSaveChat != 0)
        game_mode_log_whisper(self, text.first);
    vec_free(&text);
}
