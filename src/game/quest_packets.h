#ifndef GAMEMODE_QUEST_PACKETS_H
#define GAMEMODE_QUEST_PACKETS_H
#include <stddef.h>
struct GameMode;

void game_mode_all_quest_list3(struct GameMode *mode, const void *packet, size_t size);
void game_mode_all_quest_list4(struct GameMode *mode, const void *packet, size_t size);
void game_mode_zc_add_quest_ex(struct GameMode *mode, const void *packet, size_t size);
void game_mode_zc_add_quest_ex2(struct GameMode *mode, const void *packet, size_t size);
void game_mode_del_quest(struct GameMode *mode, const void *packet, size_t size);
void game_mode_zc_active_quest(const void *packet, size_t size);
void game_mode_zc_update_mission_hunt_ex(struct GameMode *mode, const void *packet, size_t size);
void game_mode_zc_update_mission_hunt_ex2(struct GameMode *mode, const void *packet, size_t size);
void game_mode_zc_hunting_quest_info(struct GameMode *mode, const void *packet, size_t size);
#endif
