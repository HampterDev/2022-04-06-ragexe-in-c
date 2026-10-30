#ifndef GAMEMODE_REPLAY_NAVIGATION_H
#define GAMEMODE_REPLAY_NAVIGATION_H
#include <stddef.h>
int game_replay_navigation_results(const void *data, size_t size);
int game_replay_navigation_route(const void *data, size_t size);
int game_replay_navigation_map(const void *data, size_t size);
#endif
