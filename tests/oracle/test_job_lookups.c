/* test_job_lookups.c - the job, party and pet lookups of the original against the lift.
 *
 * Every routine here takes one integer on the stack (stdcall) and answers from its own tables:
 * the job remap, the mounted-job map, the six job-class predicates, the maximum job level (which
 * reads eight globals the client fills from Lua's MaxLevelTable; both sides are seeded with the
 * same distinct values), the party colour by sequence number and the pet act text. The lifted
 * functions are copied out of their files by extract_functions.py (job_lookups.inc), so nothing
 * else of the tree is linked. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "oracle_image.h"
#include "session/state.h"
#include "session/session_jobs.h"
#include "session/session_names.h"
#include "session/session_party.h"
#include "session/session_talk.h"
#include "session/session_state.h"
#include "job_lookups.inc"

struct Session g_session1;

#define ORIG_REMAP_JOB       0x8760c0u
#define ORIG_MOUNTED_JOB     0x878cb0u
#define ORIG_IS_SUPER_NOVICE 0x8840d0u
#define ORIG_IS_EXPANDED_SN  0x8840a0u
#define ORIG_IS_FOURTH       0x869980u
#define ORIG_IS_THIRD        0x8698e0u
#define ORIG_IS_EXPANDED     0x884160u
#define ORIG_IS_DORAM        0x890310u
#define ORIG_MAX_JOB_LEVEL   0x88dec0u
#define ORIG_PARTY_COLOR     0x855840u
#define ORIG_PET_ACT_TEXT    0x8771d0u

/* the MaxLevelTable globals the maximum-job-level routine reads, in the order the client's Lua loader fills them */
#define ORIG_JOB_LEVEL_NOVICE   0x113b564u
#define ORIG_JOB_LEVEL_SN       0x113b568u
#define ORIG_JOB_LEVEL_BASE     0x113b56cu
#define ORIG_JOB_LEVEL_2ND      0x113b570u
#define ORIG_JOB_LEVEL_3RD      0x113b574u
#define ORIG_JOB_LEVEL_UPPER    0x113b57cu
#define ORIG_JOB_LEVEL_DORAM    0x113b584u
#define ORIG_JOB_LEVEL_4TH      0x113b5d0u

#define JOB_FIRST  -2
#define JOB_LAST   0x2400
#define MAX_REPORTED 40

typedef int (__stdcall *int_fn)(int);

static unsigned mismatches, faults, reported;
static unsigned fault_code, fault_va;

static int note_fault(EXCEPTION_POINTERS *ep)
{
    fault_code = ep->ExceptionRecord->ExceptionCode;
    fault_va = (unsigned)((unsigned char *)ep->ExceptionRecord->ExceptionAddress - (unsigned char *)oracle_addr(0x400000u)) + 0x400000u;
    return EXCEPTION_EXECUTE_HANDLER;
}

static int call1(int_fn f, int arg, int *out)
{
    __try { *out = f(arg); return 1; }
    __except (note_fault(GetExceptionInformation())) { return 0; }
}

static void differ(const char *what, int arg, int orig, int lift)
{
    mismatches++;
    if (reported++ < MAX_REPORTED)
        printf("  %s(0x%x): original %d (0x%x), lift %d (0x%x)\n", what, arg, orig, orig, lift, lift);
}

static void faulted(const char *what, int arg)
{
    faults++;
    if (reported++ < MAX_REPORTED)
        printf("  %s(0x%x): the original faulted (code %08x at 0x%x)\n", what, arg, fault_code, fault_va);
}

/* compares one stdcall(int) routine over [first, last]; bytes_only compares the low byte, for the
 * predicates that answer in al and leave the rest of eax as it was */
static void compare(const char *what, unsigned orig_va, int (*lift)(int), int first, int last, int bytes_only, int *compared)
{
    int_fn orig = (int_fn)oracle_addr(orig_va);
    int arg, o, l;
    for (arg = first; arg <= last; arg++) {
        (*compared)++;
        if (!call1(orig, arg, &o)) { faulted(what, arg); continue; }
        l = lift(arg);
        if (bytes_only ? (unsigned char)o != (unsigned char)l : o != l)
            differ(what, arg, bytes_only ? (unsigned char)o : o, bytes_only ? (unsigned char)l : l);
    }
}

static int lift_remap(int job)        { return session_remap_job(&g_session1, job); }
static int lift_mounted(int job)      { return session_get_mounted_job(job); }
static int lift_super_novice(int job) { return session_is_super_novice_job(&g_session1, job); }
static int lift_expanded_sn(int job)  { return session_is_expanded_super_novice_job(&g_session1, job); }
static int lift_fourth(int job)       { return session_is_fourth_job(&g_session1, job); }
static int lift_third(int job)        { return session_is_third_job(&g_session1, job); }
static int lift_expanded(int job)     { return session_is_expanded_job(&g_session1, job); }
static int lift_doram(int job)        { return session_is_doram_job(&g_session1, job); }
static int lift_max_level(int job)    { return session_get_max_job_level(&g_session1, job); }
static int lift_party_color(int seq)  { return (int)session_get_party_color_by_seq(&g_session1, (unsigned)seq); }

int main(int argc, char **argv)
{
    const char *exe = argc > 1 ? argv[1] : getenv("ORACLE_EXE");
    int compared = 0, action;
    int_fn orig_pet_text;

    if (!exe) {
        fprintf(stderr, "usage: test_job_lookups <original exe>\n");
        return 2;
    }
    if (!oracle_load(exe))
        return 2;
    memset(&g_session1, 0, sizeof g_session1);

    /* the same eight distinct levels on both sides */
    *(int *)oracle_addr(ORIG_JOB_LEVEL_NOVICE) = g_session1.jobLevelNovice = 61;
    *(int *)oracle_addr(ORIG_JOB_LEVEL_SN) = g_session1.jobLevelSuperNovice = 62;
    *(int *)oracle_addr(ORIG_JOB_LEVEL_BASE) = g_session1.jobLevelBase = 63;
    *(int *)oracle_addr(ORIG_JOB_LEVEL_2ND) = g_session1.jobLevel2nd = 64;
    *(int *)oracle_addr(ORIG_JOB_LEVEL_3RD) = g_session1.jobLevel3rd = 65;
    *(int *)oracle_addr(ORIG_JOB_LEVEL_UPPER) = g_session1.jobLevelUpperJob = 66;
    *(int *)oracle_addr(ORIG_JOB_LEVEL_DORAM) = g_session1.jobLevelDoram = 67;
    *(int *)oracle_addr(ORIG_JOB_LEVEL_4TH) = g_session1.jobLevel4th = 68;

    compare("remap_job", ORIG_REMAP_JOB, lift_remap, JOB_FIRST, JOB_LAST, 0, &compared);
    compare("mounted_job", ORIG_MOUNTED_JOB, lift_mounted, JOB_FIRST, JOB_LAST, 0, &compared);
    compare("is_super_novice", ORIG_IS_SUPER_NOVICE, lift_super_novice, JOB_FIRST, JOB_LAST, 1, &compared);
    compare("is_expanded_super_novice", ORIG_IS_EXPANDED_SN, lift_expanded_sn, JOB_FIRST, JOB_LAST, 1, &compared);
    compare("is_fourth_job", ORIG_IS_FOURTH, lift_fourth, JOB_FIRST, JOB_LAST, 1, &compared);
    compare("is_third_job", ORIG_IS_THIRD, lift_third, JOB_FIRST, JOB_LAST, 1, &compared);
    compare("is_expanded_job", ORIG_IS_EXPANDED, lift_expanded, JOB_FIRST, JOB_LAST, 1, &compared);
    compare("is_doram_job", ORIG_IS_DORAM, lift_doram, JOB_FIRST, JOB_LAST, 1, &compared);
    compare("max_job_level", ORIG_MAX_JOB_LEVEL, lift_max_level, JOB_FIRST, JOB_LAST, 0, &compared);
    compare("party_color_by_seq", ORIG_PARTY_COLOR, lift_party_color, -1, 64, 0, &compared);

    orig_pet_text = (int_fn)oracle_addr(ORIG_PET_ACT_TEXT);
    for (action = -2; action <= 40; action++) {
        int text;
        const char *lift = session_get_pet_act_text(action);
        compared++;
        if (!call1(orig_pet_text, action, &text)) { faulted("pet_act_text", action); continue; }
        if (!text || strcmp((const char *)text, lift) != 0) {
            mismatches++;
            if (reported++ < MAX_REPORTED)
                printf("  pet_act_text(%d): original \"%s\", lift \"%s\"\n", action, text ? (const char *)text : "(null)", lift);
        }
    }

    printf("Job lookups: %d calls compared against the original, %u mismatches, %u faults%s\n", compared, mismatches,
           faults, reported > MAX_REPORTED ? " (first 40 shown)" : "");
    return mismatches || faults ? 1 : 0;
}
