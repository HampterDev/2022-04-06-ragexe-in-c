#include "ui/window_access.h"
#include "ui/chat/balloon.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "owned_storage_test_allocator.h"
#include "../src/game/mode_whisper.c"

struct Session g_session1;
struct UIWindowMgr g_windowMgr;
static struct Renderer renderer;
struct Renderer *g_renderer = &renderer;
static struct GameMode mode;
static struct UIWindow chat;
static unsigned legacy_live;
static int window_takes, is_friend, is_member, looks_party, looks_friend;
static struct String transcript, expected;

void *mem_alloc(size_t size) { void *p = calloc(1, size); assert(p); ++legacy_live; return p; }
void mem_free(void *p)
{  if (p) { assert(legacy_live); --legacy_live; free(p); } }
__declspec(noreturn) void fatal_error(const char *text) { (void)text; abort(); }

#ifndef INCOMING_WHISPER_REFERENCE
#endif
char *msg_str(int id)
{
    if (id == MSG_AWAY_PREFIX) return "[Away] ";
    if (id == MSG_NOT_IN_FRIEND_LIST) return "not a friend";
    assert(id == MSG_NOT_IN_GUILD); return "not in guild";
}
const char *session_character_name(const struct Session *session) { assert(session == &g_session1); return "Self"; }
struct Mode *client_current_mode(void) { return (struct Mode *)&mode; }
int mode_send_message(struct Mode *self, int message, int a, int b, int c, int d)
{
    assert(self == (struct Mode *)&mode && message == CMODE_MSG_WHISPER && !b && !c && !d);
    string_append_format(&transcript, "away:%s:%s\n", (const char *)a, string_cstr(&g_windowMgr.chat_line));
    return 0;
}
int ui_window_mgr_is_friend(struct UIWindowMgr *mgr, const char *name)
{ assert(mgr == &g_windowMgr); string_append_format(&transcript, "friend:%s\n", name); return is_friend; }
int session_is_guild_member_by_name(struct Session *session, const char *name)
{ assert(session == &g_session1); string_append_format(&transcript, "guild:%s\n", name); return is_member; }
int session_looks_like_a_party_member(struct Session *session, const char *name)
{ assert(session == &g_session1); string_append_format(&transcript, "party:%s\n", name); return looks_party; }
int session_looks_like_a_friend(struct Session *session, const char *name)
{ assert(session == &g_session1); string_append_format(&transcript, "looks:%s\n", name); return looks_friend; }
int ui_window_mgr_add_whisper_chat_to_whisper_wnd(struct UIWindowMgr *mgr, const char *name,
                                          const char *line, unsigned colour, int cid)
{
    assert(mgr == &g_windowMgr && colour == COLOUR_WHISPER_IN_WND);
    string_append_format(&transcript, "window:%s:%d:%s\n", name, cid, line); return window_takes;
}
int ui_window_mgr_send_msg_(struct UIWindowMgr *mgr, int message, int a, int b, int c, int d)
{
    assert(mgr == &g_windowMgr);
    if (message == UIM_REFRESH) { assert(!a && !b && !c && !d); string_append_format(&transcript, "refresh\n"); }
    else {
        assert(message == UIM_CHAT_LINE || message == UIM_WHISPER_LOG_LINE);
        string_append_format(&transcript, "send:%d:%d:%d:%s:%s\n", message, b, c, (const char *)a, d ? (const char *)d : "");
    }
    return 0;
}
void ui_window_mgr_add_whisper_name(struct UIWindowMgr *mgr, const char *name)
{ assert(mgr == &g_windowMgr); string_append_format(&transcript, "name:%s\n", name); }
void game_mode_log_whisper(struct GameMode *self, const char *line)
{ assert(self == &mode); string_append_format(&transcript, "log:%s\n", line); }
struct String *session_scramble_number(struct Session *session, struct String *out, unsigned value)
{
    char number[32]; assert(session == &g_session1);
    sprintf_s(number, sizeof number, "id%u", value);
    string_init(out);
    string_assign_cstr(out, number); return out;
}
static void move_balloon(struct UIWindow *w, int x, int y)
{ assert(w == mode.admin_whisper_balloon); string_append_format(&transcript, "move:%d:%d\n", x, y); }
static struct UIWindowOps balloon_ops = {.move = move_balloon};
struct UIWindow *ui_trans_balloon_text_construct(struct UIWindow *w)
{ w->ops = &balloon_ops; string_append_format(&transcript, "construct\n"); return w; }
void ui_window_create(struct UIWindow *w, int width, int height)
{ assert(w == mode.admin_whisper_balloon); w->width = width; w->height = height; string_append_format(&transcript, "create:%d:%d\n", width, height); }
void ui_window_mgr_add_to_window_list(struct UIWindowMgr *mgr, struct UIWindow *w)
{ assert(mgr == &g_windowMgr && w == mode.admin_whisper_balloon); string_append_format(&transcript, "attach\n"); }
void ui_balloon_text_set_text(struct UIBalloonText *w, const char *text, int width)
{ assert(&w->base == mode.admin_whisper_balloon && width == 0x48); string_append_format(&transcript, "balloon:%s\n", text); }

static struct String *transform(struct String *out, struct TextView text,
                                     const char *needle, const char *replacement)
{
    struct String result = {0};
    size_t at = text_view_find(text, text_view_cstr(needle), 0);
    if (at == SIZE_MAX) string_assign(&result, text.data, text.size);
    else {
        string_assign(&result, text.data, at);
        string_append_cstr(&result, replacement);
        string_append(&result, text.data + at + strlen(needle), text.size - at - strlen(needle));
    }
    string_free(out); *out = result; return out;
}
struct String *navi_links_readable_links(struct String *out, struct TextView text)
{ string_append_format(&transcript, "navi\n"); return transform(out, text, "<NAVIL>go</NAVIL>", "Go"); }
struct String *ui_new_chat_wnd_readable_links(struct UIWindow *w, struct String *out, struct TextView text)
{ assert(w == &chat); string_append_format(&transcript, "item\n"); return transform(out, text, "<ITEML>item</ITEML>", "<Sword>"); }

static void begin(unsigned options)
{
    assert(!mode.admin_whisper_balloon);
    string_clear(&transcript);
    window_takes = options & 1; is_friend = options & 2; is_member = options & 4;
    looks_party = options & 8; looks_friend = options & 16;
    g_session1.autoMessageOn = (options & 32) != 0; g_session1.autoSaveChat = (options & 64) != 0;
    string_init(&g_session1.autoMessage); string_assign_cstr(&g_session1.autoMessage, "Back soon");
    string_init(&g_windowMgr.chat_line);
    g_windowMgr.new_chat_wnd = &chat; renderer.width = 1024;
}
static void end(void)
{
    if (mode.admin_whisper_balloon) {
        const struct UIBalloonText *balloon = (const struct UIBalloonText *)mode.admin_whisper_balloon;

        assert(balloon->colour == 0xffff && !balloon->outline);
        mem_free(mode.admin_whisper_balloon); mode.admin_whisper_balloon = NULL;
    }
    string_free(&g_session1.autoMessage); string_free(&g_windowMgr.chat_line);
    assert(legacy_live == (transcript.data != NULL) + (expected.data != NULL));
}

static void scenarios(void)
{
    struct TextView sender = text_view_cstr("가나다"), msg = text_view_cstr("hello");
    begin(64); game_mode_whisper(&mode, 42, sender, msg, 0);
    assert(strstr(string_cstr(&transcript), "log:( From 가나다 [id42] ) : hello\n")); end();
    begin(2 | 4 | 64); game_mode_whisper(&mode, 42, sender, msg, 1);
    assert(strstr(string_cstr(&transcript), "balloon:[ Friend 가나다 (id42) ] : hello\n")); end();
    begin(4 | 64); game_mode_whisper(&mode, 0, sender, msg, 0);
    assert(strstr(string_cstr(&transcript), "log:[ Member 가나다 ] : hello\n")); end();
    begin(8 | 16); game_mode_whisper(&mode, 0, sender, msg, 0);
    assert(strstr(string_cstr(&transcript), "not a friend") && !strstr(string_cstr(&transcript), "not in guild")); end();
    begin(32 | 64); game_mode_whisper(&mode, 1, text_view_cstr("Self"), msg, 1);
    assert(!strstr(string_cstr(&transcript), "refresh") && strstr(string_cstr(&transcript), "away:Self:[Away] Back soon")); end();
    begin(1 | 32); game_mode_whisper(&mode, 0, sender, text_view_cstr("[Away] busy"), 0);
    assert(!strstr(string_cstr(&transcript), "away:")); end();
    begin(1); game_mode_whisper(&mode, 0, sender, text_view_cstr("<NAVIL>go</NAVIL>"), 1);
    assert(strstr(string_cstr(&transcript), "balloon:[ 가나다 ] : <NAVIL>go</NAVIL>")); end();
    begin(1); game_mode_whisper(&mode, 0, sender, text_view_cstr("<NAVIL>go</NAVIL><ITEML>item</ITEML>"), 1);
    assert(strstr(string_cstr(&transcript), "balloon:[ 가나다 ] : Go<Sword>")); end();
}

static void packets(void)
{
    SYSTEM_INFO info;
    unsigned char *memory, *end_at;
    DWORD old;
    unsigned version, payload;
    GetSystemInfo(&info);
    memory = VirtualAlloc(NULL, 2 * info.dwPageSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE); assert(memory);
    end_at = memory + info.dwPageSize;
    assert(VirtualProtect(end_at, info.dwPageSize, PAGE_NOACCESS, &old));
    for (version = 0; version < 2; ++version) for (payload = 0; payload <= 8; ++payload) {
        unsigned short size = (unsigned short)(32 + version + payload);
        unsigned char *packet = end_at - size;
        memset(packet, 'x', size); memcpy(packet + 2, &size, 2);
        if (version) { int cid = 42; memcpy(packet + 4, &cid, 4); packet[32] = 0; }
        else memset(packet + 28, 0, 4);
        if (payload > 2) packet[32 + version + 2] = 0;
        begin(1);
        if (version) game_mode_zc_whisper02(&mode, packet); else game_mode_zc_whisper(&mode, packet);
        assert(strstr(string_cstr(&transcript), "window:xxxxxxxxxxxxxxxxxxxxxxx:"));
        end();
    }
    for (version = 0; version < 33; ++version) {
        unsigned short size = (unsigned short)version;
        memcpy(end_at - 2, &size, 2);
        begin(0);
        if (version < 32) game_mode_zc_whisper(&mode, end_at - 4);
        game_mode_zc_whisper02(&mode, end_at - 4);
        assert(!transcript.size); end();
    }
    assert(VirtualFree(memory, 0, MEM_RELEASE));
}

#ifdef INCOMING_WHISPER_REFERENCE
static struct String *reference_readable(struct UIWindow *w, struct String *out, const struct String *text)
{
    struct String native = {0};
    ui_new_chat_wnd_readable_links(w, &native, string_view(text));
    string_init(out); string_assign(out, string_cstr(&native), native.size);
    string_free(&native); return out;
}
#include "incoming_whisper_reference.c"
static void reference_cases(void)
{
    static const char *messages[] = {"", "hello 한글", "[Away] busy", "<NAVIL>go</NAVIL>",
        "<ITEML>item</ITEML>", "<NAVIL>go</NAVIL><ITEML>item</ITEML>", "before\0after"};
    unsigned options, admin, cid, name, i, count = 0;
    for (options = 0; options < 128; ++options) for (admin = 0; admin < 2; ++admin)
    for (cid = 0; cid < 2; ++cid) for (name = 0; name < 2; ++name)
    for (i = 0; i < sizeof messages / sizeof *messages; ++i) {
        struct String sender, text;
        const char *who = name ? "Self" : "긴이름abcdefghijklmnopqrstuv";
        size_t length = i == 6 ? 12 : strlen(messages[i]);
        begin(options);
        string_init(&sender);
        string_assign_cstr(&sender, who); string_init(&text); string_assign(&text, messages[i], length);
        reference_GameMode_Whisper(&mode, cid ? 42 : 0, sender, text, (unsigned char)admin);
        string_assign(&expected, string_cstr(&transcript), transcript.size); end();
        begin(options);
        game_mode_whisper(&mode, cid ? 42 : 0, text_view_cstr(who), text_view(messages[i], length), (unsigned char)admin);
        assert(expected.size == transcript.size && !memcmp(expected.data, transcript.data, expected.size));
        end(); ++count;
    }
    string_free(&expected);
    printf("Whispers: %u archived routing/formatting/side-effect comparisons passed.\n", count);
}
#endif

int main(void)
{
    scenarios(); packets();
#ifdef INCOMING_WHISPER_REFERENCE
    reference_cases();
#endif
    string_free(&transcript); assert(!outstanding && !legacy_live);
    puts("Incoming whispers: routing, Korean names, IDs, warnings, admin links, away replies and guarded packets passed.");
    return 0;
}
