#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "oracle_image.h"
#include "session/state.h"
#include "effects/skills/ez2str_effect.h"
#include "effects/str_path.h"
#include "fileio/resource_api.h"
#include "contents/client_lua_calls.h"
#include "core/memory.h"
#include "std/string.h"

void ez2_str_particle_load(struct Ez2StrParticle *self, const char *path);
#include "str_path_open.inc"

#define ORIG_OPEN_FOR_TYPE   0x9997a0u
#define ORIG_STR_LOAD        0x9828b0u
#define ORIG_LUA_CALL        0x93dfc0u
#define ORIG_RES_AVAILABLE   0x501540u
#define ORIG_MIN_PATH        0x982340u
#define ORIG_EFFECT_OPTION   0x1138cbcu

#define TYPE_LIMIT 0x1000
#define MAX_REPORTED 60
#define EVENTS 8
#define EVENT_LEN 160

struct Session g_session1;

static char events[EVENTS][EVENT_LEN];
static int event_count;
static unsigned fault_code, fault_va;

static void event(const char *fmt, ...)
{
    va_list ap;
    if (event_count >= EVENTS)
        return;
    va_start(ap, fmt);
    vsnprintf(events[event_count++], EVENT_LEN, fmt, ap);
    va_end(ap);
}

static unsigned char orig_effect[0x400], orig_particle[0x400];
static struct Ez2StrEffect lift_effect;
static unsigned char lift_particle[0x400];
static int lift_res_mgr;

void ez2_str_particle_load(struct Ez2StrParticle *self, const char *path) { (void)self; event("load %s", path); }
int client_lua_call(struct ClientLua *lua, const char *function, const char *format, ...) { (void)lua; (void)function; (void)format; return 0; }
struct ResMgr *g_resMgr(void) { return (struct ResMgr *)&lift_res_mgr; }
unsigned char res_mgr_is_available(struct ResMgr *self, const char *name) { (void)self; (void)name; return 1; }
void *mem_alloc(size_t bytes) { return malloc(bytes); }
void mem_free(void *memory) { free(memory); }
void fatal_error(const char *what) { fprintf(stderr, "fatal: %s\n", what); ExitProcess(5); }

static void __fastcall o_str_load(void *self, void *edx, unsigned s0, unsigned s1, unsigned s2, unsigned s3, unsigned size, unsigned capacity)
{
    unsigned buffer[6] = { s0, s1, s2, s3, size, capacity };
    const char *text = capacity >= 16 ? *(const char **)buffer : (const char *)buffer;
    (void)self; (void)edx;
    event("load %s", text);
}
static int __cdecl o_lua_call(void *lua, const char *function, const char *format, ...) { (void)lua; (void)function; (void)format; return 0; }

static const char *std_string_text(const unsigned *s) { return s[5] >= 16 ? *(const char **)s : (const char *)s; }

static void std_string_set(unsigned *s, const char *text)
{
    size_t n = strlen(text);
    char *heap = (char *)malloc(n + 1);
    memcpy(heap, text, n + 1);
    *(char **)s = heap;
    s[4] = (unsigned)n;
    s[5] = (unsigned)n + 1;
}

static int word_char(char c) { return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_'; }

static unsigned *__stdcall o_min_path(unsigned *out, const unsigned *path)
{
    const char *text = std_string_text(path);
    size_t n = strlen(text), slash = n, i;
    int matches = 0;
    char made[512];

    while (slash > 0 && text[slash - 1] != '\\')
        slash--;
    if (slash >= 2 && n - slash >= 5 && memcmp(text + n - 4, ".str", 4) == 0) {
        matches = 1;
        for (i = 0; i < slash - 1 && matches; i++)
            if (text[i] == '\n' || text[i] == '\r')
                matches = 0;
        for (i = slash; i < n - 4 && matches; i++)
            if (!word_char(text[i]))
                matches = 0;
    }
    if (matches && n + 4 < sizeof made) {
        memcpy(made, text, slash);
        memcpy(made + slash, "min_", 4);
        memcpy(made + slash + 4, text + slash, n - slash + 1);
        std_string_set(out, made);
    } else {
        std_string_set(out, text);
    }
    return out;
}

typedef void (__fastcall *open_for_type_fn)(void *self, void *edx, void *particle, int type);

static LONG WINAPI first_look(EXCEPTION_POINTERS *ep)
{
    unsigned char *at = (unsigned char *)ep->ExceptionRecord->ExceptionAddress;
    unsigned char *image = (unsigned char *)oracle_addr(0x400000u);
    const unsigned *sp = (const unsigned *)ep->ContextRecord->Esp;
    int i;
    fprintf(stderr, "  fault %08lx at %p (original 0x%x) eax=%08lx ecx=%08lx esi=%08lx\n", ep->ExceptionRecord->ExceptionCode, at,
            (unsigned)(at - image) + 0x400000u, ep->ContextRecord->Eax, ep->ContextRecord->Ecx, ep->ContextRecord->Esi);
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

static int call_original(open_for_type_fn f, int type)
{
    __try { f(orig_effect, NULL, orig_particle, type); return 1; }
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

int main(int argc, char **argv)
{
    const char *exe = argc > 1 ? argv[1] : getenv("ORACLE_EXE");
    open_for_type_fn orig;
    int type, option, compared = 0;
    unsigned mismatches = 0, faults = 0, reported = 0, recorded = 0;
    const int verbose = getenv("ORACLE_VERBOSE") != NULL;

    if (!exe) {
        fprintf(stderr, "usage: test_ez2str_files <original exe>\n");
        return 2;
    }
    if (!oracle_load(exe))
        return 2;
    if (verbose)
        AddVectoredExceptionHandler(1, first_look);
    oracle_patch_jump(ORIG_STR_LOAD, (void *)o_str_load);
    oracle_patch_jump(ORIG_LUA_CALL, (void *)o_lua_call);
    oracle_patch_return(ORIG_RES_AVAILABLE, 1, 4);
    oracle_patch_jump(ORIG_MIN_PATH, (void *)o_min_path);
    orig = (open_for_type_fn)oracle_addr(ORIG_OPEN_FOR_TYPE);

    for (option = 0; option < 2; option++) {
        for (type = 0; type < TYPE_LIMIT; type++) {
            char orig_text[EVENTS * EVENT_LEN], lift_text[EVENTS * EVENT_LEN];

            compared++;
            if (verbose) { fprintf(stderr, "option %d type %d\n", option, type); fflush(stderr); }
            memset(orig_effect, 0, sizeof orig_effect);
            memset(orig_particle, 0, sizeof orig_particle);
            *(int *)oracle_addr(ORIG_EFFECT_OPTION) = option;
            event_count = 0;
            srand(1);
            if (!call_original(orig, type)) {
                faults++;
                if (reported++ < MAX_REPORTED)
                    printf("  option %d type %d: the original faulted (code %08x at 0x%x)\n", option, type, fault_code, fault_va);
                continue;
            }
            recorded += event_count;
            joined(orig_text, sizeof orig_text);

            memset(&lift_effect, 0, sizeof lift_effect);
            memset(lift_particle, 0, sizeof lift_particle);
            memset(&g_session1, 0, sizeof g_session1);
            g_session1.map_rule_flags[0] = option;
            event_count = 0;
            srand(1);
            ez2_str_effect_open_for_type(&lift_effect, (struct Ez2StrParticle *)lift_particle, type);
            joined(lift_text, sizeof lift_text);

            if (strcmp(orig_text, lift_text) != 0) {
                mismatches++;
                if (reported++ < MAX_REPORTED)
                    printf("  option %d type %d:\n    original: %s\n    lift:     %s\n", option, type, orig_text, lift_text);
            }
        }
    }
    printf("Str files by effect type: %d calls compared against the original (%u loads recorded), %u mismatches, %u faults%s\n",
           compared, recorded, mismatches, faults, reported > MAX_REPORTED ? " (first 60 shown)" : "");
    return mismatches || faults ? 1 : 0;
}
