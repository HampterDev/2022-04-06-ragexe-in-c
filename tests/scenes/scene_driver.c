/* Scene driver: replaces core/drive_state.c in the scene harness. The client calls
   drive_state_frame_shown() for every frame it shows, which advances the virtual clock by one
   frame, and drive_state_tick() once per message pump, which resumes the Lua scene script. The
   script's `scene` table puts the client into a state (map, actors, chat, windows, input) and
   captures PNGs, each with the counters of its frame (diag/perf.h); the driver also answers the
   client's map server requests.

   Environment: RAGEXE_SCENE=<script.lua>, RAGEXE_SCENE_OUT=<directory for captures>,
   RAGEXE_SCENE_TRACE=1 for the packets and mode changes, RAGEXE_SCENE_SHADOW_FIT=1 to check each
   capture's shadow fit on the GPU against render/shadow_fit.c, RAGEXE_SCENE_LAYOUT=<n> for memory
   laid out otherwise (layout_heap). See tests/scenes/README.md. */
#include "core/app.h"
#include "core/drive_state.h"
#include "core/user_input.h"
#include "core/window_state.h"
#include "actor/types.h"
#include "contents/account_settings_internal.h"
#include "diag/perf.h"
#include "effects/rag/rag_effect_types.h"
#include "game/camera.h"
#include "game/context.h"
#include "game/gamemode.h"
#include "game/mode_msg.h"
#include "game/packet_ids.h"
#include "game/packets_entry.h"
#include "login/char_info.h"
#include "login/loginmode.h"
#include "platform/clock.h"
#include "platform/net.h"
#include "platform/vk.h"
#include "platform/window.h"
#include "render/device.h"
#include "render/gfx.h"
#include "render/shadow_fit.h"
#include "session/character_name.h"
#include "session/state.h"
#include "std/list.h"
#include "std/string.h"
#include "ui/window_access.h"
#include "world/mode_manager.h"
#include "world/models.h"
#include "world/state.h"
#include "scene.h"
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stb_image_write.h"
#ifdef _WIN32
#include <direct.h>
#define make_directory(path) _mkdir(path)
#else
#include <sys/stat.h>
#define make_directory(path) mkdir((path), 0777)
#endif

#define SCENE_AID 2000000
#define SCENE_GID 150000
#define READY_STABLE_FRAMES 30
#define READY_TIMEOUT_FRAMES 3000

enum WaitKind { WAIT_NONE, WAIT_FRAMES, WAIT_READY };

static struct {
    int started, finished, failures;
    lua_State *L, *co;
    enum WaitKind wait;
    int wait_frames, stable_frames, timeout;
    size_t last_models;
    unsigned frame, shown;
    char out[1024];
    int enter_x, enter_y, enter_dir, traced_mode;
    struct perf_counters counters; /* of the last frame shown */
    int64_t shown_ns;              /* when the driver was done with the last frame shown */
    int64_t frame_ns, script_ns;   /* the last frame drawn: the client's time, the script's */
    int64_t script_running_ns;     /* the script's time since the last frame shown */
    struct {
        FILE *file;
        char path[1200];
        int left, logged;
    } measure;                     /* scene.measure: the frames still to log */
} S;

/* RAGEXE_SCENE_LAYOUT=<n>, n > 0: the client's memory laid out and left over otherwise than in a
   run without it or with another n, the same way in every run with this n, so that two runs whose
   frames differ betray an order kept by addresses or a read of memory nothing wrote. Before the
   client starts, blocks of n's sizes are taken from the heap, filled with n's byte, and every other
   one given back; at every message pump the stack below the driver is filled with that byte. */
#define LAYOUT_BLOCKS 1024
#define LAYOUT_STACK (64 * 1024)

static unsigned char layout_byte;

static void layout_heap(unsigned n)
{
    static void *kept[LAYOUT_BLOCKS];
    unsigned i, x = n;
    layout_byte = (unsigned char)(n * 0x5b + 0x11);
    for (i = 0; i < LAYOUT_BLOCKS; i++) {
        size_t size;
        x = x * 1103515245u + 12345u;
        size = 16 + (x >> 16) % 4096;
        kept[i] = malloc(size);
        if (kept[i])
            memset(kept[i], layout_byte, size);
    }
    for (i = n & 1; i < LAYOUT_BLOCKS; i += 2) {
        free(kept[i]);
        kept[i] = NULL;
    }
}

static void layout_stack(void)
{
    volatile unsigned char below[LAYOUT_STACK];
    size_t i;
    for (i = 0; i < sizeof below; i++)
        below[i] = layout_byte;
}

static void scene_layout(void)
{
    const char *layout = getenv("RAGEXE_SCENE_LAYOUT");
    unsigned long n = layout ? strtoul(layout, NULL, 10) : 0;
    if (n)
        layout_heap((unsigned)n);
}

/* Headless, silent and without vsync unless the caller says otherwise; set before the client
   reads them. */
#ifdef _WIN32
static void scene_defaults(void)
{
    if (!getenv("RAGEXE_VSYNC"))
        _putenv_s("RAGEXE_VSYNC", "0");
    if (!getenv("SDL_AUDIODRIVER"))
        _putenv_s("SDL_AUDIODRIVER", "dummy");
    scene_layout();
}
#pragma section(".CRT$XCU", read)
__declspec(allocate(".CRT$XCU")) void (*scene_defaults_entry)(void) = scene_defaults;
#else
__attribute__((constructor)) static void scene_defaults(void)
{
#ifndef __APPLE__
    /* nothing on a desktop, and no need for one: the client's window is never shown
       (scene_app_hidden.c); macOS keeps Cocoa's driver, the one SDL's Metal driver runs with */
    setenv("SDL_VIDEODRIVER", "offscreen", 0);
#endif
    setenv("SDL_AUDIODRIVER", "dummy", 0);
    setenv("RAGEXE_VSYNC", "0", 0);
    scene_layout();
}
#endif

void drive_state_note_chat(const char *text)
{
    (void)text;
}

static void finish(int failed)
{
    if (S.measure.file) {
        fprintf(stderr, "scene: the run ended %d frames into the %d the measure of %s asked for\n", S.measure.logged,
                S.measure.logged + S.measure.left, S.measure.path);
        fclose(S.measure.file);
        S.measure.file = NULL;
        failed = 1;
    }
    fflush(stdout);
    fflush(stderr);
    exit(failed || S.failures ? 1 : 0);
}

/* ---- packets the fake map server sends ---- */

static void put16(unsigned char *p, unsigned v) { p[0] = (unsigned char)v; p[1] = (unsigned char)(v >> 8); }
static void put32(unsigned char *p, unsigned long v)
{
    put16(p, (unsigned)(v & 0xffff));
    put16(p + 2, (unsigned)(v >> 16));
}

static void pos_dir(unsigned char *out, int x, int y, int dir)
{
    out[0] = (unsigned char)(x >> 2);
    out[1] = (unsigned char)(((x & 3) << 6) | ((y >> 4) & 0x3f));
    out[2] = (unsigned char)(((y & 0xf) << 4) | (dir & 0xf));
}

static void deliver(const void *data, int size)
{
    if (!scene_net_deliver(SCENE_MAP_PORT, data, size)) {
        fprintf(stderr, "scene: no map connection to deliver to\n");
        S.failures++;
    }
}

/* A 24x24 guild emblem as a 24-bit BMP: magenta (transparent in emblems) around a disc in the
   guild's own colour with a white cross, so different guilds look different. */
static int make_emblem(int guild, unsigned char *bmp)
{
    enum { SIDE = 24, ROW = SIDE * 3, PIXELS = ROW * SIDE, SIZE = 54 + PIXELS };
    unsigned char r = (unsigned char)(80 + guild * 53 % 176), g = (unsigned char)(80 + guild * 97 % 176);
    unsigned char b = (unsigned char)(80 + guild * 31 % 176);
    int x, y;
    memset(bmp, 0, 54);
    bmp[0] = 'B';
    bmp[1] = 'M';
    put32(bmp + 2, SIZE);
    put32(bmp + 10, 54);
    put32(bmp + 14, 40);
    put32(bmp + 18, SIDE);
    put32(bmp + 22, SIDE);
    put16(bmp + 26, 1);
    put16(bmp + 28, 24);
    put32(bmp + 34, PIXELS);
    for (y = 0; y < SIDE; y++)
        for (x = 0; x < SIDE; x++) {
            unsigned char *p = bmp + 54 + y * ROW + x * 3;
            int dx = 2 * x - 23, dy = 2 * y - 23;
            int inside = dx * dx + dy * dy <= 22 * 22, cross = inside && (abs(dx) <= 3 || abs(dy) <= 3);
            p[0] = cross ? 255 : inside ? b : 255;
            p[1] = cross ? 255 : inside ? g : 0;
            p[2] = cross ? 255 : inside ? r : 255;
        }
    return SIZE;
}

/* The client asks for a guild emblem with 0x0b46 {guild, version}; the map server answers with
   CR_NEW_EMBLEM_DOWNLOAD (0x0b36) in three parts: start, data, end (docs/server/README.md). */
static void send_emblem(int guild, int version)
{
    unsigned char bmp[54 + 24 * 24 * 3], packet[14 + sizeof bmp];
    int size = make_emblem(guild, bmp), part;
    for (part = 0; part < 3; part++) {
        int data = part == 1 ? size : 0;
        put16(packet, 0xb36);
        put16(packet + 2, (unsigned)(14 + data));
        put16(packet + 4, (unsigned)part);
        put32(packet + 6, (unsigned long)guild);
        put32(packet + 10, (unsigned long)version);
        memcpy(packet + 14, bmp, (size_t)data);
        scene_net_deliver(SCENE_MAP_PORT, packet, 14 + data);
    }
}

/* ZC_NOTIFY_PLAYERCHAT: a line in the player's own chat. */
static void own_chat(const char *text, size_t size)
{
    unsigned char packet[4 + 256];
    if (size > 255)
        size = 255;
    put16(packet, HEADER_ZC_NOTIFY_PLAYERCHAT);
    put16(packet + 2, (unsigned)(4 + size + 1));
    memcpy(packet + 4, text, size);
    packet[4 + size] = 0;
    deliver(packet, (int)(4 + size + 1));
}

static void on_client_packet(int port, const unsigned char *packet, int length)
{
    int id = packet[0] | packet[1] << 8;
    if (getenv("RAGEXE_SCENE_TRACE"))
        fprintf(stderr, "scene: client sent 0x%04x (%d bytes) on port %d at frame %u\n", id, length, port, S.frame);
    if (port != SCENE_MAP_PORT)
        return;
    if (id == 0x436) {
        /* ZC_AID, then ZC_ACCEPT_ENTER2 with the start position. */
        unsigned char reply[6 + 13];
        put16(reply, 0x283);
        put32(reply + 2, SCENE_AID);
        put16(reply + 6, 0x2eb);
        put32(reply + 8, platform_ticks_ms());
        pos_dir(reply + 12, S.enter_x, S.enter_y, S.enter_dir);
        reply[15] = 5;
        reply[16] = 5;
        put16(reply + 17, 0);
        scene_net_deliver(SCENE_MAP_PORT, reply, sizeof reply);
    } else if (id == 0x7d) {
        /* CZ_NOTIFY_ACTORINIT: the map is loaded. ZC_NOTIFY_ACTORINIT confirms it, as rAthena
           does, which lets the hotkeys work. */
        unsigned char reply[2];
        put16(reply, 0xb1b);
        scene_net_deliver(SCENE_MAP_PORT, reply, sizeof reply);
    } else if (id == 0xf3 && length > 4) {
        /* CZ_REQUEST_CHAT: the map server says the line back to the one who said it. */
        own_chat((const char *)packet + 4, strnlen((const char *)packet + 4, (size_t)length - 4));
    } else if (id == 0xb46 && length >= 10) {
        send_emblem((int)(packet[2] | packet[3] << 8 | packet[4] << 16 | (unsigned)packet[5] << 24),
                    (int)(packet[6] | packet[7] << 8 | packet[8] << 16 | (unsigned)packet[9] << 24));
    }
}

/* ---- Lua API ---- */

static int opt_int(lua_State *L, int table, const char *key, int fallback)
{
    int value = fallback;
    if (table && lua_istable(L, table)) {
        lua_getfield(L, table, key);
        if (lua_isnumber(L, -1))
            value = (int)lua_tointeger(L, -1);
        lua_pop(L, 1);
    }
    return value;
}

static const char *opt_str(lua_State *L, int table, const char *key, const char *fallback)
{
    const char *value = fallback;
    if (table && lua_istable(L, table)) {
        lua_getfield(L, table, key);
        if (lua_isstring(L, -1))
            value = lua_tostring(L, -1);
        lua_pop(L, 1);
    }
    return value;
}

/* scene.enter_map(map, x, y, {dir, job, sex, hair, hair_color, name, level, weapon, shield, head_top,
   head_mid, head_bottom, robe, body_palette, distance, latitude}) — from the login screen straight
   into a map; with distance and latitude, the free camera's (game/camera.c) in place of the saved
   options' (Outdoor_ViewDistance, Outdoor_ViewLatitude). */
static int l_enter_map(lua_State *L)
{
    const char *map = luaL_checkstring(L, 1);
    int x = (int)luaL_checkinteger(L, 2), y = (int)luaL_checkinteger(L, 3), opts = lua_gettop(L) >= 4 ? 4 : 0;
    struct LoginMode *login;
    struct CharInfo c;
    char gat[40];
    if (g_modeMgr.curModeType != MODE_TYPE_LOGIN || !g_modeMgr.curMode)
        return luaL_error(L, "enter_map: the client is not on the login screen");
    login = (struct LoginMode *)g_modeMgr.curMode;
    memset(&c, 0, sizeof c);
    c.gid = SCENE_GID;
    c.job = (short)opt_int(L, opts, "job", 0);
    c.head = (short)opt_int(L, opts, "hair", 1);
    c.head_palette = (short)opt_int(L, opts, "hair_color", 0);
    c.body_palette = (short)opt_int(L, opts, "body_palette", 0);
    c.weapon = (short)opt_int(L, opts, "weapon", 0);
    c.shield = (short)opt_int(L, opts, "shield", 0);
    c.accessory = (short)opt_int(L, opts, "head_bottom", 0);
    c.accessory2 = (short)opt_int(L, opts, "head_top", 0);
    c.accessory3 = (short)opt_int(L, opts, "head_mid", 0);
    c.robe = opt_int(L, opts, "robe", 0);
    c.level = (short)opt_int(L, opts, "level", 1);
    c.job_level = opt_int(L, opts, "job_level", 1);
    c.hp = c.max_hp = opt_int(L, opts, "hp", 40);
    c.sp = c.max_sp = opt_int(L, opts, "sp", 11);
    c.walk_speed = 150;
    c.sex = (BYTE)opt_int(L, opts, "sex", 1);
    memset(c.stats, 1, sizeof c.stats);
    strncpy(c.name, opt_str(L, opts, "name", "Tester"), sizeof c.name - 1);
    snprintf(gat, sizeof gat, "%s.gat", map);
    strncpy(c.map_name, gat, sizeof c.map_name - 1);
    login->chars[0] = c;
    login->numChar = 1;
    g_loginCharInfo = c;
    g_session1.ownAid = SCENE_AID;
    g_session1.authCode = 1;
    g_session1.sex = c.sex;
    g_session1.accountSex = c.sex;
    session_set_character_name(&g_session1, text_view(c.name, sizeof c.name));
    /* Initial preferences: no account settings file is read or written. */
    if (g_accountUserDataMgr) {
        g_accountUserDataMgr->loaded = 1;
        g_accountUserDataMgr->path[0] = 0;
    }
    if (opts && lua_istable(L, opts)) {
        lua_getfield(L, opts, "distance");
        if (lua_isnumber(L, -1))
            g_freeViewDistance = (float)lua_tonumber(L, -1);
        lua_pop(L, 1);
        lua_getfield(L, opts, "latitude");
        if (lua_isnumber(L, -1))
            g_freeViewPitch = (float)lua_tonumber(L, -1);
        lua_pop(L, 1);
    }
    S.enter_x = x;
    S.enter_y = y;
    S.enter_dir = opt_int(L, opts, "dir", 0);
    login_mode_enter_zone(login, SCENE_GID, string_from_cstr(gat), platform_net_parse_ipv4("127.0.0.1"),
                          SCENE_MAP_PORT, string_from_cstr(""));
    return 0;
}

/* scene.spawn{aid, job, x, y, dir, name, kind = "monster"|"npc"|"player", hair, hair_color,
   weapon, shield, head_top, head_mid, head_bottom, robe, sex, level, hp, max_hp, speed, state,
   option, boss} */
static int l_spawn(lua_State *L)
{
    static const struct { const char *name; int type; } kinds[] = {
        {"player", 0x0}, {"npc", 0x6}, {"monster", 0x5}, {"pet", 0x7}, {"homunculus", 0x8},
        {"mercenary", 0x9},
    };
    unsigned char packet[sizeof(struct PacketStandEntry11) + 24];
    struct PacketStandEntry10 *e = (struct PacketStandEntry10 *)packet;
    const char *kind, *name;
    int type = 0x5;
    size_t i;
    luaL_checktype(L, 1, LUA_TTABLE);
    memset(packet, 0, sizeof packet);
    kind = opt_str(L, 1, "kind", "monster");
    for (i = 0; i < sizeof kinds / sizeof kinds[0]; i++)
        if (!strcmp(kind, kinds[i].name))
            type = kinds[i].type;
    e->id = HEADER_ZC_NOTIFY_STANDENTRY11;
    e->length = (short)sizeof packet;
    e->object_type = (unsigned char)type;
    e->aid = (unsigned)opt_int(L, 1, "aid", 110000000);
    e->char_id = type == 0 ? e->aid : 0;
    e->speed = (short)opt_int(L, 1, "speed", 150);
    e->effect_state = opt_int(L, 1, "option", 0);
    e->is_boss = (unsigned char)opt_int(L, 1, "boss", 0);
    e->job = (short)opt_int(L, 1, "job", 1002);
    e->head = (short)opt_int(L, 1, "hair", 0);
    e->weapon = opt_int(L, 1, "weapon", 0);
    e->shield = opt_int(L, 1, "shield", 0);
    e->accessory = (short)opt_int(L, 1, "head_bottom", 0);
    e->accessory2 = (short)opt_int(L, 1, "head_top", 0);
    e->accessory3 = (short)opt_int(L, 1, "head_mid", 0);
    e->head_palette = (short)opt_int(L, 1, "hair_color", 0);
    e->body_palette = (short)opt_int(L, 1, "body_palette", 0);
    e->head_dir = (short)opt_int(L, 1, "head_dir", 0);
    e->robe = (short)opt_int(L, 1, "robe", 0);
    e->guild_id = (unsigned)opt_int(L, 1, "guild", 0);
    e->emblem_version = (short)opt_int(L, 1, "emblem", 0);
    e->sex = (unsigned char)opt_int(L, 1, "sex", 1);
    pos_dir(e->pos.b, opt_int(L, 1, "x", 0), opt_int(L, 1, "y", 0), opt_int(L, 1, "dir", 0));
    e->x_size = e->y_size = 5;
    e->state = (unsigned char)opt_int(L, 1, "state", 0);
    e->clevel = (short)opt_int(L, 1, "level", 1);
    e->max_hp = opt_int(L, 1, "max_hp", 100);
    e->hp = opt_int(L, 1, "hp", e->max_hp);
    ((struct PacketStandEntry11 *)packet)->body = (short)opt_int(L, 1, "body", 0);
    name = opt_str(L, 1, "name", "");
    strncpy((char *)packet + sizeof(struct PacketStandEntry11), name, 23);
    deliver(packet, sizeof packet);
    return 0;
}

static int l_vanish(lua_State *L)
{
    unsigned char packet[7];
    put16(packet, HEADER_ZC_NOTIFY_VANISH);
    put32(packet + 2, (unsigned long)luaL_checkinteger(L, 1));
    packet[6] = (unsigned char)luaL_optinteger(L, 2, 0);
    deliver(packet, sizeof packet);
    return 0;
}

/* scene.say(aid, "Name : text") over an actor; scene.chat("text") in the player's own chat. */
static int l_say(lua_State *L)
{
    size_t size;
    unsigned long aid = (unsigned long)luaL_checkinteger(L, 1);
    const char *text = luaL_checklstring(L, 2, &size);
    unsigned char packet[8 + 256];
    if (size > 255)
        size = 255;
    put16(packet, HEADER_ZC_NOTIFY_CHAT);
    put16(packet + 2, (unsigned)(8 + size + 1));
    put32(packet + 4, aid);
    memcpy(packet + 8, text, size);
    packet[8 + size] = 0;
    deliver(packet, (int)(8 + size + 1));
    return 0;
}

static int l_chat(lua_State *L)
{
    size_t size;
    const char *text = luaL_checklstring(L, 1, &size);
    own_chat(text, size);
    return 0;
}

/* scene.send(bytes): any packet, as the map server would send it. */
static int l_send(lua_State *L)
{
    size_t size;
    const char *data = luaL_checklstring(L, 1, &size);
    deliver(data, (int)size);
    return 0;
}

/* scene.key_event(vk, scan, down): one key transition, as a keyboard gives it to core/user_input.c;
   while the scene holds an Alt key (VK_MENU) down, the keys it presses come with Alt. */
static int l_key_event(lua_State *L)
{
    static int alt;
    struct InputKey key;
    key.vk = (int)luaL_checkinteger(L, 1);
    key.scancode = (int)luaL_optinteger(L, 2, 0);
    key.down = lua_toboolean(L, 3);
    key.repeat = 0;
    key.count = 1;
    if (key.vk == VK_MENU)
        alt = key.down;
    key.alt = alt;
    input_key(&key);
    return 0;
}

static void key_transition(int vk, int down)
{
    struct InputKey key = {vk, 0, down, 0, 0, 1};
    input_key(&key);
}

/* scene.text(s): typed as a keyboard types it, a letter, digit or space with its key going down
   first (the window manager decides on the key whether characters go to the chat line) and up
   after. */
static int l_text(lua_State *L)
{
    const char *text = luaL_checkstring(L, 1);
    for (; *text; text++) {
        int ch = (unsigned char)*text;
        int vk = ch >= 'a' && ch <= 'z' ? ch - 'a' + 'A' : (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || ch == ' ' ? ch : 0;
        if (vk)
            key_transition(vk, 1);
        input_char(ch);
        if (vk)
            key_transition(vk, 0);
    }
    return 0;
}

static int l_mouse(lua_State *L)
{
    input_pointer((int)luaL_checkinteger(L, 1), (int)luaL_checkinteger(L, 2));
    return 0;
}

/* scene.wheel(delta): the mouse wheel turned by delta (120 a notch, positive away from the player) */
static int l_wheel(lua_State *L)
{
    input_wheel((int)luaL_checkinteger(L, 1));
    return 0;
}

/* scene.ticks(): the client's clock in milliseconds, which the fake map server's times are on */
static int l_ticks(lua_State *L)
{
    lua_pushinteger(L, (lua_Integer)platform_ticks_ms());
    return 1;
}

/* scene.button(x, y, down[, "right"]) */
static int l_button(lua_State *L)
{
    int x = (int)luaL_checkinteger(L, 1), y = (int)luaL_checkinteger(L, 2), down = lua_toboolean(L, 3);
    int right = lua_isstring(L, 4) && !strcmp(lua_tostring(L, 4), "right");
    input_button(right ? INPUT_BUTTON_RIGHT : INPUT_BUTTON_LEFT, down, x, y);
    return 0;
}

/* scene.resize(w, h): the windowed client area becomes w by h pixels, as when the player drags
   the window's frame, no smaller than the window's minimum size; the device follows on the next
   frame. */
static int l_resize(lua_State *L)
{
    RECT frame;
    platform_window_rect(g_hMainWnd, &frame);
    platform_window_place_windowed(g_hMainWnd, frame.left, frame.top, (int)luaL_checkinteger(L, 1),
                                   (int)luaL_checkinteger(L, 2), 0, 0);
    return 0;
}

static int l_open(lua_State *L)
{
    lua_pushboolean(L, ui_window_open((int)luaL_checkinteger(L, 1)) != NULL);
    return 1;
}

static void write_file(void *context, void *data, int size)
{
    fwrite(data, 1, (size_t)size, context);
}

/* The last frame shown's counters (README.md, "Counters") as one JSON object, its fields
   `separator` apart: what a capture's counters file and each line of a measure hold. */
static int print_counters(FILE *file, const char *separator)
{
    const struct perf_counters *c = &S.counters;
    struct World *world = g_modeMgr.curModeType == MODE_TYPE_GAME && g_modeMgr.curMode
                              ? ((struct GameMode *)g_modeMgr.curMode)->world : NULL;
    const char *s = separator;
    return fprintf(file,
                   "{%s\"frame\": %u,%s\"frame_ms\": %.3f,%s\"script_ms\": %.3f,%s\"objects\": %u,%s\"draws\": %u,"
                   "%s\"indirect_draws\": %u,%s\"draw_calls\": %u,%s\"primitives\": %u,%s\"modes_sent\": %u,"
                   "%s\"modes_skipped\": %u,%s\"texture_binds\": %u,%s\"vertex_bytes\": %u,%s\"instance_bytes\": %u,"
                   "%s\"texture_bytes\": %u,%s\"draw_scene_ms\": %.3f,%s\"view_render_ms\": %.3f,%s\"present_ms\": %.3f,"
                   "%s\"packets_ms\": %.3f,%s\"effects_ms\": %.3f,%s\"actors_ms\": %.3f,"
                   "%s\"objects_render_ms\": %.3f,%s\"effects_render_ms\": %.3f,%s\"ui_ms\": %.3f,"
                   "%s\"atlas_ms\": %.3f,%s\"texels_ms\": %.3f%s}\n",
                   s, S.frame, s, (double)S.frame_ns / 1e6, s, (double)S.script_ns / 1e6, s,
                   world && world->gameObjects.head ? (unsigned)world->gameObjects.size : 0u, s, c->draws, s,
                   c->indirectDraws, s, c->drawCalls, s, c->prims, s, c->modesSent, s, c->modesSkipped, s, c->texSets,
                   s, c->vertexBytes,
                   s, c->instanceBytes, s, c->textureBytes, s, (double)c->drawSceneNs / 1e6, s,
                   (double)c->viewRenderNs / 1e6, s, (double)c->presentNs / 1e6, s, (double)c->packetsNs / 1e6, s,
                   (double)c->effectsNs / 1e6, s, (double)c->actorsNs / 1e6, s, (double)c->objectsRenderNs / 1e6,
                   s, (double)c->effectsRenderNs / 1e6, s, (double)c->uiNs / 1e6, s, (double)c->atlasNs / 1e6, s,
                   (double)c->texelsNs / 1e6, *s == '\n' ? "\n" : "") > 0;
}

/* <out>/<name>.counters.json: the counters of the frame a capture shows (README.md). */
static int write_counters(const char *name)
{
    char path[1200];
    FILE *file;
    int ok;
    snprintf(path, sizeof path, "%s/%s.counters.json", S.out, name);
    file = fopen(path, "w");
    if (!file)
        return 0;
    ok = print_counters(file, "\n  ");
    if (fclose(file))
        ok = 0;
    return ok;
}

/* how many units in the last place two floats lie apart */
static unsigned ulps_apart(float a, float b)
{
    int32_t x, y;
    memcpy(&x, &a, 4);
    memcpy(&y, &b, 4);
    x = x < 0 ? INT32_MIN - x : x;
    y = y < 0 ? INT32_MIN - y : y;
    return x > y ? (unsigned)(x - y) : (unsigned)(y - x);
}

/* <out>/<name>.shadow_fit.json: the GPU's shadow fit of the frame a capture shows
   (gfx_read_shadow_fit) against render/shadow_fit.c, each step given the GPU's input to it: the
   range, the range kept and the extents to the bit, the split within the rounding of a division
   and two square roots, the squares, the sizes kept and the snapped translations to the bit, and
   the matrices, lookups and far depths within 1e-5 of the largest element of their row. A frame
   without shadows writes nothing. A mismatch fails the scene. Answers the mismatches, -1 where the
   frame drew no shadows. */
static int check_shadow_fit(const char *name)
{
    static struct GfxShadowFitRead g;
    static float *front, *back;
    static size_t room;
    const struct ShadowFitGpuRead *f = &g.fit;
    struct ShadowFitRange range, kept;
    struct ShadowFitDepths depths;
    struct ShadowFitExtent extents[SHADOW_FIT_CASCADES];
    struct ShadowFitCascades c;
    float split[SHADOW_FIT_CASCADES + 1], gpuSplit[SHADOW_FIT_CASCADES + 1];
    double worst = 0.0;
    unsigned splitUlps = 0;
    int mismatches = 0, k, i, j, receiving = 0;
    char path[1200];
    FILE *file;
    /* the size first, then the buffers with it */
    g.front = g.back = NULL;
    if (gfx_read_shadow_fit(&g) < 0)
        return -1;
    if ((size_t)g.width * (size_t)g.height > room) {
        free(front);
        free(back);
        room = (size_t)g.width * (size_t)g.height;
        front = malloc(room * sizeof *front);
        back = malloc(room * sizeof *back);
        if (!front || !back) {
            room = 0;
            mismatches = 1;
        }
    }
    g.front = front;
    g.back = back;
    if (mismatches || gfx_read_shadow_fit(&g) < 0) {
        fprintf(stderr, "scene: the GPU's shadow fit of %s could not be read\n", name);
        S.failures++;
        return 1;
    }
    depths.front = g.front;
    depths.back = g.back;
    depths.width = g.width;
    depths.height = g.height;
    for (i = 0; i < g.width * g.height; i++)
        receiving += g.front[i] >= g.frame.nearZ && g.front[i] <= g.frame.reach;
    shadow_fit_range(&range, &g.frame, &depths);
    if (range.nearest <= range.farthest ? memcmp(&range, &f->range, sizeof range)
                                        : !(f->range.nearest == HUGE_VALF && f->range.farthest == 0.0f))
        mismatches++;
    shadow_fit_keep(&kept, &f->range, &f->previousRange, &g.frame);
    mismatches += memcmp(&kept, &f->kept, sizeof kept) != 0;
    shadow_fit_split(split, &f->kept);
    gpuSplit[0] = split[0];
    memcpy(gpuSplit + 1, f->cascades.farDepth, sizeof f->cascades.farDepth);
    for (k = 1; k <= SHADOW_FIT_CASCADES; k++)
        splitUlps = ulps_apart(split[k], gpuSplit[k]) > splitUlps ? ulps_apart(split[k], gpuSplit[k]) : splitUlps;
    mismatches += splitUlps > 8;
    shadow_fit_extents(extents, &g.frame, gpuSplit, &depths);
    if (memcmp(extents, f->extents, sizeof extents))
        mismatches++;
    shadow_fit_cascades(&c, &g.frame, gpuSplit, f->extents, f->previous);
    mismatches += c.cascades != f->cascades.cascades;
    for (k = 0; k < SHADOW_FIT_CASCADES; k++) {
        const float *rows[2] = { c.world[k], c.fromView[k] }, *gpu[2] = { f->cascades.world[k], f->cascades.fromView[k] };
        float lookups[4] = { c.farDepth[k], c.offset[k], c.blur[k], c.slant[k] };
        float gpuLookups[4] = { f->cascades.farDepth[k], f->cascades.offset[k], f->cascades.blur[k], f->cascades.slant[k] };
        if (memcmp(c.lo[k], f->cascades.lo[k], sizeof c.lo[k]) || memcmp(c.hi[k], f->cascades.hi[k], sizeof c.hi[k])
            || c.kept[k] != f->cascades.kept[k] || memcmp(&c.world[k][12], &f->cascades.world[k][12], 2 * sizeof(float)))
            mismatches++;
        for (i = 0; i < 2; i++)
            for (j = 0; j < 16; j += 4) {
                double scale = 0.0, d;
                int e;
                for (e = 0; e < 4; e++)
                    scale = fmax(scale, fabs(rows[i][j + e]));
                for (e = 0; e < 4; e++) {
                    d = fabs((double)rows[i][j + e] - gpu[i][j + e]) / (scale > 0.0 ? scale : 1.0);
                    worst = d > worst ? d : worst;
                }
            }
        for (i = 0; i < 4; i++) {
            double d = fabs((double)lookups[i] - gpuLookups[i]) / fmax(fabs(lookups[i]), 1e-30);
            worst = d > worst ? d : worst;
        }
    }
    mismatches += worst > 1e-5;
    snprintf(path, sizeof path, "%s/%s.shadow_fit.json", S.out, name);
    file = fopen(path, "w");
    if (file) {
        fprintf(file, "{\n  \"frame\": %u,\n  \"pixels\": %d,\n  \"receiving\": %d,\n  \"cascades\": %d,\n"
                      "  \"nearest\": %.9g,\n  \"farthest\": %.9g,\n  \"kept\": [%.9g, %.9g],\n  \"split_ulps\": %u,\n"
                      "  \"worst_relative\": %.3g,\n  \"radius\": [%g, %g, %g, %g],\n  \"previous\": [%g, %g, %g, %g],\n"
                      "  \"mismatches\": %d\n}\n",
                S.frame, g.width * g.height, receiving, f->cascades.cascades, f->range.nearest, f->range.farthest,
                f->kept.nearest, f->kept.farthest, splitUlps, worst,
                (f->cascades.hi[0][0] - f->cascades.lo[0][0]) * 0.5f, (f->cascades.hi[1][0] - f->cascades.lo[1][0]) * 0.5f,
                (f->cascades.hi[2][0] - f->cascades.lo[2][0]) * 0.5f, (f->cascades.hi[3][0] - f->cascades.lo[3][0]) * 0.5f,
                f->previous[0], f->previous[1], f->previous[2], f->previous[3], mismatches);
        fclose(file);
    }
    if (mismatches) {
        fprintf(stderr, "scene: the GPU's shadow fit of %s is not render/shadow_fit.c's\n", name);
        S.failures++;
    }
    return mismatches;
}

/* scene.shadow_fit(): the shadows of the frame just shown as the GPU fitted them
   (gfx_read_shadow_fit), or nil without: a table of the cascades (4, or 0 where nothing receives),
   the range of view depths the slices were cut from (`nearest`, `farthest`) and, by cascade, its
   square's half size (`radius`), the size it keeps (`kept`), its far view depth (`far`) and its
   centre across the light (`x`, `y`). */
static int l_shadow_fit(lua_State *L)
{
    static struct GfxShadowFitRead g;
    static const char *const names[5] = { "radius", "kept", "far", "x", "y" };
    const struct ShadowFitCascades *c = &g.fit.cascades;
    int k, f;
    if (gfx_read_shadow_fit(&g) < 0) {
        lua_pushnil(L);
        return 1;
    }
    lua_newtable(L);
    lua_pushinteger(L, c->cascades);
    lua_setfield(L, -2, "cascades");
    lua_pushnumber(L, g.fit.kept.nearest);
    lua_setfield(L, -2, "nearest");
    lua_pushnumber(L, g.fit.kept.farthest);
    lua_setfield(L, -2, "farthest");
    for (f = 0; f < 5; f++) {
        lua_newtable(L);
        for (k = 0; k < SHADOW_FIT_CASCADES; k++) {
            float v = f == 0 ? (c->hi[k][0] - c->lo[k][0]) * 0.5f
                    : f == 1 ? c->kept[k]
                    : f == 2 ? c->farDepth[k]
                             : (c->lo[k][f - 3] + c->hi[k][f - 3]) * 0.5f;
            lua_pushnumber(L, v);
            lua_rawseti(L, -2, k + 1);
        }
        lua_setfield(L, -2, names[f]);
    }
    return 1;
}

/* scene.shadow_fit_check(name): the GPU's shadow fit of the frame just shown against
   render/shadow_fit.c, as a capture checks it with RAGEXE_SCENE_SHADOW_FIT=1: writes
   <out>/<name>.shadow_fit.json and fails the scene where they differ; answers the mismatches, or
   nil where the frame drew no shadows */
static int l_shadow_fit_check(lua_State *L)
{
    int mismatches = check_shadow_fit(luaL_checkstring(L, 1));
    if (mismatches < 0)
        lua_pushnil(L);
    else
        lua_pushinteger(L, mismatches);
    return 1;
}

/* scene.quake([ms]): the camera shakes as an effect shakes it (game/msg_misc.c, the game message
   the effects send): for 650 ms, or for ms where that is more */
static int l_quake(lua_State *L)
{
    game_send_message(MODE_MSG_QUAKE, (intptr_t)luaL_optinteger(L, 1, 0), 0, 0, 0);
    return 0;
}

/* A line of the measure's file for the frame just shown; the file is closed after the last. */
static void measure_frame(void)
{
    if (!print_counters(S.measure.file, " ")) {
        fprintf(stderr, "scene: cannot write %s\n", S.measure.path);
        S.failures++;
    }
    S.measure.logged++;
    if (--S.measure.left > 0)
        return;
    if (fclose(S.measure.file)) {
        fprintf(stderr, "scene: cannot write %s\n", S.measure.path);
        S.failures++;
    }
    S.measure.file = NULL;
    printf("scene: measured %s (%d frames, the last frame %u)\n", S.measure.path, S.measure.logged, S.frame);
}

/* scene.measure(frames, name): the counters of each of the next `frames` frames drawn and shown, a
   line of JSON each, in <out>/<name>.frames.jsonl. The script goes on at once; until the last of
   them is shown, a capture is an error, as reading the frame back stalls the GPU, and so is another
   measure. scene.measuring() answers the frames still to come. */
static int l_measure(lua_State *L)
{
    int frames = (int)luaL_checkinteger(L, 1);
    const char *name = luaL_checkstring(L, 2);
    if (S.measure.file)
        return luaL_error(L, "measure: %s is still being measured", S.measure.path);
    if (frames <= 0)
        return luaL_error(L, "measure: %d frames", frames);
    snprintf(S.measure.path, sizeof S.measure.path, "%s/%s.frames.jsonl", S.out, name);
    S.measure.file = fopen(S.measure.path, "w");
    if (!S.measure.file)
        return luaL_error(L, "measure: cannot write %s", S.measure.path);
    S.measure.left = frames;
    S.measure.logged = 0;
    return 0;
}

static int l_measuring(lua_State *L)
{
    lua_pushinteger(L, S.measure.file ? S.measure.left : 0);
    return 1;
}

/* scene.capture(name[, {w, h, x, y, jpeg}]): the whole frame, or a w x h part of it whose top
   left corner is x, y (default: centred on the frame), as <out>/<name>.png, or with jpeg = quality
   (1 to 100) as <out>/<name>.jpg, and the frame's counters beside it. The frame is the one just
   shown, downloaded from the GPU (gfx_read_frame). */
static int l_capture(lua_State *L)
{
    const char *name = luaL_checkstring(L, 1);
    int crop = lua_istable(L, 2), jpeg = opt_int(L, crop ? 2 : 0, "jpeg", 0);
    unsigned char *bgra, *rgb;
    char path[1200];
    FILE *file;
    int frameW = 0, frameH = 0, w, h, x, y, ok;
    RECT r;
    if (S.measure.file)
        return luaL_error(L, "capture: %s is still being measured", S.measure.path);
    if (!g_3dDevice.target || gfx_get_size(g_3dDevice.target, &frameW, &frameH) < 0 || frameW <= 0 || frameH <= 0)
        return luaL_error(L, "capture: no frame");
    w = frameW;
    h = frameH;
    r.left = r.top = 0;
    if (crop) {
        w = opt_int(L, 2, "w", frameW);
        h = opt_int(L, 2, "h", frameH);
        r.left = opt_int(L, 2, "x", (frameW - w) / 2);
        r.top = opt_int(L, 2, "y", (frameH - h) / 2);
        if (w <= 0 || h <= 0 || r.left < 0 || r.top < 0 || r.left + w > frameW || r.top + h > frameH)
            return luaL_error(L, "capture: %dx%d at %d,%d is not inside the %dx%d frame", w, h, (int)r.left,
                              (int)r.top, frameW, frameH);
    }
    r.right = r.left + w;
    r.bottom = r.top + h;
    bgra = malloc((size_t)w * h * 4);
    rgb = malloc((size_t)w * h * 3);
    if (!bgra || !rgb) {
        free(bgra);
        free(rgb);
        return luaL_error(L, "capture: out of memory");
    }
    if (gfx_read_frame(&r, bgra, (size_t)w * 4) < 0) {
        free(bgra);
        free(rgb);
        return luaL_error(L, "capture: cannot read the frame");
    }
    for (y = 0; y < h; y++)
        for (x = 0; x < w; x++) {
            const unsigned char *p = bgra + ((size_t)y * w + x) * 4;
            unsigned char *o = rgb + ((size_t)y * w + x) * 3;
            o[0] = p[2];
            o[1] = p[1];
            o[2] = p[0];
        }
    free(bgra);
    snprintf(path, sizeof path, "%s/%s.%s", S.out, name, jpeg > 0 ? "jpg" : "png");
    file = fopen(path, "wb");
    if (jpeg > 0)
        ok = file && stbi_write_jpg_to_func(write_file, file, w, h, 3, rgb, jpeg > 100 ? 100 : jpeg);
    else
        ok = file && stbi_write_png_to_func(write_file, file, w, h, 3, rgb, w * 3);
    if (file && fclose(file))
        ok = 0;
    free(rgb);
    if (!ok)
        return luaL_error(L, "capture: cannot write %s", path);
    if (!write_counters(name))
        return luaL_error(L, "capture: cannot write the counters of %s", path);
    printf("scene: captured %s (frame %u, %u shown)\n", path, S.frame, S.shown);
    if (getenv("RAGEXE_SCENE_SHADOW_FIT"))
        check_shadow_fit(name);
    return 0;
}

/* scene.effects(): a list of the effect IDs attached to the player (oldest first), and the number
   of objects in the world's object list (actors, effects and what they spawned). */
static int l_effects(lua_State *L)
{
    struct World *world = g_modeMgr.curModeType == MODE_TYPE_GAME && g_modeMgr.curMode
                              ? ((struct GameMode *)g_modeMgr.curMode)->world : NULL;
    struct AbleToMakeEffect *player = world ? (struct AbleToMakeEffect *)world->player : NULL;
    struct ListNode *node;
    int n = 0;
    lua_newtable(L);
    if (player && player->effects.head)
        for (node = player->effects.head->next; node != player->effects.head; node = node->next) {
            struct RagEffect *effect = *LIST_VALUE(node, struct RagEffect *);
            if (effect) {
                lua_pushinteger(L, effect->effect_id);
                lua_rawseti(L, -2, ++n);
            }
        }
    lua_pushinteger(L, world && world->gameObjects.head ? (lua_Integer)world->gameObjects.size : 0);
    return 2;
}

static int l_frame(lua_State *L)
{
    lua_pushinteger(L, (lua_Integer)S.frame);
    return 1;
}

static int l_mode(lua_State *L)
{
    lua_pushstring(L, g_modeMgr.curModeType == MODE_TYPE_GAME ? "game" : "login");
    return 1;
}

static int l_wait(lua_State *L)
{
    S.wait = WAIT_FRAMES;
    S.wait_frames = (int)luaL_optinteger(L, 1, 1);
    return lua_yield(L, 0);
}

static int l_wait_ready(lua_State *L)
{
    S.wait = WAIT_READY;
    S.stable_frames = 0;
    S.last_models = (size_t)-1;
    S.timeout = (int)luaL_optinteger(L, 1, READY_TIMEOUT_FRAMES);
    return lua_yield(L, 0);
}

static int l_fail(lua_State *L)
{
    fprintf(stderr, "scene: %s\n", luaL_checkstring(L, 1));
    S.failures++;
    return 0;
}

static const luaL_Reg scene_api[] = {
    {"enter_map", l_enter_map}, {"spawn", l_spawn}, {"vanish", l_vanish}, {"say", l_say},
    {"chat", l_chat}, {"send", l_send}, {"key_event", l_key_event}, {"text", l_text}, {"mouse", l_mouse},
    {"wheel", l_wheel}, {"button", l_button}, {"open", l_open}, {"resize", l_resize}, {"capture", l_capture},
    {"measure", l_measure}, {"measuring", l_measuring}, {"effects", l_effects}, {"frame", l_frame},
    {"ticks", l_ticks}, {"mode", l_mode}, {"wait", l_wait}, {"wait_ready", l_wait_ready}, {"fail", l_fail},
    {"shadow_fit", l_shadow_fit}, {"shadow_fit_check", l_shadow_fit_check}, {"quake", l_quake},
    {NULL, NULL},
};

static const char prelude[] =
    "function scene.click(x, y, button)\n"
    "  scene.mouse(x, y) scene.wait(1)\n"
    "  scene.button(x, y, true, button) scene.wait(2)\n"
    "  scene.button(x, y, false, button) scene.wait(1)\n"
    "end\n"
    "scene.VK = { RETURN = 0x0d, ESCAPE = 0x1b, TAB = 0x09, BACK = 0x08, F1 = 0x70, INSERT = 0x2d }\n"
    "-- scene.key(vk[, scan]): press and release a key.\n"
    "function scene.key(vk, scancode)\n"
    "  scene.key_event(vk, scancode or 0, true) scene.wait(1)\n"
    "  scene.key_event(vk, scancode or 0, false) scene.wait(1)\n"
    "end\n"
    "-- scene.alt_key(vk[, scan]): a key with Alt held, as the window shortcuts are pressed.\n"
    "function scene.alt_key(vk, scancode)\n"
    "  scene.key_event(0x12, 0x38, true) scene.wait(1)\n"
    "  scene.key(vk, scancode)\n"
    "  scene.key_event(0x12, 0x38, false) scene.wait(1)\n"
    "end\n";

/* ---- frame loop ---- */

static int world_ready(void)
{
    struct GameMode *game;
    struct World *world;
    size_t models;
    if (g_modeMgr.curModeType != MODE_TYPE_GAME || !g_modeMgr.curMode)
        return 0;
    game = (struct GameMode *)g_modeMgr.curMode;
    world = game->world;
    if (!world)
        return 0;
    models = world->models.count;
    if (models != S.last_models || world->models.pending || world->models.inserted != models) {
        S.last_models = models;
        S.stable_frames = 0;
        return 0;
    }
    return ++S.stable_frames >= READY_STABLE_FRAMES;
}

static void start(void)
{
    const char *script = getenv("RAGEXE_SCENE"), *out = getenv("RAGEXE_SCENE_OUT");
    S.started = 1;
    if (!script) {
        fprintf(stderr, "scene: set RAGEXE_SCENE to a scene script\n");
        finish(1);
    }
    snprintf(S.out, sizeof S.out, "%s", out ? out : ".");
    make_directory(S.out);
    S.L = luaL_newstate();
    luaL_openlibs(S.L);
    luaL_register(S.L, "scene", scene_api);
    {
        /* scene.dir: the script's directory, with its separator, for the files it loads */
        const char *end = script, *p;
        for (p = script; *p; p++)
            if (*p == '/' || *p == '\\')
                end = p + 1;
        lua_pushlstring(S.L, script, (size_t)(end - script));
        lua_setfield(S.L, -2, "dir");
    }
    lua_pop(S.L, 1);
    if (luaL_dostring(S.L, prelude) || luaL_loadfile(S.L, script)) {
        fprintf(stderr, "scene: %s\n", lua_tostring(S.L, -1));
        finish(1);
    }
    S.co = lua_newthread(S.L);
    lua_pushvalue(S.L, -2);
    lua_xmove(S.L, S.co, 1);
    scene_net_set_handler(on_client_packet);
    S.wait = WAIT_FRAMES;
    S.wait_frames = 2;
}

/* One frame of virtual time per shown frame: loops that wait on the clock (fades, timeouts)
   always present while they wait. The counters start again with each frame shown, so that a
   capture writes those of the frame it shows. A frame's time is the client's alone: from the
   moment the driver was done with the frame before to the moment this one was shown, less the
   script's steps in between, which count as the frame's script time. */
void drive_state_frame_shown(void)
{
    int64_t now = platform_stopwatch_ns();
    S.shown++;
    /* a frame shown again without being drawn (a move while the window is inactive) keeps the
       counters of the frame it shows, and a measure does not count it */
    if (g_perf.drawCalls || g_perf.drawSceneNs) {
        S.counters = g_perf;
        memset(&g_perf, 0, sizeof g_perf);
        S.frame_ns = S.shown_ns ? now - S.shown_ns - S.script_running_ns : 0;
        S.script_ns = S.script_running_ns;
        if (S.measure.file)
            measure_frame();
    }
    S.script_running_ns = 0;
    scene_clock_advance(SCENE_FRAME_MS);
    S.shown_ns = platform_stopwatch_ns();
}

void drive_state_tick(void)
{
    int status;
    int64_t resumed;
    if (layout_byte)
        layout_stack();
    S.frame++;
    if (getenv("RAGEXE_SCENE_TRACE") && g_modeMgr.curModeType != S.traced_mode) {
        fprintf(stderr, "scene: mode %d at frame %u (%s)\n", g_modeMgr.curModeType, S.frame, g_modeMgr.curModeName);
        S.traced_mode = g_modeMgr.curModeType;
    }
    if (S.finished)
        return;
    if (!S.started)
        start();
    switch (S.wait) {
    case WAIT_FRAMES:
        if (--S.wait_frames > 0)
            return;
        break;
    case WAIT_READY:
        if (!world_ready()) {
            if (--S.timeout > 0)
                return;
            struct World *world = g_modeMgr.curModeType == MODE_TYPE_GAME && g_modeMgr.curMode
                                      ? ((struct GameMode *)g_modeMgr.curMode)->world : NULL;
            fprintf(stderr, "scene: the map did not finish loading (mode %d, map connection %d, models %zu of %zu, "
                    "pending %d)\n", g_modeMgr.curModeType, scene_net_connected(SCENE_MAP_PORT),
                    world ? world->models.inserted : 0, world ? world->models.count : 0,
                    world ? (int)world->models.pending : -1);
            finish(1);
        }
        break;
    case WAIT_NONE:
        break;
    }
    S.wait = WAIT_NONE;
    resumed = platform_stopwatch_ns();
    status = lua_resume(S.co, 0);
    S.script_running_ns += platform_stopwatch_ns() - resumed;
    if (status == LUA_YIELD)
        return;
    S.finished = 1;
    if (status != 0) {
        fprintf(stderr, "scene: %s\n", lua_tostring(S.co, -1));
        finish(1);
    }
    printf("scene: done after %u frames\n", S.frame);
    finish(0);
}
