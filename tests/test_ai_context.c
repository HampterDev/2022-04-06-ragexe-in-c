#include "skills/skill_info.h"
#include "login/merc_ai.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

void lua_settop(lua_State *L, int index);
double lua_tonumber(lua_State *L, int index);
size_t lua_objlen(lua_State *L, int index);
void lua_rawgeti(lua_State *L, int index, int key);

struct Session g_session1;
const unsigned char CMercenaryAI_col[1] = {0};
static BYTE actor[0x400], target[0x400], player[0x400], owner[0x400];
static struct GameActorOps actor_ops;
static int timed_actions, timed_action, timed_mode;
static int active, has_actor, has_target, has_player, has_owner, hidden;
static int conversions, valid, closest, skill_found = 1, skill_destroys;
static int packet_count, packet_id, message_count, message_ids[8], use_data[4];
static BYTE last_packet[16];
static struct List actor_list;

struct GameMode *game_current_mode(void) { return active ? (struct GameMode *)&active : NULL; }
struct Player *game_current_player(void) { return active && has_player ? (struct Player *)player : NULL; }
const struct List *game_current_actor_list(void) { return active ? &actor_list : NULL; }
struct GameActor *game_find_actor(int id)
{
    if (!active) return NULL;
    if (id == 20 && has_actor) return (struct GameActor *)actor;
    if (id == 30 && has_target) return (struct GameActor *)target;
    if (id == 40 && has_owner) return (struct GameActor *)owner;
    return NULL;
}
int game_world_to_cell(float x, float z, int *cx, int *cy)
{
    if (!active) return 0;
    ++conversions;
    *cx = (int)x;
    *cy = (int)z;
    return 1;
}
int game_can_walk_to_cell(int x, int y) { (void)x; (void)y; return active && valid; }
int game_closest_walkable_cell(int *x, int *y)
{
    if (!active || !closest) return 0;
    *x = 5; *y = 6;
    return 1;
}
int game_map_has_party_marks(void) { return 0; }
int game_siege_state(void) { return 0; }
int game_send_message(int message, int a, int b, int c, int d)
{
    assert(active && message_count < 8);
    message_ids[message_count++] = message;
    if (message == 0x46) memcpy(use_data, (const void *)a, sizeof use_data);
    if (message == 0x45) { use_data[0] = a; use_data[1] = b; use_data[2] = c; }
    assert(d == 0);
    return 0;
}
struct RagConnection *rag_connection_instance(void) { return (struct RagConnection *)&packet_count; }
int rag_connection_get_packet_length(struct RagConnection *connection, int id)
{
    assert(connection == rag_connection_instance());
    return id == 0x232 ? 9 : id == 0x233 ? 11 : 6;
}
char rag_connection_send_packet(struct RagConnection *connection, int length, const void *packet)
{
    assert(connection == rag_connection_instance());
    ++packet_count;
    packet_id = *(const WORD *)packet;
    memcpy(last_packet, packet, length);
    return 1;
}
int crt_sprintf(char *buffer, const char *format, ...)
{ (void)buffer; (void)format; assert(!"unexpected Lua error"); return 0; }
void mem_free(void *memory)
{ (void)memory;  assert(!"unexpected actor deletion"); }
void *mem_alloc(size_t bytes)
{ (void)bytes; assert(!"unexpected option insertion"); return NULL; }

int merc_ai_trace(lua_State *L) { (void)L; return 0; }
int merc_ai_trace_ai(lua_State *L) { (void)L; return 0; }
int merc_ai_trace_value(lua_State *L) { (void)L; return 0; }

int session_is_homunculus_aid(void *s, DWORD aid) { assert(s == &g_session1); return aid == 20; }
int session_is_mercenary_aid(void *s, DWORD aid) { assert(s == &g_session1); return aid == 21; }
int session_is_own_unit_aid(void *s, DWORD aid) { assert(s == &g_session1); return aid == 20 || aid == 21; }
int session_is_homunculus_skill(void *s, int skill) { assert(s == &g_session1); return skill == 100; }
int session_is_mercenary_skill(void *s, int skill) { assert(s == &g_session1); return skill == 101; }
int session_is_mercenary_job(void *s, int job) { (void)s; (void)job; return 0; }
int session_is_player_job(void *s, int job) { (void)s; (void)job; return 0; }
int session_is_own_guild_id(void *s, int id) { (void)s; (void)id; return 0; }
int session_is_party_member_aid(struct Session *s, int aid, unsigned char flag) { (void)s; (void)aid; (void)flag; return 0; }
int game_actor_get_job(void *a) { (void)a; return 1001; }
BYTE skill_info_is_good_skill_for_actor(int skill) { return skill == 100; }
BYTE effect_state_bit6(DWORD state) { return state != 0; }
BYTE is_effect_state_burrow(int state) { (void)state; return 0; }
BYTE is_effect_state_star2(int state) { (void)state; return 0; }

struct SkillInfo *session_get_skill_info(struct SkillInfo *out, int skill)
{
    (void)skill;
    memset(out, 0, sizeof *out);
    SKILL_INFO_FOUND(out) = skill_found;
    SKILL_INFO_RANGE(out) = 5;
    return out;
}
struct SkillInfo *session_get_homun_skill_by_id(struct Session *s, struct SkillInfo *out, int skill)
{  return session_get_skill_info(out, skill); }
struct SkillInfo *session_get_mer_skill_by_id(struct Session *s, struct SkillInfo *out, int skill)
{  return session_get_skill_info(out, skill); }
void skill_info_destruct(struct SkillInfo *info) { assert(SKILL_INFO_RANGE(info) == 5); ++skill_destroys; }
int skill_attack_range(int skill, int level)
{ assert(skill == 100 && level == 2); return 7; }
float float_sqrt(float value) { return sqrtf(value); }
float float_cos(float value) { return cosf(value); }
float float_sin(float value) { return sinf(value); }
static int effect_state(struct RenderObject *self)
{  return (void *)self == target ? hidden : 0; }
int game_actor_get_npc_location_radius(struct GameActor *self)
{  assert((void *)self == owner); return 4; }
int game_actor_get_guild_id(struct GameActor *self)
{  (void)self; assert(!"no guild queries"); return 0; }
int *game_actor_get_request(struct GameActor *self, int which)
{  (void)self; (void)which; assert(!"no request reads"); return NULL; }
int *game_actor_get_request2(struct GameActor *self, int which)
{  (void)self; (void)which; assert(!"no request reads"); return NULL; }
void game_actor_set_request(struct GameActor *self, const int *record, int which)
{  (void)self; (void)record; (void)which; assert(!"no request writes"); }
void game_actor_set_request2(struct GameActor *self, const int *record, int which)
{  (void)self; (void)record; (void)which; assert(!"no request writes"); }

static void timed_action_start(struct RenderObject *self, int action, int mode)
{
    assert((void *)self == actor);
    ++timed_actions;
    timed_action = action;
    timed_mode = mode;
}

static void check_timed_order(void)
{
    const int jobs[] = {0x179f, 0x17a0, 0x17a4, 0x17a5};
    const int actions[] = {0x38, 0x38, 0x38, 0x38};
    size_t i;
    actor_ops.base.set_action = timed_action_start;
    for (i = 0; i < sizeof jobs / sizeof jobs[0]; ++i) {
        struct MercenaryAI ai = {0};
        g_session1.homunJob = jobs[i];
        g_session1.homun_ai_state[0] = 0x1d;
        timed_actions = 0;
        timed_mode = -1;
        assert(merc_ai_run(&ai, 20) == 0);
        assert(timed_actions == 1 && timed_action == actions[i] && timed_mode == 0);
        assert(ai.duration == 3000 && ai.lastRun == 0);
    }
    g_session1.homun_ai_state[0] = 0;
    puts("AI timed order: feeding keeps looping playback for homunculus jobs on both sides of the former Homunculus S range");
}

static void args(lua_State *L, double a, double b, double c, double d, double e)
{
    lua_settop(L, 0);
    lua_pushnumber(L, a); lua_pushnumber(L, b); lua_pushnumber(L, c);
    lua_pushnumber(L, d); lua_pushnumber(L, e);
    message_count = packet_count = 0;
}
static void check_missing(lua_State *L)
{
    struct MercenaryAI ai = {0};
    int before = skill_destroys;
    assert(merc_ai_run(&ai, 20) == 0 && ai.lastRun == 0);
    args(L, 1, 20, 0, 0, 0);
    assert(merc_ai_homun_get_v(L) == 2 && lua_tonumber(L, -1) == -1 && lua_tonumber(L, -2) == -1);
    args(L, 1, 21, 0, 0, 0);
    assert(merc_ai_merc_get_v(L) == 2 && lua_tonumber(L, -1) == -1 && lua_tonumber(L, -2) == -1);
    args(L, 20, 1, 100, 3, 4);
    assert(merc_ai_skill_ground(L) == 0 && message_count == 0);
    args(L, 20, 1, 100, 30, 0);
    assert(merc_ai_skill_object(L) == 0 && message_count == 0 && skill_destroys == before + 1);
    args(L, 20, 5, 6, 0, 0);
    assert(merc_ai_move(L) == 0 && packet_count == 0);
    assert(merc_ai_move_to_owner(L) == 0 && packet_count == 0);
    assert(merc_ai_get_msg(L) == 0 && merc_ai_get_res_msg(L) == 0);
}

extern DWORD g_homunLastAttackTick;

static void check_session_options(lua_State *L)
{
    struct TreeNode head = {0};
    struct { struct TreeNode links; int key, padding; long long value; } entry = {0};
    int keys[] = {16, 17, 18};
    int i;

    head.is_head = 1;
    head.parent = &entry.links;
    entry.links.left = entry.links.right = entry.links.parent = &head;
    g_session1.instantVars.head = &head;
    g_session1.instantVars.size = 1;
    active = has_actor = has_target = 1;
    hidden = 0;
    g_session1.homunStats[7] = 0;
    for (i = 0; i < 3; i++) {
        entry.key = keys[i]; entry.value = 1;
        g_homunLastAttackTick = 0;
        args(L, 20, 30, 0, 0, 0);
        assert(merc_ai_homun_attack(L) == 0);
        assert(packet_count == (keys[i] != 17));
    }
    entry.key = 17;
    entry.value = 1LL << 32;
    g_homunLastAttackTick = 0;
    args(L, 20, 30, 0, 0, 0);
    assert(merc_ai_homun_attack(L) == 0 && packet_count == 1 && packet_id == 0x233);
    entry.value = -1;
    g_homunLastAttackTick = 0;
    args(L, 20, 30, 0, 0, 0);
    assert(merc_ai_homun_attack(L) == 0 && packet_count == 1);
    head.parent = &head;
    g_session1.instantVars.size = 0;
    g_homunLastAttackTick = 0;
    args(L, 20, 30, 0, 0, 0);
    assert(merc_ai_homun_attack(L) == 0 && packet_count == 1);
    g_session1.instantVars.head = NULL;

    g_session1.ownAid = (int)0x80000000u;
    args(L, 0, 20, 0, 0, 0);
    assert(merc_ai_homun_get_v(L) == 1 && lua_tonumber(L, -1) == 2147483648.0);
    g_session1.ownAid = -1;
    args(L, 0, 21, 0, 0, 0);
    assert(merc_ai_merc_get_v(L) == 1 && lua_tonumber(L, -1) == 4294967295.0);
    g_session1.ownAid = 10;
}

int main(void)
{
    struct { struct ListNode links; BYTE *value; } head, first, second;
    lua_State *L = luaL_newstate();
    int x, y, before;
    assert(L);
    g_session1.ownAid = 10; g_session1.homunAid = 20; g_session1.mercAid = 21;
    actor_ops.base.get_effect_state = effect_state;
    ACTOR(actor)->ops = ACTOR(target)->ops = ACTOR(player)->ops = ACTOR(owner)->ops = &actor_ops.base.base;
    ACTOR(actor)->aid = 20;
    ACTOR(target)->aid = 30;
    ACTOR(actor)->requests[5] = 40;
    ACTOR(target)->pos.x = 3; ACTOR(target)->pos.z = 4;
    head.links.next = &first.links; head.links.prev = &second.links;
    first.links.next = &second.links; first.links.prev = &head.links; first.value = actor;
    second.links.next = &head.links; second.links.prev = &first.links; second.value = target;
    actor_list.head = &head.links; actor_list.size = 2;

    check_missing(L);
    lua_settop(L, 0);
    assert(merc_ai_get_actors(L) == 1 && lua_objlen(L, -1) == 0);
    active = 1;
    check_missing(L);
    has_actor = has_target = has_player = has_owner = 1;
    check_timed_order();
    lua_settop(L, 0);
    assert(merc_ai_get_actors(L) == 1 && lua_objlen(L, -1) == 3);
    lua_rawgeti(L, -1, 1); assert(lua_tonumber(L, -1) == 20); lua_settop(L, -2);
    lua_rawgeti(L, -1, 2); assert(lua_tonumber(L, -1) == 30); lua_settop(L, -2);
    lua_rawgeti(L, -1, 3); assert(lua_tonumber(L, -1) == 10);
    assert(merc_ai_owner_attack_range_at_least(20, 2) == 4);
    assert(merc_ai_owner_attack_range_at_least(20, 9) == 9);

    args(L, 1, 30, 0, 0, 0); hidden = 1;
    before = conversions;
    assert(merc_ai_homun_get_v(L) == 2 && lua_tonumber(L, -1) == -1 && conversions == before);
    hidden = 0;
    args(L, 1, 10, 0, 0, 0);
    assert(merc_ai_homun_get_v(L) == 2 && lua_tonumber(L, -1) == 0);
    has_player = 0;
    args(L, 1, 10, 0, 0, 0);
    assert(merc_ai_homun_get_v(L) == 2 && lua_tonumber(L, -1) == -1);
    has_player = 1;

    ACTOR(actor)->pos.x = 3; ACTOR(actor)->pos.z = 4;
    x = 3; y = 4;
    merc_ai_apply_skill_attack_range(20, target, &x, &y, 100, 2);
    assert(x == 3 && y == 4);

    args(L, 20, 2, 100, 5, 6);
    assert(merc_ai_skill_ground(L) == 0 && message_count == 2);
    assert(message_ids[0] == 0x46 && message_ids[1] == 0x1a);
    assert(use_data[0] == 100 && use_data[1] == 5 && use_data[2] == 6 && use_data[3] == 2);
    args(L, 20, 2, 100, 30, 0);
    before = skill_destroys;
    assert(merc_ai_skill_object(L) == 1 && message_count == 2 && skill_destroys == before + 1);
    assert(message_ids[0] == 0x45 && message_ids[1] == 0x1a);
    assert(use_data[0] == 100 && use_data[1] == 30 && use_data[2] == 2);
    hidden = 1;
    args(L, 20, 2, 100, 30, 0);
    assert(merc_ai_skill_object(L) == 0 && message_count == 0 && skill_destroys == before + 2);
    hidden = 0; skill_found = 0;
    args(L, 20, 2, 100, 30, 0);
    assert(merc_ai_skill_object(L) == 0 && message_count == 0 && skill_destroys == before + 3);
    skill_found = 1;

    valid = 1;
    args(L, 20, 5, 6, 0, 0);
    assert(merc_ai_move(L) == 0 && packet_count == 1 && packet_id == 0x232);
    assert(last_packet[0] == 0x32 && last_packet[1] == 2 && last_packet[2] == 20);
    assert(last_packet[6] == 1 && last_packet[7] == 64 && last_packet[8] == 96);
    args(L, 20, 100, 100, 0, 0);
    assert(merc_ai_move(L) == 0 && packet_count == 0);
    valid = 0; closest = 1;
    args(L, 20, 100, 100, 0, 0);
    assert(merc_ai_move(L) == 0 && packet_count == 1);
    active = 0;
    check_missing(L);
    check_session_options(L);
    lua_close(L);
    puts("AI context: real Lua queries, missing actors, movement packets, skill cleanup, shared option gates and unsigned session IDs passed");
    return 0;
}
