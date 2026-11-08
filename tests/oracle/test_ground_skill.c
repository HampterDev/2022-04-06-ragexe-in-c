#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "oracle_image.h"
#include "custom_removed.h"
#include "session/state.h"
#include "session/session_skills.h"
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
#include "game/world_objects.h"
#include "world/state.h"
#include "resources/attr_queries.h"
#include "resources/animation.h"
#include "effects/effect.h"
#include "effects/effect_mgr.h"
#include "effects/durations.h"
#include "effects/rag/rag_effect.h"
#include "contents/sound.h"

#define ORIG_AM_GROUNDSKILL   0x765b90u
#define ORIG_CALL_GET_MODE    0x765ba1u
#define ORIG_GET_BEGIN_EFFECT 0x875350u
#define ORIG_GET_CLIENT_COOR  0x76ec30u
#define ORIG_ATTR_HEIGHT      0x9f2780u
#define ORIG_HOLDS_POSE       0xa138f0u
#define ORIG_GET_ACT_RES      0x74a920u
#define ORIG_FRAME_COUNT      0x9f1340u
#define ORIG_TURN_TOWARD      0x748960u
#define ORIG_LUA_GROUND_FX    0x88aba0u
#define ORIG_DUMMY_EFFECT     0x779bc0u
#define ORIG_SPAWN            0x96aca0u
#define ORIG_LAUNCH_EFFECT    0x74bf80u
#define ORIG_PLAY_MOTION      0x75f0c0u
#define ORIG_PLAY_WAVE        0x90e8a0u
#define ORIG_EFFECT_MGR_PTR   0x115c6c0u
#define ORIG_DURATIONS        0xe907a0u

#define ACTOR_ATTACK_TARGET   0x250
#define ACTOR_ACTION          0x38
#define ACTOR_MOTION          0x3c
#define VTABLE_SET_STATE      (0x3c / 4)

#define SKILL_LIMIT  0x1600
#define MAX_REPORTED 40
#define EVENTS 32
#define EVENT_LEN 64

struct Session g_session1;
struct EffectMgr *g_effectMgr;

static char events[EVENTS][EVENT_LEN];
static int event_count;
static unsigned mismatches, faults, reported, fault_code, fault_va, launches, skills_with_launches;

static void event(const char *fmt, ...)
{
    va_list ap;
    if (event_count >= EVENTS)
        return;
    va_start(ap, fmt);
    vsnprintf(events[event_count++], EVENT_LEN, fmt, ap);
    va_end(ap);
}

static struct GameMode lift_mode;
static struct World lift_world;
static struct GameActor lift_actor;
static struct GameActorOps lift_ops;
static int lift_act, lift_mgr;

int session_get_begin_effect_id(int id) { (void)id; return -1; }
struct SkillActionMgr *skill_action_mgr_instance(void) { return (struct SkillActionMgr *)&lift_mgr; }
struct SkillActionAnswer *skill_action_mgr_use_action(struct SkillActionMgr *self, struct SkillActionAnswer *out, int skill_id)
{ (void)self; (void)skill_id; out->found = 0; out->value = 0; return out; }
unsigned char skill_action_mgr_holds_ground_pose(struct SkillActionMgr *self, int skill_id) { (void)self; (void)skill_id; return 0; }
struct GameMode *game_current_mode(void) { return &lift_mode; }
void game_mode_cell_to_world(struct GameMode *self, int x, int y, float *world_x, float *world_z)
{ (void)self; *world_x = (float)x * 5.0f; *world_z = (float)y * 5.0f; }
float c3d_attr_get_height(struct C3dAttr *self, float x, float z) { (void)self; (void)x; (void)z; return 0.0f; }
void *render_object_get_act_res(struct RenderObject *self) { (void)self; return &lift_act; }
unsigned actres_frame_count(const struct ActRes *self, unsigned action) { (void)self; (void)action; return 1; }
void render_object_turn_toward(struct RenderObject *self, float x, float z) { (void)self; (void)x; (void)z; event("turn"); }
static void lift_set_state(struct GameActor *actor, int state, unsigned char snap) { (void)actor; (void)snap; event("state %d", state); }
BYTE session_play_ground_skill_effect(struct Session *self, int skill, struct Vec3 pos) { (void)self; (void)skill; (void)pos; return 0; }
unsigned long long game_mode_launch_dummy_effect(struct GameMode *mode, const Vec3 *at, int effect_id, int frames, int value)
{ (void)mode; (void)at; (void)value; event("dummy %x for %d", effect_id, frames); return 0; }
struct Effect *effect_mgr_spawn(struct EffectMgr *self, int aid, int type, Vec3 pos, int target_aid, int level, unsigned char reuse, int instance_key)
{ (void)self; (void)aid; (void)pos; (void)target_aid; (void)reuse; (void)instance_key; event("spawn %x level %d", type, level); return NULL; }
void sound_play_wave(const char *name, float x, float y, float z, int max_distance, int full_distance, float volume)
{ (void)x; (void)y; (void)z; (void)max_distance; (void)full_distance; (void)volume; event("wave %s", name); }
struct RagEffect *game_actor_launch_effect(struct GameActor *self, int effect_id, Vec3 offset, float rotation)
{ (void)self; (void)offset; (void)rotation; event("launch %x", effect_id); return NULL; }
void render_object_play_motion_range(struct RenderObject *self, unsigned char action, unsigned char motion, unsigned char last,
                                     unsigned char close_action, unsigned char close_motion, int timed, int duration, int loop)
{ (void)self; (void)close_action; (void)close_motion; (void)timed; (void)duration; (void)loop; event("motion %x %d %d", action, motion, last); }
void rag_effect_send_msg(struct RagEffect *self, struct RenderObject *sender, RenderMsgArg msg, RenderMsgArg a, RenderMsgArg b, RenderMsgArg c)
{ (void)self; (void)sender; (void)a; (void)b; (void)c; event("ragmsg %x", (int)msg); }

static unsigned char orig_mode[0x2000], orig_world[0x1000], orig_actor[0x1000], orig_mgr[0x100], orig_act[0x100];
static void *orig_vtable[64];

static void *__fastcall o_get_mode(void *self, void *edx) { (void)self; (void)edx; return orig_mode; }
static void __fastcall o_client_coor(void *self, void *edx, int cx, int cy, float *x, float *z)
{ (void)self; (void)edx; *x = (float)cx * 5.0f; *z = (float)cy * 5.0f; }
static double __fastcall o_height(void *self, void *edx, float x, float z) { (void)self; (void)edx; (void)x; (void)z; return 0.0; }
static int __fastcall o_holds_pose(void *self, void *edx, int skill) { (void)self; (void)edx; (void)skill; return 0; }
static void *__fastcall o_get_act_res(void *self) { (void)self; return orig_act; }
static int __fastcall o_frame_count(void *self, void *edx, int action) { (void)self; (void)edx; (void)action; return 1; }
static int __fastcall o_turn_toward(void *self, void *edx, float x, float z) { (void)self; (void)edx; (void)x; (void)z; event("turn"); return 0; }
static void __fastcall o_set_state(void *self, void *edx, int state, int snap) { (void)self; (void)edx; (void)snap; event("state %d", state); }
static int __fastcall o_lua_ground_fx(void *self, void *edx, int skill, float x, float y, float z)
{ (void)self; (void)edx; (void)skill; (void)x; (void)y; (void)z; return 0; }
static int __fastcall o_dummy(void *self, void *edx, const float *at, int kind, int frames, int flag)
{ (void)self; (void)edx; (void)at; (void)flag; event("dummy %x for %d", kind, frames); return 0; }
static void *__fastcall o_spawn(void *self, void *edx, int aid, int type, float x, float y, float z, int target, int level, int reuse, int key)
{ (void)self; (void)edx; (void)aid; (void)x; (void)y; (void)z; (void)target; (void)reuse; (void)key; event("spawn %x level %d", type, level); return NULL; }
static void *__fastcall o_launch(void *self, void *edx, int effect, float x, float y, float z, float rotation)
{ (void)self; (void)edx; (void)x; (void)y; (void)z; (void)rotation; event("launch %x", effect); return NULL; }
static int __fastcall o_motion(void *self, void *edx, int action, int motion, int last, int close_action, int close_motion, int timed, int duration, int loop)
{ (void)self; (void)edx; (void)close_action; (void)close_motion; (void)timed; (void)duration; (void)loop; event("motion %x %d %d", action, motion, last); return 0; }
static void __fastcall o_play_wave(void *self, void *edx, const char *name, float x, float y, float z, int max_distance, int full_distance, float volume)
{ (void)self; (void)edx; (void)x; (void)y; (void)z; (void)max_distance; (void)full_distance; (void)volume; event("wave %s", name); }
static void __cdecl o_trap(void) { fprintf(stderr, "  the original called an actor method the test does not provide\n"); ExitProcess(3); }

typedef void (__fastcall *groundskill_fn)(void *self, void *edx, int a0, struct GroundSkillInfo *rec, int a2, int a3);

static LONG WINAPI first_look(EXCEPTION_POINTERS *ep)
{
    unsigned char *at = (unsigned char *)ep->ExceptionRecord->ExceptionAddress;
    unsigned char *image = (unsigned char *)oracle_addr(0x400000u);
    fprintf(stderr, "  fault %08lx at %p (original 0x%x) eax=%08lx ecx=%08lx esp=%08lx\n", ep->ExceptionRecord->ExceptionCode, at,
            (unsigned)(at - image) + 0x400000u, ep->ContextRecord->Eax, ep->ContextRecord->Ecx, ep->ContextRecord->Esp);
    {
        const unsigned *sp = (const unsigned *)ep->ContextRecord->Esp;
        int i;
        for (i = 0; i < 96; i++) {
            unsigned char *v = (unsigned char *)sp[i];
            if (v >= image + 0x1000 && v < image + 0x8da000)
                fprintf(stderr, "    stack[%d] -> original 0x%x\n", i, (unsigned)(v - image) + 0x400000u);
        }
        fflush(stderr);
        ExitProcess(4);
    }
    fflush(stderr);
    return EXCEPTION_CONTINUE_SEARCH;
}

static int note_fault(EXCEPTION_POINTERS *ep)
{
    fault_code = ep->ExceptionRecord->ExceptionCode;
    fault_va = (unsigned)((unsigned char *)ep->ExceptionRecord->ExceptionAddress - (unsigned char *)oracle_addr(0x400000u)) + 0x400000u;
    return EXCEPTION_EXECUTE_HANDLER;
}

static int call_original(groundskill_fn f, struct GroundSkillInfo *rec)
{
    __try { f(orig_actor, NULL, 0, rec, 0, 0); return 1; }
    __except (note_fault(GetExceptionInformation())) { return 0; }
}

static void joined(char *out, size_t size)
{
    int i;
    out[0] = 0;
    for (i = 0; i < event_count; i++) {
        strncat(out, events[i], size - strlen(out) - 1);
        strncat(out, "; ", size - strlen(out) - 1);
    }
}

static unsigned removed_skills;

int main(int argc, char **argv)
{
    const char *exe = argc > 1 ? argv[1] : getenv("ORACLE_EXE");
    static const int levels[] = { 1, 5, 6, 10 };
    groundskill_fn orig;
    int skill, li, compared = 0, i;

    if (!exe) {
        fprintf(stderr, "usage: test_ground_skill <original exe>\n");
        return 2;
    }
    if (!oracle_load(exe))
        return 2;
    if (getenv("ORACLE_VERBOSE"))
        AddVectoredExceptionHandler(1, first_look);

    if (getenv("ORACLE_VERBOSE")) { fprintf(stderr, "patching; the mode getter is at 0x%x\n", oracle_call_target(ORIG_CALL_GET_MODE)); fflush(stderr); }
    oracle_patch_return(ORIG_GET_BEGIN_EFFECT, -1, 4);
    oracle_patch_jump(oracle_call_target(ORIG_CALL_GET_MODE), (void *)o_get_mode);
    oracle_patch_jump(ORIG_GET_CLIENT_COOR, (void *)o_client_coor);
    oracle_patch_jump(ORIG_ATTR_HEIGHT, (void *)o_height);
    oracle_patch_jump(ORIG_HOLDS_POSE, (void *)o_holds_pose);
    oracle_patch_jump(ORIG_GET_ACT_RES, (void *)o_get_act_res);
    oracle_patch_jump(ORIG_FRAME_COUNT, (void *)o_frame_count);
    oracle_patch_jump(ORIG_TURN_TOWARD, (void *)o_turn_toward);
    oracle_patch_jump(ORIG_LUA_GROUND_FX, (void *)o_lua_ground_fx);
    oracle_patch_jump(ORIG_DUMMY_EFFECT, (void *)o_dummy);
    oracle_patch_jump(ORIG_SPAWN, (void *)o_spawn);
    oracle_patch_jump(ORIG_LAUNCH_EFFECT, (void *)o_launch);
    oracle_patch_jump(ORIG_PLAY_MOTION, (void *)o_motion);
    oracle_patch_jump(ORIG_PLAY_WAVE, (void *)o_play_wave);
    if (getenv("ORACLE_VERBOSE")) {
        static const unsigned patched[] = { ORIG_GET_CLIENT_COOR, ORIG_ATTR_HEIGHT, ORIG_HOLDS_POSE, ORIG_GET_ACT_RES,
                                            ORIG_FRAME_COUNT, ORIG_TURN_TOWARD, ORIG_LUA_GROUND_FX, ORIG_DUMMY_EFFECT, ORIG_SPAWN,
                                            ORIG_LAUNCH_EFFECT, ORIG_PLAY_MOTION, ORIG_PLAY_WAVE };
        for (i = 0; i < (int)(sizeof patched / sizeof *patched); i++)
            fprintf(stderr, "  0x%x: %02x\n", patched[i], *(unsigned char *)oracle_addr(patched[i]));
        fprintf(stderr, "patched; seeding\n"); fflush(stderr);
    }
    *(void **)oracle_addr(ORIG_EFFECT_MGR_PTR) = orig_mgr;
    memcpy(oracle_addr(ORIG_DURATIONS), g_effectDurations, sizeof g_effectDurations);
    *(void **)(orig_mode + 0xcc) = orig_world;
    for (i = 0; i < 64; i++)
        orig_vtable[i] = (void *)o_trap;
    orig_vtable[VTABLE_SET_STATE] = (void *)o_set_state;
    orig = (groundskill_fn)oracle_addr(ORIG_AM_GROUNDSKILL);

    if (getenv("ORACLE_VERBOSE")) { fprintf(stderr, "seeded\n"); fflush(stderr); }
    memset(&g_session1, 0, sizeof g_session1);
    lift_mode.world = &lift_world;
    lift_ops.set_state = lift_set_state;
    g_effectMgr = (struct EffectMgr *)&lift_mgr;

    for (skill = 0; skill < SKILL_LIMIT; skill++) {
        if (custom_removed_skill(skill)) {
            removed_skills++;
            continue;
        }
        for (li = 0; li < (int)(sizeof levels / sizeof *levels); li++) {
            struct GroundSkillInfo rec;
            char orig_text[EVENTS * EVENT_LEN], lift_text[EVENTS * EVENT_LEN];
            char orig_pose[32], lift_pose[32];

            rec.skill_id = skill;
            rec.start_time = 0;
            rec.x = 100;
            rec.y = 120;
            rec.level = levels[li];
            compared++;
            if (getenv("ORACLE_VERBOSE")) {
                fprintf(stderr, "skill 0x%x level %d\n", skill, rec.level);
                fflush(stderr);
            }

            memset(orig_actor, 0, sizeof orig_actor);
            *(void **)orig_actor = orig_vtable;
            *(int *)(orig_actor + ACTOR_ACTION) = 7;
            *(int *)(orig_actor + ACTOR_MOTION) = 3;
            *(int *)(orig_actor + ACTOR_ATTACK_TARGET) = 99;
            event_count = 0;
            if (!call_original(orig, &rec)) {
                faults++;
                if (reported++ < MAX_REPORTED)
                    printf("  skill 0x%x level %d: the original faulted (code %08x at 0x%x)\n", skill, rec.level, fault_code, fault_va);
                continue;
            }
            if (getenv("ORACLE_VERBOSE")) { fprintf(stderr, "  original returned\n"); fflush(stderr); }
            launches += event_count;
            if (event_count && li == 0)
                skills_with_launches++;
            joined(orig_text, sizeof orig_text);
            snprintf(orig_pose, sizeof orig_pose, "action %d motion %d target %d", *(int *)(orig_actor + ACTOR_ACTION),
                     *(int *)(orig_actor + ACTOR_MOTION), *(int *)(orig_actor + ACTOR_ATTACK_TARGET));

            memset(&lift_actor, 0, sizeof lift_actor);
            lift_actor.ops = (const struct GameObjectOps *)&lift_ops;
            lift_actor.action = 7;
            lift_actor.motion = 3;
            lift_actor.attack_target_aid = 99;
            event_count = 0;
            game_actor_am_groundskill(&lift_actor, &rec);
            if (getenv("ORACLE_VERBOSE")) { fprintf(stderr, "  lift returned\n"); fflush(stderr); }
            joined(lift_text, sizeof lift_text);
            snprintf(lift_pose, sizeof lift_pose, "action %d motion %d target %d", lift_actor.action, lift_actor.motion,
                     lift_actor.attack_target_aid);

            if (strcmp(orig_text, lift_text) != 0 || strcmp(orig_pose, lift_pose) != 0) {
                mismatches++;
                if (reported++ < MAX_REPORTED)
                    printf("  skill 0x%x level %d:\n    original: %s | %s\n    lift:     %s | %s\n", skill, rec.level,
                           orig_text, orig_pose, lift_text, lift_pose);
            }
        }
    }
    printf("  %u skill inputs left out: removed on custom (custom_removed.h)\n", removed_skills);
    printf("Ground skill table: %d calls compared against the original (%u launches, waves, poses and motions recorded for %u skills), "
           "%u mismatches, %u faults%s\n", compared, launches, skills_with_launches, mismatches, faults,
           reported > MAX_REPORTED ? " (first 40 shown)" : "");
    return mismatches || faults ? 1 : 0;
}
