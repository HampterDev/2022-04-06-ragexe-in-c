/* test_skill_poses.c - the skill use and cast pose lookups of the original against the lift.
 *
 * session_get_use_effect lifts sub_87af10 and session_get_cast_effect lifts the routine at
 * 0x8797a0: given a skill, both answer the effect launched and the actor state (pose) taken,
 * through switches of several hundred cases and jump tables. Both begin by asking the Lua skill
 * table for a begin effect (sub_875350, session_get_begin_effect_id); the original's routine is
 * patched to answer "none" and the lift's is stubbed the same way, so the switches are compared
 * on their own. Skill ids from 0x1519 on go to the skill-action registry, stubbed here, so the
 * comparison stops below it. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "oracle_image.h"
#include "session/state.h"
#include "session/session_skills.h"
#include "skills/skill_answer.h"
#include "skills/skill_action.h"
#include "skills/skill_action_registry.h"
#include "contents/sound.h"

#define ORIG_GET_BEGIN_EFFECT_ID 0x875350u   /* stdcall(int skill) -> int, -1 for none */
#define ORIG_USE_EFFECT          0x87af10u   /* stdcall(int skill, int *effect, int *action) */
#define ORIG_CAST_EFFECT         0x8797a0u   /* stdcall(int skill, int *effect, int *action, int level, int job) */
#define SKILL_LIMIT              0x1519      /* the skill-action registry serves the ids from here on */
#define LEVELS                   8
#define MAX_REPORTED             40

typedef void (__stdcall *use_fn)(int skill, int *effect, int *action);
typedef void (__stdcall *cast_fn)(int skill, int *effect, int *action, int level, int job);

struct Session g_session1;

/* the lift's dependencies, held to the same answers the patched original gives */
int session_get_begin_effect_id(int id) { (void)id; return -1; }
struct SkillActionMgr *skill_action_mgr_instance(void) { static int mgr; return (struct SkillActionMgr *)&mgr; }
struct SkillActionAnswer *skill_action_mgr_use_action(struct SkillActionMgr *self, struct SkillActionAnswer *out, int skill_id)
{ (void)self; (void)skill_id; out->found = 0; out->value = 0; return out; }
struct SkillActionAnswer *skill_action_mgr_cast_action(struct SkillActionMgr *self, struct SkillActionAnswer *out, int skill_id)
{ (void)self; (void)skill_id; out->found = 0; out->value = 0; return out; }
void sound_play_wave(const char *name, float x, float y, float z, int max_distance, int full_distance, float volume)
{ (void)name; (void)x; (void)y; (void)z; (void)max_distance; (void)full_distance; (void)volume; }

static unsigned mismatches, reported, faults;

/* a fault inside the original is reported with its address in the original's image and counted */
static unsigned fault_code, fault_va;
static int note_fault(EXCEPTION_POINTERS *ep)
{
    fault_code = ep->ExceptionRecord->ExceptionCode;
    fault_va = (unsigned)((unsigned char *)ep->ExceptionRecord->ExceptionAddress - (unsigned char *)oracle_addr(0x400000u)) + 0x400000u;
    return EXCEPTION_EXECUTE_HANDLER;
}
static int call_use(use_fn f, int skill, int *e, int *a)
{
    __try { f(skill, e, a); return 1; }
    __except (note_fault(GetExceptionInformation())) { return 0; }
}
static int call_cast(cast_fn f, int skill, int *e, int *a, int level, int job)
{
    __try { f(skill, e, a, level, job); return 1; }
    __except (note_fault(GetExceptionInformation())) { return 0; }
}
static void report_fault(const char *what, int skill, int level)
{
    faults++;
    if (reported++ < MAX_REPORTED)
        printf("  %s skill 0x%x level %d: the original faulted (code %08x at 0x%x)\n", what, skill, level, fault_code, fault_va);
}

static void report(const char *what, int skill, int level, int oe, int oa, int le, int la)
{
    mismatches++;
    if (reported++ < MAX_REPORTED)
        printf("  %s skill 0x%x level %d: original effect %d action %d, lift effect %d action %d\n",
               what, skill, level, oe, oa, le, la);
}

int main(int argc, char **argv)
{
    const char *exe = argc > 1 ? argv[1] : getenv("ORACLE_EXE");
    use_fn orig_use;
    cast_fn orig_cast;
    int skill, level, compared = 0;

    if (!exe) {
        fprintf(stderr, "usage: test_skill_poses <original exe>\n");
        return 2;
    }
    if (!oracle_load(exe))
        return 2;
    if (getenv("ORACLE_VERBOSE")) {
        const unsigned char *p = oracle_addr(ORIG_USE_EFFECT);
        setvbuf(stdout, NULL, _IONBF, 0);
        printf("image at %p; bytes at 0x%x: %02x %02x %02x %02x %02x\n", oracle_addr(0x400000u), ORIG_USE_EFFECT,
               p[0], p[1], p[2], p[3], p[4]);
    }
    oracle_patch_return(ORIG_GET_BEGIN_EFFECT_ID, -1, 4);
    orig_use = (use_fn)oracle_addr(ORIG_USE_EFFECT);
    orig_cast = (cast_fn)oracle_addr(ORIG_CAST_EFFECT);
    memset(&g_session1, 0, sizeof g_session1);

    for (skill = 0; skill < SKILL_LIMIT; skill++) {
        int oe = 0, oa = 0, le = 0, la = 0;
        if (getenv("ORACLE_VERBOSE") && skill % 0x100 == 0)
            printf("skill 0x%x\n", skill);
        compared++;
        if (!call_use(orig_use, skill, &oe, &oa)) {
            report_fault("use", skill, -1);
        } else {
            session_get_use_effect(&g_session1, skill, &le, &la);
            if (oe != le || oa != la)
                report("use", skill, -1, oe, oa, le, la);
        }
        for (level = 0; level < LEVELS; level++) {
            oe = oa = le = la = 0;
            compared++;
            if (!call_cast(orig_cast, skill, &oe, &oa, level, 0)) {
                report_fault("cast", skill, level);
                continue;
            }
            session_get_cast_effect(&g_session1, skill, &le, &la, level, 0);
            if (oe != le || oa != la)
                report("cast", skill, level, oe, oa, le, la);
        }
    }
    printf("Skill poses: %d calls compared against the original, %u mismatches, %u faults%s\n", compared, mismatches,
           faults, reported > MAX_REPORTED ? " (first 40 shown)" : "");
    return mismatches || faults ? 1 : 0;
}
