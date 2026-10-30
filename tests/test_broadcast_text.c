#include "game/broadcast_text.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>

static void plain(const char *input, const char *text, unsigned colour, int balloon, int log_line, int channel)
{
    struct BroadcastText result;
    assert(broadcast_text_parse(&result, input, strlen(input)));
    assert(strcmp(result.text, text) == 0);
    assert(result.colour == colour && result.balloon == balloon);
    assert(result.log_line == log_line && result.channel == channel);
}

int main(void)
{
    struct BroadcastText result;
    char micc[500] = "micc";
    SYSTEM_INFO info;
    unsigned char *pages, *end;
    DWORD old;
    size_t n, i;
    const char *tags[] = {"tool", "micc", "blue", "ssss", "블루"};

    plain("hello", "hello", 0xffff, 1, 1, BROADCAST_CHANNEL);
    plain("TOOLaB12Efhello", "hello", 0xef12ab, 1, 1, BROADCAST_CHANNEL);
    plain("BlUehello", "hello", 0xffff00, 1, 1, BROADCAST_CHANNEL);
    plain("블루hello", "hello", 0xffff00, 1, 1, BROADCAST_CHANNEL);
    plain("sssshello", "hello", 0xffff, 0, 0, BROADCAST_CHANNEL_SSSS);
    plain("name#tag: hello", "name: hello", 0xffff, 0, 1, BROADCAST_CHANNEL);
    plain("name#tag", "name", 0xffff, 0, 1, BROADCAST_CHANNEL);
    plain("#tag: hello", "#tag: hello", 0xffff, 1, 1, BROADCAST_CHANNEL);
    assert(!broadcast_text_parse(&result, "tool", 4));
    assert(!broadcast_text_parse(&result, "tool12345xhello", 15));
    assert(!broadcast_text_parse(&result, "micc", 4));
    assert(broadcast_text_parse(&result, NULL, 0) && !result.text[0]);
    assert(!broadcast_text_parse(&result, NULL, 1));

    memcpy(micc + 4, "sender", 6);
    memcpy(micc + 28, "123456", 6);
    memset(micc + 34, 'x', 392);
    assert(broadcast_text_parse(&result, micc, 34 + 391));
    assert(strlen(result.text) == 391 && strcmp(result.sender, "sender") == 0);
    assert(result.colour == 0x563412 && !result.balloon && result.log_line && !result.channel);
    assert(!broadcast_text_parse(&result, micc, 34 + 392));
    micc[34 + 3] = 0;
    assert(broadcast_text_parse(&result, micc, sizeof micc) && strlen(result.text) == 3);

    GetSystemInfo(&info);
    pages = VirtualAlloc(NULL, 2 * info.dwPageSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    assert(pages && VirtualProtect(pages + info.dwPageSize, info.dwPageSize, PAGE_NOACCESS, &old));
    end = pages + info.dwPageSize;
    for (i = 0; i < sizeof tags / sizeof tags[0]; ++i) {
        for (n = 0; n <= 1100; ++n) {
            unsigned char *payload = end - n;
            struct { unsigned before; struct BroadcastText value; unsigned after; } guarded;
            memset(payload, 'a', n);
            memcpy(payload, tags[i], n < 4 ? n : 4);
            guarded.before = guarded.after = 0x12345678;
            broadcast_text_parse(&guarded.value, (const char *)payload, n);
            assert(guarded.before == 0x12345678 && guarded.after == 0x12345678);
            assert(memchr(guarded.value.text, 0, sizeof guarded.value.text));
            assert(memchr(guarded.value.sender, 0, sizeof guarded.value.sender));
        }
    }
    memset(end - 1100, 'x', 1100);
    assert(broadcast_text_parse(&result, (const char *)end - 1100, 1100));
    assert(strlen(result.text) == 1023);
    assert(VirtualFree(pages, 0, MEM_RELEASE));
    puts("PASS: broadcast tags, colours, sender fields, limits and 5505 guarded payload boundaries");
    return 0;
}
