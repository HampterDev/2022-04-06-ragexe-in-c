#include "game/packet_handlers.h"
#include "game/gamemode.h"
#include "game/context.h"
#include "game/cells.h"
#include "game/net_clock.h"
#include "actor/actor_ops.h"
#include "actor/game_actor.h"
#include "actor/render_object.h"
#include "session/state.h"
#include "ui/window_messages.h"
#include "ui/window_manager_state.h"
#include "world/actor_lookup.h"
#include "world/object_lookup.h"
#include "world/pathfinder_state.h"
#include "world/mode_dispatch.h"
#include "world/state.h"
#include "attack_records_session.h"
#include <assert.h>
#include <mmsystem.h>
#include <string.h>

struct Session g_session1;
struct UIWindowMgr g_windowMgr;

#define OWN_AID     1
#define MONSTER_AID 100
#define UNIT_AID    200
#define MONSTER_JOB 1002

static struct GameMode mode;
static struct World world;
static struct GameActor player, monster, unit;
static struct GameActorOps table;

enum { SENT_ATTACK, SENT_SKILL_USE, SENT_HIT, SENT_TALK, SENT_STATE, SENT_SPEED, SENT_CHAT, SENT_MODE,
       SENT_SWING_BEGIN, SENT_SWING_CANCEL, SENT_HP_BAR, SENT_CAST, SENT_CAST_STOP };

struct Sent {
    int what;
    void *to;
    long long par2;
    int value[5];
    char text[32];
    struct ActorAttack attack;
    struct ActorSkillUse use;
    struct ActorHit hit;
};
static struct Sent sent[64];
static int sent_count;

static struct Sent *record(int what, void *to)
{
    struct Sent *s;

    assert(sent_count < (int)(sizeof sent / sizeof sent[0]));
    s = &sent[sent_count++];
    memset(s, 0, sizeof *s);
    s->what = what;
    s->to = to;
    return s;
}

static void send_msg(struct GameObject *object, struct GameObject *sender, RenderMsgArg msg,
                     RenderMsgArg par1, RenderMsgArg par2, RenderMsgArg par3, RenderMsgArg par4)
{
    struct Sent *s;

    (void)sender; (void)par4;
    switch (msg) {
    case 0xb:
        record(SENT_ATTACK, object)->attack = *(const struct ActorAttack *)MSG_PTR(par1);
        game_actor_msg_attack((struct GameActor *)object, MSG_PTR(par1));
        break;
    case 0x25:
        record(SENT_SKILL_USE, object)->use = *(const struct ActorSkillUse *)MSG_PTR(par1);
        break;
    case 0x1b:
        s = record(SENT_HIT, object);
        s->hit = *(const struct ActorHit *)MSG_PTR(par1);
        s->par2 = par2;
        break;
    case 7:
        strncpy(record(SENT_TALK, object)->text, MSG_PTR(par1), sizeof s->text - 1);
        break;
    case ACTOR_MSG_SWING_BEGIN:
        s = record(SENT_SWING_BEGIN, object);
        s->value[0] = (int)par1;
        s->value[1] = (int)par2;
        s->value[2] = (int)par3;
        game_actor_swing_begin((struct GameActor *)object, (int)par1, (int)par2, (int)par3);
        break;
    case ACTOR_MSG_SWING_CANCEL:
        record(SENT_SWING_CANCEL, object);
        game_actor_swing_cancel((struct GameActor *)object);
        break;
    case 0x91:
        s = record(SENT_HP_BAR, object);
        s->value[0] = (int)par1;
        s->value[1] = (int)par2;
        break;
    default:
        assert(!"an unexpected actor message");
    }
}

/* The frame delay the test's attacks have on their own, and the frame their hit is on: the hit
   shows 144 ms after an attack begins. */
#define OWN_FRAME_DELAY 3.0f
#define HIT_FRAME       2.0f

static void set_state(struct GameActor *actor, int state, unsigned char snap)
{
    struct Sent *s = record(SENT_STATE, actor);

    s->value[0] = state;
    s->value[1] = snap;
    /* as the setup of an attack does: the attack's own delay, the frame its hit is on, and the
       pace of an announced swing */
    if (state == 2) {
        actor->motion_delay = OWN_FRAME_DELAY;
        actor->attack_motion_count = HIT_FRAME;
        game_actor_pace_attack(actor);
    }
}

/* Whether the hit frame of the attack `actor` shows `ms` after the attack began. */
static int hit_shows(const struct GameActor *actor, int ms)
{
    float off = actor->attack_motion_count * actor->motion_delay * 24.0f - (float)ms;

    return off < 0.01f && off > -0.01f;
}

static void set_attack_speed(struct GameActor *actor, int ms)
{
    record(SENT_SPEED, actor)->value[0] = ms;
}

void game_mode_zc_useskill_ack(struct GameMode *m, unsigned src_aid, unsigned target_aid, int x, int y, int skill_id,
                               int property, unsigned delay_ms, unsigned char is_disposable, int attack_motion)
{
    struct Sent *s = record(SENT_CAST, m);

    s->value[0] = (int)src_aid; s->value[1] = (int)target_aid; s->value[2] = skill_id; s->value[3] = (int)delay_ms;
    s->value[4] = property;
    s->par2 = ((long long)x << 48) | ((long long)y << 32) | ((long long)is_disposable << 16) | attack_motion;
}

void game_actor_stop_cast(struct GameActor *actor, int own)
{
    record(SENT_CAST_STOP, actor)->value[0] = own;
}

/* The swing the own body has in its windup, as the world of the test has it: what was said
   last (says), which the frame's play of the tracks asks for again. */
static struct { int has; unsigned target, motion_ms; DWORD at_tick, lands; } world_swing;

void game_mode_own_body_gone(unsigned id, DWORD tick)
{
    (void)id;
    (void)tick;
}

void game_mode_own_body_heard(DWORD tick)
{
    (void)tick;
}

int game_mode_own_body_swing(unsigned *target, DWORD *at_tick, DWORD *lands, unsigned *motion_ms)
{
    *target = world_swing.target;
    *at_tick = world_swing.at_tick;
    *lands = world_swing.lands;
    *motion_ms = world_swing.motion_ms;
    return world_swing.has;
}

/* The render clock, which the tracks' items of other actors are played on. */
static DWORD render_clock;

DWORD session_get_server_time(struct Session *self) { assert(self == &g_session1); return render_clock; }
struct GameMode *game_current_mode(void) { return &mode; }
int game_actor_get_job(struct GameActor *actor) { return actor->job; }
/* The monster has left the player's sight. */
static int monster_gone;

/* More monsters, in sight while crowd_here is set. */
#define CROWD_AID 300
static struct GameActor crowd[TIMED_AIMED_ACTORS];
static int crowd_here;

struct GameActor *world_get_game_actor_by_aid(struct World *w, unsigned aid)
{
    assert(w == &world);
    if (crowd_here && aid >= CROWD_AID && aid < CROWD_AID + TIMED_AIMED_ACTORS)
        return &crowd[aid - CROWD_AID];
    return aid == MONSTER_AID && !monster_gone ? &monster : NULL;
}
struct Skill *world_get_skill_by_aid(struct World *w, int aid)
{
    assert(w == &world);
    return aid == UNIT_AID ? (struct Skill *)&unit : NULL;
}
intptr_t ui_window_mgr_send_msg_(struct UIWindowMgr *self, int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4)
{
    struct Sent *s = record(SENT_CHAT, self);

    assert(msg == UIWM_MSG_CHAT_LINE && val3 == 0 && val4 == 0);
    strncpy(s->text, (const char *)(intptr_t)val1, sizeof s->text - 1);
    s->value[0] = val2;
    return 0;
}
intptr_t game_mode_send_msg(struct Mode *m, int message, intptr_t a, intptr_t b, intptr_t c, intptr_t d)
{
    struct Sent *s = record(SENT_MODE, m);

    s->value[0] = message; s->value[1] = a; s->value[2] = b; s->value[3] = c; s->value[4] = d;
    return 0;
}
struct RagEffect *game_actor_launch_effect(struct GameActor *self, int effect_id, Vec3 offset, float height)
{ (void)self; (void)effect_id; (void)offset; (void)height; assert(!"nothing casts"); return NULL; }
void rag_effect_send_msg(struct RagEffect *self, struct RenderObject *sender, RenderMsgArg msg,
    RenderMsgArg par1, RenderMsgArg par2, RenderMsgArg par3)
{ (void)self; (void)sender; (void)msg; (void)par1; (void)par2; (void)par3; assert(!"nothing casts"); }
void render_object_turn_toward(struct RenderObject *self, float x, float z) { (void)self; (void)x; (void)z; }
/* a cell is a unit of the world here */
void game_mode_world_to_cell(struct GameMode *self, float world_x, float world_z, int *x, int *y)
{ (void)self; *x = (int)world_x; *y = (int)world_z; }

static void actor_init(struct GameActor *actor, int aid, int job)
{
    memset(actor, 0, sizeof *actor);
    ((struct RenderObject *)actor)->ops = &table.base.base;
    actor->aid = aid;
    actor->job = job;
    actor->attack_motion_count = HIT_FRAME;
    actor->motion_delay = OWN_FRAME_DELAY;
}

static void setup(void)
{
    table.base.base.send_msg = send_msg;
    table.set_state = set_state;
    table.set_attack_speed = set_attack_speed;
    mode.world = &world;
    world.player = (struct Player *)&player;
    actor_init(&player, OWN_AID, 0);
    actor_init(&monster, MONSTER_AID, MONSTER_JOB);
    actor_init(&unit, UNIT_AID, 0x7e);
    g_session1.ownAid = OWN_AID;
    g_session1.aid = 0;
    sent_count = 0;
    mode.open_swing_count = 0;
    timed_queue_clear(&mode.tracks);
    mode.aimed_waiting_count = 0;
    mode.record_wait.count = 0;
    /* every unit came into view before its records, as the server stamps its entry */
    view_floors_clear(&mode.view_floors);
    view_floors_pin(&mode.view_floors, OWN_AID);
    {
        struct ViewStamp stamp;
        int i;

        memset(&stamp, 0, sizeof stamp);
        stamp.kind = VIEW_COMES;
        for (i = -2; i < TIMED_AIMED_ACTORS; i++) {
            stamp.gid = i == -2 ? MONSTER_AID : i == -1 ? UNIT_AID : CROWD_AID + (uint32_t)i;
            view_floors_stamp(&mode.view_floors, &stamp);
        }
    }
    mode.view_stamp_waits = 0;
    mode.hold_back_ms = 0;
    clock_sync_reset(&mode.clock_sync);
    way_up_reset(&mode.way_up);
    own_swing_reset(&mode.own_swing);
    memset(&world_swing, 0, sizeof world_swing);
    g_session1.attackRange = 1;
    g_session1.walkSpeed = 150;
    attack_test_monster_actor = 1;
}

static void put16(unsigned char *p, int at, int v) { p[at] = (unsigned char)v; p[at + 1] = (unsigned char)(v >> 8); }
static void put32(unsigned char *p, int at, int v) { put16(p, at, v); put16(p, at + 2, v >> 16); }

static __declspec(noinline) void dirty_stack(void)
{
    volatile unsigned char junk[4096];
    int i;

    for (i = 0; i < (int)sizeof junk; i++)
        junk[i] = 0xa5;
}

static void expect_hit(const struct Sent *s, void *to, int kind, int damage, long long total, int attacker,
                       int cell_x, int cell_y, float motion, int sp, DWORD t0, DWORD t1, DWORD delay)
{
    assert(s->what == SENT_HIT && s->to == to);
    assert(s->hit.target_aid == ((struct GameActor *)to)->aid && s->hit.kind == kind);
    assert(s->hit.damage == damage && s->par2 == total && s->hit.attacker_aid == attacker);
    assert(s->hit.cell_x == cell_x && s->hit.cell_y == cell_y);
    assert(s->hit.motion == motion && s->hit.is_sp_damage == sp && s->hit.skill_id == 0);
    assert(s->hit.tick - delay - t0 <= t1 - t0);
}

static void act_position(void)
{
    unsigned char packet[0x17];
    DWORD t0, t1;

    setup();
    memset(packet, 0, sizeof packet);
    put16(packet, 0, 0x8b);
    put32(packet, 2, MONSTER_AID);
    put32(packet, 6, OWN_AID);
    put32(packet, 0xa, 777);
    put16(packet, 0xe, 50);
    put16(packet, 0x10, 60);
    put16(packet, 0x12, 25);
    put16(packet, 0x14, 1);
    packet[0x16] = 0;
    t0 = timeGetTime();
    dirty_stack();
    game_mode_zc_notify_act_position(&mode, (const struct PACKET_ZC_NOTIFY_ACT_POSITION *)packet);
    t1 = timeGetTime();

    assert(sent_count == 4);
    assert(sent[0].what == SENT_ATTACK && sent[0].to == &monster);
    assert(sent[0].attack.damage == 25 && sent[0].attack.left_damage == 0 && sent[0].attack.action == 0);
    assert(sent[0].attack.target_aid == OWN_AID && sent[0].attack.start_time == 777 && sent[0].attack.count == 1);
    assert(sent[0].attack.cell_x == 50 && sent[0].attack.cell_y == 60);
    assert(sent[0].attack.attack_mt == 0x1b0 && sent[0].attack.attacked_mt == 0x120);
    assert(sent[0].attack.is_sp_damage == 0);
    assert(sent[1].what == SENT_SPEED && sent[1].to == &monster && sent[1].value[0] == 0x1b0);
    assert(sent[2].what == SENT_STATE && sent[2].to == &monster && sent[2].value[0] == 2 && sent[2].value[1] == 1);
    expect_hit(&sent[3], &player, 0x1a, 25, 0, MONSTER_AID, 50, 60, 0x120 / 288.0f, 0, t0, t1, 144);
    assert(sent[3].hit.effect == -1 && sent[3].hit.hit_effect == 0 && sent[3].hit.value == 1);

    setup();
    put16(packet, 0x12, -30000);
    game_mode_zc_notify_act_position(&mode, (const struct PACKET_ZC_NOTIFY_ACT_POSITION *)packet);
    put16(packet, 0x12, 25);
    put32(packet, 2, UNIT_AID);
    game_mode_zc_notify_act_position(&mode, (const struct PACKET_ZC_NOTIFY_ACT_POSITION *)packet);
    assert(sent_count == 0);
}

static void act(void)
{
    DWORD t0, t1;
    int i;

    setup();
    t0 = timeGetTime();
    game_mode_notify_act(&mode, MONSTER_AID, OWN_AID, 5, 0, 0, 12, 0x10001, 0x10a, 0, 7);
    t1 = timeGetTime();
    assert(sent_count == 4 && sent[0].what == SENT_ATTACK && sent[0].to == &monster);
    assert(sent[0].attack.action == 0xa && sent[0].attack.count == 1 && sent[0].attack.is_sp_damage == 1);
    assert(sent[0].attack.cell_x == -1 && sent[0].attack.cell_y == -1 && sent[0].attack.start_time == 5);
    assert(sent[0].attack.attack_mt == 1440 && sent[0].attack.attacked_mt == 1440);
    expect_hit(&sent[3], &player, 0x5d, 12, 0, MONSTER_AID, -1, -1, 1440 / 288.0f, 1, t0, t1, 144);
    assert(sent[3].hit.effect == 1);

    setup();
    attack_test_is_attack = 0;
    game_mode_notify_act(&mode, MONSTER_AID, OWN_AID, 0, 300, 0, 12, 1, 0, 0, 0);
    assert(sent[0].attack.attack_mt == 300 && sent[0].attack.attacked_mt == 0 && sent[0].attack.is_sp_damage == 0);
    attack_test_is_attack = -1;

    setup();
    t0 = timeGetTime();
    game_mode_notify_act(&mode, MONSTER_AID, OWN_AID, 0, 400, 500, 30, 3, 8, 0, 0);
    t1 = timeGetTime();
    assert(sent_count == 6);
    for (i = 0; i < 3; i++)
        expect_hit(&sent[3 + i], &player, 0x1a, 10, 10 * (i + 1), MONSTER_AID, -1, -1, 500 / 288.0f, 0,
                   t0, t1, 144 + 200 * i);

    setup();
    game_mode_notify_act(&mode, MONSTER_AID, OWN_AID, 0, 400, 500, 30, 1, 0, 0, 0);
    assert(sent_count == 4 && sent[0].what == SENT_ATTACK && sent[0].to == &monster);
    assert(sent[1].what == SENT_SPEED && sent[2].what == SENT_STATE && sent[3].what == SENT_HIT);
    setup();
    monster.attack_target_aid = OWN_AID;
    game_mode_notify_act(&mode, MONSTER_AID, OWN_AID, 0, 400, 500, 30, 1, 0, 0, 0);
    assert(sent[0].what == SENT_ATTACK);
    setup();
    g_session1.hideChat = 1;
    game_mode_notify_act(&mode, MONSTER_AID, OWN_AID, 0, 400, 500, 30, 1, 0, 0, 0);
    assert(sent_count == 4 && sent[0].what == SENT_ATTACK);
    g_session1.hideChat = 0;

    setup();
    game_mode_notify_act(&mode, UNIT_AID, OWN_AID, 0, 400, 500, 30, 1, 1, 0, 0);
    assert(sent_count == 2 && sent[0].what == SENT_ATTACK && sent[0].to == &unit);
    assert(sent[1].what == SENT_STATE && sent[1].value[0] == 5);
    setup();
    game_mode_notify_act(&mode, MONSTER_AID, OWN_AID, 0, 400, 500, -30000, 1, 0, 0, 0);
    game_mode_notify_act(&mode, 999, OWN_AID, 0, 400, 500, 30, 1, 0, 0, 0);
    assert(sent_count == 0);
}

static void expect_use(const struct Sent *s, void *to, int skill, int damage, int action, int start, int level,
                       int count, int cell_x, int cell_y)
{
    assert(s->what == SENT_SKILL_USE && s->to == to);
    assert(s->use.skill_id == skill && s->use.damage == damage && s->use.action == action);
    assert(s->use.target_aid == OWN_AID && s->use.start_time == start && s->use.level == level);
    assert(s->use.count == count && s->use.cell_x == cell_x && s->use.cell_y == cell_y);
    assert(s->use.attack_mt == 600 && s->use.attacked_mt == 700);
}

static void skill_use(void)
{
    unsigned char packet[0x24];

    setup();
    memset(packet, 0, sizeof packet);
    put16(packet, 0, 0x114);
    put16(packet, 2, 0x13);
    put32(packet, 4, MONSTER_AID);
    put32(packet, 8, OWN_AID);
    put32(packet, 0xc, 9);
    put32(packet, 0x10, 600);
    put32(packet, 0x14, 700);
    put16(packet, 0x18, -40);
    put16(packet, 0x1a, 5);
    put16(packet, 0x1c, 2);
    packet[0x1e] = 6;
    game_mode_on_notify_skill(&mode, (const struct PacketNotifySkill *)packet);
    assert(sent_count == 1);
    expect_use(&sent[0], &monster, 0x13, -40, 6, 9, 5, 2, -1, -1);

    setup();
    attack_test_monster_actor = 0;
    memset(packet, 0, sizeof packet);
    put16(packet, 0, 0x115);
    put16(packet, 2, 0x13);
    put32(packet, 4, MONSTER_AID);
    put32(packet, 8, OWN_AID);
    put32(packet, 0xc, 9);
    put32(packet, 0x10, 600);
    put32(packet, 0x14, 700);
    put16(packet, 0x18, 70);
    put16(packet, 0x1a, 80);
    put16(packet, 0x1c, 40);
    put16(packet, 0x1e, 5);
    put16(packet, 0x20, 2);
    packet[0x22] = 8;
    game_mode_on_notify_skill_position(&mode, (const struct PacketNotifySkillPosition *)packet);
    assert(sent_count == 2);
    expect_use(&sent[0], &monster, 0x13, 40, 8, 9, 5, 2, 70, 80);
    assert(sent[1].what == SENT_MODE && sent[1].value[0] == 0x56 && sent[1].value[1] == 0x13);
    assert(sent[1].value[2] == (int)(intptr_t)&monster && sent[1].value[3] == 5);

    setup();
    memset(packet, 0, sizeof packet);
    put16(packet, 0, 0x1de);
    put16(packet, 2, 0x13);
    put32(packet, 4, UNIT_AID);
    put32(packet, 8, OWN_AID);
    put32(packet, 0xc, 9);
    put32(packet, 0x10, 600);
    put32(packet, 0x14, 700);
    put32(packet, 0x18, 100000);
    put16(packet, 0x1c, 5);
    put16(packet, 0x1e, 2);
    packet[0x20] = 8;
    game_mode_on_notify_skill2(&mode, (const struct PacketNotifySkill2 *)packet);
    assert(sent_count >= 1);
    expect_use(&sent[0], &unit, 0x13, 100000, 8, 9, 5, 2, -1, -1);
}

/* ZC_OWN_INTENT: the server's word about the request numbered `seq`, with the cell the
   character is on. */
static void send_own_intent(unsigned seq, int state, int x, int y)
{
    unsigned char packet[14];

    memset(packet, 0, sizeof packet);
    put16(packet, 0, 0x0f24);
    put16(packet, 2, (int)seq);
    packet[4] = (unsigned char)state;
    put16(packet, 10, x);
    put16(packet, 12, y);
    game_mode_zc_own_intent(&mode, packet);
}

/* The server counts a revision for each record it makes: these are made one after the other. */
static unsigned short g_rev;

static void send_act(unsigned gid, unsigned tick, int kind, unsigned target, int motion)
{
    unsigned char packet[37];

    memset(packet, 0, sizeof packet);
    put16(packet, 35, ++g_rev);
    put16(packet, 0, 0x0f21);
    put32(packet, 2, (int)gid);
    put32(packet, 6, (int)tick);
    packet[10] = (unsigned char)kind;
    put32(packet, 11, (int)(tick + (unsigned)motion));
    put32(packet, 15, (int)(tick + (unsigned)motion));
    put32(packet, 19, (int)target);
    put16(packet, 31, motion);
    game_mode_zc_track_act(&mode, packet);
}

static void send_hit(unsigned source, unsigned target, unsigned tick, int damage, int flinch)
{
    unsigned char packet[32];

    memset(packet, 0, sizeof packet);
    put16(packet, 0, 0x0f23);
    put32(packet, 2, (int)source);
    put32(packet, 6, (int)target);
    put32(packet, 10, (int)tick);
    put32(packet, 14, damage);
    put16(packet, 22, 1);
    put16(packet, 26, flinch);
    game_mode_zc_notice_hit(&mode, packet);
}

static void send_vitals(unsigned gid, unsigned tick, int hp, int max_hp)
{
    unsigned char packet[20];

    put16(packet, 18, ++g_rev);
    put16(packet, 0, 0x0f22);
    put32(packet, 2, (int)gid);
    put32(packet, 6, (int)tick);
    put32(packet, 10, hp);
    put32(packet, 14, max_hp);
    game_mode_zc_track_vitals(&mode, packet);
}

static void play_at(DWORD clock)
{
    render_clock = clock;
    game_mode_play_tracks(&mode);
}

/* The server's action timing: a swing begins by its record, then lands or is cancelled. */
static void swing(void)
{
    DWORD t0, t1;

    /* The record of a swing starts the animation at its tick. */
    setup();
    send_act(OWN_AID, 700, 1, MONSTER_AID, 300);
    play_at(700);
    assert(sent_count == 3);
    assert(sent[0].what == SENT_SWING_BEGIN && sent[0].to == &player);
    assert(sent[0].value[0] == MONSTER_AID && sent[0].value[1] == 300 && sent[0].value[2] == 300);
    assert(sent[1].what == SENT_SPEED && sent[1].to == &player && sent[1].value[0] == 300);
    assert(hit_shows(&player, 300));
    assert(sent[2].what == SENT_STATE && sent[2].to == &player && sent[2].value[0] == 2 && sent[2].value[1] == 1);
    assert(mode.open_swing_count == 1 && player.attack_target_aid == MONSTER_AID);

    /* Its impact shows the hit now and leaves the animation alone. */
    t0 = timeGetTime();
    game_mode_notify_act(&mode, OWN_AID, MONSTER_AID, 0, 300, 500, 30, 1, 0, 0, 0);
    t1 = timeGetTime();
    assert(sent_count == 5 && sent[3].what == SENT_ATTACK && sent[3].to == &player);
    assert(sent[3].attack.impact == 1);
    expect_hit(&sent[4], &monster, 0x1a, 30, 0, OWN_AID, -1, -1, 500 / 288.0f, 0, t0, t1, 0);
    assert(mode.open_swing_count == 0);

    /* With no swing open the same hit is a whole attack, as ZC_NOTIFY_ACT always was. */
    t0 = timeGetTime();
    game_mode_notify_act(&mode, OWN_AID, MONSTER_AID, 0, 300, 500, 30, 1, 0, 0, 0);
    t1 = timeGetTime();
    assert(sent_count == 9 && sent[5].what == SENT_ATTACK && sent[5].attack.impact == 0);
    assert(sent[6].what == SENT_SPEED && sent[7].what == SENT_STATE);
    expect_hit(&sent[8], &monster, 0x1a, 30, 0, OWN_AID, -1, -1, 500 / 288.0f, 0, t0, t1, 144);

    /* A zero attack motion gets the default, as in ZC_NOTIFY_ACT. */
    setup();
    send_act(OWN_AID, 700, 1, MONSTER_AID, 0);
    play_at(700);
    assert(sent_count == 3 && sent[0].value[1] == 1440);

    /* A hit on another target is not this swing's impact, and closes it. */
    setup();
    send_act(OWN_AID, 700, 1, MONSTER_AID, 300);
    play_at(700);
    game_mode_notify_act(&mode, OWN_AID, UNIT_AID, 0, 300, 500, 30, 1, 0, 0, 0);
    assert(sent_count == 7 && sent[3].what == SENT_ATTACK && sent[4].what == SENT_SPEED);
    assert(sent[5].what == SENT_STATE && sent[6].what == SENT_HIT && mode.open_swing_count == 0);

    /* An idle record ends the attack animation of an open swing, and only of an open swing. */
    setup();
    send_act(OWN_AID, 700, 1, MONSTER_AID, 300);
    play_at(700);
    player.state = 2;
    send_act(OWN_AID, 710, 0, 0, 0);
    play_at(710);
    assert(sent_count == 5 && sent[3].what == SENT_SWING_CANCEL && sent[3].to == &player);
    assert(sent[4].what == SENT_STATE && sent[4].value[0] == 0 && sent[4].value[1] == 1);
    assert(mode.open_swing_count == 0);
    send_act(OWN_AID, 720, 0, 0, 0);
    play_at(720);
    assert(sent_count == 5);

    /* A swing cancelled while the player already walks leaves the walk alone. */
    setup();
    send_act(OWN_AID, 700, 1, MONSTER_AID, 300);
    play_at(700);
    player.state = 1;
    send_act(OWN_AID, 710, 0, 0, 0);
    play_at(710);
    assert(sent_count == 4 && sent[3].what == SENT_SWING_CANCEL && mode.open_swing_count == 0);

    /* An attacker that is not here changes nothing: its records wait for it. */
    setup();
    send_act(999, 700, 1, MONSTER_AID, 300);
    play_at(700);
    send_act(999, 710, 0, 0, 0);
    play_at(710);
    assert(sent_count == 0 && mode.open_swing_count == 0);

    /* A second swing of the same attacker replaces the first. */
    send_act(OWN_AID, 720, 1, MONSTER_AID, 300);
    play_at(720);
    send_act(OWN_AID, 730, 1, UNIT_AID, 300);
    play_at(730);
    assert(mode.open_swing_count == 1 && mode.open_swings[0].target == UNIT_AID);
}

/* A swing, a hit and a monster's HP come with the tick they belong to (plans/core-loop.md) and
   wait for the clock to reach it. Played, they are a swing's begin, the impact of
   ZC_NOTIFY_ACT and what ZC_HP_INFO is. */
static void tracks(void)
{
    DWORD t0, t1, own;
    int i;
    unsigned char unknown[37], cast[37];

    /* A monster's swing and its hit, sent together when the swing starts: each waits for its
       tick, and the hit is the swing's impact. */
    setup();
    send_act(MONSTER_AID, 1000, 1, OWN_AID, 600);
    send_hit(MONSTER_AID, OWN_AID, 1600, 7, 300);
    assert(mode.tracks.count == 2);
    play_at(999);
    assert(sent_count == 0);
    play_at(1000);
    assert(sent_count == 3 && sent[0].what == SENT_SWING_BEGIN && sent[0].to == &monster);
    assert(sent[0].value[0] == OWN_AID && sent[0].value[1] == 600 && sent[0].value[2] == 600);
    assert(hit_shows(&monster, 600) && mode.open_swing_count == 1);
    play_at(1599);
    assert(sent_count == 3);
    t0 = timeGetTime();
    play_at(1600);
    t1 = timeGetTime();
    assert(sent_count == 5 && sent[3].what == SENT_ATTACK && sent[3].to == &monster && sent[3].attack.impact == 1);
    expect_hit(&sent[4], &player, 0x1a, 7, 0, MONSTER_AID, -1, -1, 300 / 288.0f, 0, t0, t1, 0);
    assert(mode.tracks.count == 0 && mode.open_swing_count == 0);

    /* The HP a hit took moves with its number; a later HP record waits for its own tick. */
    setup();
    send_act(OWN_AID, 2000, 1, MONSTER_AID, 432);
    play_at(2000);
    assert(sent_count == 3 && sent[0].to == &player);
    send_hit(OWN_AID, MONSTER_AID, 2216, 15, 288);
    send_vitals(MONSTER_AID, 2216, 40, 55);
    send_vitals(MONSTER_AID, 2300, 30, 55);
    play_at(2216);
    assert(sent_count == 6 && sent[3].what == SENT_HP_BAR && sent[3].to == &monster);
    assert(sent[3].value[0] == 40 && sent[3].value[1] == 55);
    assert(sent[4].what == SENT_ATTACK && sent[4].attack.impact == 1 && sent[5].what == SENT_HIT && sent[5].to == &monster);
    play_at(2299);
    assert(sent_count == 6);
    play_at(2300);
    assert(sent_count == 7 && sent[6].what == SENT_HP_BAR && sent[6].value[0] == 30);
    /* HP of an actor that is not here is not shown: it waits for the actor */
    send_vitals(999, 2300, 1, 2);
    play_at(2300);
    assert(sent_count == 7 && mode.tracks.count == 0 && mode.record_wait.count == 1);

    /* A swing cancelled before its tick is never shown. One under way ends. */
    setup();
    send_act(OWN_AID, 3000, 1, MONSTER_AID, 432);
    send_act(OWN_AID, 2900, 0, 0, 0);
    assert(mode.tracks.count == 1);
    play_at(3500);
    assert(sent_count == 0 && mode.open_swing_count == 0 && mode.tracks.count == 0);
    send_act(OWN_AID, 4000, 1, MONSTER_AID, 432);
    play_at(4000);
    player.state = 2;
    send_act(OWN_AID, 4100, 0, 0, 0);
    play_at(4099);
    assert(sent_count == 3);
    play_at(4100);
    assert(sent_count == 5 && sent[3].what == SENT_SWING_CANCEL && sent[3].to == &player);
    assert(mode.open_swing_count == 0);

    /* A kind this client does not know is not kept. */
    setup();
    memset(unknown, 0, sizeof unknown);
    put16(unknown, 0, 0x0f21);
    put32(unknown, 2, OWN_AID);
    unknown[10] = 3;
    game_mode_zc_track_act(&mode, unknown);
    assert(mode.tracks.count == 0);

    /* A cast is played as ZC_USESKILL_ACK with the time it still has: all of it at its tick,
       what is left when the record is played late, none once it is over. */
    setup();
    memset(cast, 0, sizeof cast);
    put16(cast, 0, 0x0f21);
    put32(cast, 2, MONSTER_AID);
    put32(cast, 6, 5000);
    cast[10] = 2;
    put32(cast, 11, 6500);
    put32(cast, 19, OWN_AID);
    put16(cast, 23, 41);
    put16(cast, 25, 42);
    put16(cast, 27, 28);
    put16(cast, 29, 5);
    put16(cast, 31, 432);
    cast[33] = 0xff;
    cast[34] = 1;
    put16(cast, 35, ++g_rev);
    game_mode_zc_track_act(&mode, cast);
    play_at(4999);
    assert(sent_count == 0);
    play_at(5000);
    assert(sent_count == 1 && sent[0].what == SENT_CAST && sent[0].to == &mode);
    assert(sent[0].value[0] == MONSTER_AID && sent[0].value[1] == OWN_AID && sent[0].value[2] == 28);
    assert(sent[0].value[3] == 1500 && sent[0].value[4] == -1);
    assert(sent[0].par2 == (((long long)41 << 48) | ((long long)42 << 32) | ((long long)1 << 16) | 432));
    /* the server says it again, and again after the cast is over */
    put16(cast, 35, ++g_rev);
    game_mode_zc_track_act(&mode, cast);
    play_at(6100);
    assert(sent_count == 2 && sent[1].value[3] == 400);
    put16(cast, 35, ++g_rev);
    game_mode_zc_track_act(&mode, cast);
    play_at(7000);
    assert(sent_count == 3 && sent[2].value[3] == 0);

    /* Idle ends the cast an actor shows, as ZC_DISPEL does, and leaves one that shows none alone;
       the player's own cast is stopped as its own. */
    setup();
    monster.casting = 1;
    send_act(MONSTER_AID, 8000, 0, 0, 0);
    play_at(8000);
    assert(sent_count == 1 && sent[0].what == SENT_CAST_STOP && sent[0].to == &monster && sent[0].value[0] == 0);
    monster.casting = 0;
    send_act(MONSTER_AID, 8100, 0, 0, 0);
    play_at(8100);
    assert(sent_count == 1);
    player.state = 8;
    send_act(OWN_AID, 8200, 0, 0, 0);
    play_at(8200);
    assert(sent_count == 2 && sent[1].what == SENT_CAST_STOP && sent[1].to == &player && sent[1].value[0] == 1);

    /* What the player's own character does is played on the own clock, which runs ahead of
       the render clock: its swing shows while a monster's of the same tick, at someone else,
       still waits, and is then played as it is. */
    setup();
    t0 = timeGetTime();
    assert(clock_sync_sample(&mode.clock_sync, t0, t0 + 0x10000000, t0 + 0x10000000, t0));
    own = t0 + 0x10000000;
    send_act(OWN_AID, own - 5000, 1, MONSTER_AID, 432);
    send_act(MONSTER_AID, own - 5000, 1, UNIT_AID, 600);
    play_at(own - 100000);
    assert(sent_count == 3 && sent[0].what == SENT_SWING_BEGIN && sent[0].to == &player && mode.tracks.count == 1);
    play_at(own - 5000);
    assert(sent_count == 6 && sent[3].what == SENT_SWING_BEGIN && sent[3].to == &monster && sent[3].value[1] == 600);

    /* An act record sent later than it was made, as the one with an entry, is played at once
       and says nothing about the render delay; one sent when it was made is measured */
    setup();
    t0 = timeGetTime();
    assert(clock_sync_sample(&mode.clock_sync, t0, t0 + 0x10000000, t0 + 0x10000000, t0));
    own = t0 + 0x10000000;
    i = mode.render_delay.count;
    send_act(MONSTER_AID, own - 50, 0x80 | 1, UNIT_AID, 600);
    assert(mode.render_delay.count == i && mode.tracks.count == 1 && mode.tracks.items[0].u.act.kind == 1);
    assert(mode.tracks.items[0].u.act.at_tick == own - 50 && (int)(mode.tracks.items[0].tick - (own - 50)) < 0);
    send_act(MONSTER_AID, own - 40, 0, 0, 0);
    assert(mode.render_delay.count == i + 1 && mode.tracks.count == 2 && mode.tracks.items[1].tick == own - 40);
    setup();
}

/* The own clock of a test: the server's clock is known from now on, and the own clock is `own`
   and the milliseconds the test has taken since. */
static DWORD know_clock(void)
{
    DWORD t0 = timeGetTime();

    assert(clock_sync_sample(&mode.clock_sync, t0, t0 + 0x10000000, t0 + 0x10000000, t0));
    return t0 + 0x10000000;
}

/* The render clock of these: so far behind that it has reached nothing. */
#define FAR_BEHIND 100000

static void send_cast(unsigned gid, unsigned tick, unsigned end, unsigned target)
{
    unsigned char packet[37];

    memset(packet, 0, sizeof packet);
    put16(packet, 35, ++g_rev);
    put16(packet, 0, 0x0f21);
    put32(packet, 2, (int)gid);
    put32(packet, 6, (int)tick);
    packet[10] = 2;
    put32(packet, 11, (int)end);
    put32(packet, 19, (int)target);
    put16(packet, 27, 28);
    put16(packet, 31, 432);
    game_mode_zc_track_act(&mode, packet);
}

/* What another actor aims at the player's own character is played on the own clock
   (world/act_track.h): its swing begins as soon as the record is here, played faster so that it
   lands when the own clock reaches the tick it lands at; the hit shows as soon as its notice is
   here; and so does the swing's end. The render clock has reached none of them. */
static void threats(void)
{
    DWORD own;
    int i;

    /* A swing at the own character that began 150 ms ago on the own clock and lands 600 ms
       after its start: it begins now, and the frame its hit is on shows in the 450 ms it has
       left (less what the test took). */
    setup();
    own = know_clock();
    send_act(MONSTER_AID, own - 150, 1, OWN_AID, 600);
    play_at(own - FAR_BEHIND);
    assert(sent_count == 3 && sent[0].what == SENT_SWING_BEGIN && sent[0].to == &monster);
    assert(sent[0].value[0] == OWN_AID && sent[0].value[1] == 600 && sent[0].value[2] <= 450 && sent[0].value[2] >= 350);
    assert(sent[1].what == SENT_SPEED && sent[1].value[0] == 600);
    assert(hit_shows(&monster, sent[0].value[2]));
    assert(mode.open_swing_count == 1 && mode.tracks.count == 0);
    /* The server takes it back (the character got away): told at once. */
    monster.state = 2;
    send_act(MONSTER_AID, own - 20, 0, 0, 0);
    play_at(own - FAR_BEHIND);
    assert(sent_count == 5 && sent[3].what == SENT_SWING_CANCEL && sent[3].to == &monster);
    assert(mode.open_swing_count == 0 && mode.tracks.count == 0);
    /* Its next idle ends nothing aimed at the own character: it waits for the render clock. */
    send_act(MONSTER_AID, own - 10, 0, 0, 0);
    play_at(own - FAR_BEHIND);
    assert(sent_count == 5 && mode.tracks.count == 1);

    /* A swing whose record comes after it landed is played twice as fast as it is on its own,
       no faster, and its hit is the swing's impact as soon as the notice is here. */
    setup();
    own = know_clock();
    send_act(MONSTER_AID, own - 700, 1, OWN_AID, 600);
    play_at(own - FAR_BEHIND);
    assert(sent_count == 3 && sent[0].what == SENT_SWING_BEGIN && sent[0].value[1] == 600 && sent[0].value[2] < 0);
    assert(hit_shows(&monster, 72));
    send_hit(MONSTER_AID, OWN_AID, own - 100, 7, 300);
    play_at(own - FAR_BEHIND);
    assert(sent_count == 5 && sent[3].what == SENT_ATTACK && sent[3].to == &monster && sent[3].attack.impact == 1);
    assert(sent[4].what == SENT_HIT && sent[4].to == &player && sent[4].hit.damage == 7);
    assert(mode.open_swing_count == 0 && mode.tracks.count == 0);

    /* A zero attack motion is the default's. */
    setup();
    own = know_clock();
    send_act(MONSTER_AID, own - 5000, 1, OWN_AID, 0);
    play_at(own - FAR_BEHIND);
    assert(sent_count == 3 && sent[0].value[1] == 1440);

    /* What the monster did before, at someone else, is played first: its hit on the other one,
       which the render clock has not reached, then the swing. */
    setup();
    own = know_clock();
    send_hit(MONSTER_AID, UNIT_AID, own - 300, 9, 300);
    play_at(own - FAR_BEHIND);
    assert(sent_count == 0 && mode.tracks.count == 1);
    send_act(MONSTER_AID, own - 150, 1, OWN_AID, 600);
    play_at(own - FAR_BEHIND);
    assert(sent_count >= 5 && sent[0].what == SENT_ATTACK && sent[0].to == &monster && sent[0].attack.target_aid == UNIT_AID);
    for (i = 1; i < sent_count && sent[i].what != SENT_SWING_BEGIN; i++)
        ;
    assert(i < sent_count && sent[i].to == &monster && sent[i].value[0] == OWN_AID && mode.tracks.count == 0);

    /* A cast at the own character shows the time it has left on the own clock; one at someone
       else waits for the render clock. */
    setup();
    own = know_clock();
    send_cast(MONSTER_AID, own - 200, own + 1300, OWN_AID);
    play_at(own - FAR_BEHIND);
    assert(sent_count == 1 && sent[0].what == SENT_CAST && sent[0].value[0] == MONSTER_AID);
    assert(sent[0].value[3] <= 1300 && sent[0].value[3] >= 1200);
    assert((sent[0].par2 & 0xffff) == 432); /* its attack motion is the skill's, not a swing's to fit */
    /* it is broken off: told at once */
    monster.casting = 1;
    send_act(MONSTER_AID, own - 10, 0, 0, 0);
    play_at(own - FAR_BEHIND);
    assert(sent_count == 2 && sent[1].what == SENT_CAST_STOP && sent[1].to == &monster);
    monster.casting = 0;
    send_cast(MONSTER_AID, own - 200, own + 1300, UNIT_AID);
    play_at(own - FAR_BEHIND);
    assert(sent_count == 2 && mode.tracks.count == 1);
    play_at(own - 200);
    assert(sent_count == 3 && sent[2].what == SENT_CAST && sent[2].value[3] == 1500);

    /* A swing at someone else is played on the render clock, paced to the tick it lands at on
       that clock; the own character's swing on the own clock, to when its number is here. */
    setup();
    own = know_clock();
    send_act(MONSTER_AID, own - 5000, 1, UNIT_AID, 600);
    play_at(own - 5001);
    assert(sent_count == 0);
    play_at(own - 4900);
    assert(sent_count == 3 && sent[0].what == SENT_SWING_BEGIN && sent[0].value[0] == UNIT_AID && sent[0].value[1] == 600);
    assert(sent[0].value[2] == 500 && hit_shows(&monster, 500));
    setup();
    own = know_clock();
    send_act(OWN_AID, own - 150, 1, MONSTER_AID, 300);
    play_at(own - FAR_BEHIND);
    assert(sent_count == 3 && sent[0].what == SENT_SWING_BEGIN && sent[0].to == &player && sent[0].value[1] == 300);
    assert(sent[0].value[2] <= 150 && sent[0].value[2] >= 50 && hit_shows(&player, sent[0].value[2]));

    /* Until the server's clock is known the own clock is the render clock: a swing at the own
       character waits for its tick and is played as it is (tracks, above). */
    setup();
}

/* `actor` is drawn on a walk that ends at `ends`. */
static struct PathCell walk_end[1 + TIMED_AIMED_ACTORS];

static void walks(struct GameActor *actor, DWORD ends)
{
    struct PathCell *cell = &walk_end[actor == &monster ? 0 : 1 + (actor - crowd)];

    memset(cell, 0, sizeof *cell);
    cell->time = ends;
    actor->state = 1;
    actor->path.cells = cell;
    actor->path.count = 1;
}

/* The walk of another actor is drawn on the render clock, behind the clock its swing at the
   player's own character is played on: the swing waits while the actor has yet to arrive where
   it swings from, and begins when it is there, in the time it then has left. */
static void threats_wait_for_the_walk(void)
{
    DWORD own;
    int i;

    /* On a walk the server had ended before the swing; then standing, with that walk waiting
       for its first step; then with a move record that waits and will move it. */
    setup();
    own = know_clock();
    walks(&monster, own - 200);
    send_act(MONSTER_AID, own - 150, 1, OWN_AID, 600);
    play_at(own - FAR_BEHIND);
    assert(sent_count == 0 && mode.tracks.count == 0 && mode.aimed_waiting_count == 1 && mode.open_swing_count == 0);
    play_at(own - FAR_BEHIND);
    assert(sent_count == 0);
    monster.state = 0;
    monster.track_waiting = 1;
    play_at(own - FAR_BEHIND);
    assert(sent_count == 0);
    monster.track_waiting = 0;
    monster.move_track.count = 1;
    monster.move_track.held[0].at_tick = own - 400;
    monster.move_track.held[0].steps = 1;
    play_at(own - FAR_BEHIND);
    assert(sent_count == 0 && mode.aimed_waiting_count == 1);
    monster.move_track.held[0].steps = 0;
    monster.move_track.held[0].x = 7;
    play_at(own - FAR_BEHIND);
    assert(sent_count == 0 && mode.aimed_waiting_count == 1);
    /* the word that it stands where it is drawn, which comes when it stops to swing, holds nothing */
    monster.move_track.held[0].x = 0;
    play_at(own - FAR_BEHIND);
    assert(sent_count == 3 && sent[0].what == SENT_SWING_BEGIN && sent[0].to == &monster && sent[0].value[0] == OWN_AID);
    assert(sent[0].value[1] == 600 && sent[0].value[2] <= 450 && sent[0].value[2] >= 350);
    assert(mode.aimed_waiting_count == 0 && mode.open_swing_count == 1);
    play_at(own - FAR_BEHIND);
    assert(sent_count == 3);

    /* The wait is over when the render clock, which the walk is drawn on, reaches the swing's
       tick: the walk before the swing is over on that clock, and the swing begins as it would
       on the render clock, whatever the actor is still drawn doing. */
    for (i = 0; i < 3; i++) {
        setup();
        own = know_clock();
        if (i == 0) {
            walks(&monster, own - 200);
        } else if (i == 1) {
            walks(&monster, own - 200);
            monster.state = 0;
            monster.track_waiting = 1;
        } else {
            monster.move_track.count = 1;
            monster.move_track.held[0].at_tick = own - 200;
            monster.move_track.held[0].steps = 1;
        }
        send_act(MONSTER_AID, own - 150, 1, OWN_AID, 600);
        play_at(own - 151);
        assert(sent_count == 0 && mode.aimed_waiting_count == 1);
        play_at(own - 150);
        assert(sent_count == 3 && sent[0].what == SENT_SWING_BEGIN && sent[0].to == &monster);
        assert(sent[0].value[2] <= 450 && sent[0].value[2] >= 350 && mode.aimed_waiting_count == 0);
    }
    /* and a swing whose record comes after that is not held at all */
    setup();
    own = know_clock();
    walks(&monster, own - 200);
    render_clock = own - 100;
    send_act(MONSTER_AID, own - 150, 1, OWN_AID, 600);
    play_at(own - 100);
    assert(sent_count == 3 && sent[0].what == SENT_SWING_BEGIN && mode.aimed_waiting_count == 0);

    /* What it does after the swing began is not waited for. Standing, with a later walk that
       waits for its first step and a move record of a later tick, it is where it swings from:
       the swing begins. */
    setup();
    own = know_clock();
    walks(&monster, own - 100);
    monster.state = 0;
    monster.track_waiting = 1;
    monster.move_track.count = 1;
    monster.move_track.held[0].at_tick = own - 100;
    monster.move_track.held[0].steps = 1;
    send_act(MONSTER_AID, own - 150, 1, OWN_AID, 600);
    play_at(own - FAR_BEHIND);
    assert(sent_count == 3 && sent[0].what == SENT_SWING_BEGIN && mode.aimed_waiting_count == 0);
    /* Drawn on that later walk already, it is past the swing's beginning: no attack stops the
       walk, and the swing's hit is still its impact. */
    setup();
    own = know_clock();
    walks(&monster, own - 50);
    send_act(MONSTER_AID, own - 700, 1, OWN_AID, 600);
    play_at(own - FAR_BEHIND);
    assert(sent_count == 0 && mode.aimed_waiting_count == 0 && mode.open_swing_count == 1);
    send_hit(MONSTER_AID, OWN_AID, own - 100, 7, 300);
    play_at(own - FAR_BEHIND);
    assert(sent_count == 2 && sent[0].what == SENT_ATTACK && sent[0].attack.impact == 1 && sent[1].what == SENT_HIT);
    assert(mode.open_swing_count == 0);
    /* so is a swing that waited for the walk before it, when the actor walks on at once */
    setup();
    own = know_clock();
    walks(&monster, own - 200);
    send_act(MONSTER_AID, own - 150, 1, OWN_AID, 600);
    play_at(own - FAR_BEHIND);
    assert(sent_count == 0 && mode.aimed_waiting_count == 1 && mode.open_swing_count == 0);
    walks(&monster, own + 500);
    play_at(own - FAR_BEHIND);
    assert(sent_count == 0 && mode.aimed_waiting_count == 0 && mode.open_swing_count == 1);
    /* a cast is not shown on a caster that walks on */
    setup();
    own = know_clock();
    walks(&monster, own - 50);
    send_cast(MONSTER_AID, own - 200, own + 1300, OWN_AID);
    play_at(own - FAR_BEHIND);
    assert(sent_count == 0 && mode.aimed_waiting_count == 0 && mode.open_swing_count == 0);
    /* each alone, of the swing's very tick, is before it */
    for (i = 0; i < 3; i++) {
        setup();
        own = know_clock();
        if (i == 0) {
            walks(&monster, own - 150);
        } else if (i == 1) {
            walks(&monster, own - 150);
            monster.state = 0;
            monster.track_waiting = 1;
        } else {
            monster.move_track.count = 1;
            monster.move_track.held[0].at_tick = own - 150;
            monster.move_track.held[0].steps = 1;
        }
        send_act(MONSTER_AID, own - 150, 1, OWN_AID, 600);
        play_at(own - FAR_BEHIND);
        assert(sent_count == 0 && mode.aimed_waiting_count == 1);
    }
    /* a walk without a cell is no walk */
    setup();
    own = know_clock();
    monster.state = 1;
    send_act(MONSTER_AID, own - 150, 1, OWN_AID, 600);
    play_at(own - FAR_BEHIND);
    assert(sent_count == 3 && mode.aimed_waiting_count == 0);

    /* Its hit comes while the swing still waits and the actor is drawn walking: the hit is the
       swing's impact, and no attack stops the walk. */
    setup();
    own = know_clock();
    walks(&monster, own - 800);
    send_act(MONSTER_AID, own - 700, 1, OWN_AID, 600);
    play_at(own - FAR_BEHIND);
    assert(sent_count == 0 && mode.aimed_waiting_count == 1);
    send_hit(MONSTER_AID, OWN_AID, own - 100, 7, 300);
    play_at(own - FAR_BEHIND);
    assert(sent_count == 2 && sent[0].what == SENT_ATTACK && sent[0].to == &monster && sent[0].attack.impact == 1);
    assert(sent[1].what == SENT_HIT && sent[1].to == &player && sent[1].hit.damage == 7);
    assert(mode.aimed_waiting_count == 0 && mode.open_swing_count == 0);
    /* ... and the actor stands, with a walk still to come: the swing begins, then its impact */
    setup();
    own = know_clock();
    walks(&monster, own - 900);
    monster.state = 0;
    monster.track_waiting = 1;
    send_act(MONSTER_AID, own - 700, 1, OWN_AID, 600);
    send_hit(MONSTER_AID, OWN_AID, own - 100, 7, 300);
    play_at(own - FAR_BEHIND);
    assert(sent_count == 5 && sent[0].what == SENT_SWING_BEGIN && sent[0].to == &monster && sent[0].value[2] < 0);
    assert(sent[3].what == SENT_ATTACK && sent[3].attack.impact == 1 && sent[4].what == SENT_HIT && sent[4].to == &player);
    assert(mode.aimed_waiting_count == 0 && mode.open_swing_count == 0);

    /* The server takes it back while it waits: it is never shown. */
    setup();
    own = know_clock();
    walks(&monster, own - 1000);
    send_act(MONSTER_AID, own - 150, 1, OWN_AID, 600);
    play_at(own - FAR_BEHIND);
    send_act(MONSTER_AID, own - 50, 0, 0, 0);
    play_at(own - FAR_BEHIND);
    assert(sent_count == 0 && mode.aimed_waiting_count == 0 && mode.tracks.count == 0);
    monster.state = 0;
    play_at(own - FAR_BEHIND);
    assert(sent_count == 0 && mode.open_swing_count == 0);

    /* A newer swing takes the place of the one that waited. */
    setup();
    own = know_clock();
    walks(&monster, own - 1000);
    send_act(MONSTER_AID, own - 900, 1, OWN_AID, 600);
    play_at(own - FAR_BEHIND);
    send_act(MONSTER_AID, own - 150, 1, OWN_AID, 600);
    play_at(own - FAR_BEHIND);
    assert(sent_count == 0 && mode.aimed_waiting_count == 1 && mode.aimed_waiting[0].at_tick == own - 150);

    /* A cast waits the same way, and shows the time it has left when its caster is there. */
    setup();
    own = know_clock();
    walks(&monster, own - 1000);
    send_cast(MONSTER_AID, own - 200, own + 1300, OWN_AID);
    play_at(own - FAR_BEHIND);
    assert(sent_count == 0 && mode.aimed_waiting_count == 1);
    monster.state = 0;
    play_at(own - FAR_BEHIND);
    assert(sent_count == 1 && sent[0].what == SENT_CAST && sent[0].value[3] <= 1300 && sent[0].value[3] >= 1200);

    /* The actor goes while its swing waits: the swing is forgotten. */
    setup();
    own = know_clock();
    walks(&monster, own - 1000);
    send_act(MONSTER_AID, own - 150, 1, OWN_AID, 600);
    play_at(own - FAR_BEHIND);
    monster_gone = 1;
    play_at(own - FAR_BEHIND);
    assert(sent_count == 0 && mode.aimed_waiting_count == 0);
    monster_gone = 0;
    monster.state = 0;
    play_at(own - FAR_BEHIND);
    assert(sent_count == 0);

    /* The hit of that actor on someone else begins nothing that waits. */
    setup();
    own = know_clock();
    walks(&monster, own - 1000);
    send_act(MONSTER_AID, own - 150, 1, OWN_AID, 600);
    play_at(own - FAR_BEHIND);
    send_hit(MONSTER_AID, UNIT_AID, own - FAR_BEHIND, 9, 300);
    play_at(own - FAR_BEHIND);
    assert(mode.aimed_waiting_count == 1 && sent[0].what == SENT_ATTACK && sent[0].attack.impact == 0);
    for (i = 0; i < sent_count; i++)
        assert(sent[i].what != SENT_SWING_BEGIN);

    /* A swing at someone else is played when its tick comes, whatever its actor is drawn doing:
       its clock is the walk's. Also on a walk that ends after it, on which a swing at the own
       character would not show. */
    setup();
    own = know_clock();
    walks(&monster, own - 6000);
    send_act(MONSTER_AID, own - 5000, 1, UNIT_AID, 600);
    play_at(own - 5000);
    assert(sent_count == 3 && sent[0].what == SENT_SWING_BEGIN && mode.aimed_waiting_count == 0);
    setup();
    own = know_clock();
    walks(&monster, own - 4000);
    send_act(MONSTER_AID, own - 5000, 1, UNIT_AID, 600);
    play_at(own - 5000);
    assert(sent_count == 3 && sent[0].what == SENT_SWING_BEGIN && mode.aimed_waiting_count == 0);

    /* As many wait as there is room for; one more is played as it comes. */
    setup();
    own = know_clock();
    crowd_here = 1;
    for (i = 0; i < TIMED_AIMED_ACTORS; i++) {
        actor_init(&crowd[i], CROWD_AID + i, MONSTER_JOB);
        walks(&crowd[i], own - 1000);
        send_act((unsigned)(CROWD_AID + i), own - 150, 1, OWN_AID, 600);
    }
    play_at(own - FAR_BEHIND);
    assert(sent_count == 0 && mode.aimed_waiting_count == TIMED_AIMED_ACTORS);
    walks(&monster, own - 1000);
    send_act(MONSTER_AID, own - 150, 1, OWN_AID, 600);
    play_at(own - FAR_BEHIND);
    assert(sent_count == 3 && sent[0].what == SENT_SWING_BEGIN && sent[0].to == &monster);
    assert(mode.aimed_waiting_count == TIMED_AIMED_ACTORS);
    /* the first of them is there: it begins, the others wait on */
    crowd[0].state = 0;
    play_at(own - FAR_BEHIND);
    assert(sent_count == 6 && sent[3].what == SENT_SWING_BEGIN && sent[3].to == &crowd[0]);
    assert(mode.aimed_waiting_count == TIMED_AIMED_ACTORS - 1 && mode.aimed_waiting[0].gid == CROWD_AID + 1);
    crowd_here = 0;
    setup();
}

/* An announced swing's attack is paced so that the frame its hit is on shows when the swing
   lands (game_actor_swing_begin, std/swing_pace.h), by the setup of the attack state where it
   has that frame. */
static void paced(void)
{
    /* a slow attacker's swing of 912 ms, whose attack shows its hit after 144 ms on its own */
    setup();
    game_actor_swing_begin(&monster, OWN_AID, 912, 912);
    assert(sent_count == 2 && sent[0].what == SENT_SPEED && sent[1].what == SENT_STATE && sent[1].value[0] == 2);
    assert(hit_shows(&monster, 912) && !monster.swing_paces);
    /* one that lands sooner than its attack shows its hit: faster, down to twice as fast */
    game_actor_swing_begin(&monster, OWN_AID, 912, 96);
    assert(hit_shows(&monster, 96));
    game_actor_swing_begin(&monster, OWN_AID, 912, 71);
    assert(hit_shows(&monster, 72));
    game_actor_swing_begin(&monster, OWN_AID, 912, -500);
    assert(hit_shows(&monster, 72));
    /* and no slower than eight times */
    game_actor_swing_begin(&monster, OWN_AID, 912, 60000);
    assert(hit_shows(&monster, 1152));
    /* an attack that is no announced swing is not paced: the delay it has is kept */
    monster.motion_delay = 5.0f;
    game_actor_pace_attack(&monster);
    assert(monster.motion_delay == 5.0f);
    /* a swing at itself sets no state and paces nothing */
    sent_count = 0;
    game_actor_swing_begin(&monster, MONSTER_AID, 912, 912);
    assert(sent_count == 1 && sent[0].what == SENT_SPEED && monster.motion_delay == 5.0f && !monster.swing_paces);
    /* a setup that comes to know another frame after the first paces again from the attack's
       own delay, not from the paced one: a player's hit frame, by her job, after the common
       one's, by the sprite */
    monster.motion_delay = OWN_FRAME_DELAY;
    monster.attack_motion_count = HIT_FRAME;
    monster.swing_lands_ms = 4800;
    monster.swing_own_delay = 0.0f;
    monster.swing_paces = 1;
    game_actor_pace_attack(&monster);
    assert(monster.motion_delay == 24.0f); /* 100 would show it on time: eight times its own is the most */
    monster.attack_motion_count = 1.0f;
    game_actor_pace_attack(&monster);
    assert(monster.motion_delay == 24.0f); /* 200 would: eight times its own again, not eight times 24 */
    monster.attack_motion_count = 10.0f;
    game_actor_pace_attack(&monster);
    assert(monster.motion_delay == 20.0f && hit_shows(&monster, 4800));
    monster.swing_paces = 0;
    setup();
}

/* ZC_OWN_INTENT: what the server holds for an intent, by the intent's number. Only its end is
   read: the end of the newest intent takes the attack cursor state, and the end of an older one
   changes nothing. */
static void own_intent(void)
{
    static struct Player own;
    unsigned char packet[14];

    setup();
    world.player = &own;
    own.intent_hold = 1;
    mode.intent_seq = 7;
    memset(packet, 0, sizeof packet);
    put16(packet, 0, 0x0f24);
    put16(packet, 2, 7);
    put32(packet, 6, 123456);
    put16(packet, 10, 156);
    put16(packet, 12, 153);

    /* held, under way and acting: nothing */
    packet[4] = 0;
    game_mode_zc_own_intent(&mode, packet);
    packet[4] = 1;
    game_mode_zc_own_intent(&mode, packet);
    packet[4] = 2;
    game_mode_zc_own_intent(&mode, packet);
    assert(sent_count == 0 && own.intent_hold == 1);

    /* ended: the intent of that number is over */
    packet[4] = 3;
    packet[5] = 5;
    game_mode_zc_own_intent(&mode, packet);
    assert(sent_count == 1 && sent[0].what == SENT_MODE && sent[0].value[0] == 0x1a && sent[0].value[1] == 0 && own.intent_hold == 0);

    /* the end of an older request leaves the newest intent alone */
    own.intent_hold = 1;
    put16(packet, 2, 6);
    game_mode_zc_own_intent(&mode, packet);
    assert(sent_count == 1 && own.intent_hold == 1);

    /* a state this client does not know changes nothing */
    packet[4] = 9;
    put16(packet, 2, 7);
    game_mode_zc_own_intent(&mode, packet);
    assert(sent_count == 1 && own.intent_hold == 1);
    world.player = (struct Player *)&player;
}

/* The player's own swing follows the predicted world's word of it (game_mode_own_swing,
   world/own_swing.h): shown at once when the world's body begins it, its animation paced to
   meet the damage number; the server's record of it is not played again; it is taken back when
   the world has it no more before it landed. A swing the server tells that the world had not
   is played from its record and is not the world's to take back. */
/* How many of the messages sent since setup are of the kind `what`, and the last of them. */
static int sent_of(int what, const struct Sent **last)
{
    int i, n = 0;

    for (i = 0; i < sent_count; i++) {
        if (sent[i].what != what)
            continue;
        n++;
        if (last)
            *last = &sent[i];
    }
    return n;
}

/* The world says its swing, and the frame shows what follows. */
static void says(int has, unsigned target, DWORD at_tick, DWORD lands, unsigned motion_ms)
{
    world_swing.has = has;
    world_swing.target = target;
    world_swing.at_tick = at_tick;
    world_swing.lands = lands;
    world_swing.motion_ms = motion_ms;
    game_mode_own_swing(&mode, has, target, at_tick, lands, motion_ms);
}

static void own_swing(void)
{
    static struct Player whole;
    struct GameActor *me = (struct GameActor *)&whole;
    const struct Sent *last = NULL;
    DWORD t0, own;

    setup();
    memset(&whole, 0, sizeof whole);
    actor_init(me, OWN_AID, 0);
    world.player = &whole;
    t0 = timeGetTime();
    /* the server's clock is 0x10000000 ahead of this machine's, and a round trip takes 80 ms, 40
       of them up: the own clock leads the server's by 40 */
    assert(clock_sync_sample(&mode.clock_sync, t0 - 80, t0 - 40 + 0x10000000, t0 - 40 + 0x10000000, t0));
    way_up_sample(&mode.way_up, t0 - 80, t0 - 40 + 0x10000000);
    assert(mode.clock_sync.round_trip == 80);
    own = game_mode_own_time(&mode);

    /* the world's body has no swing: nothing */
    says(0, 0, 0, 0, 0);
    assert(sent_count == 0 && !mode.own_swing.shown);

    /* it begins one at the monster that lands five seconds on, with an attack motion of 300 ms.
       While the character still walks the swing waits */
    whole.state = 1;
    says(1, MONSTER_AID, own, own + 5000, 300);
    assert(sent_count == 0 && !mode.own_swing.shown);
    whole.state = 0;
    /* and on one that stands it is shown at once, its hit frame paced to when the number is
       here: 80 ms after the swing lands (half the round trip down, and the own clock's lead of
       half a round trip) */
    says(1, MONSTER_AID, own, own + 5000, 300);
    assert(sent_of(SENT_SWING_BEGIN, &last) == 1 && last->to == me && last->value[0] == MONSTER_AID && last->value[1] == 300);
    assert(last->value[2] <= 5080 && last->value[2] >= 5030);
    assert(mode.open_swing_count == 1 && mode.own_swing.shown == OWN_SWING_AHEAD);
    /* the frames after: nothing more */
    says(1, MONSTER_AID, own, own + 5000, 300);
    assert(sent_of(SENT_SWING_BEGIN, NULL) == 1);
    /* the server's record of that swing is not played again, nor kept */
    send_act(OWN_AID, own + 20, 1, MONSTER_AID, 300);
    play_at(own + 100);
    assert(sent_of(SENT_SWING_BEGIN, NULL) == 1 && mode.tracks.count == 0);
    /* the world has it no more before it landed: it is taken back, once */
    assert(sent_of(SENT_SWING_CANCEL, NULL) == 0);
    says(0, 0, 0, 0, 0);
    assert(sent_of(SENT_SWING_CANCEL, &last) == 1 && last->to == me && mode.open_swing_count == 0);
    says(0, 0, 0, 0, 0);
    assert(sent_of(SENT_SWING_CANCEL, NULL) == 1);
    /* without an attack motion in the body the default is played */
    says(1, MONSTER_AID, own + 1000, own + 6000, 0);
    assert(sent_of(SENT_SWING_BEGIN, &last) == 2 && last->value[1] == 1440 && last->value[2] <= 6080 && last->value[2] >= 6030);
    says(0, 0, 0, 0, 0);
    assert(sent_of(SENT_SWING_CANCEL, NULL) == 2);

    /* a swing the server tells and the world had not: played from its record on the own clock,
       left alone while the world has none, and the world's swing at that target is that one */
    setup();
    world.player = &whole;
    assert(clock_sync_sample(&mode.clock_sync, t0 - 80, t0 - 40 + 0x10000000, t0 - 40 + 0x10000000, t0));
    way_up_sample(&mode.way_up, t0 - 80, t0 - 40 + 0x10000000);
    own = game_mode_own_time(&mode);
    send_act(OWN_AID, own - 100, 1, MONSTER_AID, 5000);
    assert(mode.tracks.count == 1 && mode.own_swing.shown == OWN_SWING_TOLD);
    play_at(own - 100000);
    assert(sent_of(SENT_SWING_BEGIN, &last) == 1 && last->to == me && last->value[1] == 5000);
    assert(last->value[2] <= 4980 && last->value[2] >= 4930);
    says(0, 0, 0, 0, 0);
    says(1, MONSTER_AID, own - 400, own + 4600, 300);
    assert(sent_of(SENT_SWING_BEGIN, NULL) == 1 && sent_of(SENT_SWING_CANCEL, NULL) == 0);
    assert(mode.own_swing.shown == OWN_SWING_TOLD);
    /* a monster's swing at the character is paced to the tick it lands at, the last moment a
       request still comes before it, and not to the number that follows it */
    send_act(MONSTER_AID, own - 100, 1, OWN_AID, 600);
    play_at(own - 100000);
    assert(sent_of(SENT_SWING_BEGIN, &last) == 2 && last->to == &monster && last->value[2] <= 500 && last->value[2] >= 450);
    world.player = (struct Player *)&player;
}

int main(void)
{
    act_position();
    act();
    skill_use();
    swing();
    tracks();
    threats();
    threats_wait_for_the_walk();
    paced();
    own_intent();
    own_swing();
    return 0;
}
