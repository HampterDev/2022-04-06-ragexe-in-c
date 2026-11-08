#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "oracle_image.h"
#include "session/state.h"
#include "effects/rag/rag_effect.h"
#include "effects/durations.h"

#define ORIG_INIT            0x6e8c70u
#define ORIG_RES_MGR         0x5032b0u
#define ORIG_RES_GET         0x500d00u
#define ORIG_EFFECT_OPTION   0x1138cbcu
#define ORIG_DURATIONS       0xe907a0u
#define ORIG_VT_SET_DURATION (0x48 / 4)
#define ORIG_THIS_POS        0x10
#define ORIG_THIS_MASTER     0x138
#define ORIG_THIS_MASTER_ROTY 0x14c
#define ORIG_THIS_EFFECT_ID  0x168
#define ORIG_THIS_OFFSET     0x180
#define ORIG_THIS_STR_RES    0x7ec
#define ORIG_THIS_STR_MAX_LOOPS 0x11c80
#define ORIG_MASTER_POS      0x10
#define ORIG_MASTER_ROTY     0x4c

#define MAX_REPORTED 60
#define EVENTS 8
#define EVENT_LEN 160
#define MASTER_ROTY 123.0f

struct Session g_session1;

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

static unsigned char orig_effect[0x12000], orig_master[0x100], orig_res_mgr[0x100];
static void *orig_vt[64];
static struct RagEffect lift_effect;
static struct RenderObject lift_master;

void rag_str_load(struct RagEffect *self, const char *name) { (void)self; event("load %s", name); }
void rag_effect_set_duration(struct RagEffect *self, int frame) { (void)self; event("duration %d", frame); }

#include "rag_effect_init.inc"

static void __fastcall o_set_duration(void *self, void *edx, int frame) { (void)self; (void)edx; event("duration %d", frame); }
static void *__cdecl o_res_mgr(void) { return orig_res_mgr; }
static void *__fastcall o_res_get(void *self, void *edx, const char *name) { (void)self; (void)edx; event("load %s", name); return NULL; }
static void __fastcall o_trap(void *self, void *edx) { (void)self; (void)edx; fprintf(stderr, "  a vtable slot other than SetDuration was called\n"); ExitProcess(3); }

typedef void (__fastcall *init_fn)(void *self, void *edx, void *master, int effect_id, float dx, float dy, float dz);

static LONG WINAPI first_look(EXCEPTION_POINTERS *ep)
{
    unsigned char *at = (unsigned char *)ep->ExceptionRecord->ExceptionAddress;
    unsigned char *image = (unsigned char *)oracle_addr(0x400000u);
    const unsigned *sp = (const unsigned *)ep->ContextRecord->Esp;
    int i;
    fprintf(stderr, "  fault %08lx at %p (original 0x%x) eax=%08lx ecx=%08lx esi=%08lx edi=%08lx\n", ep->ExceptionRecord->ExceptionCode, at,
            (unsigned)(at - image) + 0x400000u, ep->ContextRecord->Eax, ep->ContextRecord->Ecx, ep->ContextRecord->Esi, ep->ContextRecord->Edi);
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

static int call_original(init_fn init, int effect_id, Vec3 offset)
{
    __try { init(orig_effect, NULL, orig_master, effect_id, offset.x, offset.y, offset.z); return 1; }
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
    if (overflow)
        strncat(out, "...", size - strlen(out) - 1);
}

static void orig_state(char *out, size_t size)
{
    const float *pos = (const float *)(orig_effect + ORIG_THIS_POS), *off = (const float *)(orig_effect + ORIG_THIS_OFFSET);
    snprintf(out, size, "id %d master %d pos %.0f %.0f %.0f roty %.1f offset %.0f %.0f %.0f res %d loops %d",
             *(int *)(orig_effect + ORIG_THIS_EFFECT_ID), *(void **)(orig_effect + ORIG_THIS_MASTER) == orig_master, pos[0], pos[1], pos[2],
             *(float *)(orig_effect + ORIG_THIS_MASTER_ROTY), off[0], off[1], off[2], *(void **)(orig_effect + ORIG_THIS_STR_RES) != NULL,
             *(int *)(orig_effect + ORIG_THIS_STR_MAX_LOOPS));
}

static void lift_state(char *out, size_t size)
{
    snprintf(out, size, "id %d master %d pos %.0f %.0f %.0f roty %.1f offset %.0f %.0f %.0f res %d loops %d",
             lift_effect.effect_id, lift_effect.master == &lift_master, lift_effect.base.pos.x, lift_effect.base.pos.y, lift_effect.base.pos.z,
             lift_effect.master_roty, lift_effect.offset.x, lift_effect.offset.y, lift_effect.offset.z, lift_effect.str_res != NULL,
             lift_effect.str_max_loops);
}

int main(int argc, char **argv)
{
    const char *exe = argc > 1 ? argv[1] : getenv("ORACLE_EXE");
    init_fn init;
    Vec3 offset = { 3.0f, 4.0f, 5.0f };
    int effect_id, option, compared = 0, i;
    unsigned mismatches = 0, faults = 0, reported = 0, recorded = 0;
    const int verbose = getenv("ORACLE_VERBOSE") != NULL;

    if (!exe) {
        fprintf(stderr, "usage: test_rag_effect_init <original exe>\n");
        return 2;
    }
    if (!oracle_load(exe))
        return 2;
    if (verbose)
        AddVectoredExceptionHandler(1, first_look);
    for (i = 0; i < 64; i++)
        orig_vt[i] = (void *)o_trap;
    orig_vt[ORIG_VT_SET_DURATION] = (void *)o_set_duration;
    oracle_patch_jump(ORIG_RES_MGR, (void *)o_res_mgr);
    oracle_patch_jump(ORIG_RES_GET, (void *)o_res_get);
    memcpy(oracle_addr(ORIG_DURATIONS), g_effectDurations, sizeof g_effectDurations);
    init = (init_fn)oracle_addr(ORIG_INIT);

    memset(orig_master, 0, sizeof orig_master);
    ((float *)(orig_master + ORIG_MASTER_POS))[0] = 100.0f;
    ((float *)(orig_master + ORIG_MASTER_POS))[1] = 5.0f;
    ((float *)(orig_master + ORIG_MASTER_POS))[2] = 200.0f;
    *(float *)(orig_master + ORIG_MASTER_ROTY) = MASTER_ROTY;
    memset(&lift_master, 0, sizeof lift_master);
    lift_master.pos.x = 100.0f;
    lift_master.pos.y = 5.0f;
    lift_master.pos.z = 200.0f;
    lift_master.roty = MASTER_ROTY;

    for (option = 0; option < 2; option++) {
        for (effect_id = 0; effect_id < EFFECT_DURATION_COUNT; effect_id++) {
            char orig_text[EVENTS * EVENT_LEN], lift_text[EVENTS * EVENT_LEN], orig_fields[200], lift_fields[200];

            compared++;
            if (verbose) { fprintf(stderr, "option %d effect %d\n", option, effect_id); fflush(stderr); }
            memset(orig_effect, 0, sizeof orig_effect);
            *(void **)orig_effect = orig_vt;
            *(int *)oracle_addr(ORIG_EFFECT_OPTION) = option;
            event_count = 0;
            overflow = 0;
            srand(1);
            if (!call_original(init, effect_id, offset)) {
                faults++;
                if (reported++ < MAX_REPORTED)
                    printf("  option %d effect %d: the original faulted (code %08x at 0x%x)\n", option, effect_id, fault_code, fault_va);
                continue;
            }
            recorded += event_count;
            joined(orig_text, sizeof orig_text);
            orig_state(orig_fields, sizeof orig_fields);

            memset(&lift_effect, 0, sizeof lift_effect);
            memset(&g_session1, 0, sizeof g_session1);
            g_session1.map_rule_flags[0] = option;
            event_count = 0;
            overflow = 0;
            srand(1);
            rag_effect_init(&lift_effect, &lift_master, effect_id, offset);
            joined(lift_text, sizeof lift_text);
            lift_state(lift_fields, sizeof lift_fields);

            if (strcmp(orig_text, lift_text) != 0 || strcmp(orig_fields, lift_fields) != 0) {
                mismatches++;
                if (reported++ < MAX_REPORTED)
                    printf("  option %d effect %d:\n    original: %s | %s\n    lift:     %s | %s\n", option, effect_id,
                           orig_text, orig_fields, lift_text, lift_fields);
            }
        }
    }
    printf("Rag effect init: %d calls compared against the original (%u events recorded), %u mismatches, %u faults%s\n", compared, recorded,
           mismatches, faults, reported > MAX_REPORTED ? " (first 60 shown)" : "");
    return mismatches || faults ? 1 : 0;
}
