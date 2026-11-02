#include "game/broadcast_text.h"
#include <limits.h>
#include <stdio.h>
#include <string.h>

static size_t bounded_length(const char *text, size_t bytes)
{
    const char *end = memchr(text, 0, bytes);
    return end ? (size_t)(end - text) : bytes;
}

static unsigned long separator_index(const char *text, size_t length, int separator)
{
    const char *found = memchr(text, separator, length);
    return found ? (unsigned long)(found - text) : ULONG_MAX;
}

static unsigned read_colour(const char *text, unsigned long value)
{
    char hex[7];

    memcpy(hex, text, 6);
    hex[6] = 0;
    sscanf_s(hex, "%lx", &value);
    return (unsigned)((value & 0xff) << 16 | (value & 0xff00) | (value >> 16 & 0xff));
}

int broadcast_text_strip_annotation(char *text, size_t capacity)
{
    char *hash = strchr(text, '#');
    char *colon = strchr(text, ':');
    size_t kept, tail = 0;

    if (!hash || hash == text)
        return 0;
    kept = (size_t)(hash - text);
    if (colon && colon != text) {
        tail = strlen(colon);
        if (tail > capacity - 1 - kept)
            tail = capacity - 1 - kept;
        memmove(hash, colon, tail);
    }
    text[kept + tail] = 0;
    return 1;
}

int broadcast_text_parse(struct BroadcastText *out, const char *payload, size_t bytes)
{
    char tag[5] = {0};
    size_t offset = 0, whole, length, i;

    memset(out, 0, sizeof *out);
    out->colour = 0xffff;
    out->channel = BROADCAST_CHANNEL;
    out->balloon = out->log_line = 1;
    if (!payload && bytes)
        return 0;
    if (bytes > sizeof out->text - 1)
        bytes = sizeof out->text - 1;
    whole = bytes ? bounded_length(payload, bytes) : 0;
    for (i = 0; i < 4 && i < bytes; ++i) {
        unsigned char c = (unsigned char)payload[i];
        tag[i] = c >= 'A' && c <= 'Z' ? c + ('a' - 'A') : c;
    }
    if (strcmp(tag, "tool") == 0) {
        if (bytes < 10)
            return 0;
        out->colour = read_colour(payload + 4, separator_index(payload, whole, ':'));
        offset = 10;
    } else if (strcmp(tag, "blue") == 0 || strcmp(tag, "블루") == 0) {
        out->colour = 0xffff00;
        offset = 4;
    } else if (strcmp(tag, "micc") == 0) {
        if (bytes < 34)
            return 0;
        out->colour = read_colour(payload + 28, separator_index(payload, whole, '#'));
        memcpy(out->sender, payload + 4, 24);
        offset = 34;
        if (bounded_length(payload + offset, bytes - offset) >= 392)
            return 0;
        out->channel = 0;
        out->balloon = 0;
    } else if (strcmp(tag, "ssss") == 0) {
        offset = 4;
        out->channel = BROADCAST_CHANNEL_SSSS;
        out->balloon = out->log_line = 0;
    }
    length = bytes ? bounded_length(payload + offset, bytes - offset) : 0;
    if (length)
        memcpy(out->text, payload + offset, length);
    out->text[length] = 0;
    if (!offset && broadcast_text_strip_annotation(out->text, sizeof out->text))
        out->balloon = 0;
    return 1;
}
