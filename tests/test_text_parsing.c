#include "ui/controls/item_id_tag.h"
#include "ui/chat/speaker_name.h"
#include "game/scream_text.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>

int main(void)
{
    const char *id = "^nItemID^501abc suffix";
    int length = -1, value = -1;
    char name[25], line[256], large[257];
    SYSTEM_INFO info;
    char *pages, *end;
    DWORD old;
    size_t n;

    assert(parse_item_id_tag(id, id + strlen(id), &length, &value));
    assert(value == 501 && length == 15);
    id = "^nItemID^999999999";
    assert(parse_item_id_tag(id, id + strlen(id), &length, &value) && value == 999999999 && length == 18);
    id = "^nItemID^2147483647";
    length = value = -1;
    assert(!parse_item_id_tag(id, id + strlen(id), &length, &value));
    assert(length == -1 && value == -1);
    id = "^nItemID^123AF";
    assert(parse_item_id_tag(id, id + strlen(id), &length, &value) && value == 123 && length == 14);
    id = "^nItemID^f";
    assert(parse_item_id_tag(id, id + strlen(id), &length, &value) && value == 0 && length == 10);
    id = "^nItemID^abc";
    assert(parse_item_id_tag(id, id + strlen(id), &length, &value) && value == 0 && length == 12);
    id = "^nItemID^1234";
    assert(parse_item_id_tag(id, id + 11, &length, &value) && value == 12 && length == 11);
    id = "^nItemID^ 5";
    assert(!parse_item_id_tag(id, id + strlen(id), &length, &value));
    id = "^nItemID^0";
    assert(parse_item_id_tag(id, id + strlen(id), &length, &value) && value == 0);

    chat_speaker_name(name, "(alice) entered", 6, -1, -1);
    assert(strcmp(name, "alice") == 0);
    chat_speaker_name(name, "alice : hello", -1, -1, 5);
    assert(strcmp(name, "alice") == 0);
    chat_speaker_name(name, "(abcdefghijklmnopqrstuvwx) entered", 25, -1, -1);
    assert(strlen(name) == 24);
    chat_speaker_name(name, "(abcdefghijklmnopqrstuvwxy) entered", 26, -1, -1);
    assert(!*name);
    chat_speaker_name(name, "short", 1000, -1, -1);
    assert(!*name);

    assert(scream_text_line(line, sizeof line, "header\r\nhe\tllo\r\nlast",
                           sizeof "header\r\nhe\tllo\r\nlast" - 1, 1));
    assert(strcmp(line, "hello") == 0);
    assert(!scream_text_line(line, sizeof line, "header", 6, 1) && !*line);
    memset(large, 'x', sizeof large);
    assert(scream_text_line(line, sizeof line, large, 255, 0) && strlen(line) == 255);
    assert(!scream_text_line(line, sizeof line, large, 256, 0) && !*line);

    GetSystemInfo(&info);
    pages = VirtualAlloc(NULL, 2 * info.dwPageSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    assert(pages && VirtualProtect(pages + info.dwPageSize, info.dwPageSize, PAGE_NOACCESS, &old));
    end = pages + info.dwPageSize;
    for (n = 0; n <= 19; ++n) {
        memcpy(end - n, "^nItemID^2147483647", n);
        parse_item_id_tag(end - n, end, &length, &value);
    }
    assert(VirtualFree(pages, 0, MEM_RELEASE));
    puts("PASS: item tags read as the original reads their hex-flagged digits, speaker fields and CR-delimited resource lines");
    return 0;
}
