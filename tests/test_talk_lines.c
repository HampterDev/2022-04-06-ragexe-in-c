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

/* Run the production pet and monster talk lines (session_get_pet_speak,
   session_get_monster_talk_info, session_format_chat_line) over parsed
   PetTalkTable.xml and MonsterTalkTable.xml, writing into 256-byte outputs with
   a guard after them. Long lines and names must be cut to the output by whole
   CP949 characters, printf conversions must arrive as text, and the
   talk-word replacement must give the lines it gave before. */

struct Session g_session1;
unsigned int g_codePage = 949;
static struct GameMode *const test_mode = (struct GameMode *)0x6d6f;
static int cases;

__declspec(noreturn) void fatal_error(const char *message) { (void)message; abort(); }
/* Other services than Korea leave the text as it is. */
char *text_decode(char *text) { return text; }
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

/* The line in a caller's buffer whose bytes after the NUL are not NUL. */
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

    /* The talk words, as before: each search starts after the last replacement. */
    format_line("형님 좋아요", 100, 1, 0x100, "Poring : 누님 좋아요");
    format_line("아빠 오빠 아빠", 100, 1, 0x100, "Poring : 엄마 오빠 엄마");
    format_line("형님 좋아요", 100, 0, 0x100, "Poring : 형님 좋아요");
    /* 형씨 is shorter than 아가씨, so as before the rest skips two more bytes, but no
       longer from past the end of the line. */
    format_line("형씨는 좋아", 100, 1, 0x100, "Poring : 아가씨 좋아");
    format_line("abc형씨", 100, 1, 0x100, "Poring : abc아가씨");

    repeat(input, 'x', 300);
    strcpy(expected, "Poring : ");
    repeat(expected + 9, 'x', 0xff - 9);
    format_line(input, 100, 0, 0x100, expected);
    /* 9 + 245 bytes, then a two-byte character over the 255-byte limit. */
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
    const char *jobs[4];
    struct Guarded g;

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
    release(&g_session1.monsterTalkTable);
    g_session1.reqJobNames.first = g_session1.reqJobNames.last = g_session1.reqJobNames.end = NULL;
}

int main(void)
{
    check_format();
    check_pet_speak();
    check_monster_talk();
    printf("Talk lines: %d lines; long lines and names are cut, conversions copied, words replaced as before.\n",
           cases);
    return 0;
}
