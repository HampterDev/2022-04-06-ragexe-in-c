#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "oracle_image.h"
#include "session/state.h"
#include "session/session_jobs.h"
#include "actor/types.h"
#include "actor/game_actor.h"
#include "effects/skills/ez2str_effect.h"
#include "contents/client_lua_calls.h"

struct Session g_session1;
static int lift_baby, lift_doram;

int client_lua_call(struct ClientLua *lua, const char *function, const char *format, ...) { (void)lua; (void)function; (void)format; return 0; }
BYTE session_is_baby_job(struct Session *self, int id) { (void)self; (void)id; return (BYTE)lift_baby; }
BYTE session_is_doram_job(struct Session *self, int job) { (void)self; (void)job; return (BYTE)lift_doram; }
int game_actor_get_mount_height(struct GameActor *self) { (void)self; return 5; }
int game_actor_get_job(struct GameActor *self) { (void)self; return 0; }

#include "ez2str_head.inc"

#define ORIG_CLASSIFIER     0x998cf0u
#define ORIG_HEAD_X_OFFSET  0x9955e0u
#define ORIG_HEAD_HEIGHT    0x9959b0u
#define ORIG_LUA_CALL       0x93dfc0u
#define ORIG_IS_BABY_JOB    0x883960u
#define ORIG_IS_DORAM_JOB   0x890310u
#define ORIG_MOUNT_HEIGHT   0x74b4e0u
#define ORIG_GET_JOB        0x74aea0u
#define ORIG_ACTOR_RIDING   0x2d0
#define TYPE_LIMIT 0x1000
#define MAX_REPORTED 40

static int orig_baby, orig_doram;
static unsigned fault_code, fault_va;

static int __cdecl o_lua_call(void *lua, const char *function, const char *format, ...) { (void)lua; (void)function; (void)format; return 0; }
static int __stdcall o_is_baby_job(int job) { (void)job; return orig_baby; }
static int __stdcall o_is_doram_job(int job) { (void)job; return orig_doram; }
static int __fastcall o_mount_height(void *self, void *edx) { (void)self; (void)edx; return 5; }
static int __fastcall o_get_job(void *self, void *edx) { (void)self; (void)edx; return 0; }

typedef unsigned char (__fastcall *classifier_fn)(void *self);
typedef float (__cdecl *head_x_offset_fn)(int type, const char *path);
typedef int (__cdecl *head_height_fn)(int type, void *actor);

static int note_fault(EXCEPTION_POINTERS *ep)
{
    fault_code = ep->ExceptionRecord->ExceptionCode;
    fault_va = (unsigned)((unsigned char *)ep->ExceptionRecord->ExceptionAddress - (unsigned char *)oracle_addr(0x400000u)) + 0x400000u;
    return EXCEPTION_EXECUTE_HANDLER;
}

static int call_classifier(classifier_fn f, void *self, unsigned char *out)
{
    __try { *out = f(self); return 1; }
    __except (note_fault(GetExceptionInformation())) { return 0; }
}

static int call_x_offset(head_x_offset_fn f, int type, const char *path, float *out)
{
    __try { *out = f(type, path); return 1; }
    __except (note_fault(GetExceptionInformation())) { return 0; }
}

static int call_height(head_height_fn f, int type, void *actor, int *out)
{
    __try { *out = f(type, actor); return 1; }
    __except (note_fault(GetExceptionInformation())) { return 0; }
}

int main(int argc, char **argv)
{
    const char *exe = argc > 1 ? argv[1] : getenv("ORACLE_EXE");
    static const char *const paths[] = { "", "effect\\strangelights\\a.str", "x\\superstar\\b.str", "ljosalfar.str", "other\\path.str" };
    static unsigned char orig_effect[0x200], orig_actor[0x1000];
    static struct Ez2StrEffect lift_effect;
    static struct GameActor lift_actor;
    classifier_fn classifier;
    head_x_offset_fn x_offset;
    head_height_fn height;
    int type, pi, vi, compared = 0;
    unsigned head_mismatches = 0, headgear_mismatches = 0, x_mismatches = 0, h_mismatches = 0, faults = 0, reported = 0;

    if (!exe) {
        fprintf(stderr, "usage: test_ez2str_head <original exe>\n");
        return 2;
    }
    if (!oracle_load(exe))
        return 2;
    oracle_patch_jump(ORIG_LUA_CALL, (void *)o_lua_call);
    oracle_patch_jump(ORIG_IS_BABY_JOB, (void *)o_is_baby_job);
    oracle_patch_jump(ORIG_IS_DORAM_JOB, (void *)o_is_doram_job);
    oracle_patch_jump(ORIG_MOUNT_HEIGHT, (void *)o_mount_height);
    oracle_patch_jump(ORIG_GET_JOB, (void *)o_get_job);
    classifier = (classifier_fn)oracle_addr(ORIG_CLASSIFIER);
    x_offset = (head_x_offset_fn)oracle_addr(ORIG_HEAD_X_OFFSET);
    height = (head_height_fn)oracle_addr(ORIG_HEAD_HEIGHT);

    for (type = 0; type < TYPE_LIMIT; type++) {
        unsigned char orig_class, lift_head, lift_headgear;

        memset(orig_effect, 0, sizeof orig_effect);
        *(int *)(orig_effect + 4) = type;
        memset(&lift_effect, 0, sizeof lift_effect);
        lift_effect.base.type = type;
        compared++;
        if (!call_classifier(classifier, orig_effect, &orig_class)) {
            faults++;
            if (reported++ < MAX_REPORTED)
                printf("  classifier type %d: the original faulted (code %08x at 0x%x)\n", type, fault_code, fault_va);
            continue;
        }
        lift_head = ez2_str_effect_is_head_type(&lift_effect);
        lift_headgear = ez2_str_effect_is_headgear_type(&lift_effect);
        if (orig_class != lift_head) {
            head_mismatches++;
            if (orig_class != lift_headgear && reported++ < MAX_REPORTED)
                printf("  classifier type %d: original %d, head type %d, headgear type %d\n", type, orig_class, lift_head, lift_headgear);
        }
        if (orig_class != lift_headgear)
            headgear_mismatches++;
    }

    for (type = 0; type < TYPE_LIMIT; type++) {
        for (pi = 0; pi < (int)(sizeof paths / sizeof *paths); pi++) {
            float orig_x, lift_x;
            compared++;
            if (!call_x_offset(x_offset, type, paths[pi], &orig_x)) {
                faults++;
                if (reported++ < MAX_REPORTED)
                    printf("  x offset type %d path %d: the original faulted (code %08x at 0x%x)\n", type, pi, fault_code, fault_va);
                continue;
            }
            lift_x = ez2str_head_x_offset(type, paths[pi]);
            if (orig_x != lift_x) {
                x_mismatches++;
                if (reported++ < MAX_REPORTED)
                    printf("  x offset type %d path '%s': original %.3f, lift %.3f\n", type, paths[pi], orig_x, lift_x);
            }
        }
    }

    for (type = 0; type < TYPE_LIMIT; type++) {
        for (vi = 0; vi < 9; vi++) {
            int orig_h, lift_h;
            void *orig_a = vi ? orig_actor : NULL, *lift_a = vi ? &lift_actor : NULL;
            int riding = vi ? (vi - 1) & 1 : 0;

            orig_baby = lift_baby = vi ? ((vi - 1) >> 1) & 1 : 0;
            orig_doram = lift_doram = vi ? ((vi - 1) >> 2) & 1 : 0;
            memset(orig_actor, 0, sizeof orig_actor);
            orig_actor[ORIG_ACTOR_RIDING] = (unsigned char)riding;
            memset(&lift_actor, 0, sizeof lift_actor);
            lift_actor.mounted = (unsigned char)riding;
            memset(&g_session1, 0, sizeof g_session1);
            compared++;
            if (!call_height(height, type, orig_a, &orig_h)) {
                faults++;
                if (reported++ < MAX_REPORTED)
                    printf("  height type %d variant %d: the original faulted (code %08x at 0x%x)\n", type, vi, fault_code, fault_va);
                continue;
            }
            lift_h = ez2str_head_height(type, lift_a);
            if (orig_h != lift_h) {
                h_mismatches++;
                if (reported++ < MAX_REPORTED)
                    printf("  height type %d variant %d (riding %d baby %d doram %d): original %d, lift %d\n", type, vi, riding, orig_baby,
                           orig_doram, orig_h, lift_h);
            }
        }
    }
    printf("Ez2str head: %d calls compared against the original; the head-type test: %u mismatches (the headgear test would have %u, "
           "so the routine at 0x998cf0 is the head-type test); head x offset: %u mismatches; head height: %u mismatches; %u faults%s\n",
           compared, head_mismatches, headgear_mismatches, x_mismatches, h_mismatches, faults, reported > MAX_REPORTED ? " (first 40 shown)" : "");
    return head_mismatches || x_mismatches || h_mismatches || faults ? 1 : 0;
}
