/* Playground driver: replaces core/drive_state.c in the playground build, as the scene driver
   does in the scene harness. The client calls drive_state_frame_shown() for every frame it
   shows, which advances the virtual clock by one frame, and drive_state_tick() once per message
   pump. Each tick the driver tells the runner what the client sent and where every actor is
   drawn, and waits for the runner's answer: what the server sends the client now, and what the
   player does (enter a map, walk to a cell, attack). So client and server run in lockstep on
   one clock and a run repeats exactly.

   What travels through the delivery piece in the real thing travels through it here, with a
   wire between its two ends that can lose and repeat datagrams by a seed
   (tools/lossy_link/wire.h): what the client sends on the delivery channel, its requests, on
   its way to the runner, and what of the server's the route table names
   (delivery/delivery_route.h: the units' records and hits) on its way to the client. The wire
   holds nothing back: how long the line is stays the runner's to say. On a wire that loses
   nothing everything arrives in the frame it was sent, in the order it was sent.

   Environment: RAGEXE_PLAY=<the runner's port>, and for the wire RAGEXE_PLAY_LOSS and
   RAGEXE_PLAY_REPEAT (datagrams of a hundred, each way) and RAGEXE_PLAY_SEED.
   tests/playground/README.md has the messages. */
#include "core/app.h"
#include "core/drive_state.h"
#include "actor/actor_ops.h"
#include "actor/types.h"
#include "actor/render_ops.h"
#include "contents/account_settings_internal.h"
#include "contents/connection_api.h"
#include "contents/sound.h"
#include "core/packet_table.h"
#include "delivery/delivery.h"
#include "delivery/delivery_route.h"
#include "game/cells.h"
#include "game/gamemode.h"
#include "game/net_clock.h"
#include "game/own_body.h"
#include "body/body.h"
#include "login/char_info.h"
#include "login/loginmode.h"
#include "platform/clock.h"
#include "platform/net.h"
#include "session/character_name.h"
#include "session/state.h"
#include "std/list.h"
#include "std/string.h"
#include "world/mode_manager.h"
#include "world/state.h"
#include "world/view_floors.h"
#include "lossy_link/wire.h"
#include "scene.h"
#include "play.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PLAY_AID 2000000
#define PLAY_GID 150000
#define ZC_OWN_BODY 0x0f25

enum { MSG_FRAME = 1, MSG_STEP = 2 };
enum { DO_ENTER = 1, DO_WALK = 2, DO_ATTACK = 3, DO_CANCEL = 4, DO_QUIT = 5, DO_SKILL = 6 };
enum { PLAYER_MSG_ATTACK = 0xa, PLAYER_MSG_CANCEL = 0xd, PLAYER_MSG_WALK = 0x5b };

#define OUT_MAX 0x80000
#define SENT_MAX 0x20000

static struct {
    int started;
    unsigned frame;
    unsigned char sent[SENT_MAX]; /* what the client sent since the last tick: u16 size, bytes */
    size_t sent_size;
    unsigned sent_count;
    unsigned char out[OUT_MAX];
    unsigned char *in;
    size_t in_capacity;
} P;

/* Headless, silent and without vsync, set before the client reads them. */
static void play_defaults(void)
{
    if (!getenv("RAGEXE_VSYNC"))
        _putenv_s("RAGEXE_VSYNC", "0");
    if (!getenv("SDL_AUDIODRIVER"))
        _putenv_s("SDL_AUDIODRIVER", "dummy");
}
#pragma section(".CRT$XCU", read)
__declspec(allocate(".CRT$XCU")) void (*play_defaults_entry)(void) = play_defaults;

void drive_state_note_chat(const char *text)
{
    (void)text;
}

/* How late the player's own body is through the piece: each ZC_OWN_BODY the server's end is
   handed, by the low half of its tick, with when it was handed, and of each one the client's end
   hands on, how long after that it was. One that a newer replaced before it arrived is not
   handed on and not measured. */
#define BODY_SENT_KEPT 64
static struct {
    struct { uint32_t tick, at; } sent[BODY_SENT_KEPT];
    unsigned next;
    unsigned long taken, total_ms, worst_ms;
} B;

static void leave(int code, const char *why)
{
    if (why)
        fprintf(stderr, "playground: %s\n", why);
    /* read by the runner */
    fprintf(stderr, "playground: own body through the piece: %lu taken, %lu ms late in all, %lu at worst\n", B.taken,
            B.total_ms, B.worst_ms);
    fprintf(stderr, "playground: own body words that differed unexplained: %u\n", game_mode_own_body_unexplained());
    fflush(stdout);
    fflush(stderr);
    exit(code);
}

static void put16(unsigned char *p, unsigned v) { p[0] = (unsigned char)v; p[1] = (unsigned char)(v >> 8); }
static void put32(unsigned char *p, unsigned long v) { put16(p, (unsigned)(v & 0xffff)); put16(p + 2, (unsigned)(v >> 16)); }
static unsigned get16(const unsigned char *p) { return p[0] | (unsigned)p[1] << 8; }
static unsigned long get32(const unsigned char *p) { return get16(p) | (unsigned long)get16(p + 2) << 16; }

/* The requests' way to the runner: the client's end of the delivery piece, the wire, and the
   end the gateway holds in the real thing. A message is the packets of one write, each behind
   its size in two bytes, as the runner is told them. */
static struct {
    struct Delivery *client, *server;
    struct LossyWire wire;
    unsigned char message[DELIVERY_MESSAGE_MAX]; /* a stamp that waits for its request */
    size_t message_size;
} L;

static void body_sent(const unsigned char *packet, size_t size, uint32_t now)
{
    if (size < 12 || get16(packet) != ZC_OWN_BODY)
        return;
    B.sent[B.next % BODY_SENT_KEPT].tick = (uint32_t)get32(packet + 8);
    B.sent[B.next % BODY_SENT_KEPT].at = now;
    B.next++;
}

static void body_taken(const unsigned char *packet, size_t size, uint32_t now)
{
    uint32_t tick;
    unsigned i;

    if (size < 12 || get16(packet) != ZC_OWN_BODY)
        return;
    tick = (uint32_t)get32(packet + 8);
    for (i = 0; i < BODY_SENT_KEPT && i < B.next; i++) {
        unsigned k = (B.next - 1 - i) % BODY_SENT_KEPT;

        if (B.sent[k].tick == tick) {
            uint32_t late = now - B.sent[k].at;

            B.taken++;
            B.total_ms += late;
            if (late > B.worst_ms)
                B.worst_ms = late;
            return;
        }
    }
}

static void line_open(void)
{
    /* as platform/net_enet.c and the gateway's src/enet_listener.c have them */
    static const struct DeliveryConfig client = { 64, 16 * 1024, 4096, 0 }, server = { 2048, 128 * 1024, 8 * 1024, 0 };
    const char *loss = getenv("RAGEXE_PLAY_LOSS"), *repeat = getenv("RAGEXE_PLAY_REPEAT");
    const char *seed = getenv("RAGEXE_PLAY_SEED");
    struct LossyWireConfig wire = { 0, 0, 0, 0 };
    size_t client_size = delivery_size(&client), server_size = delivery_size(&server);

    wire.loss_pct = loss ? (unsigned)atoi(loss) : 0;
    wire.repeat_pct = repeat ? (unsigned)atoi(repeat) : 0;
    if (wire.loss_pct > 90 || wire.repeat_pct > 100)
        leave(2, "RAGEXE_PLAY_LOSS is at most 90 and RAGEXE_PLAY_REPEAT at most 100");
    lossy_wire_init(&L.wire, seed ? (uint32_t)strtoul(seed, NULL, 10) : 1, &wire);
    L.client = delivery_init(malloc(client_size), client_size, &client, platform_ticks_ms());
    L.server = delivery_init(malloc(server_size), server_size, &server, platform_ticks_ms());
    if (!L.client || !L.server)
        leave(2, "no memory for the line's delivery piece");
}

/* A packet of the client's for the runner's next frame. */
static void tell_packet(const unsigned char *packet, size_t length)
{
    if (P.sent_size + 2 + length > sizeof P.sent)
        leave(2, "the client sent more in a frame than the driver keeps");
    put16(P.sent + P.sent_size, (unsigned)length);
    memcpy(P.sent + P.sent_size + 2, packet, length);
    P.sent_size += 2 + length;
    P.sent_count++;
}

/* The packets of a message, each behind its size as on_client_packet put them together. */
static void tell_message(const unsigned char *message, size_t size)
{
    size_t at, length;

    for (at = 0; at + 2 <= size && (length = get16(message + at)) <= size - at - 2; at += 2 + length)
        tell_packet(message + at + 2, length);
}

/* Lets the two ends say what they have due, over the wire, until neither has more: datagrams
   out, what the wire lets through in, what the server's end hands on to the runner and what the
   client's end hands on to the client. */
static void line_run(void)
{
    unsigned char datagram[LOSSY_WIRE_DATAGRAM_MAX];
    const unsigned char *message;
    uint32_t now = platform_ticks_ms();
    size_t size;
    int direction, moved;

    do {
        moved = 0;
        while ((size = delivery_make(L.client, now, datagram, sizeof datagram)) != 0) {
            lossy_wire_put(&L.wire, now, 0, datagram, size);
            moved = 1;
        }
        while ((size = delivery_make(L.server, now, datagram, sizeof datagram)) != 0) {
            lossy_wire_put(&L.wire, now, 1, datagram, size);
            moved = 1;
        }
        while ((size = lossy_wire_take(&L.wire, now, &direction, datagram)) != 0) {
            struct Delivery *end = direction ? L.client : L.server;

            delivery_arrived(end, now, datagram, size);
            while ((message = delivery_take(end, &size)) != NULL) {
                if (!direction) {
                    tell_message(message, size);
                    continue;
                }
                body_taken(message, size, now);
                if (!scene_net_deliver(SCENE_MAP_PORT, message, (int)size))
                    leave(2, "the server sent before the client connected to the map");
            }
            moved = 1;
        }
    } while (moved);
}

/* A message for the delivery piece is complete: it leaves. One the piece has no room for goes
   the way everything else goes, as the socket sends it on the world channel then. */
static void send_message(void)
{
    if (!L.message_size)
        return;
    if (!delivery_send(L.client, DELIVERY_ORDERED, 0, 0, 0, L.message, L.message_size))
        tell_message(L.message, L.message_size);
    L.message_size = 0;
    line_run();
}

/* What the client sends on the map connection is kept for the runner. What leaves on the
   delivery channel goes through the piece: a stamp waits for the request it goes before, and
   the two are one message, as they are one write of the socket. */
static void on_client_packet(int port, const unsigned char *packet, int length)
{
    unsigned id;
    int channel;

    if (port != SCENE_MAP_PORT || length < 2 || length > 0xffff)
        return;
    id = get16(packet);
    channel = rag_connection_packet_channel((unsigned short)id);
    /* as the socket keeps them: a world packet sent while requests are in the piece goes through
       the piece behind them */
    if (channel == PLATFORM_CHANNEL_WORLD && delivery_waiting(L.client))
        channel = PLATFORM_CHANNEL_DELIVERY;
    if (channel != PLATFORM_CHANNEL_DELIVERY || L.message_size + 2 + (size_t)length > sizeof L.message) {
        /* a stamp that still waits goes first */
        send_message();
        tell_packet(packet, (size_t)length);
        return;
    }
    put16(L.message + L.message_size, (unsigned)length);
    memcpy(L.message + L.message_size + 2, packet, (size_t)length);
    L.message_size += 2 + (size_t)length;
    if (id != CZ_INPUT_STAMP)
        send_message();
}

/* From the login screen straight into a map, as the scene harness enters one: the character
   the runner describes, with no login and no character server. */
static void enter(const unsigned char *p, size_t size)
{
    struct LoginMode *login;
    struct CharInfo c;
    char gat[40], map[25], name[25];

    if (size < 24 + 9 * 4 + 24 || g_modeMgr.curModeType != MODE_TYPE_LOGIN || !g_modeMgr.curMode)
        leave(2, "told to enter a map when not on the login screen");
    login = (struct LoginMode *)g_modeMgr.curMode;
    memcpy(map, p, 24);
    map[24] = 0;
    memcpy(name, p + 24 + 9 * 4, 24);
    name[24] = 0;
    memset(&c, 0, sizeof c);
    c.gid = PLAY_GID;
    c.job = (short)get32(p + 24 + 12);
    c.head = 1;
    c.level = (short)get32(p + 24 + 16);
    c.job_level = (int)get32(p + 24 + 20);
    c.hp = c.max_hp = (int)get32(p + 24 + 24);
    c.sp = c.max_sp = (int)get32(p + 24 + 28);
    c.walk_speed = 150;
    c.sex = (BYTE)get32(p + 24 + 32);
    memset(c.stats, 1, sizeof c.stats);
    strncpy(c.name, name, sizeof c.name - 1);
    snprintf(gat, sizeof gat, "%s.gat", map);
    strncpy(c.map_name, gat, sizeof c.map_name - 1);
    login->chars[0] = c;
    login->numChar = 1;
    g_loginCharInfo = c;
    g_session1.ownAid = PLAY_AID;
    g_session1.authCode = 1;
    g_session1.sex = c.sex;
    g_session1.accountSex = c.sex;
    session_set_character_name(&g_session1, text_view(c.name, sizeof c.name));
    if (g_accountUserDataMgr) {
        g_accountUserDataMgr->loaded = 1;
        g_accountUserDataMgr->path[0] = 0;
    }
    login_mode_enter_zone(login, PLAY_GID, string_from_cstr(gat), platform_net_parse_ipv4("127.0.0.1"),
                          SCENE_MAP_PORT, string_from_cstr(""));
}

static struct GameMode *game(void)
{
    return g_modeMgr.curModeType == MODE_TYPE_GAME ? (struct GameMode *)g_modeMgr.curMode : NULL;
}

/* Where an actor is drawn, in 256ths of a cell, a cell's middle at a whole multiple of 256: the
   drawn point of its place (world/drawn.h), or 0 before its first record. */
static void point_of(const struct GameActor *actor, long *x, long *y)
{
    int32_t px, py;

    drawn_point(&actor->place, &px, &py);
    *x = (long)px;
    *y = (long)py;
}

/* The own body, for the runner to hold against the server's by tick: the cell the walk the
   character is drawn on has it on at the own clock, with the tick the step under way lands
   at; and the body the predicted world has (game/own_body.h), with its tick. Returns what it
   wrote. */
static size_t report_body(struct GameMode *mode, struct GameActor *own, unsigned char *p)
{
    unsigned char state[BODY_STATE_SIZE];
    DWORD clock = 0, lands = 0, tick = 0;
    int x = 0, y = 0, sub_x, sub_y, known = 0, predicted;
    size_t i;

    if (mode && own && game_mode_knows_clock(mode)) {
        clock = game_mode_own_time(mode);
        known = 1;
        game_mode_world_to_sub_cell(mode, own->pos.x, own->pos.z, &x, &y, &sub_x, &sub_y);
        if (drawn_record(&own->place)) {
            /* the last cell of its record whose time has come, and the one it is on the way to */
            struct MoveCell cells[MOVE_TRACK_STEPS + 1];
            size_t count = (size_t)move_record_cells(drawn_record(&own->place), cells);

            x = cells[0].x;
            y = cells[0].y;
            for (i = 1; i < count && (int)(cells[i].tick - clock) <= 0; i++) {
                x = cells[i].x;
                y = cells[i].y;
            }
            if (i < count)
                lands = cells[i].tick;
        }
    }
    predicted = game_mode_own_body(&tick, state);
    p[0] = (unsigned char)known;
    p[1] = (unsigned char)predicted;
    p[2] = p[3] = 0;
    put32(p + 4, clock);
    put32(p + 8, (unsigned long)x);
    put32(p + 12, (unsigned long)y);
    put32(p + 16, lands);
    put32(p + 20, predicted ? tick : 0);
    if (predicted)
        memcpy(p + 24, state, sizeof state);
    else
        memset(p + 24, 0, sizeof state);
    return 24 + sizeof state;
}

/* The frame's report: the clock, the mode, where the player and every actor is drawn, what the
   client sent, and the own body. Returns its size. */
static size_t report(void)
{
    struct GameMode *mode = game();
    struct World *world = mode ? mode->world : NULL;
    struct GameActor *own = world ? (struct GameActor *)world->player : NULL;
    unsigned char *p = P.out + 4, *count;
    unsigned actors = 0;
    long x = 0, y = 0;

    *p++ = MSG_FRAME;
    *p++ = (unsigned char)(g_modeMgr.curMode ? g_modeMgr.curModeType : 255);
    *p++ = own != NULL;
    *p++ = 0;
    put32(p, platform_ticks_ms());
    p += 4;
    if (own)
        point_of(own, &x, &y);
    put32(p, (unsigned long)x);
    put32(p + 4, (unsigned long)y);
    p[8] = (unsigned char)(own ? own->state : 0);
    p[9] = p[10] = p[11] = 0;
    p += 12;
    count = p;
    p += 4;
    if (world && world->actors.head) {
        struct ListNode *head = world->actors.head, *n;

        for (n = head->next; n != head; n = n->next) {
            struct GameActor *actor = *LIST_VALUE(n, struct GameActor *);

            if (!actor || actor == own)
                continue;
            if ((size_t)(p - P.out) + 24 + 4 + P.sent_size + 16 + 24 + BODY_STATE_SIZE > sizeof P.out)
                break;
            point_of(actor, &x, &y);
            put32(p, (unsigned long)actor->aid);
            put32(p + 4, (unsigned long)x);
            put32(p + 8, (unsigned long)y);
            p[12] = (unsigned char)actor->state;
            p[13] = actor->is_pc != 0;
            put16(p + 14, (unsigned)actor->job);
            /* the frame its action is on, and the frame its attack shows its hit on */
            p[16] = (unsigned char)(actor->motion < 0 ? 0 : actor->motion > 254 ? 254 : actor->motion);
            p[17] = (unsigned char)(actor->attack_motion_count > 0.0f && actor->attack_motion_count < 255.0f
                                        ? (int)actor->attack_motion_count : 255);
            /* whether the client has the actor's unit in view (world/view_floors.h): an actor that
               is shown after its going, dying, is not */
            p[18] = mode && view_floors_in_view(&mode->view_floors, (uint32_t)actor->aid) ? 1 : 0;
            p[19] = 0;
            /* the server's tick the actor is drawn on */
            put32(p + 20, game_mode_knows_clock(mode) ? game_actor_walk_time(actor) : 0);
            p += 24;
            actors++;
        }
    }
    put32(count, actors);
    put32(p, P.sent_count);
    p += 4;
    memcpy(p, P.sent, P.sent_size);
    p += P.sent_size;
    P.sent_size = 0;
    P.sent_count = 0;
    p += report_body(mode, own, p);
    put32(P.out, (unsigned long)(p - P.out - 4));
    return (size_t)(p - P.out);
}

static void act(int kind, const unsigned char *p, size_t size)
{
    struct GameMode *mode = game();
    struct Player *player = mode && mode->world ? mode->world->player : NULL;

    switch (kind) {
    case DO_ENTER:
        enter(p, size);
        break;
    case DO_WALK:
        if (player && size >= 8)
            render_object_send(player, NULL, PLAYER_MSG_WALK, (int)get32(p), (int)get32(p + 4), 0, 0);
        break;
    case DO_ATTACK:
        if (player && size >= 8)
            render_object_send(player, NULL, PLAYER_MSG_ATTACK, (int)get32(p), (int)get32(p + 4), 0, 0);
        break;
    case DO_CANCEL:
        if (player)
            render_object_send(player, NULL, PLAYER_MSG_CANCEL, 0, 0, 0, 0);
        break;
    case DO_QUIT:
        leave(size >= 4 ? (int)get32(p) : 0, NULL);
        break;
    case DO_SKILL:
        /* the intent a skill on a body leaves as: what the skill bar ends in */
        if (player && mode && size >= 12)
            game_mode_send_intent(mode, INTENT_SKILL_ACTOR, 0, (unsigned)get32(p), 0, 0, (int)get32(p + 4), (int)get32(p + 8));
        break;
    default:
        leave(2, "the runner asked for something the driver does not know");
    }
}

/* What the server sends the client now, a packet at a time in the order sent: what the route
   table names through the server's end of the piece and the wire, the rest as it is. Bytes the
   packet table cannot divide go as they are. */
static void deliver(const unsigned char *bytes, size_t size)
{
    while (size) {
        struct PacketLengthInfo info;
        struct DeliveryRoute route;
        size_t length = size;

        if (size >= 2) {
            packet_table_get_length(&g_packetLengthMap, &info, (int)get16(bytes));
            if (info.kind == PACKET_KIND_FIXED && info.length >= 2)
                length = (size_t)info.length;
            else if (info.kind == PACKET_KIND_VARIABLE && size >= 4)
                length = get16(bytes + 2);
            if (length < 2 || length > size)
                length = size;
        }
        if (delivery_route(bytes, length, &route) && route.kind != DELIVERY_ORDERED
            && delivery_send(L.server, route.kind, route.key, route.from, route.priority, bytes, length)) {
            body_sent(bytes, length, platform_ticks_ms());
            line_run();
        } else if (!scene_net_deliver(SCENE_MAP_PORT, bytes, (int)length)) {
            leave(2, "the server sent before the client connected to the map");
        }
        bytes += length;
        size -= length;
    }
}

/* The runner's answer to a frame: what the server sends the client now, then what the player
   does. */
static void step(void)
{
    unsigned char head[4];
    const unsigned char *p, *end;
    unsigned long size, bytes, commands;

    if (!play_link_receive(head, 4))
        leave(2, "the runner is gone");
    size = get32(head);
    if (size > P.in_capacity) {
        unsigned char *grown = realloc(P.in, size);

        if (!grown)
            leave(2, "no memory for the runner's answer");
        P.in = grown;
        P.in_capacity = size;
    }
    if (size < 12 || !play_link_receive(P.in, size) || P.in[0] != MSG_STEP)
        leave(2, "the runner's answer is not a step");
    p = P.in + 4;
    end = P.in + size;
    bytes = get32(p);
    p += 4;
    if (bytes > (unsigned long)(end - p) - 4)
        leave(2, "the runner's answer is cut off");
    deliver(p, bytes);
    p += bytes;
    commands = get32(p);
    p += 4;
    while (commands--) {
        unsigned length;

        if (end - p < 4 || (length = get16(p + 2)) > (unsigned)(end - p - 4))
            leave(2, "the runner's answer is cut off");
        act(p[0], p + 4, length);
        p += 4 + length;
    }
}

static void start(void)
{
    const char *port = getenv("RAGEXE_PLAY");

    P.started = 1;
    if (!port || !play_link_open(atoi(port)))
        leave(2, "set RAGEXE_PLAY to the port of a playground runner");
    line_open();
    scene_net_set_handler(on_client_packet);
}

/* One frame of virtual time per shown frame, as in the scene harness. */
void drive_state_frame_shown(void)
{
    scene_clock_advance(SCENE_FRAME_MS);
}

void drive_state_tick(void)
{
    size_t size;

    P.frame++;
    if (!P.started)
        start();
    /* silent, whatever the saved options say: nobody sits in front of a run */
    g_sound_settings.effects_on = 0;
    g_sound_settings.bgm_on = 0;
    sound_set_bgm_volume(0);
    sound_set_effect_volume(0);
    /* what the line still has for the runner: a request whose datagram the wire lost */
    line_run();
    size = report();
    if (!play_link_send(P.out, size))
        leave(2, "the runner is gone");
    step();
}
