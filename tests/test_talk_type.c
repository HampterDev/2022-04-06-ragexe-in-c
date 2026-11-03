#include "session/csession.h"
#include "session/session_talk.h"
#include "session/session_names.h"
#include "ui/window_manager_state.h"
#include "ui/wndmgr/wndmgr.h"
#include "world/mode_dispatch.h"
#include "game/context.h"
#include "contents/sound.h"
#include "core/msg_str.h"
#include "core/text_util.h"
#include "std/string.h"
#include "std/string_vec.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Korean commands in UTF-8. */
#define BROADCAST   "/\xe3\x85\xa0"                                              /* /ㅠ */
#define AREA_BLUE   "/\xec\xa7\x80\xec\x97\xad\xed\x8c\x8c\xeb\x9e\x80\xeb\xa7\x90" /* /지역파란말 */
#define BLUE        "/\xed\x8c\x8c\xeb\x9e\x80\xeb\xa7\x90"                       /* /파란말 */
#define EXCLUDE     "/\xec\xb0\xa8\xeb\x8b\xa8"                                  /* /차단 */
#define INCLUDE     "/\xed\x95\xb4\xec\xa0\x9c"                                  /* /해제 */
#define GUILD       "/\xea\xb8\xb8\xeb\x93\x9c"                                  /* /길드 */
#define GUILD_CP949 "/\xb1\xe6\xb5\xe5"                                          /* /길드 in CP949 */

struct UIWindowMgr g_windowMgr;
static struct Session session;
static int lookups, chat_lines;

void fatal_error(const char *message) { (void)message; abort(); }
int sound_effect_volume(void) { return 7; }
int sound_bgm_volume(void) { return 9; }
char *msg_str(int id) { (void)id; return "NO MSG"; }
void ui_window_mgr_set_chat_line(struct UIWindowMgr *self, const char *text)
{ assert(self == &g_windowMgr && text); ++chat_lines; }
struct GameMode *game_current_mode(void) { abort(); }
int mode_send_message(struct Mode *mode, int message, int a, int b, int c, int d)
{ (void)mode; (void)message; (void)a; (void)b; (void)c; (void)d; abort(); }
int session_lookup_talk_type(struct Session *self, const char *name)
{ assert(self == &session && name); ++lookups; return 0x7777; }
struct String *tokens_at(struct Vec *tokens, int i)
{
    struct String *first = tokens->first;
    assert(i >= 0 && first + i < (struct String *)tokens->last);
    return first + i;
}

/* Returns where the message text starts, or -1; the type and first output string. */
static int scan(const char *text, int *type, char *first, size_t capacity)
{
    struct String out[4];
    int i, at;
    for (i = 0; i < 4; ++i)
        string_init(&out[i]);
    at = session_get_talk_type(&session, text, type, out);
    assert(out[0].size < capacity);
    memcpy(first, string_cstr(&out[0]), out[0].size + 1);
    for (i = 0; i < 4; ++i)
        string_free(&out[i]);
    return at;
}

static void prefix(const char *text, int type, int at)
{
    char first[64];
    int got;
    assert(scan(text, &got, first, sizeof first) == at && got == type);
}

static void argument(const char *text, int type, const char *expected)
{
    char first[64];
    int got;
    assert(scan(text, &got, first, sizeof first) == -1 && got == type && !strcmp(first, expected));
}

int main(void)
{
    /* Broadcast prefixes: the message starts after the command's UTF-8 bytes and blanks. */
    prefix(BROADCAST " hello", 0x1, 5);
    prefix(BROADCAST "   hello", 0x1, 7);
    prefix("/b hello", 0x1, 3);
    prefix(BLUE " hi", 0x64, 11);
    prefix(AREA_BLUE " hi", 0x65, 17);
    prefix("/mb hi", 0x65, 4);
    /* Commands with an argument take the text after the command and one blank. */
    argument(EXCLUDE " Some Name", 0x12, "Some Name");
    argument("/ex Some Name", 0x12, "Some Name");
    argument(INCLUDE " Some Name", 0x13, "Some Name");
    argument("/in Some Name", 0x13, "Some Name");
    argument("/gc hello there", 0x39, "hello there");
    argument("/guildchat hello there", 0x39, "hello there");
    argument(GUILD " Name", 0x37, "Name");
    /* The CP949 spelling is not a command any more; it goes to the command table. */
    assert(!lookups);
    prefix(GUILD_CP949 " Name", 0x7777, -1);
    assert(lookups == 1 && !chat_lines);
    puts("Talk types: UTF-8 Korean prefixes and argument commands start the text after the command's bytes.");
    return 0;
}
