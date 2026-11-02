#include "game/packet_handlers.h"
#include "game/gamemode.h"
#include "game/context.h"
#include "game/cells.h"
#include "actor/actor_ops.h"
#include "actor/game_actor.h"
#include "actor/render_object.h"
#include "session/state.h"
#include "ui/window_messages.h"
#include "ui/window_manager_state.h"
#include "world/actor_lookup.h"
#include "world/object_lookup.h"
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

enum { SENT_ATTACK, SENT_SKILL_USE, SENT_HIT, SENT_TALK, SENT_STATE, SENT_SPEED, SENT_CHAT, SENT_MODE };

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

    (void)sender; (void)par3; (void)par4;
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
    default:
        assert(!"an unexpected actor message");
    }
}

static void set_state(struct GameActor *actor, int state, unsigned char snap)
{
    struct Sent *s = record(SENT_STATE, actor);

    s->value[0] = state;
    s->value[1] = snap;
}

static void set_attack_speed(struct GameActor *actor, int ms)
{
    record(SENT_SPEED, actor)->value[0] = ms;
}

struct GameMode *game_current_mode(void) { return &mode; }
int game_actor_get_job(struct GameActor *actor) { return actor->job; }
struct GameActor *world_get_game_actor_by_aid(struct World *w, unsigned aid)
{
    assert(w == &world);
    return aid == MONSTER_AID ? &monster : NULL;
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
void game_mode_world_to_cell(struct GameMode *self, float world_x, float world_z, int *x, int *y)
{ (void)self; (void)world_x; (void)world_z; *x = 0; *y = 0; }

static void actor_init(struct GameActor *actor, int aid, int job)
{
    memset(actor, 0, sizeof *actor);
    ((struct RenderObject *)actor)->ops = &table.base.base;
    actor->aid = aid;
    actor->job = job;
    actor->attack_motion_count = 2.0f;
    actor->motion_delay = 3.0f;
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

int main(void)
{
    act_position();
    act();
    skill_use();
    return 0;
}
