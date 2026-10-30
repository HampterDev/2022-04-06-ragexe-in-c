#include "ui/chat/reported_players.h"

struct ReportedPlayer *reported_player_init(struct ReportedPlayer *player,
    const struct String *name, int aid, uint16_t job)
{
    string_copy(&player->name, name);
    player->aid = aid;
    player->job = job;
    return player;
}

void reported_players_clear(struct Vec *players)
{
    struct ReportedPlayer *p;

    for (p = players->first; p != players->last; p++)
        string_free(&p->name);
    vec_clear(players);
}

void reported_players_free(struct Vec *players)
{
    reported_players_clear(players);
    vec_free(players);
}
