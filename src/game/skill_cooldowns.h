#ifndef GAMEMODE_SKILL_COOLDOWNS_H
#define GAMEMODE_SKILL_COOLDOWNS_H
#include <stddef.h>

int skill_cooldowns_receive_delay(const void *packet, size_t size);
int skill_cooldowns_receive_delay2(const void *packet, size_t size);
int skill_cooldowns_receive_list(const void *packet, size_t size);
int skill_cooldowns_receive_list2(const void *packet, size_t size);
#endif
