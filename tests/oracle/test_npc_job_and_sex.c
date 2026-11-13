/* test_npc_job_and_sex.c - CNpc's job-and-sex setter of the original against npc_set_job_and_sex.
 *
 * The original routine (0x82c820, the NPC's vtable slot 0x48) sets the job and the sex through
 * CGameActor's setters, clears the body palette and asks the session (0x8680f0) whether the job
 * has a named body palette, passing a 64-byte name buffer for the answer. The setters and the
 * job getter are stood in on the original's side and recorded; the session's question runs as it
 * is. The lift is copied out of npc.c (npc_job_and_sex.inc) with recording setters. Both sides
 * get jobs -2..0x2400, sexes 0..2 and an actor filled with a pattern; each must set the job and
 * then the sex of that actor once, leave the body palette at 0 and change no other byte. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "oracle_image.h"
#include "actor/types.h"
#include "actor/actor_common.h"
#include "actor/game_actor.h"

#define ORIG_SET_JOB_AND_SEX 0x82c820u
#define ORIG_CALL_SET_JOB    0x82c836u
#define ORIG_CALL_SET_SEX    0x82c840u
#define ORIG_CALL_GET_JOB    0x82c852u
#define ORIG_CALL_PALETTE    0x82c85du
#define ORIG_PALETTE_CHECK   0x8680f0u
#define ORIG_BODY_PALETTE    0x48
#define ORIG_ACTOR_SIZE      0x600
#define JOB_FIRST            -2
#define JOB_LAST             0x2400
#define MAX_REPORTED         40

typedef void (__fastcall *set_job_and_sex_fn)(void *self, void *edx, int job, int sex, int extra);

struct Call {
    char what;
    const void *actor;
    int value;
};

static struct Call calls[4];
static int call_count, job_set;
static unsigned mismatches, faults, reported;
static unsigned fault_code, fault_va;

static void record(char what, const void *actor, int value)
{
    if (call_count < 4) {
        calls[call_count].what = what;
        calls[call_count].actor = actor;
        calls[call_count].value = value;
    }
    call_count++;
}

static void __fastcall o_set_job(void *self, void *edx, int job)
{
    (void)edx;
    job_set = job;
    record('j', self, job);
}

static void __fastcall o_set_sex(void *self, void *edx, int sex)
{
    (void)edx;
    record('s', self, sex);
}

static int __fastcall o_get_job(void *self, void *edx)
{
    (void)self;
    (void)edx;
    return job_set;
}

void game_actor_set_job(struct GameActor *self, int job)
{
    record('j', self, job);
}

void game_actor_set_sex(struct GameActor *self, int value)
{
    record('s', self, value);
}

#include "npc_job_and_sex.inc"

static int note_fault(EXCEPTION_POINTERS *ep)
{
    fault_code = ep->ExceptionRecord->ExceptionCode;
    fault_va = (unsigned)((unsigned char *)ep->ExceptionRecord->ExceptionAddress - (unsigned char *)oracle_addr(0x400000u)) + 0x400000u;
    return EXCEPTION_EXECUTE_HANDLER;
}

static int call_original(set_job_and_sex_fn f, void *actor, int job, int sex)
{
    __try { f(actor, NULL, job, sex, 0x5a5a5a5a); return 1; }
    __except (note_fault(GetExceptionInformation())) { return 0; }
}

static void differ(const char *side, int job, int sex, const char *what)
{
    mismatches++;
    if (reported++ < MAX_REPORTED)
        printf("  %s, job %d sex %d: %s\n", side, job, sex, what);
}

static void fill(unsigned char *bytes, size_t size)
{
    size_t i;

    for (i = 0; i < size; i++)
        bytes[i] = (unsigned char)(0x5b + i * 7);
}

static void check_calls(const char *side, const void *actor, int job, int sex)
{
    if (call_count != 2 || calls[0].what != 'j' || calls[0].actor != actor || calls[0].value != job
        || calls[1].what != 's' || calls[1].actor != actor || calls[1].value != sex)
        differ(side, job, sex, "did not set the job and then the sex of the actor once each");
}

int main(int argc, char **argv)
{
    const char *exe = argc > 1 ? argv[1] : getenv("ORACLE_EXE");
    static unsigned char orig_actor[ORIG_ACTOR_SIZE], orig_before[ORIG_ACTOR_SIZE];
    static struct Npc lift_actor, lift_before;
    set_job_and_sex_fn orig;
    int job, sex, palette, compared = 0;

    if (!exe) {
        fprintf(stderr, "usage: test_npc_job_and_sex <original exe>\n");
        return 2;
    }
    if (!oracle_load(exe))
        return 2;
    if (oracle_call_target(ORIG_CALL_PALETTE) != ORIG_PALETTE_CHECK) {
        printf("NPC job and sex: the call at 0x%x does not go to 0x%x\n", ORIG_CALL_PALETTE, ORIG_PALETTE_CHECK);
        return 1;
    }
    oracle_patch_jump(oracle_call_target(ORIG_CALL_SET_JOB), (void *)o_set_job);
    oracle_patch_jump(oracle_call_target(ORIG_CALL_SET_SEX), (void *)o_set_sex);
    oracle_patch_jump(oracle_call_target(ORIG_CALL_GET_JOB), (void *)o_get_job);
    orig = (set_job_and_sex_fn)oracle_addr(ORIG_SET_JOB_AND_SEX);

    for (job = JOB_FIRST; job <= JOB_LAST; job++) {
        for (sex = 0; sex <= 2; sex++) {
            compared++;
            fill(orig_actor, sizeof orig_actor);
            *(int *)(orig_actor + ORIG_BODY_PALETTE) = 7;
            memcpy(orig_before, orig_actor, sizeof orig_actor);
            call_count = 0;
            if (!call_original(orig, orig_actor, job, sex)) {
                faults++;
                if (reported++ < MAX_REPORTED)
                    printf("  job %d sex %d: the original faulted (code %08x at 0x%x)\n", job, sex, fault_code, fault_va);
                continue;
            }
            check_calls("original", orig_actor, job, sex);
            palette = *(int *)(orig_actor + ORIG_BODY_PALETTE);
            *(int *)(orig_actor + ORIG_BODY_PALETTE) = 7;
            if (palette != 0 || memcmp(orig_actor, orig_before, sizeof orig_actor))
                differ("original", job, sex, "changed more than the body palette, or not to 0");

            fill((unsigned char *)&lift_actor, sizeof lift_actor);
            lift_actor.body_palette = 7;
            memcpy(&lift_before, &lift_actor, sizeof lift_actor);
            call_count = 0;
            npc_set_job_and_sex((struct GameActor *)&lift_actor, job, sex);
            check_calls("lift", &lift_actor, job, sex);
            palette = lift_actor.body_palette;
            lift_actor.body_palette = 7;
            if (palette != 0 || memcmp(&lift_actor, &lift_before, sizeof lift_actor))
                differ("lift", job, sex, "changed more than the body palette, or not to 0");
        }
    }

    printf("NPC job and sex: %d calls compared against the original, %u mismatches, %u faults%s\n", compared, mismatches,
           faults, reported > MAX_REPORTED ? " (first 40 shown)" : "");
    return mismatches || faults ? 1 : 0;
}
