#ifndef SESSION_RANKING_H
#define SESSION_RANKING_H

struct Session;
struct RankPacket;

void session_on_ranking_packet(struct Session *session, int show, const struct RankPacket *packet);

#endif
