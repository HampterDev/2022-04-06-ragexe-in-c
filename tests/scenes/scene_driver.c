/* Scene driver: replaces core/drive_state.c in the scene harness. The client calls
   drive_state_frame_shown() for every frame it shows, which advances the virtual clock by one
   frame, and drive_state_tick() once per message pump, which resumes the Lua scene script. The
   script's `scene` table puts the client into a state (map, actors, chat, windows, input) and
   captures PNGs; the driver also answers the client's map server requests.

   Environment: RAGEXE_SCENE=<script.lua>, RAGEXE_SCENE_OUT=<directory for captures>,
   RAGEXE_SCENE_TRACE=1 for the packets and mode changes. See tests/scenes/README.md. */
#include "core/app.h"
#include "core/drive_state.h"
#include "core/window_events.h"
#include "contents/account_settings_internal.h"
#include "contents/mouse.h"
#include "game/gamemode.h"
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
#include "session/character_name.h"
#include "session/state.h"
#include "std/string.h"
#include "ui/window_access.h"
#include "ui/window_keyboard.h"
#include "ui/window_manager_state.h"
#include "world/mode_manager.h"
#include "world/models.h"
#include "world/state.h"
#include "scene.h"
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
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
} S;

/* Headless, silent and without vsync unless the caller says otherwise; set before the client
   reads them. */
#ifdef _WIN32
static void scene_defaults(void)
{
    if (!getenv("RAGEXE_VSYNC"))
        _putenv_s("RAGEXE_VSYNC", "0");
}
#pragma section(".CRT$XCU", read)
__declspec(allocate(".CRT$XCU")) void (*scene_defaults_entry)(void) = scene_defaults;
#else
__attribute__((constructor)) static void scene_defaults(void)
{
    setenv("SDL_VIDEODRIVER", "offscreen", 0);
    setenv("SDL_AUDIODRIVER", "dummy", 0);
    setenv("RAGEXE_VSYNC", "0", 0);
}
#endif

void drive_state_note_chat(const char *text)
{
    (void)text;
}

static void finish(int failed)
{
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
   head_mid, head_bottom, robe, body_palette}) — from the login screen straight into a map. */
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
    S.enter_x = x;
    S.enter_y = y;
    S.enter_dir = opt_int(L, opts, "dir", 0);
    login_mode_enter_zone(login, SCENE_GID, string_from_cstr(gat), platform_net_parse_ipv4("127.0.0.1"),
                          SCENE_MAP_PORT, string_from_cstr(""));
    return 0;
}

/* scene.spawn{aid, job, x, y, dir, name, kind = "monster"|"npc"|"player", hair, hair_color,
   weapon, shield, head_top, head_mid, head_bottom, robe, sex, level, hp, max_hp, speed, state} */
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
    unsigned char packet[4 + 256];
    if (size > 255)
        size = 255;
    put16(packet, HEADER_ZC_NOTIFY_PLAYERCHAT);
    put16(packet + 2, (unsigned)(4 + size + 1));
    memcpy(packet + 4, text, size);
    packet[4 + size] = 0;
    deliver(packet, (int)(4 + size + 1));
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

/* scene.key_event(vk, scan, down, alt): one key transition, routed the way core/app_sdl.c routes
   SDL key events (Alt combinations and F10 go straight to the window manager). */
static int l_key_event(lua_State *L)
{
    int vk = (int)luaL_checkinteger(L, 1), scan = (int)luaL_optinteger(L, 2, 0);
    int down = lua_toboolean(L, 3), alt = lua_toboolean(L, 4);
    int lparam = scan << 16 | 1 | (alt ? 1 << 29 : 0);
    platform_note_key(vk, down);
    if (!down)
        window_events_key_up(vk, lparam | (int)0xc0000000u);
    else if (alt || vk == VK_F10)
        ui_window_mgr_on_key_down(&g_windowMgr, vk, lparam, 1);
    else
        window_events_key_down(vk, lparam);
    return 0;
}

static int l_text(lua_State *L)
{
    const char *text = luaL_checkstring(L, 1);
    for (; *text; text++)
        window_events_char((unsigned char)*text);
    return 0;
}

static int l_mouse(lua_State *L)
{
    int x = (int)luaL_checkinteger(L, 1), y = (int)luaL_checkinteger(L, 2);
    platform_note_cursor(x, y);
    mouse_on_move(&g_mouse);
    return 0;
}

/* scene.button(x, y, down[, "right"]) */
static int l_button(lua_State *L)
{
    int x = (int)luaL_checkinteger(L, 1), y = (int)luaL_checkinteger(L, 2), down = lua_toboolean(L, 3);
    int right = lua_isstring(L, 4) && !strcmp(lua_tostring(L, 4), "right");
    platform_note_cursor(x, y);
    platform_note_key(right ? VK_RBUTTON : VK_LBUTTON, down);
    mouse_on_button(&g_mouse, g_hMainWnd, right ? MOUSE_RIGHT : MOUSE_LEFT, down);
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

static int l_capture(lua_State *L)
{
    const char *name = luaL_checkstring(L, 1);
    struct gfx_lock lk;
    unsigned char *rgb;
    char path[1200];
    FILE *file;
    DWORD x, y;
    int ok;
    if (!g_3dDevice.target || gfx_lock(g_3dDevice.target, NULL, GFX_LOCK_WAIT, &lk) < 0)
        return luaL_error(L, "capture: cannot read the frame");
    rgb = malloc((size_t)lk.width * lk.height * 3);
    if (!rgb) {
        gfx_unlock(g_3dDevice.target);
        return luaL_error(L, "capture: out of memory");
    }
    for (y = 0; y < lk.height; y++) {
        const unsigned char *row = (const unsigned char *)lk.bits + (size_t)y * lk.pitch;
        for (x = 0; x < lk.width; x++) {
            const unsigned char *p = row + (size_t)x * (lk.bpp / 8);
            unsigned char *o = rgb + ((size_t)y * lk.width + x) * 3;
            if (lk.bpp == 32) {
                o[0] = p[2];
                o[1] = p[1];
                o[2] = p[0];
            } else {
                unsigned v = p[0] | p[1] << 8;
                o[0] = (unsigned char)(((v >> 11) & 31) * 255 / 31);
                o[1] = (unsigned char)(((v >> 5) & 63) * 255 / 63);
                o[2] = (unsigned char)((v & 31) * 255 / 31);
            }
        }
    }
    gfx_unlock(g_3dDevice.target);
    snprintf(path, sizeof path, "%s/%s.png", S.out, name);
    file = fopen(path, "wb");
    ok = file && stbi_write_png_to_func(write_file, file, (int)lk.width, (int)lk.height, 3, rgb, (int)lk.width * 3);
    if (file && fclose(file))
        ok = 0;
    free(rgb);
    if (!ok)
        return luaL_error(L, "capture: cannot write %s", path);
    printf("scene: captured %s (frame %u, %u shown)\n", path, S.frame, S.shown);
    return 0;
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
    {"button", l_button}, {"open", l_open}, {"capture", l_capture}, {"frame", l_frame},
    {"mode", l_mode}, {"wait", l_wait}, {"wait_ready", l_wait_ready}, {"fail", l_fail},
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
    "  scene.key_event(vk, scancode or 0, true, false) scene.wait(1)\n"
    "  scene.key_event(vk, scancode or 0, false, false) scene.wait(1)\n"
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
#ifdef _WIN32
    /* The Windows client has a real window: hidden, the desktop's mouse cannot reach it and
       focus goes back where it was. */
    ShowWindow(g_hMainWnd, SW_HIDE);
#endif
    S.L = luaL_newstate();
    luaL_openlibs(S.L);
    luaL_register(S.L, "scene", scene_api);
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
   always present while they wait. */
void drive_state_frame_shown(void)
{
    S.shown++;
    scene_clock_advance(SCENE_FRAME_MS);
}

void drive_state_tick(void)
{
    int status;
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
    status = lua_resume(S.co, 0);
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
