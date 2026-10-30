#ifndef GAMEMODE_BROADCAST_TEXT_H
#define GAMEMODE_BROADCAST_TEXT_H
#include <stddef.h>

enum { BROADCAST_CHANNEL = 0x16, BROADCAST_CHANNEL_SSSS = 0x0d };
struct BroadcastText {
    char text[1024];
    char sender[25];
    unsigned colour;
    int channel;
    int balloon;
    int log_line;
};

int broadcast_text_parse(struct BroadcastText *out, const char *payload, size_t bytes);
int broadcast_text_strip_annotation(char *text);
#endif
