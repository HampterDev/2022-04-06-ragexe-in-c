#include "contents/quest.h"
#include "core/msg_str.h"
#include "std/string.h"
#include "utf8_text.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* 무형족 and 무형 in UTF-8. */
#define FORMLESS_RACE "\xeb\xac\xb4\xed\x98\x95\xec\xa1\xb1"
#define FORMLESS "\xeb\xac\xb4\xed\x98\x95"

static const char *texts[3];

void fatal_error(const char *message) { (void)message; abort(); }
char *msg_str(int id)
{
    switch (id) {
    case 0x8d7: return (char *)texts[0];   /* drops its last character */
    case 0x8da: return (char *)texts[1];   /* drops its last character */
    case 0x8e3: return (char *)texts[2];   /* kept whole */
    default: abort();
    }
}

static void expect(unsigned mask, int min, int max, const char *expected)
{
    struct String out;
    renew_quest_condition_text(NULL, &out, mask, min, max);
    assert(!strcmp(string_cstr(&out), expected));
    string_free(&out);
}

int main(void)
{
    texts[0] = FORMLESS_RACE;
    texts[1] = "Formless";
    texts[2] = U8_HAN U8_GUK;
    /* The last two bytes were a Korean syllable in CP949; in UTF-8 the syllable goes whole. */
    expect(0x00020000, -1, -1, FORMLESS " ");
    expect(0x00040000, -1, -1, "Formle ");
    expect(0x00000010, -1, -1, U8_HAN U8_GUK " ");
    expect(0x00020000 | 0x00000010, -1, -1, FORMLESS " " U8_HAN U8_GUK " ");
    /* A character before an ASCII ending goes with it; an emoji goes whole. */
    texts[0] = U8_HAN "a";
    expect(0x00020000, -1, -1, " ");
    texts[0] = "ab" U8_GRIN;
    expect(0x00020000, -1, -1, "ab ");
    /* Level marks are UTF-8. */
    expect(0, 0, 30, " Lv.30\xe2\x96\xbc");
    expect(0, 10, 0, " Lv.10\xe2\x96\xb2");
    expect(0, 10, 20, " Lv.10~Lv.20");
    expect(0, 0, 0, "");
    puts("Renewed quest conditions: names cut by whole UTF-8 characters and UTF-8 level marks passed.");
    return 0;
}
