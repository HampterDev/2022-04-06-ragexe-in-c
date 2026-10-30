#include "game/mode_msg.h"
#include "game/chat.h"
#include "ui/window_manager_state.h"
#include "core/fatal.h"
#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Session g_session1;
struct UIWindowMgr g_windowMgr;
static char input[8193];
static unsigned char expected[366];
static size_t expected_length;
static unsigned sent, cleared, reads, cases;
static int connection_token;
static FILE *trace;
static uint32_t digest = 2166136261u;
static int recording = 1;

static void record(const void *data, size_t size)
{
    const unsigned char *bytes = data;
    size_t i;
    if (!recording)
        return;
    for (i = 0; i < size; ++i)
        digest = (digest ^ bytes[i]) * 16777619u;
    if (trace)
        assert(fwrite(data, 1, size, trace) == size);
}

const char *window_mgr_get_chat_line(struct UIWindowMgr *manager)
{
    assert(manager == &g_windowMgr && !cleared);
    ++reads;
    return input;
}

void window_mgr_clear_chat_line(struct UIWindowMgr *manager)
{
    const unsigned char event = 0xff;
    assert(manager == &g_windowMgr && !cleared);
    assert(sent == (expected_length != 0));
    input[0] = 0;
    ++cleared;
    record(&event, 1);
}

struct RagConnection *rag_connection_instance(void)
{
    assert(!cleared);
    return (struct RagConnection *)&connection_token;
}

char rag_connection_send_packet(struct RagConnection *connection, int length, const void *packet)
{
    assert(connection == (struct RagConnection *)&connection_token && !sent && !cleared);
    assert((size_t)length == expected_length && length >= 5 && length <= 366);
    assert(memcmp(packet, expected, expected_length) == 0);
    ++sent;
    record(packet, expected_length);
    return 1;
}

#ifdef BROADCAST_REFERENCE
struct WordFilter g_nameFilter;
static unsigned allocations;

int word_filter_is_clean(const struct WordFilter *filter, const char *text)
{
    assert(filter == &g_nameFilter && strcmp(text, input) == 0);
    return cases & 1;
}

void *mem_alloc(size_t size)
{
    void *p = malloc(size);
    assert(p);
    ++allocations;
    return p;
}

void mem_free(void *p)
{
    if (p) {
        assert(allocations);
        --allocations;
        free(p);
    }
}

__declspec(noreturn) void fatal_error(const char *message)
{
    fprintf(stderr, "%s\n", message);
    abort();
}

void string_insert(struct String *s, size_t pos, const char *text, size_t len)
{
    abort();
}

unsigned __int64 *crt_printf_options(void)
{
    abort();
}
#endif

static void check_valid(int kind, const char *name, const char *text)
{
    static const unsigned ids[] = {0x99, 0x19c, 0x99, 0x19c, 0x99, 0x19c};
    char formatted[sizeof input + 64];
    size_t length;
    assert(strlen(name) < sizeof g_session1.charName);
    strcpy(g_session1.charName, name);
    strcpy(input, text);
    sent = cleared = reads = 0;
    expected_length = 0;
    if (*text) {
        formatted[0] = 0;
        switch (kind) {
        case 0:
        case 1:
            strcat(formatted, name);
            strcat(formatted, ": ");
            break;
        case 4:
        case 5:
            strcat(formatted, "blue");
            break;
        }
        strcat(formatted, text);
        length = strlen(formatted);
        if (length > 361)
            length = 361;
        expected_length = length + 5;
        expected[0] = (unsigned char)ids[kind];
        expected[1] = (unsigned char)(ids[kind] >> 8);
        expected[2] = (unsigned char)expected_length;
        expected[3] = (unsigned char)(expected_length >> 8);
        memcpy(expected + 4, formatted, length);
        expected[4 + length] = 0;
    }
#ifdef BROADCAST_REFERENCE
    assert(game_mode_msg_broadcast(NULL, kind) == 0);
    assert(!allocations);
#else
    assert(game_mode_msg_broadcast(kind) == 0);
#endif
    assert(cleared == 1 && !input[0] && reads && sent == (*text != 0));
    ++cases;
}

int main(int argc, char **argv)
{
    static const char *names[] = {"", "Alice", "가나다라마바사", "abcdefghijklmnopqrstuvw"};
    static const char *patterns[] = {"Ab%12 <iteml>42</iteml>", "한글 메시지%"};
    char text[sizeof input];
    size_t n, p, length, i;
    int kind;
    assert(strlen("한") == 2);
    if (argc > 1) {
        trace = fopen(argv[1], "wb");
        assert(trace);
    }
    for (n = 0; n < sizeof names / sizeof *names; ++n)
        for (p = 0; p < sizeof patterns / sizeof *patterns; ++p)
            for (length = 0; length <= 511; ++length) {
                for (i = 0; i < length; ++i)
                    text[i] = patterns[p][i % strlen(patterns[p])];
                text[length] = 0;
                for (kind = 0; kind < 6; ++kind)
                    check_valid(kind, names[n], text);
            }
    if (trace)
        assert(fclose(trace) == 0);
    assert(digest == 0x1bbadc4du);
    printf("Broadcast packets: %u valid cases; digest=%08x.\n", cases, digest);
    recording = 0;
#ifndef BROADCAST_REFERENCE
    {
        static const int invalid[] = {INT_MIN, -100, -1, 6, 7, 100, INT_MAX};
        for (i = 0; i < sizeof invalid / sizeof *invalid; ++i) {
            strcpy(input, "keep this message");
            sent = cleared = reads = 0;
            assert(game_mode_msg_broadcast(invalid[i]) == 0);
            assert(!sent && !cleared && !reads && strcmp(input, "keep this message") == 0);
        }
    }
    memset(text, 'X', sizeof text - 1);
    text[sizeof text - 1] = 0;
    for (kind = 0; kind < 6; ++kind)
        check_valid(kind, names[3], text);
    puts("Invalid kinds preserve input; oversized input is bounded.");
#endif
    return 0;
}
