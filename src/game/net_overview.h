#ifndef GAME_NET_OVERVIEW_H
#define GAME_NET_OVERVIEW_H

struct GameMode;

/* custom: two lines at the top of the game's frame with what a moment of play is found by in
   the server's logs and journal: the server's tick now, how far ahead of it the player's own
   character is drawn and how far behind everyone else, the round trip, the number of the last
   request sent (the server logs every request with its number and tick), the last move's and
   the last intent's, and the cell and state of the player's character. Shown when the
   environment has RAGEXE_NET_OVERVIEW set to anything but 0, so that a recording of the screen
   carries it. Drawn after the scene, over everything. */
void game_mode_draw_net_overview(struct GameMode *self);

#endif
