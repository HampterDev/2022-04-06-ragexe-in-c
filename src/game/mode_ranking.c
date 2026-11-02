#include "game/ranking_packets.h"
#include "session/state.h"
#include "contents/client_res.h"
#include "ui/window_messages.h"
#include "contents/ranking.h"
#include "contents/ranking_style.h"
#include "contents/ranking_output.h"
#include "session/ranking.h"
#include <string.h>
#include "core/msg_str.h"

#pragma pack(push, 1)
struct PacketAckRanking {
    unsigned short header;
    short type;
    char names[RANK_ENTRIES][24];
    int points[RANK_ENTRIES];
    int my_points;
};
struct PacketUpdateRankingPoint {
    unsigned short header;
    short type;
    int points, total;
};
#pragma pack(pop)
SIZE_ASSERT(PacketAckRanking, 288);
SIZE_ASSERT(PacketUpdateRankingPoint, 12);

static void say(const char *text, int color)
{
    ui_window_mgr_send_msg_(&g_windowMgr, UIWM_MSG_CHAT_LINE, (intptr_t)text, color, 0, 0);
}

int ranking_receive_names(const void *packet, size_t size)
{
    struct PacketAckRanking p;
    const struct RankingStyle *style;
    int rank;

    if (!packet || size < sizeof p) return 0;
    memcpy(&p, packet, sizeof p);
    style = ranking_style(p.type);
    say(style->title, style->title_color);
    for (rank = 1; rank <= RANK_ENTRIES; ++rank) {
        ranking_line(style->line_color, "[ %2d ] %-18.24s  :  %4d POINT", rank, p.names[rank - 1], p.points[rank - 1]);
    }
    say(RANK_SEPARATOR, style->title_color);
    ranking_line(style->line_color, "MY POINT    :  %4d POINT ", p.my_points);
    say(RANK_SEPARATOR, style->title_color);
    return 1;
}

int ranking_receive_points(const void *packet, size_t size)
{
    struct PacketUpdateRankingPoint p;
    int msg_id;

    if (!packet || size < sizeof p) return 0;
    memcpy(&p, packet, sizeof p);
    switch (p.type) {
    case RANKING_BLACKSMITH: msg_id = 0x385; break;
    case RANKING_ALCHEMIST: msg_id = 0x386; break;
    case RANKING_TAEKWON: msg_id = 0x39e; break;
    case RANKING_KILLER: msg_id = 0x3b9; break;
    default: return 1;
    }
    ranking_line(ranking_style(p.type)->line_color, msg_str(msg_id), p.points, p.total);
    return 1;
}

int ranking_receive_ids(const void *packet, size_t size)
{
    struct RankPacket p;
    if (!packet || size < sizeof p) return 0;
    memcpy(&p, packet, sizeof p);
    session_on_ranking_packet(&g_session1, 1, &p);
    return 1;
}
