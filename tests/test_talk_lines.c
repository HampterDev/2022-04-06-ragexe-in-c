#include <windows.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "core/msg_str.h"
#include "core/text_format.h"
#include "core/xml.h"
#include "game/actor_names.h"
#include "game/context.h"
#include "session/session_talk.h"
#include "session/state.h"
#include "std/string.h"


struct Session g_session1;
unsigned int g_codePage = 949;
static struct GameMode *const test_mode = (struct GameMode *)0x6d6f;
static int cases, korean;

__declspec(noreturn) void fatal_error(const char *message) { (void)message; abort(); }

#include "text_mirror.inc"

char *text_decode(char *text) { return korean ? text_decode_mirror(text) : text; }
const char *session_get_pet_name(int petJob) { assert(petJob == 1); return "poring"; }
const char *session_get_pet_act_text(int action) { assert(action == 0); return "feeding"; }
struct GameMode *game_current_mode(void) { return test_mode; }
struct String *game_mode_get_actor_name(struct GameMode *self, struct String *out, int aid)
{
    assert(self == test_mode);
    string_init(out);
    if (aid == 100)
        string_assign_cstr(out, "Poring");
    return out;
}

#include "talk_lines.inc"

struct Guarded {
    char out[0x100];
    char guard[0x40];
};

static void guard(struct Guarded *g)
{
    memset(g, 'C', sizeof *g);
}

static void check_out(const struct Guarded *g, size_t size, const char *expected)
{
    size_t i;

    assert(strcmp(g->out, expected) == 0);
    for (i = size; i < sizeof g->out; ++i)
        assert(g->out[i] == 'C');
    for (i = 0; i < sizeof g->guard; ++i)
        assert(g->guard[i] == 'C');
    ++cases;
}

static char *repeat(char *out, char c, size_t count)
{
    memset(out, c, count);
    out[count] = 0;
    return out;
}

static void format_line(const char *input, int aid, int replace, size_t size, const char *expected)
{
    char text[0x400];
    struct Guarded g;

    assert(strlen(input) < sizeof text - 1);
    memset(text, 'G', sizeof text - 1);
    text[sizeof text - 1] = 0;
    memcpy(text, input, strlen(input) + 1);
    guard(&g);
    session_format_chat_line(&g_session1, text, g.out, size, aid, replace);
    check_out(&g, size, expected);
}

static void check_format(void)
{
    char input[0x200], expected[0x200];

    g_session1.aid = 7;
    strcpy(g_session1.petName, "Pet");
    format_line("hello", 100, 0, 0x100, "Poring : hello");
    format_line("\thello\t", 100, 0, 0x100, "Poring : hello");
    format_line("%s%n%d", 100, 0, 0x100, "Poring : %s%n%d");
    format_line("hi", 7, 0, 0x100, "Pet : hi");
    format_line("hi", 999, 0, 0x100, "");
    format_line("hello", 100, 0, 4, "Por");

    format_line("형님 좋아요", 100, 1, 0x100, "Poring : 누님 좋아요");
    format_line("아빠 오빠 아빠", 100, 1, 0x100, "Poring : 엄마 오빠 엄마");
    format_line("형님 좋아요", 100, 0, 0x100, "Poring : 형님 좋아요");
    format_line("형씨는 좋아", 100, 1, 0x100, "Poring : 아가씨 좋아");
    format_line("abc형씨", 100, 1, 0x100, "Poring : abc아가씨");

    repeat(input, 'x', 300);
    strcpy(expected, "Poring : ");
    repeat(expected + 9, 'x', 0xff - 9);
    format_line(input, 100, 0, 0x100, expected);
    repeat(input, 'x', 245);
    strcat(input, "가yyy");
    repeat(expected + 9, 'x', 245);
    format_line(input, 100, 0, 0x100, expected);
}

static void parse(struct XmlDocument *doc, const char *xml)
{
    xml_element_construct(&doc->root, "XMLDocument");
    doc->copy = NULL;
    assert(xml_parse(doc, xml, xml + strlen(xml)));
}

static void release(struct XmlDocument *doc)
{
    xml_document_release(doc);
    xml_element_destruct(&doc->root);
}

static void check_pet_speak(void)
{
    static char xml[0x800];
    char line[0x200];
    struct Guarded g;

    snprintf(xml, sizeof xml, "<monster_talk_table><poring>"
             "<hungry><feeding>Yum %%s%%n</feeding></hungry>"
             "<full><feeding>%s</feeding></full>"
             "</poring></monster_talk_table>", repeat(line, 'y', 300));
    parse(&g_session1.petTalkTable, xml);
    guard(&g);
    session_get_pet_speak(&g_session1, 1, 0, 0, g.out, sizeof g.out);
    check_out(&g, sizeof g.out, "Yum %s%n");
    guard(&g);
    session_get_pet_speak(&g_session1, 1, 3, 0, g.out, sizeof g.out);
    check_out(&g, sizeof g.out, repeat(line, 'y', 0xff));
    guard(&g);
    session_get_pet_speak(&g_session1, 1, 2, 0, g.out, sizeof g.out);
    check_out(&g, sizeof g.out, "");
    release(&g_session1.petTalkTable);
}

static void check_monster_talk(void)
{
    static char xml[0x1000];
    char line[0x300], expected[0x200], name[0x140];
    const char *storage[6], **jobs = storage + 1;
    struct Guarded g;

    storage[0] = storage[5] = "PORING";
    jobs[0] = "NOVICE";
    jobs[1] = "PORING";
    jobs[2] = "DROPS";
    jobs[3] = repeat(name, 'p', 0x13f);
    g_session1.reqJobNames.first = (void *)jobs;
    g_session1.reqJobNames.last = g_session1.reqJobNames.end = (void *)(jobs + 4);
    snprintf(xml, sizeof xml, "<enemy_monster_talk_table>"
             "<poring><attack>Grr %%s</attack></poring>"
             "<drops><attack>%s</attack></drops>"
             "</enemy_monster_talk_table>", repeat(line, 'g', 0x2f0));
    parse(&g_session1.monsterTalkTable, xml);
    guard(&g);
    session_get_monster_talk_info(&g_session1, 1, 0, 0, g.out, sizeof g.out, 100);
    check_out(&g, sizeof g.out, "Poring : Grr %s");
    guard(&g);
    session_get_monster_talk_info(&g_session1, 2, 0, 0, g.out, sizeof g.out, 100);
    strcpy(expected, "Poring : ");
    check_out(&g, sizeof g.out, strcat(expected, repeat(line, 'g', 0xff - 9)));
    guard(&g);
    session_get_monster_talk_info(&g_session1, 3, 0, 0, g.out, sizeof g.out, 100);
    check_out(&g, sizeof g.out, "");
    guard(&g);
    session_get_monster_talk_info(&g_session1, 0, 0, 0, g.out, sizeof g.out, 100);
    check_out(&g, sizeof g.out, "");
    guard(&g);
    session_get_monster_talk_info(&g_session1, 4, 0, 0, g.out, sizeof g.out, 100);
    check_out(&g, sizeof g.out, "");
    guard(&g);
    session_get_monster_talk_info(&g_session1, -1, 0, 0, g.out, sizeof g.out, 100);
    check_out(&g, sizeof g.out, "");
    release(&g_session1.monsterTalkTable);
    g_session1.reqJobNames.first = g_session1.reqJobNames.last = g_session1.reqJobNames.end = NULL;
}

static void stored(char *xml, const char *text)
{
    char copy[0x100];

    assert(strlen(text) < sizeof copy);
    strcpy(copy, text);
    strcat(xml, text_decode_mirror(copy));
}

static void tag(char *xml, const char *name, int close)
{
    strcat(xml, close ? "</" : "<");
    stored(xml, name);
    strcat(xml, ">");
}

static void check_monster_talk_korean(void)
{
    static char xml[0x400];
    const char *jobs[2] = { "NOVICE", "PORING" }, *literal;
    char key[0x40];
    MEMORY_BASIC_INFORMATION page;
    struct XmlElement *node;
    struct Guarded g;
    int i, seen[2] = { 0, 0 };

    literal = session_get_state_name(&g_session1, 0);
    assert(strcmp(literal, "attack") == 0 && VirtualQuery(literal, &page, sizeof page) == sizeof page);
    assert(!(page.Protect & (PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READWRITE |
                             PAGE_EXECUTE_WRITECOPY)));

    g_session1.reqJobNames.first = (void *)jobs;
    g_session1.reqJobNames.last = g_session1.reqJobNames.end = (void *)(jobs + 2);
    xml[0] = 0;
    tag(xml, "enemy_monster_talk_table", 0);
    tag(xml, "poring", 0);
    tag(xml, "attack", 0);
    stored(xml, "Grr one");
    tag(xml, "attack", 1);
    tag(xml, "attack", 0);
    stored(xml, "Grr two");
    tag(xml, "attack", 1);
    tag(xml, "poring", 1);
    tag(xml, "enemy_monster_talk_table", 1);
    parse(&g_session1.monsterTalkTable, xml);
    korean = 1;
    srand(7);
    for (i = 0; i < 64; ++i) {
        guard(&g);
        session_get_monster_talk_info(&g_session1, 1, 0, 0, g.out, sizeof g.out, 100);
        if (strcmp(g.out, "Poring : Grr one") == 0)
            seen[0] = 1;
        else
            check_out(&g, sizeof g.out, "Poring : Grr two"), seen[1] = 1;
    }
    assert(seen[0] && seen[1]);
    korean = 0;
    strcpy(key, "enemy_monster_talk_table");
    node = xml_find_child(&g_session1.monsterTalkTable.root, text_decode_mirror(key));
    strcpy(key, "poring");
    node = node ? xml_find_child(node, text_decode_mirror(key)) : NULL;
    strcpy(key, "attack");
    node = node ? xml_find_child(node, text_decode_mirror(key)) : NULL;
    assert(node);
    strcpy(key, "Grr one");
    assert(strcmp(string_cstr(xml_node_text(node)), text_decode_mirror(key)) == 0);
    ++cases;
    release(&g_session1.monsterTalkTable);
    g_session1.reqJobNames.first = g_session1.reqJobNames.last = g_session1.reqJobNames.end = NULL;
}

int main(void)
{
    check_format();
    check_pet_speak();
    check_monster_talk();
    check_monster_talk_korean();
    printf("Talk lines: %d lines; long lines and names are cut, conversions copied, words replaced as before.\n",
           cases);
    return 0;
}
