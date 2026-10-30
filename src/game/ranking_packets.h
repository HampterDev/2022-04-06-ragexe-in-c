#ifndef GAMEMODE_RANKING_PACKETS_H
#define GAMEMODE_RANKING_PACKETS_H

#include <stddef.h>

int ranking_receive_names(const void *packet, size_t size);
int ranking_receive_points(const void *packet, size_t size);
int ranking_receive_ids(const void *packet, size_t size);

#endif
