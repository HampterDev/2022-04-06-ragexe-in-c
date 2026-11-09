#ifndef SKILL_ORACLE_H
#define SKILL_ORACLE_H

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <math.h>
#include <setjmp.h>
#include <windows.h>
#include "oracle_image.h"
#include "session/state.h"
#include "session/session_skills.h"
#include "session/session_jobs.h"
#include "session/session_names.h"
#include "session/session_state.h"
#include "session/session_talk.h"
#include "skills/skill_answer.h"
#include "skills/skill_action.h"
#include "skills/skill_action_registry.h"
#include "actor/types.h"
#include "actor/actor_ops.h"
#include "actor/game_actor.h"
#include "actor/render_object.h"
#include "actor/actor_common.h"
#include "game/gamemode.h"
#include "game/context.h"
#include "game/cells.h"
#include "game/camera.h"
#include "game/world_objects.h"
#include "game/skill_use.h"
#include "game/skill_fx.h"
#include "game/actor_names.h"
#include "world/state.h"
#include "world/object_lookup.h"
#include "world/mode_types.h"
#include "resources/attr_queries.h"
#include "resources/attr_types.h"
#include "effects/effect.h"
#include "effects/effect_mgr.h"
#include "effects/durations.h"
#include "effects/effect_controls.h"
#include "effects/rag/rag_effect.h"
#include "contents/sound.h"
#include "core/msg_str.h"
#include "ui/window_manager_state.h"
#include "ui/window_messages.h"

#define ORIG_SHOW_SKILL_USE    0x8068e0u
#define ORIG_SHOW_SKILL_USE_END 0x814140u
#define ORIG_GET_GAME_MODE     0x6617e0u
#define ORIG_GET_BEGIN_EFFECT  0x875350u
#define ORIG_ACTOR_OR_PLAYER   0x65c540u
#define ORIG_ACTOR_BY_AID      0x65c580u
#define ORIG_SKILL_BY_AID      0x65c6a0u
#define ORIG_IS_EFFECT_EXIST   0x74be60u
#define ORIG_LAUNCH_EFFECT     0x74bf80u
#define ORIG_SPAWN             0x96aca0u
#define ORIG_PLAY_WAVE         0x90e8a0u
#define ORIG_PLAY_MOTION       0x75f0c0u
#define ORIG_TURN_TOWARD       0x748960u
#define ORIG_FIND_EFFECT       0x96b6c0u
#define ORIG_SERVANT_COUNT     0x957b80u
#define ORIG_SERVANT_FIRE      0x957960u
#define ORIG_EFFECT_CUE        0x992050u
#define ORIG_IS_REGISTERED     0x97fff0u
#define ORIG_ACTION_USE        0xa14060u
#define ORIG_ACTION_HIT        0xa13e30u
#define ORIG_IS_MONSTER_ACTOR  0x883e20u
#define ORIG_IS_PC_JOB         0x8699b0u
#define ORIG_JOB_NAME          0x853fb0u
#define ORIG_GET_CLIENT_COOR   0x76ec30u
#define ORIG_GET_ACTOR_NAME    0x76ea10u
#define ORIG_ATTR_HEIGHT       0x9f2780u
#define ORIG_CLEAR_HOMUN       0x85a220u
#define ORIG_VIEW_HEADING      0x789610u
#define ORIG_STEP_FRAMES       0x75ef00u
#define ORIG_WNDMGR_SEND_MSG   0x642fb0u
#define ORIG_MSG_STR           0x477530u
#define ORIG_LAUNCH_EFFECT_AT  0x772740u
#define ORIG_EFFECT_ON_CASTER  0x8842a0u
#define ORIG_COUNTED_EFFECT    0x7f0820u
#define ORIG_COUNTED_EFFECT_END 0x7f08c0u
#define ORIG_SET_COUNTER       0x7e64b0u
#define ORIG_EFFECT_MGR_PTR    0x115c6c0u
#define ORIG_OWN_AID           0x113520cu
#define ORIG_EFFECT_OPTION     0x1138cbcu
#define ORIG_DURATIONS         0xe907a0u

#define ORIG_ACTOR_AID         0x110
#define ORIG_ACTOR_POS         0x10
#define ORIG_ACTOR_COUNTERS    0x118
#define ORIG_MODE_WORLD        0xcc
#define ORIG_WORLD_PLAYER      0x2c
#define ORIG_WORLD_ATTR        0x30
#define ORIG_WORLD_OBJECTS     0x08
#define ORIG_CHAT_LINE         0x7eb880u
#define ORIG_GROUND_SCOPE_CTOR 0x89a900u
#define ORIG_ACTION_CAST       0xa13b20u
#define ORIG_ACTION_CAST_ON    0xa13ba0u
#define ORIG_ACTION_CAST_AT    0xa13c20u
#define ORIG_MOTION_DELAY      0x75f6e0u
#define ORIG_IS_GOOD_SKILL     0xa11700u
#define ORIG_GET_JOB           0x74aea0u
#define ORIG_MODE_SERVER_COOR  0x7705a0u
#define ORIG_WORLD_SERVER_COOR 0x770750u
#define ORIG_EFFECT_VT_INIT      (0x04 / 4)
#define ORIG_EFFECT_VT_STOP_FRAME (0x38 / 4)
#define ORIG_MODE_VT_SEND_MSG  (0x18 / 4)
#define ORIG_ACTOR_VT_SEND_MSG (0x08 / 4)
#define ORIG_ACTOR_VT_SET_ACTION (0x1c / 4)
#define ORIG_ACTOR_VT_EFFECT_STATE (0x34 / 4)
#define ORIG_ACTOR_VT_SET_STATE  (0x3c / 4)
#define ORIG_ACTOR_VT_ATTACK_SPEED (0x9c / 4)
#define ORIG_RAG_VT_SEND_MSG     (0x40 / 4)
#define ORIG_EFFECT_VT_DELAY     (0x40 / 4)
#define ORIG_EFFECT_VT_SPIN      (0x44 / 4)
#define ORIG_EFFECT_VT_ZOOM      (0x50 / 4)

#define AID_CASTER 1001
#define AID_TARGET 1002
#define AID_OWN    7777
#define ATTR_ZOOM  5
#define EVENTS 48
#define EVENT_LEN 2112

struct Session g_session1;
struct EffectMgr *g_effectMgr;
struct UIWindowMgr g_windowMgr;
int g_serviceType;
float g_sinTable[361], g_cosTable[361];

static char events[EVENTS][EVENT_LEN];
static int event_count, overflow;
static unsigned fault_code, fault_va;

static void event(const char *fmt, ...)
{
    va_list ap;
    if (event_count >= EVENTS) {
        overflow = 1;
        return;
    }
    va_start(ap, fmt);
    vsnprintf(events[event_count++], EVENT_LEN, fmt, ap);
    va_end(ap);
}

static unsigned char orig_mode[0x2000], orig_world[0x1000], orig_caster[0x1000], orig_target[0x1000], orig_player[0x3000];
static unsigned char orig_rag[0x400], orig_effect[0x1000], orig_mgr[0x100];
static unsigned orig_attr[0x40];
static void *orig_mode_vt[64], *orig_actor_vt[64], *orig_rag_vt[64], *orig_effect_vt[64];

static struct GameMode lift_mode;
static struct ModeOps lift_mode_ops;
static struct World lift_world;
static struct C3dAttr lift_attr;
static struct GameActor lift_caster, lift_target;
static unsigned char lift_player[0x3000];
static struct GameActorOps lift_actor_ops;
static struct RagEffect lift_rag;
static union { struct Effect effect; unsigned char raw[0x1000]; } lift_effect_storage;
#define lift_effect lift_effect_storage.effect
static struct EffectOps lift_effect_ops;
static unsigned char lift_scope[0x200];
static struct RenderObjectOps lift_scope_ops;
static void *orig_scope;
static void *orig_ground_vt[64];
static unsigned orig_list_head[3];
static int lift_mgr, lift_action_mgr;

static const char *(*who_extra)(const void *p);

static const char *who(const void *p)
{
    static char other[16];
    if (!p) return "0";
    if (who_extra) {
        const char *extra = who_extra(p);
        if (extra) return extra;
    }
    if (p == orig_caster || p == &lift_caster) return "C";
    if (p == orig_target || p == &lift_target) return "T";
    if (p == orig_player || p == lift_player) return "P";
    if (p == orig_rag || p == &lift_rag) return "R";
    if (p == orig_effect || p == &lift_effect) return "E";
    if (p == orig_mode || p == &lift_mode) return "M";
    if ((orig_scope && p == orig_scope) || p == lift_scope) return "G";
    snprintf(other, sizeof other, "?%p", p);
    return other;
}

static int readable(long long v)
{
    MEMORY_BASIC_INFORMATION mbi;
    if (v < 0x10000 || v > 0xffffffffll)
        return 0;
    if (!VirtualQuery((const void *)(UINT_PTR)v, &mbi, sizeof mbi) || mbi.State != MEM_COMMIT || (mbi.Protect & PAGE_GUARD))
        return 0;
    return (mbi.Protect & (PAGE_READONLY | PAGE_READWRITE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE)) != 0;
}

static const char *(*msg_text_extra)(long long msg, long long a, long long b, long long c, long long d);

static const char *msg_text(long long msg, long long a, long long b, long long c, long long d)
{
    static char text[256];
    if (msg_text_extra) {
        const char *extra = msg_text_extra(msg, a, b, c, d);
        if (extra) return extra;
    }
    if (msg == 0xe && readable(a)) {
        const float *at = (const float *)(int)a;
        snprintf(text, sizeof text, "%llx at %.0f %.0f %.0f %llx %llx %llx", msg, at[0], at[1], at[2], b, c, d);
    } else if (msg == 0x2c && readable(c)) {
        const float *at = (const float *)(int)c;
        snprintf(text, sizeof text, "%llx %llx %llx at %.0f %.0f %.0f %llx", msg, a, b, at[0], at[1], at[2], d);
    } else if (msg == 0x25 && readable(a)) {
        const struct ActorSkillUse *u = (const struct ActorSkillUse *)(int)a;
        snprintf(text, sizeof text, "25 hit skill %x dmg %d act %d tgt %d t %d lv %d n %d cell %d %d mt %d %d", u->skill_id, u->damage,
                 u->action, u->target_aid, u->start_time, u->level, u->count, u->cell_x, u->cell_y, u->attack_mt, u->attacked_mt);
    } else {
        snprintf(text, sizeof text, "%llx %llx %llx %llx %llx", msg, a, b, c, d);
    }
    return text;
}


void *world_get_actor_or_player(struct World *self, int aid)
{
    (void)self;
    if (aid == AID_CASTER) return &lift_caster;
    if (aid == AID_TARGET) return &lift_target;
    if (aid == AID_OWN) return lift_player;
    return NULL;
}
struct GameActor *world_get_game_actor_by_aid(struct World *world, unsigned aid)
{
    (void)world;
    if (aid == AID_CASTER) return &lift_caster;
    if (aid == AID_TARGET) return &lift_target;
    return NULL;
}
struct Skill *world_get_skill_by_aid(struct World *world, int aid) { (void)world; (void)aid; return NULL; }
struct RagEffect *game_actor_launch_effect(struct GameActor *self, int effect_id, Vec3 offset, float rotation)
{ (void)offset; (void)rotation; event("launch %x %s", effect_id, who(self)); return &lift_rag; }
struct Effect *effect_mgr_spawn(struct EffectMgr *self, int aid, int type, Vec3 pos, int target_aid, int level, unsigned char reuse, int instance_key)
{ (void)self; (void)reuse; (void)instance_key; event("spawn %x aid %d tgt %d lvl %d at %.0f %.0f %.0f", type, aid, target_aid, level, pos.x, pos.y, pos.z); return &lift_effect; }
void sound_play_wave(const char *name, float x, float y, float z, int max_distance, int full_distance, float volume)
{ (void)max_distance; (void)full_distance; (void)volume; event("wave %s %.0f %.0f %.0f", name, x, y, z); }
void render_object_play_motion_range(struct RenderObject *self, unsigned char action, unsigned char motion, unsigned char last,
                                     unsigned char close_action, unsigned char close_motion, int timed, int duration, int loop)
{ event("motion %s %x %d %d %d %d %d %d %d", who(self), action, motion, last, close_action, close_motion, timed, duration, loop); }
void render_object_turn_toward(struct RenderObject *self, float x, float z) { event("turn %s %.0f %.0f", who(self), x, z); }
unsigned char able_to_make_effect_is_effect_exist(void *self, int effect_id) { event("exist %x %s", effect_id, who(self)); return 0; }
struct Effect *effect_mgr_find_effect(struct EffectMgr *self, int aid, int type) { (void)self; event("find %x aid %d", type, aid); return NULL; }
int servant_weapon_effect_count_armed(struct Effect *effect) { (void)effect; return 0; }
void servant_weapon_effect_fire(struct Effect *effect, int effect_id) { (void)effect; event("servant fire %x", effect_id); }
void effect_add_sound_cue(struct Effect *self, int kind, const char *wave, int frame, float height)
{ (void)self; event("cue %d %s %d %.0f", kind, wave ? wave : "-", frame, height); }
unsigned char effect_mgr_is_registered(struct EffectMgr *self, int type) { (void)self; event("registered %x", type); return 0; }
unsigned char effect_type_is_listed(int type) { event("listed %x", type); return 0; }
struct SkillActionMgr *skill_action_mgr_instance(void) { return (struct SkillActionMgr *)&lift_action_mgr; }
unsigned char skill_action_mgr_use(struct SkillActionMgr *self, int skill_id, int level, SkillActor *caster, SkillActor *target)
{ (void)self; event("action use %x %d %s %s", skill_id, level, who(caster), who(target)); return 0; }
unsigned char skill_action_mgr_hit(struct SkillActionMgr *self, int skill_id, SkillActor *caster, SkillActor *target, SkillActor *unit)
{ (void)self; event("action hit %x %s %s %s", skill_id, who(caster), who(target), who(unit)); return 0; }
struct SkillActionAnswer *skill_action_mgr_use_action(struct SkillActionMgr *self, struct SkillActionAnswer *out, int skill_id)
{ (void)self; (void)skill_id; out->found = 0; out->value = 0; return out; }
unsigned char skill_action_mgr_holds_ground_pose(struct SkillActionMgr *self, int skill_id) { (void)self; (void)skill_id; return 0; }
int session_get_begin_effect_id(int id) { (void)id; return -1; }
int session_is_monster_actor(struct Session *self, struct GameActor *actor) { (void)self; (void)actor; return 0; }
int session_is_pc_job(struct Session *self, int job) { (void)self; (void)job; return 1; }
int session_is_mercenary_or_homun_s_job(struct Session *self, int id) { (void)self; (void)id; return 0; }
void session_clear_homun_commands(struct Session *self) { (void)self; event("clear homun"); }
void session_get_monster_skill_info(struct Session *self, const char *monsterName, int skillId, int *action, int *loop_type)
{ (void)self; (void)monsterName; (void)skillId; *action = -1; *loop_type = 0; }
const char *session_get_job_name(struct Session *self, unsigned job, int sex) { (void)self; (void)job; (void)sex; return ""; }
float game_mode_get_view_heading(struct GameMode *self) { (void)self; return 0.0f; }
struct String *game_mode_get_actor_name(struct GameMode *self, struct String *out, int aid)
{ (void)self; (void)aid; memset(out, 0, sizeof *out); return out; }
void render_object_set_script_step_frames(struct RenderObject *self, unsigned char frames) { event("step %s %d", who(self), frames); }
BYTE ui_window_mgr_post_quit1(struct UIWindowMgr *self, int window_id) { (void)self; event("quit1 %x", window_id); return 1; }
int ui_window_mgr_send_msg_(struct UIWindowMgr *self, int msg, int val1, int val2, int val3, int val4)
{ (void)self; event("wndmsg %x '%s' %x %x %x", msg, msg == 1 ? (const char *)val1 : "", val2, val3, val4); return 0; }
static int msg_str_named;
char *msg_str(int id)
{
    static char text[16];
    if (msg_str_named) snprintf(text, sizeof text, "msg%d", id); else text[0] = 0;
    return text;
}
char game_mode_launch_effect_at(struct GameMode *mode, int effect_id, struct GameActor *caster, struct GameActor *target, int level)
{ (void)mode; event("launch-at %x %s %s %d", effect_id, who(caster), who(target), level); return 1; }
BYTE skill_effect_on_caster(int skill, struct GameActor *caster, struct GameActor *target)
{ event("on-caster %x %s %s", skill, who(caster), who(target)); return 0; }
int game_send_message(int message, int a, int b, int c, int d)
{ event("modemsg %x %x %s %x %x", message, a, who((void *)b), c, d); return 0; }
struct GameMode *game_current_mode(void) { return &lift_mode; }
struct Player *game_current_player(void) { return (struct Player *)lift_player; }
int game_mode_send_msg(struct Mode *mode, int msg, int a, int b, int c, int d)
{ (void)mode; event("modemsg %x %x %s %x %x", msg, a, who((void *)b), c, d); return 0; }
static int (*job_hook)(const struct GameActor *actor);
int game_actor_get_job(struct GameActor *self) { return job_hook ? job_hook(self) : self->job; }
int game_actor_get_counter(struct GameActor *self, int key) { (void)self; (void)key; return 0; }
void game_actor_set_counter(struct GameActor *self, int which, long long value) { event("counter %s %d %lld", who(self), which, value); }
void string_free(struct String *s) { (void)s; }
float c3d_attr_get_height(struct C3dAttr *self, float x, float z) { (void)self; (void)x; (void)z; return 0.0f; }
void rag_effect_send_msg(struct RagEffect *self, struct RenderObject *sender, RenderMsgArg msg, RenderMsgArg a, RenderMsgArg b, RenderMsgArg c)
{ (void)sender; event("ragmsg %s %s", who(self), msg_text(msg, a, b, c, 0)); }
void skill_cast_chat_line(struct GameActor *caster, struct GameActor *target, int skill_id, unsigned char quiet)
{ event("chat %x %s %s %d", skill_id, who(caster), who(target), quiet); }
struct GroundScope *ground_scope_new(void)
{ memset(lift_scope, 0, sizeof lift_scope); *(const struct RenderObjectOps **)lift_scope = &lift_scope_ops; return (struct GroundScope *)lift_scope; }
void *list_insert(struct List *list, struct ListNode *where, const void *value, size_t size) { (void)where; (void)size; list->size++; return (void *)value; }
unsigned char skill_action_mgr_cast(struct SkillActionMgr *self, int skill_id, unsigned cast_time, SkillActor *caster)
{ (void)self; event("action cast %x %u %s", skill_id, cast_time, who(caster)); return 0; }
unsigned char skill_action_mgr_cast_on_target(struct SkillActionMgr *self, int skill_id, SkillActor *target)
{ (void)self; event("action cast-on %x %s", skill_id, who(target)); return 0; }
unsigned char skill_action_mgr_cast_at_cell(struct SkillActionMgr *self, int skill_id, unsigned cast_time, short x, short y)
{ (void)self; event("action cast-at %x %u %d %d", skill_id, cast_time, x, y); return 0; }
unsigned char skill_info_is_good_skill_for_actor(int skill_id) { (void)skill_id; return 1; }
void render_object_set_motion_delay(struct RenderObject *self, float delay) { event("mdelay %s %.2f", who(self), delay); }
struct SkillActionAnswer *skill_action_mgr_cast_action(struct SkillActionMgr *self, struct SkillActionAnswer *out, int skill_id)
{ (void)self; (void)skill_id; out->found = 0; out->value = 0; return out; }
static void *(*mem_alloc_hook)(size_t bytes);
void *mem_alloc(size_t bytes)
{
    void *memory = mem_alloc_hook ? mem_alloc_hook(bytes) : NULL;
    return memory ? memory : malloc(bytes);
}
void mem_free(void *memory) { free(memory); }
static void lift_set_attack_speed(struct GameActor *actor, int value) { event("aspd %s %d", who(actor), value); }

static int lift_mode_send(struct Mode *mode, int message, int a, int b, int c, int d)
{ (void)mode; event("modemsg %x %x %s %x %x", message, a, who((void *)b), c, d); return 0; }
static void lift_send_msg(struct GameObject *object, struct GameObject *sender, RenderMsgArg msg, RenderMsgArg p1, RenderMsgArg p2, RenderMsgArg p3, RenderMsgArg p4)
{ (void)sender; event("msg %s %s", who(object), msg_text(msg, p1, p2, p3, p4)); }
static void lift_set_action(struct RenderObject *self, int action, int type) { event("setaction %s %d %d", who(self), action, type); }
static void lift_set_state(struct GameActor *actor, int state, unsigned char snap) { event("state %s %d %d", who(actor), state, snap); }
static void lift_effect_delay(struct Effect *self, int ms) { event("delay %s %d", who(self), ms); }
static void lift_effect_zoom(struct Effect *self, float zoom) { event("zoom %s %.2f", who(self), zoom); }
static void lift_effect_spin(struct Effect *self, float degrees) { event("spin %s %.2f", who(self), degrees); }
static void lift_effect_init(struct Effect *self) { event("init %s", who(self)); }
static void lift_effect_stop_frame(struct Effect *self, float frame) { event("stopframe %s %.2f", who(self), frame); }


static void *__fastcall o_get_mode(void *self, void *edx) { (void)self; (void)edx; return orig_mode; }
static void *__fastcall o_actor_or_player(void *self, void *edx, int aid)
{
    (void)self; (void)edx;
    if (aid == AID_CASTER) return orig_caster;
    if (aid == AID_TARGET) return orig_target;
    if (aid == AID_OWN) return orig_player;
    return NULL;
}
static void *__fastcall o_actor_by_aid(void *self, void *edx, int aid)
{
    (void)self; (void)edx;
    if (aid == AID_CASTER) return orig_caster;
    if (aid == AID_TARGET) return orig_target;
    return NULL;
}
static void *__fastcall o_skill_by_aid(void *self, void *edx, int aid) { (void)self; (void)edx; (void)aid; return NULL; }
static void *__fastcall o_launch(void *self, void *edx, int effect, float x, float y, float z, float rotation)
{ (void)edx; (void)x; (void)y; (void)z; (void)rotation; event("launch %x %s", effect, who(self)); return orig_rag; }
static void *__fastcall o_spawn(void *self, void *edx, int aid, int type, float x, float y, float z, int target, int level, int reuse, int key)
{ (void)self; (void)edx; (void)reuse; (void)key; event("spawn %x aid %d tgt %d lvl %d at %.0f %.0f %.0f", type, aid, target, level, x, y, z); return orig_effect; }
static void __fastcall o_play_wave(void *self, void *edx, const char *name, float x, float y, float z, int max_distance, int full_distance, float volume)
{ (void)self; (void)edx; (void)max_distance; (void)full_distance; (void)volume; event("wave %s %.0f %.0f %.0f", name, x, y, z); }
static int __fastcall o_motion(void *self, void *edx, int action, int motion, int last, int close_action, int close_motion, int timed, int duration, int loop)
{ (void)edx; event("motion %s %x %d %d %d %d %d %d %d", who(self), action & 0xff, motion & 0xff, last & 0xff, close_action & 0xff, close_motion & 0xff, timed, duration, loop); return 0; }
static int __fastcall o_turn_toward(void *self, void *edx, float x, float z) { (void)edx; event("turn %s %.0f %.0f", who(self), x, z); return 0; }
static int __fastcall o_is_effect_exist(void *self, void *edx, int effect) { (void)edx; event("exist %x %s", effect, who(self)); return 0; }
static void *__fastcall o_find_effect(void *self, void *edx, int aid, int type) { (void)self; (void)edx; event("find %x aid %d", type, aid); return NULL; }
static int __fastcall o_servant_count(void *self, void *edx) { (void)self; (void)edx; return 0; }
static int __fastcall o_servant_fire(void *self, void *edx, int effect) { (void)self; (void)edx; event("servant fire %x", effect); return 0; }
static void *__fastcall o_effect_cue(void *self, void *edx, int kind, const char *wave, int frame, int height)
{ (void)self; (void)edx; event("cue %d %s %d %.0f", kind, wave ? wave : "-", frame, (double)height); return NULL; }
static int __fastcall o_is_registered(void *self, void *edx, int type) { (void)self; (void)edx; event("registered %x", type); return 0; }
static int __fastcall o_action_use(void *self, void *edx, int skill, int level, void *caster, void *target)
{ (void)self; (void)edx; event("action use %x %d %s %s", skill, level, who(caster), who(target)); return 0; }
static int __fastcall o_action_hit(void *self, void *edx, int skill, void *caster, void *target, void *unit)
{ (void)self; (void)edx; event("action hit %x %s %s %s", skill, who(caster), who(target), who(unit)); return 0; }
static int __stdcall o_is_monster_actor(void *actor) { (void)actor; return 0; }
static int __stdcall o_is_pc_job(int job) { (void)job; return 1; }
static const char *__fastcall o_job_name(void *self, void *edx, int job, int sex) { (void)self; (void)edx; (void)job; (void)sex; return ""; }
static void __fastcall o_monster_skill_info(void *self, void *edx, const char *name, int skill, int *action, int *loop_type)
{ (void)self; (void)edx; (void)name; (void)skill; *action = -1; *loop_type = 0; }
static int __fastcall o_get_effect_state(void *self, void *edx) { (void)self; (void)edx; return 0; }
static double __fastcall o_height(void *self, void *edx, float x, float z) { (void)self; (void)edx; (void)x; (void)z; return 0.0; }
static void *__fastcall o_get_actor_name(void *self, void *edx, unsigned *out, int aid)
{ (void)self; (void)edx; (void)aid; memset(out, 0, 24); out[5] = 15; return out; }
static int __fastcall o_get_counter(void *self, void *edx, int key) { (void)self; (void)edx; (void)key; return 0; }
static void __fastcall o_set_counter(void *counters, void *edx, int key, unsigned lo, int hi)
{ (void)edx; event("counter %s %d %lld", who((char *)counters - ORIG_ACTOR_COUNTERS), key, ((long long)hi << 32) | lo); }
static int __cdecl o_clear_homun(void) { event("clear homun"); return 0; }
static double __fastcall o_view_heading(void *self, void *edx) { (void)self; (void)edx; return 0.0; }
static int __fastcall o_step_frames(void *self, void *edx, int frames) { (void)edx; event("step %s %d", who(self), frames); return 0; }
static int __fastcall o_wndmgr_send_msg(void *self, void *edx, int msg, int val1, int val2, int val3, int val4)
{ (void)self; (void)edx; event("wndmsg %x '%s' %x %x %x", msg, msg == 1 ? (const char *)val1 : "", val2, val3, val4); return 0; }
static const char *__cdecl o_msg_str(int id) { return msg_str(id); }
static int __fastcall o_launch_effect_at(void *self, void *edx, int effect, void *caster, void *target, int level)
{ (void)self; (void)edx; event("launch-at %x %s %s %d", effect, who(caster), who(target), level); return 1; }
static int __cdecl o_effect_on_caster(int skill, int level, void *caster, void *target)
{ (void)level; event("on-caster %x %s %s", skill, who(caster), who(target)); return 0; }

static int __fastcall o_mode_send_msg(void *self, void *edx, int msg, int a, int b, int c, int d)
{ (void)self; (void)edx; event("modemsg %x %x %s %x %x", msg, a, who((void *)b), c, d); return 0; }
static void __fastcall o_actor_send_msg(void *self, void *edx, void *sender, long long msg, long long p1, long long p2, long long p3, long long p4)
{ (void)edx; (void)sender; event("msg %s %s", who(self), msg_text(msg, p1, p2, p3, p4)); }
static void __fastcall o_set_action(void *self, void *edx, int action, int type, int flag) { (void)edx; (void)type; event("setaction %s %d %d", who(self), action, flag); }
static void __fastcall o_set_state(void *self, void *edx, int state, int snap) { (void)edx; event("state %s %d %d", who(self), state, snap); }
static void __fastcall o_rag_send_msg(void *self, void *edx, void *sender, long long msg, long long a, long long b, long long c)
{ (void)edx; (void)sender; event("ragmsg %s %s", who(self), msg_text(msg, a, b, c, 0)); }
static void __fastcall o_effect_delay(void *self, void *edx, int ms) { (void)edx; event("delay %s %d", who(self), ms); }
static void __fastcall o_effect_zoom(void *self, void *edx, float zoom) { (void)edx; event("zoom %s %.2f", who(self), zoom); }
static void __fastcall o_effect_spin(void *self, void *edx, float degrees) { (void)edx; event("spin %s %.2f", who(self), degrees); }
static void __fastcall o_effect_init(void *self, void *edx) { (void)edx; event("init %s", who(self)); }
static void __fastcall o_effect_stop_frame(void *self, void *edx, float frame) { (void)edx; event("stopframe %s %.2f", who(self), frame); }
static int __cdecl o_chat_line(void *caster, void *target, int skill, int quiet)
{ event("chat %x %s %s %d", skill & 0xffff, who(caster), who(target), quiet & 0xff); return 0; }
static void *__fastcall o_ground_scope_ctor(void *self, void *edx) { (void)edx; *(void **)self = orig_ground_vt; orig_scope = self; return self; }
static int __fastcall o_action_cast(void *self, void *edx, int skill, unsigned time, void *caster)
{ (void)self; (void)edx; event("action cast %x %u %s", skill, time, who(caster)); return 0; }
static int __fastcall o_action_cast_on_target(void *self, void *edx, int skill, void *caster, void *target)
{ (void)self; (void)edx; (void)caster; event("action cast-on %x %s", skill, who(target)); return 0; }
static int __fastcall o_action_cast_at_cell(void *self, void *edx, int skill, unsigned time, void *target, int x, int y)
{ (void)self; (void)edx; (void)target; event("action cast-at %x %u %d %d", skill, time, (short)x, (short)y); return 0; }
static void __fastcall o_motion_delay(void *self, void *edx, float delay) { (void)edx; event("mdelay %s %.2f", who(self), delay); }
static void __fastcall o_set_attack_speed(void *self, void *edx, int value) { (void)edx; event("aspd %s %d", who(self), value); }
static int __fastcall o_get_job(void *self, void *edx) { (void)self; (void)edx; return 0; }
static void __fastcall o_client_coor(void *self, void *edx, int cx, int cy, float *x, float *z)
{
    float half_cell = (float)ATTR_ZOOM * 8.0f * 0.0625f;
    (void)self; (void)edx;
    *x = ((float)cx - (float)ATTR_ZOOM * 0.5f) * (float)ATTR_ZOOM + half_cell;
    *z = ((float)cy - (float)ATTR_ZOOM * 0.5f) * (float)ATTR_ZOOM + half_cell;
}
static void __fastcall o_server_coor(void *self, void *edx, float x, float z, int *cx, int *cy)
{
    (void)self; (void)edx;
    *cx = (int)(float)floor((double)((float)(ATTR_ZOOM / 2) + x / (float)ATTR_ZOOM));
    *cy = (int)(float)floor((double)((float)(ATTR_ZOOM / 2) + z / (float)ATTR_ZOOM));
}
static DWORD WINAPI o_time(void) { return 0x10000000u; }
void *oracle_time_slot = (void *)o_time;
#pragma comment(linker, "/alternatename:__imp__timeGetTime@0=_oracle_time_slot")

static int trap_slot;
static void *trap_self;
static jmp_buf *trap_jump;
static void __cdecl trap_hit(void)
{
    if (trap_jump) {
        event("trap slot %d %s", trap_slot, who(trap_self));
        longjmp(*trap_jump, 1);
    }
    fprintf(stderr, "  the original called vtable slot %d (offset 0x%x) on %s, which the test does not provide\n", trap_slot,
            trap_slot * 4, who(trap_self));
    fflush(stderr);
    ExitProcess(3);
}
#define TRAP(n) static void __cdecl trap_##n(void) { void *self; __asm { mov self, ecx } trap_slot = n; trap_self = self; trap_hit(); }
TRAP(0) TRAP(1) TRAP(2) TRAP(3) TRAP(4) TRAP(5) TRAP(6) TRAP(7) TRAP(8) TRAP(9) TRAP(10) TRAP(11) TRAP(12) TRAP(13) TRAP(14) TRAP(15)
TRAP(16) TRAP(17) TRAP(18) TRAP(19) TRAP(20) TRAP(21) TRAP(22) TRAP(23) TRAP(24) TRAP(25) TRAP(26) TRAP(27) TRAP(28) TRAP(29) TRAP(30) TRAP(31)
TRAP(32) TRAP(33) TRAP(34) TRAP(35) TRAP(36) TRAP(37) TRAP(38) TRAP(39) TRAP(40) TRAP(41) TRAP(42) TRAP(43) TRAP(44) TRAP(45) TRAP(46) TRAP(47)
TRAP(48) TRAP(49) TRAP(50) TRAP(51) TRAP(52) TRAP(53) TRAP(54) TRAP(55) TRAP(56) TRAP(57) TRAP(58) TRAP(59) TRAP(60) TRAP(61) TRAP(62) TRAP(63)
static void *const traps[64] = {
    trap_0, trap_1, trap_2, trap_3, trap_4, trap_5, trap_6, trap_7, trap_8, trap_9, trap_10, trap_11, trap_12, trap_13, trap_14, trap_15,
    trap_16, trap_17, trap_18, trap_19, trap_20, trap_21, trap_22, trap_23, trap_24, trap_25, trap_26, trap_27, trap_28, trap_29, trap_30, trap_31,
    trap_32, trap_33, trap_34, trap_35, trap_36, trap_37, trap_38, trap_39, trap_40, trap_41, trap_42, trap_43, trap_44, trap_45, trap_46, trap_47,
    trap_48, trap_49, trap_50, trap_51, trap_52, trap_53, trap_54, trap_55, trap_56, trap_57, trap_58, trap_59, trap_60, trap_61, trap_62, trap_63,
};

static LONG WINAPI first_look(EXCEPTION_POINTERS *ep)
{
    unsigned char *at = (unsigned char *)ep->ExceptionRecord->ExceptionAddress;
    unsigned char *image = (unsigned char *)oracle_addr(0x400000u);
    const unsigned *sp = (const unsigned *)ep->ContextRecord->Esp;
    int i;
    fprintf(stderr, "  fault %08lx at %p (original 0x%x) eax=%08lx ecx=%08lx edx=%08lx ebx=%08lx esi=%08lx edi=%08lx ebp=%08lx esp=%08lx\n",
            ep->ExceptionRecord->ExceptionCode, at, (unsigned)(at - image) + 0x400000u, ep->ContextRecord->Eax, ep->ContextRecord->Ecx,
            ep->ContextRecord->Edx, ep->ContextRecord->Ebx, ep->ContextRecord->Esi, ep->ContextRecord->Edi, ep->ContextRecord->Ebp,
            ep->ContextRecord->Esp);
    for (i = 0; i < 96; i++) {
        unsigned char *v = (unsigned char *)sp[i];
        if (v >= image + 0x1000 && v < image + 0x8da000)
            fprintf(stderr, "    stack[%d] -> original 0x%x\n", i, (unsigned)(v - image) + 0x400000u);
    }
    fflush(stderr);
    ExitProcess(4);
    return EXCEPTION_CONTINUE_SEARCH;
}

static int note_fault(EXCEPTION_POINTERS *ep)
{
    fault_code = ep->ExceptionRecord->ExceptionCode;
    fault_va = (unsigned)((unsigned char *)ep->ExceptionRecord->ExceptionAddress - (unsigned char *)oracle_addr(0x400000u)) + 0x400000u;
    return EXCEPTION_EXECUTE_HANDLER;
}

static unsigned call_after(unsigned from, unsigned limit, unsigned target)
{
    const unsigned char *code = (const unsigned char *)oracle_addr(from);
    unsigned i, found = 0;
    for (i = 0; i + 5 <= limit - from; i++) {
        if (code[i] != 0xe8)
            continue;
        if (found) {
            if (i - found <= 16)
                return oracle_call_target(from + i);
            found = 0;
        }
        if (oracle_call_target(from + i) == target)
            found = i;
    }
    return 0;
}

static unsigned call_target_by_prefix(unsigned from, unsigned limit, const unsigned char *prefix, unsigned len)
{
    const unsigned char *code = (const unsigned char *)oracle_addr(from);
    unsigned i;
    for (i = 0; i + len + 5 <= limit - from; i++)
        if (memcmp(code + i, prefix, len) == 0 && code[i + len] == 0xe8)
            return oracle_call_target(from + i + len);
    return 0;
}

static unsigned resolve_or_die(const char *what, const unsigned char *prefix, unsigned len, unsigned expected, int verbose)
{
    unsigned target = call_target_by_prefix(ORIG_SHOW_SKILL_USE, ORIG_SHOW_SKILL_USE_END, prefix, len);
    if (!target) {
        fprintf(stderr, "the call to %s was not found in the skill-use handler\n", what);
        ExitProcess(2);
    }
    if (verbose || target != expected)
        fprintf(stderr, "%s is at 0x%x%s\n", what, target, target == expected ? "" : " (not the address the test expected)");
    return target;
}

static void joined(char *out, size_t size)
{
    int i;
    out[0] = 0;
    for (i = 0; i < event_count; i++) {
        strncat(out, events[i], size - strlen(out) - 1);
        strncat(out, "; ", size - strlen(out) - 1);
    }
    if (overflow)
        strncat(out, "...", size - strlen(out) - 1);
}

static void seed_orig_actor(unsigned char *actor, int aid, float x, float y, float z)
{
    memset(actor, 0, 0x1000);
    *(void **)actor = orig_actor_vt;
    *(int *)(actor + ORIG_ACTOR_AID) = aid;
    *(float *)(actor + ORIG_ACTOR_POS) = x;
    *(float *)(actor + ORIG_ACTOR_POS + 4) = y;
    *(float *)(actor + ORIG_ACTOR_POS + 8) = z;
}

static void seed_lift_actor(struct GameActor *actor, int aid, float x, float y, float z)
{
    memset(actor, 0, sizeof *actor);
    actor->ops = (const struct GameObjectOps *)&lift_actor_ops;
    actor->aid = aid;
    actor->pos.x = x;
    actor->pos.y = y;
    actor->pos.z = z;
}

static void seed_all(int option)
{
    int i;
    memset(orig_mode, 0, sizeof orig_mode);
    memset(orig_world, 0, sizeof orig_world);
    memset(orig_rag, 0, sizeof orig_rag);
    memset(orig_effect, 0, sizeof orig_effect);
    *(void **)orig_mode = orig_mode_vt;
    *(void **)(orig_mode + ORIG_MODE_WORLD) = orig_world;
    *(void **)(orig_world + ORIG_WORLD_PLAYER) = orig_player;
    for (i = 0; i < (int)(sizeof orig_attr / sizeof *orig_attr); i++)
        orig_attr[i] = ATTR_ZOOM;
    *(void **)(orig_world + ORIG_WORLD_ATTR) = orig_attr;
    *(void **)orig_rag = orig_rag_vt;
    *(void **)orig_effect = orig_effect_vt;
    orig_list_head[0] = orig_list_head[1] = (unsigned)(UINT_PTR)orig_list_head;
    orig_list_head[2] = 0;
    *(void **)(orig_world + ORIG_WORLD_OBJECTS) = orig_list_head;
    *(unsigned *)(orig_world + ORIG_WORLD_OBJECTS + 4) = 0;
    orig_scope = NULL;
    seed_orig_actor(orig_caster, AID_CASTER, 100.0f, 5.0f, 200.0f);
    seed_orig_actor(orig_target, AID_TARGET, 130.0f, 7.0f, 260.0f);
    memset(orig_player, 0, sizeof orig_player);
    *(void **)orig_player = orig_actor_vt;
    *(int *)(orig_player + ORIG_ACTOR_AID) = AID_OWN;
    *(float *)(orig_player + ORIG_ACTOR_POS) = 10.0f;
    *(float *)(orig_player + ORIG_ACTOR_POS + 8) = 20.0f;
    *(int *)oracle_addr(ORIG_OWN_AID) = AID_OWN;
    *(int *)oracle_addr(ORIG_EFFECT_OPTION) = option;
    *(void **)oracle_addr(ORIG_EFFECT_MGR_PTR) = orig_mgr;

    memset(&lift_mode, 0, sizeof lift_mode);
    memset(&lift_world, 0, sizeof lift_world);
    memset(&lift_rag, 0, sizeof lift_rag);
    memset(&lift_effect_storage, 0, sizeof lift_effect_storage);
    memset(lift_player, 0, sizeof lift_player);
    lift_mode.base.ops = &lift_mode_ops;
    lift_mode.world = &lift_world;
    lift_world.player = (struct Player *)lift_player;
    memset(&lift_attr, 0, sizeof lift_attr);
    lift_attr.zoom = lift_attr.width = lift_attr.height = ATTR_ZOOM;
    lift_world.attr = &lift_attr;
    memset(lift_scope, 0, sizeof lift_scope);
    lift_effect.ops = &lift_effect_ops;
    seed_lift_actor(&lift_caster, AID_CASTER, 100.0f, 5.0f, 200.0f);
    seed_lift_actor(&lift_target, AID_TARGET, 130.0f, 7.0f, 260.0f);
    ((struct GameActor *)lift_player)->ops = (const struct GameObjectOps *)&lift_actor_ops;
    ((struct GameActor *)lift_player)->aid = AID_OWN;
    ((struct GameActor *)lift_player)->pos.x = 10.0f;
    ((struct GameActor *)lift_player)->pos.z = 20.0f;
    memset(&g_session1, 0, sizeof g_session1);
    g_session1.ownAid = AID_OWN;
    memset(&g_windowMgr, 0, sizeof g_windowMgr);
    g_session1.map_rule_flags[0] = option;
    g_effectMgr = (struct EffectMgr *)&lift_mgr;
}

static void skill_oracle_setup(int verbose)
{
    int i;
    if (verbose) {
        AddVectoredExceptionHandler(1, first_look);
        fprintf(stderr, "lift offsets: aid 0x%x pos 0x%x action 0x%x job 0x%x roty 0x%x\n", (unsigned)offsetof(struct GameActor, aid),
                (unsigned)offsetof(struct GameActor, pos), (unsigned)offsetof(struct GameActor, action),
                (unsigned)offsetof(struct GameActor, job), (unsigned)offsetof(struct GameActor, roty));
    }
    oracle_patch_return(ORIG_GET_BEGIN_EFFECT, -1, 4);
    oracle_patch_jump(ORIG_GET_GAME_MODE, (void *)o_get_mode);
    oracle_patch_jump(ORIG_ACTOR_OR_PLAYER, (void *)o_actor_or_player);
    oracle_patch_jump(ORIG_ACTOR_BY_AID, (void *)o_actor_by_aid);
    {
        static const unsigned char skill_by_aid_site[] = { 0x8b, 0x88, 0xcc, 0x00, 0x00, 0x00 };
        static const unsigned char is_effect_exist_site[] = { 0x68, 0x8a, 0x01, 0x00, 0x00, 0x8b, 0xcf };
        static const unsigned char step_frames_site[] = { 0x6a, 0x1e, 0x8b, 0xcf };
        oracle_patch_jump(resolve_or_die("the skill-by-aid lookup", skill_by_aid_site, sizeof skill_by_aid_site, ORIG_SKILL_BY_AID, verbose),
                          (void *)o_skill_by_aid);
        oracle_patch_jump(resolve_or_die("the effect-exists test", is_effect_exist_site, sizeof is_effect_exist_site, ORIG_IS_EFFECT_EXIST, verbose),
                          (void *)o_is_effect_exist);
        oracle_patch_jump(resolve_or_die("the step-frames setter", step_frames_site, sizeof step_frames_site, ORIG_STEP_FRAMES, verbose),
                          (void *)o_step_frames);
    }
    oracle_patch_jump(ORIG_LAUNCH_EFFECT, (void *)o_launch);
    oracle_patch_jump(ORIG_SPAWN, (void *)o_spawn);
    oracle_patch_jump(ORIG_PLAY_WAVE, (void *)o_play_wave);
    oracle_patch_jump(ORIG_PLAY_MOTION, (void *)o_motion);
    oracle_patch_jump(ORIG_TURN_TOWARD, (void *)o_turn_toward);
    oracle_patch_jump(ORIG_FIND_EFFECT, (void *)o_find_effect);
    oracle_patch_jump(ORIG_SERVANT_COUNT, (void *)o_servant_count);
    oracle_patch_jump(ORIG_SERVANT_FIRE, (void *)o_servant_fire);
    oracle_patch_jump(ORIG_EFFECT_CUE, (void *)o_effect_cue);
    oracle_patch_jump(ORIG_IS_REGISTERED, (void *)o_is_registered);
    oracle_patch_jump(ORIG_ACTION_USE, (void *)o_action_use);
    oracle_patch_jump(ORIG_ACTION_HIT, (void *)o_action_hit);
    oracle_patch_jump(ORIG_IS_MONSTER_ACTOR, (void *)o_is_monster_actor);
    oracle_patch_jump(ORIG_IS_PC_JOB, (void *)o_is_pc_job);
    oracle_patch_jump(ORIG_JOB_NAME, (void *)o_job_name);
    oracle_patch_jump(ORIG_GET_ACTOR_NAME, (void *)o_get_actor_name);
    oracle_patch_jump(ORIG_ATTR_HEIGHT, (void *)o_height);
    {
        unsigned monster_skill_info = call_after(ORIG_SHOW_SKILL_USE, ORIG_SHOW_SKILL_USE + 0x400, ORIG_JOB_NAME);
        unsigned get_counter = call_after(ORIG_COUNTED_EFFECT, ORIG_COUNTED_EFFECT_END, ORIG_LAUNCH_EFFECT);
        if (!monster_skill_info || !get_counter) {
            fprintf(stderr, "the monster skill info call or the counter getter was not found\n");
            ExitProcess(2);
        }
        if (verbose)
            fprintf(stderr, "monster skill info at 0x%x, counter getter at 0x%x\n", monster_skill_info, get_counter);
        oracle_patch_jump(monster_skill_info, (void *)o_monster_skill_info);
        oracle_patch_jump(get_counter, (void *)o_get_counter);
        oracle_patch_jump(ORIG_SET_COUNTER, (void *)o_set_counter);
    }
    oracle_patch_jump(ORIG_CLEAR_HOMUN, (void *)o_clear_homun);
    oracle_patch_jump(ORIG_VIEW_HEADING, (void *)o_view_heading);
    oracle_patch_jump(ORIG_WNDMGR_SEND_MSG, (void *)o_wndmgr_send_msg);
    oracle_patch_jump(ORIG_MSG_STR, (void *)o_msg_str);
    oracle_patch_jump(ORIG_LAUNCH_EFFECT_AT, (void *)o_launch_effect_at);
    oracle_patch_jump(ORIG_EFFECT_ON_CASTER, (void *)o_effect_on_caster);
    oracle_patch_jump(ORIG_CHAT_LINE, (void *)o_chat_line);
    oracle_patch_jump(ORIG_GROUND_SCOPE_CTOR, (void *)o_ground_scope_ctor);
    oracle_patch_jump(ORIG_ACTION_CAST, (void *)o_action_cast);
    oracle_patch_jump(ORIG_ACTION_CAST_ON, (void *)o_action_cast_on_target);
    oracle_patch_jump(ORIG_ACTION_CAST_AT, (void *)o_action_cast_at_cell);
    oracle_patch_jump(ORIG_MOTION_DELAY, (void *)o_motion_delay);
    oracle_patch_jump(ORIG_GET_JOB, (void *)o_get_job);
    oracle_patch_jump(ORIG_GET_CLIENT_COOR, (void *)o_client_coor);
    oracle_patch_jump(ORIG_MODE_SERVER_COOR, (void *)o_server_coor);
    oracle_patch_jump(ORIG_WORLD_SERVER_COOR, (void *)o_server_coor);
    oracle_patch_return(ORIG_IS_GOOD_SKILL, 1, 0);
    if (!oracle_bind_import("WINMM.dll", "timeGetTime", (void *)o_time))
        fprintf(stderr, "the original's timeGetTime import was not found\n");
    memcpy(oracle_addr(ORIG_DURATIONS), g_effectDurations, sizeof g_effectDurations);

    for (i = 0; i < 64; i++)
        orig_mode_vt[i] = orig_actor_vt[i] = orig_rag_vt[i] = orig_effect_vt[i] = orig_ground_vt[i] = traps[i];
    orig_ground_vt[ORIG_ACTOR_VT_SEND_MSG] = (void *)o_actor_send_msg;
    orig_effect_vt[ORIG_EFFECT_VT_INIT] = (void *)o_effect_init;
    orig_effect_vt[ORIG_EFFECT_VT_STOP_FRAME] = (void *)o_effect_stop_frame;
    lift_scope_ops.base.send_msg = lift_send_msg;
    lift_actor_ops.set_attack_speed = lift_set_attack_speed;
    orig_mode_vt[ORIG_MODE_VT_SEND_MSG] = (void *)o_mode_send_msg;
    orig_actor_vt[ORIG_ACTOR_VT_SEND_MSG] = (void *)o_actor_send_msg;
    orig_actor_vt[ORIG_ACTOR_VT_SET_ACTION] = (void *)o_set_action;
    orig_actor_vt[ORIG_ACTOR_VT_SET_STATE] = (void *)o_set_state;
    orig_actor_vt[ORIG_ACTOR_VT_EFFECT_STATE] = (void *)o_get_effect_state;
    orig_actor_vt[ORIG_ACTOR_VT_ATTACK_SPEED] = (void *)o_set_attack_speed;
    orig_rag_vt[ORIG_RAG_VT_SEND_MSG] = (void *)o_rag_send_msg;
    orig_effect_vt[ORIG_EFFECT_VT_DELAY] = (void *)o_effect_delay;
    orig_effect_vt[ORIG_EFFECT_VT_SPIN] = (void *)o_effect_spin;
    orig_effect_vt[ORIG_EFFECT_VT_ZOOM] = (void *)o_effect_zoom;
    lift_mode_ops.send_message = lift_mode_send;
    lift_actor_ops.base.base.send_msg = lift_send_msg;
    lift_actor_ops.base.set_action = lift_set_action;
    lift_actor_ops.set_state = lift_set_state;
    lift_effect_ops.set_start_delay = lift_effect_delay;
    lift_effect_ops.set_zoom = lift_effect_zoom;
    lift_effect_ops.set_spin = lift_effect_spin;
    lift_effect_ops.init = lift_effect_init;
    lift_effect_ops.set_stop_frame = lift_effect_stop_frame;
}

#endif
