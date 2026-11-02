#include "game/broadcast_text.h"
#include "core/utf8.h"
#include <string.h>

static size_t bounded_length(const char *text, size_t bytes)
{
    const char *end = memchr(text, 0, bytes);
    return end ? (size_t)(end - text) : bytes;
}

static int read_colour(const char *text, unsigned *colour)
{
    unsigned rgb = 0;
    int i;

    for (i = 0; i < 6; ++i) {
        unsigned char c = (unsigned char)text[i];
        unsigned digit;
        if (c >= '0' && c <= '9') digit = c - '0';
        else if (c >= 'a' && c <= 'f') digit = c - 'a' + 10;
        else if (c >= 'A' && c <= 'F') digit = c - 'A' + 10;
        else return 0;
        rgb = (rgb << 4) | digit;
    }
    *colour = (rgb & 0xff) << 16 | (rgb & 0xff00) | (rgb >> 16);
    return 1;
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
            tail = utf8_prefix(colon, tail, capacity - 1 - kept);
        memmove(hash, colon, tail);
    }
    text[kept + tail] = 0;
    return 1;
}

/* The Korean blue tag, 블루. It is six bytes in UTF-8, so it is matched apart from the
   four-byte ASCII tags. */
#define BLUE_TAG_KR "\xeb\xb8\x94\xeb\xa3\xa8"

int broadcast_text_parse(struct BroadcastText *out, const char *payload, size_t bytes)
{
    char tag[5] = {0};
    size_t offset = 0, length, i;

    memset(out, 0, sizeof *out);
    out->colour = 0xffff;
    out->channel = BROADCAST_CHANNEL;
    out->balloon = out->log_line = 1;
    if (!payload && bytes)
        return 0;
    if (bytes > sizeof out->text - 1)
        bytes = sizeof out->text - 1;
    for (i = 0; i < 4 && i < bytes; ++i) {
        unsigned char c = (unsigned char)payload[i];
        tag[i] = c >= 'A' && c <= 'Z' ? c + ('a' - 'A') : c;
    }
    if (strcmp(tag, "tool") == 0) {
        if (bytes < 10 || !read_colour(payload + 4, &out->colour))
            return 0;
        offset = 10;
    } else if (strcmp(tag, "blue") == 0) {
        out->colour = 0xffff00;
        offset = 4;
    } else if (bytes >= sizeof BLUE_TAG_KR - 1 && memcmp(payload, BLUE_TAG_KR, sizeof BLUE_TAG_KR - 1) == 0) {
        out->colour = 0xffff00;
        offset = sizeof BLUE_TAG_KR - 1;
    } else if (strcmp(tag, "micc") == 0) {
        if (bytes < 34 || !read_colour(payload + 28, &out->colour))
            return 0;
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
