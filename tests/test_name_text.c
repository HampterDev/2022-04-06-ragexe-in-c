#include <windows.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "core/fatal.h"
#include "core/msg_str.h"
#include "core/utf8.h"
#include "game/packet_handlers.h"
#include "game/packets.h"
#include "game/packets_world.h"
#include "login/loginmode.h"
#include "login/server_entry.h"
#include "render/renderer.h"
#include "session/session_names.h"
#include "ui/character/select_server.h"
#include "ui/controls/edit.h"
#include "ui/controls/listbox.h"
#include "ui/items/pet_windows.h"

/* Run the production code that stores names and other text in fixed buffers: pet,
   homunculus and couple names and the char-server list from the servers, the cut-in
   image and map names, homunculus and pet names typed by the user or taken from the
   job names, the saved styling-shop records and a formatted state-change line. Each
   text is full of printf conversions and must arrive as it was, cut to the buffer, and
   a full packet field without a NUL must be read no further than the field. A job
   name longer than the buffer is cut by whole UTF-8 characters. */

#define CONVERSIONS "a%d%x%%b"
#define CONVERSIONS_N "%s%n%s%n"
#define SYLLABLE "\xea\xb0\x80"
#define TEN_SYLLABLES SYLLABLE SYLLABLE SYLLABLE SYLLABLE SYLLABLE SYLLABLE SYLLABLE SYLLABLE SYLLABLE SYLLABLE

struct Session g_session1;
struct UIWindowMgr g_windowMgr;
static struct Renderer renderer;
struct Renderer *g_renderer = &renderer;
static struct UIWindow test_window;
static struct UIListBox *const server_box = (struct UIListBox *)0x5150;
static char listed[0x100];
static const char *job_name, *edit_text, *busy_text = "busy", *state_text = "state";
static int cases;

__declspec(noreturn) void fatal_error(const char *message) { (void)message; abort(); }
char *msg_str(int id)
{
    switch (id) {
    case 0x784: return (char *)"%d%% x%.1f";
    case 0x1e2: return (char *)"(N) ";
    case 0xcbd: case 0xcbe: case 0xcbf: case 0xcc0: return (char *)busy_text;
    case 0xcba: case 0xcbb: case 0xcbc: return (char *)state_text;
    }
    abort();
}
const char *session_get_job_name(struct Session *self, unsigned job, int sex)
{
    assert(self == &g_session1 && sex == -1);
    (void)job;
    return job_name;
}
const char *ui_edit_ctrl_get_text(struct UIWindow *self)
{
    assert(self == &test_window);
    return edit_text;
}
struct UIWindow *ui_window_open(int id)
{
    assert(id == 2 || id == 0x4e);
    return &test_window;
}
int ui_window_send(struct UIWindow *window, struct UIWindow *sender, int message, int a, int b, int c, int d)
{
    (void)window; (void)sender; (void)message; (void)a; (void)b; (void)c; (void)d;
    return 0;
}
void ui_window_move_safe(struct UIWindow *window, int x, int y) { (void)window; (void)x; (void)y; }
BYTE ui_window_mgr_post_quit1(struct UIWindowMgr *self, int window_id) { (void)self; (void)window_id; abort(); }
void ui_pet_info_set_name(struct UIWindow *wnd, const char *name) { (void)wnd; (void)name; abort(); }
void ui_pet_info_update_intimacy(struct UIWindow *wnd) { (void)wnd; abort(); }
void ui_item_collection_set_pet(struct UIWindow *wnd, const struct PetEggDetails *pet) { (void)wnd; (void)pet; abort(); }
struct UIListBox *ui_select_server_list(struct UIWindow *wnd) { assert(wnd == &test_window); return server_box; }
void ui_list_box_add_item(struct UIListBox *self, const char *text)
{
    assert(self == server_box && strlen(text) < sizeof listed);
    strcpy(listed, text);
}
static void default_wallpaper(struct LoginMode *self) { (void)self; }

#include "name_text_homun_packet.inc"
#include "name_text_pet_named.inc"
#include "name_text_pet_packet.inc"
#include "name_text_couple_name.inc"
#include "name_text_cut_in.inc"
#include "name_text_server_list.inc"

static void map_name(const char *text)
{
    struct String name;

    string_init(&name);
    string_assign_cstr(&name, text);
#include "name_text_map_name.inc"
    string_free(&name);
}

static void homun_default(void)
{
#include "name_text_homun_default.inc"
}

static void pet_default(void)
{
#include "name_text_pet_default.inc"
}

static void homun_rename(void)
{
    struct { struct UIWindow *name_edit; } wnd = { &test_window }, *self = &wnd;
#include "name_text_homun_rename.inc"
}

static void pet_rename(void)
{
    struct { struct UIWindow *name_edit; } wnd = { &test_window }, *self = &wnd;
#include "name_text_pet_rename.inc"
}

static void style_save(const char *path, const char *text)
{
    struct String content;
    FILE *out = fopen(path, "w");

    string_init(&content);
    string_assign_cstr(&content, text);
#include "name_text_style_save.inc"
    string_free(&content);
}

static void state_line(char *text, int index, int val1, int val2)
{
    struct String s;

    switch (index) {
#include "name_text_state_line.inc"
    }
}

/* A full field of `fill` followed by bytes that are not NUL, as in a packet. */
static void full_field(char *field, size_t size, char fill)
{
    memset(field, fill, size);
}

static void check(const char *got, const char *expected)
{
    assert(strcmp(got, expected) == 0);
    ++cases;
}

static void check_homun_packet(void)
{
    struct { struct HomunPropertyHead head; char after[8]; } packet;

    memset(&packet, 'z', sizeof packet);
    strcpy(packet.head.name, CONVERSIONS);
    homun_name(&packet.head);
    check(g_session1.homunName, CONVERSIONS);
    strcpy(packet.head.name, CONVERSIONS_N);
    homun_name(&packet.head);
    check(g_session1.homunName, CONVERSIONS_N);
    full_field(packet.head.name, sizeof packet.head.name, 'H');
    homun_name(&packet.head);
    check(g_session1.homunName, "HHHHHHHHHHHHHHHHHHHHHHHH");
}

static void check_pet_names(void)
{
    char name[40];
    struct { struct PACKET_ZC_PROPERTY_PET pet; char after[8]; } packet;

    g_session1.aid = 7;
    strcpy(g_session1.petName, "kept");
    pet_named(8, CONVERSIONS);
    check(g_session1.petName, "kept");
    pet_named(7, CONVERSIONS);
    check(g_session1.petName, CONVERSIONS);
    pet_named(7, CONVERSIONS_N);
    check(g_session1.petName, CONVERSIONS_N);
    memset(name, 'q', sizeof name - 1);
    name[sizeof name - 1] = 0;
    full_field(name, NAME_LENGTH, 'P');
    pet_named(7, name);
    check(g_session1.petName, "PPPPPPPPPPPPPPPPPPPPPPPP");

    memset(&packet, 'z', sizeof packet);
    packet.pet.level = 5;
    strcpy(packet.pet.name, CONVERSIONS);
    game_mode_zc_property_pet(NULL, (unsigned char *)&packet);
    check(g_session1.petName, CONVERSIONS);
    packet.pet.level = 5;
    strcpy(packet.pet.name, CONVERSIONS_N);
    game_mode_zc_property_pet(NULL, (unsigned char *)&packet);
    check(g_session1.petName, CONVERSIONS_N);
    full_field(packet.pet.name, sizeof packet.pet.name, 'Q');
    packet.pet.level = 5;
    game_mode_zc_property_pet(NULL, (unsigned char *)&packet);
    check(g_session1.petName, "QQQQQQQQQQQQQQQQQQQQQQQQ");
}

static void check_couple_name(void)
{
    static struct GameMode mode;
    unsigned char packet[2 + NAME_LENGTH + 8];
    struct UIWindow *const after = (struct UIWindow *)0x1234;

    mode.name_balloon = after;
    memset(packet, 'z', sizeof packet);
    strcpy((char *)packet + 2, CONVERSIONS);
    game_mode_zc_couple_name(&mode, packet);
    check(mode.couple_name, CONVERSIONS);
    strcpy((char *)packet + 2, CONVERSIONS_N);
    game_mode_zc_couple_name(&mode, packet);
    check(mode.couple_name, CONVERSIONS_N);
    full_field((char *)packet + 2, NAME_LENGTH, 'C');
    game_mode_zc_couple_name(&mode, packet);
    check(mode.couple_name, "CCCCCCCCCCCCCCCCCCCCCCC");
    assert(mode.name_balloon == after);
}

static void check_cut_in(void)
{
    static struct GameMode mode;
    char longer[0x140];

    renderer.width = 800;
    renderer.height = 600;
    test_window.width = test_window.height = 100;
    string_init(&mode.illust_file);
    mode.illust_position = 1;
    string_assign_cstr(&mode.illust_file, CONVERSIONS ".bmp");
    game_mode_update_cut_in(&mode);
    check(g_illustImageName, CONVERSIONS ".bmp");
    g_windowMgr.illust_wnd = &test_window;
    string_assign_cstr(&mode.illust_file, CONVERSIONS_N ".bmp");
    game_mode_update_cut_in(&mode);
    check(g_illustImageName, CONVERSIONS_N ".bmp");
    memset(longer, 'I', sizeof longer - 1);
    longer[sizeof longer - 1] = 0;
    string_assign_cstr(&mode.illust_file, longer);
    game_mode_update_cut_in(&mode);
    longer[sizeof g_illustImageName - 1] = 0;
    check(g_illustImageName, longer);
    g_windowMgr.illust_wnd = NULL;
    string_free(&mode.illust_file);
}

static void check_server_list(void)
{
    static struct LoginMode login;
    struct LoginServerEntry *e = login.servers;

    login.numServer = 1;
    memset(e, 'z', sizeof *e);
    e->usercount = 4;
    e->state = 0;
    e->isNew = 0;
    strcpy(e->name, CONVERSIONS);
    server_list(&login);
    check(listed, CONVERSIONS);
    strcpy(e->name, CONVERSIONS_N);
    server_list(&login);
    check(listed, CONVERSIONS_N);
    full_field(e->name, sizeof e->name, 'S');
    server_list(&login);
    check(listed, "SSSSSSSSSSSSSSSSSSSS");
    e->isNew = 1;
    server_list(&login);
    check(listed, "(N) SSSSSSSSSSSSSSSSSSSS");

    /* Colored lines with long busy and state messages are cut to text[0x40]. */
    e->usercount = 1;
    busy_text = "B";
    server_list(&login);
    check(listed, "^FF7800B  ^000000(N) SSSSSSSSSSSSSSSSSSSS");
    e->usercount = 0;
    e->state = 2;
    busy_text = "BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB";
    state_text = "(maintenance)";
    server_list(&login);
    check(listed, "^00B509BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB  ^000000(N) SSS");
    e->usercount = 4;
    e->state = 3;
    state_text = "MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM";
    server_list(&login);
    check(listed, "MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM");
    /* 7 + 54 bytes, then a three-byte UTF-8 character over the 63-byte limit. */
    e->usercount = 0;
    e->state = 0;
    busy_text = "BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB\xea\xb0\x80";
    server_list(&login);
    check(listed, "^00B509BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB");
    e->isNew = 0;
    busy_text = "busy";
    state_text = "state";
}

static void check_map_name(void)
{
    char *whole = g_session1.mapName;

    map_name(CONVERSIONS);
    check(whole, CONVERSIONS);
    map_name(CONVERSIONS_N);
    check(whole, CONVERSIONS_N);
    map_name("abcdefghijklmnopqrstuvwxyz");
    check(whole, "abcdefghijklmnopqrs");
}

static void check_typed_names(void)
{
    g_session1.homunName[0] = 0;
    job_name = CONVERSIONS;
    homun_default();
    check(g_session1.homunName, CONVERSIONS);
    job_name = "unused";
    homun_default();
    check(g_session1.homunName, CONVERSIONS);
    g_session1.petName[0] = 0;
    job_name = CONVERSIONS_N;
    pet_default();
    check(g_session1.petName, CONVERSIONS_N);
    /* Eleven 3-byte characters are 33 bytes; ten whole ones fit in the 32-byte names. */
    g_session1.homunName[0] = g_session1.petName[0] = 0;
    job_name = TEN_SYLLABLES SYLLABLE;
    homun_default();
    check(g_session1.homunName, TEN_SYLLABLES);
    pet_default();
    check(g_session1.petName, TEN_SYLLABLES);

    edit_text = CONVERSIONS_N;
    homun_rename();
    check(g_session1.homunName, CONVERSIONS_N);
    edit_text = CONVERSIONS;
    pet_rename();
    check(g_session1.petName, CONVERSIONS);
    edit_text = "0123456789abcdef0123456789abcdef0123";
    homun_rename();
    check(g_session1.homunName, "0123456789abcdef0123456789abcde");
    pet_rename();
    check(g_session1.petName, "0123456789abcdef0123456789abcde");
}

static void check_style_save(void)
{
    static const char text[] = "characterName:" CONVERSIONS "\nslotNum:1\n*\n"
                               "characterName:" CONVERSIONS_N "\nslotNum:2\n*\n";
    char path[MAX_PATH], saved[sizeof text + 8];
    FILE *in;
    size_t n;

    assert(GetTempPathA(MAX_PATH, path) && strlen(path) + 20 < sizeof path);
    strcat(path, "name_text_style.lua");
    style_save(path, text);
    in = fopen(path, "r");
    assert(in);
    n = fread(saved, 1, sizeof saved - 1, in);
    fclose(in);
    remove(path);
    saved[n] = 0;
    check(saved, text);
}

static void check_state_line(void)
{
    char text[0x100];

    memset(text, 0, sizeof text);
    state_line(text, 0x27c, 100, 50);
    check(text, "50% x2.0");
}

int main(void)
{
    check_homun_packet();
    check_pet_names();
    check_couple_name();
    check_cut_in();
    check_server_list();
    check_map_name();
    check_typed_names();
    check_style_save();
    check_state_line();
    printf("Name text: %d cases; names, image and map names, saved records and state lines are copied as text.\n",
           cases);
    return 0;
}
