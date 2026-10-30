#ifndef REPLAY_EVENT_H
#define REPLAY_EVENT_H
#include <stdint.h>
#pragma pack(push, 1)
struct ReplayEventHeader { uint16_t id, kind, size; };
#pragma pack(pop)
#endif
