#ifndef REPLAY_PACKET_READER_H
#define REPLAY_PACKET_READER_H

#include <stddef.h>

int replay_read_packet(void *buffer, size_t capacity,
                       unsigned short *packet_id, size_t *size);

#endif
