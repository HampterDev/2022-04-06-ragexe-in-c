#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <windows.h>
#include "oracle_image.h"
#include "session/state.h"
#include "effects/skills/ez2str_effect.h"
#include "effects/str_path.h"
#include "effects/particle.h"
#include "effects/durations.h"
#include "effects/effect_math.h"
#include "fileio/resource_api.h"
#include "contents/client_lua_calls.h"
#include "core/memory.h"
#include "std/string.h"
#include "render/renderer.h"
#include "render/projection.h"
#include "game/context.h"
#include "effects/effect.h"

#define ORIG_INIT            0x996150u
#define ORIG_AFTER_INIT      0x996870u
#define ORIG_VTABLE          0xd4ec6cu
#define ORIG_VTABLE_SLOTS    27
#define ORIG_VT_SET_ZOOM     (0x50 / 4)
#define ORIG_PARTICLE_NEW    0x9c2d60u
#define ORIG_EFFECT_CUE      0x992050u
#define ORIG_LUA_CALL        0x93dfc0u
#define ORIG_STR_LOAD        0x9828b0u
#define ORIG_MIN_PATH        0x982340u
#define ORIG_RES_AVAILABLE   0x501540u
#define ORIG_PROJECT         0x4eb970u
#define ORIG_FIND_ACTOR      0x9818d0u
#define ORIG_GET_GAME_MODE   0x6617e0u
#define ORIG_QUAD_CORNERS    0x9843e0u
#define ORIG_ACTOR_BY_AID    0x65c580u
#define ORIG_EFFECT_OPTION   0x1138cbcu
#define ORIG_DURATIONS       0xe907a0u
#define ORIG_MODE_VIEW       0xd0
#define ORIG_VIEW_LONGITUDE  0x30
#define ORIG_THIS_LEVEL             0xb0
#define ORIG_THIS_INSTANCE_KEY_FLAG 0xc0
#define ORIG_THIS_INSTANCE_KEY      0xc4
#define ORIG_THIS_PARTICLE          0xc8
#define ORIG_THIS_ADJUST_POSITION   0xe4
#define ORIG_THIS_CAMERA_ANGLE      0xf0
#define ORIG_THIS_ATTACHED_TO_HEAD  0xf8
#define ORIG_PARTICLE_LIST_FLAGS    0x1c
#define ORIG_PARTICLE_LIFE          0xcc
#define ORIG_PARTICLE_LOOP_FOREVER  0x3f5c
#define ORIG_PARTICLE_LOOPS         0x3f9c
#define ORIG_PARTICLE_SPIN          0x3fa4
#define ORIG_PARTICLE_SCALE_BY_ACTOR 0x3fc4
#define ORIG_PARTICLE_ACTOR_AID     0x3fc8

#define TYPE_LIMIT 0x1000
#define MAX_REPORTED 60
#define EVENTS 24
#define EVENT_LEN 160
#define CAMERA_HEADING 37.0f

struct Session g_session1;
struct Renderer *g_renderer;

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

static unsigned char orig_effect[0x400], orig_particle[0x4100], orig_mode[0x200], orig_view[0x200];
static void *orig_vt[64];
static struct Ez2StrEffect lift_effect;
static struct Ez2StrParticle lift_particle;
static int lift_res_mgr, lift_renderer;

/* ---- the lift's side ---- */

struct Ez2StrParticle *ez2str_particle_new(Vec3 pos, int list_flags)
{
    event("particle list %x at %.0f %.0f %.0f", list_flags, pos.x, pos.y, pos.z);
    memset(&lift_particle, 0, sizeof lift_particle);
    lift_particle.base.list_flags = list_flags;
    lift_particle.base.pos = pos;
    return &lift_particle;
}
void particle_quad_unit_corners(struct TexDrawVertex *quad) { (void)quad; }
void ez2_str_effect_set_zoom(struct Effect *effect, float zoom) { (void)effect; event("zoom %.2f", zoom); }
int client_lua_call(struct ClientLua *lua, const char *function, const char *format, ...) { (void)lua; (void)function; (void)format; return 0; }
float game_camera_heading(void) { return CAMERA_HEADING; }
int game_copy_view_matrix(float matrix[12]) { memset(matrix, 0, 12 * sizeof *matrix); return 1; }
void renderer_project_point(struct Renderer *renderer, const Vec3 *point, const float *view, float *x, float *y, float *rhw)
{ (void)renderer; (void)view; *x = point->x * 2.0f + 1.0f; *y = point->z * 3.0f + 2.0f; *rhw = 1.0f; }
void *effect_find_actor(int aid) { (void)aid; return NULL; }
void effect_add_sound_cue(struct Effect *self, int kind, const char *wave, int frame, float height)
{ (void)self; event("cue %d %s %d %.0f", kind, wave ? wave : "-", frame, height); }
void ez2_str_particle_load(struct Ez2StrParticle *self, const char *path) { (void)self; event("load %s", path); }
struct ResMgr *g_resMgr(void) { return (struct ResMgr *)&lift_res_mgr; }
unsigned char res_mgr_is_available(struct ResMgr *self, const char *name) { (void)self; (void)name; return 1; }
void *mem_alloc(size_t bytes) { return malloc(bytes); }
void mem_free(void *memory) { free(memory); }
void fatal_error(const char *what) { fprintf(stderr, "fatal: %s\n", what); ExitProcess(5); }

#include "str_path_open.inc"
#include "ez2str_init.inc"

/* ---- the original's side ---- */

static void *__cdecl o_particle_new(float x, float y, float z, int list_flags)
{
    event("particle list %x at %.0f %.0f %.0f", list_flags, x, y, z);
    memset(orig_particle, 0, sizeof orig_particle);
    *(int *)(orig_particle + ORIG_PARTICLE_LIST_FLAGS) = list_flags;
    return orig_particle;
}
static void *__fastcall o_effect_cue(void *self, void *edx, int kind, const char *wave, int frame, float height)
{ (void)self; (void)edx; event("cue %d %s %d %.0f", kind, wave ? wave : "-", frame, height); return NULL; }
static int __cdecl o_lua_call(void *lua, const char *function, const char *format, ...) { (void)lua; (void)function; (void)format; return 0; }
static void __fastcall o_set_zoom(void *self, void *edx, float zoom) { (void)self; (void)edx; event("zoom %.2f", zoom); }
static void __fastcall o_project(void *self, void *edx, const float *point, const float *view, float *x, float *y, float *rhw)
{ (void)self; (void)edx; (void)view; *x = point[0] * 2.0f + 1.0f; *y = point[2] * 3.0f + 2.0f; *rhw = 1.0f; }
static void *__cdecl o_find_actor(int aid) { (void)aid; return NULL; }
static void *__fastcall o_get_mode(void *self, void *edx) { (void)self; (void)edx; return orig_mode; }
static void *__fastcall o_actor_by_aid(void *self, void *edx, int aid) { (void)self; (void)edx; (void)aid; return NULL; }

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
static void __fastcall o_str_load(void *self, void *edx, unsigned s0, unsigned s1, unsigned s2, unsigned s3, unsigned size, unsigned capacity)
{
    unsigned buffer[6] = { s0, s1, s2, s3, size, capacity };
    (void)self; (void)edx;
    event("load %s", std_string_text(buffer));
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

typedef void (__fastcall *method_fn)(void *self, void *edx);

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

static int call_original(method_fn init, method_fn after_init)
{
    __try { init(orig_effect, NULL); after_init(orig_effect, NULL); return 1; }
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
    const unsigned char *p = *(unsigned char **)(orig_effect + ORIG_THIS_PARTICLE);
    snprintf(out, size, "key %d adjust %d head %d camera %.1f | aid %d loop %d scale %d loops %d spin %.2f list %x life %.3f",
             orig_effect[ORIG_THIS_INSTANCE_KEY_FLAG], orig_effect[ORIG_THIS_ADJUST_POSITION], orig_effect[ORIG_THIS_ATTACHED_TO_HEAD],
             *(double *)(orig_effect + ORIG_THIS_CAMERA_ANGLE), p ? *(int *)(p + ORIG_PARTICLE_ACTOR_AID) : -1,
             p ? p[ORIG_PARTICLE_LOOP_FOREVER] : -1, p ? p[ORIG_PARTICLE_SCALE_BY_ACTOR] : -1, p ? *(int *)(p + ORIG_PARTICLE_LOOPS) : -1,
             p ? *(float *)(p + ORIG_PARTICLE_SPIN) : -1.0f, p ? *(unsigned *)(p + ORIG_PARTICLE_LIST_FLAGS) : 0,
             p ? *(float *)(p + ORIG_PARTICLE_LIFE) : -1.0f);
}

static void lift_state(char *out, size_t size)
{
    const struct Ez2StrParticle *p = lift_effect.particle;
    snprintf(out, size, "key %d adjust %d head %d camera %.1f | aid %d loop %d scale %d loops %d spin %.2f list %x life %.3f",
             lift_effect.base.has_instance_key, lift_effect.adjust_position_when_shrunk, lift_effect.attached_to_head,
             lift_effect.camera_angle, p ? p->actor_aid : -1, p ? p->loop_forever : -1, p ? p->scale_by_actor : -1, p ? p->loops : -1,
             p ? p->spin_degrees : -1.0f, p ? (unsigned)p->base.list_flags : 0, p ? p->base.life_seconds : -1.0f);
}

int main(int argc, char **argv)
{
    const char *exe = argc > 1 ? argv[1] : getenv("ORACLE_EXE");
    static const int levels[] = { 1, 3 };
    method_fn init, after_init;
    int type, option, li, compared = 0, i;
    unsigned mismatches = 0, faults = 0, reported = 0, recorded = 0;
    const int verbose = getenv("ORACLE_VERBOSE") != NULL;

    if (!exe) {
        fprintf(stderr, "usage: test_ez2str_init <original exe>\n");
        return 2;
    }
    if (!oracle_load(exe))
        return 2;
    if (verbose)
        AddVectoredExceptionHandler(1, first_look);
    for (i = 0; i < ORIG_VTABLE_SLOTS; i++)
        orig_vt[i] = ((void **)oracle_addr(ORIG_VTABLE))[i];
    orig_vt[ORIG_VT_SET_ZOOM] = (void *)o_set_zoom;
    oracle_patch_jump(ORIG_PARTICLE_NEW, (void *)o_particle_new);
    oracle_patch_jump(ORIG_EFFECT_CUE, (void *)o_effect_cue);
    oracle_patch_jump(ORIG_LUA_CALL, (void *)o_lua_call);
    oracle_patch_jump(ORIG_STR_LOAD, (void *)o_str_load);
    oracle_patch_jump(ORIG_MIN_PATH, (void *)o_min_path);
    oracle_patch_return(ORIG_RES_AVAILABLE, 1, 4);
    oracle_patch_jump(ORIG_PROJECT, (void *)o_project);
    oracle_patch_jump(ORIG_FIND_ACTOR, (void *)o_find_actor);
    oracle_patch_jump(ORIG_GET_GAME_MODE, (void *)o_get_mode);
    oracle_patch_jump(ORIG_ACTOR_BY_AID, (void *)o_actor_by_aid);
    oracle_patch_return(ORIG_QUAD_CORNERS, 0, 4);
    memcpy(oracle_addr(ORIG_DURATIONS), g_effectDurations, sizeof g_effectDurations);
    init = (method_fn)oracle_addr(ORIG_INIT);
    after_init = (method_fn)oracle_addr(ORIG_AFTER_INIT);
    g_renderer = (struct Renderer *)&lift_renderer;

    for (option = 0; option < 2; option++) {
        for (type = 0; type < TYPE_LIMIT; type++) {
            for (li = 0; li < (int)(sizeof levels / sizeof *levels); li++) {
                char orig_text[EVENTS * EVENT_LEN], lift_text[EVENTS * EVENT_LEN], orig_fields[200], lift_fields[200];

                compared++;
                if (verbose) { fprintf(stderr, "option %d type %d level %d\n", option, type, levels[li]); fflush(stderr); }
                memset(orig_effect, 0, sizeof orig_effect);
                memset(orig_mode, 0, sizeof orig_mode);
                memset(orig_view, 0, sizeof orig_view);
                *(void **)orig_effect = orig_vt;
                *(int *)(orig_effect + 4) = type;
                ((float *)(orig_effect + 0x08))[0] = 100.0f;
                ((float *)(orig_effect + 0x08))[1] = 5.0f;
                ((float *)(orig_effect + 0x08))[2] = 200.0f;
                ((float *)(orig_effect + 0x14))[0] = 130.0f;
                ((float *)(orig_effect + 0x14))[1] = 7.0f;
                ((float *)(orig_effect + 0x14))[2] = 260.0f;
                *(int *)(orig_effect + 0x20) = 1001;
                *(int *)(orig_effect + ORIG_THIS_LEVEL) = levels[li];
                *(void **)(orig_mode + ORIG_MODE_VIEW) = orig_view;
                *(float *)(orig_view + ORIG_VIEW_LONGITUDE) = CAMERA_HEADING;
                *(int *)oracle_addr(ORIG_EFFECT_OPTION) = option;
                event_count = 0;
                overflow = 0;
                srand(1);
                if (!call_original(init, after_init)) {
                    faults++;
                    if (reported++ < MAX_REPORTED)
                        printf("  option %d type %d level %d: the original faulted (code %08x at 0x%x)\n", option, type, levels[li], fault_code, fault_va);
                    continue;
                }
                recorded += event_count;
                joined(orig_text, sizeof orig_text);
                orig_state(orig_fields, sizeof orig_fields);

                memset(&lift_effect, 0, sizeof lift_effect);
                lift_effect.base.type = type;
                lift_effect.base.pos.x = 100.0f;
                lift_effect.base.pos.y = 5.0f;
                lift_effect.base.pos.z = 200.0f;
                lift_effect.base.target_pos.x = 130.0f;
                lift_effect.base.target_pos.y = 7.0f;
                lift_effect.base.target_pos.z = 260.0f;
                lift_effect.base.owner_aid = 1001;
                lift_effect.base.level = levels[li];
                memset(&g_session1, 0, sizeof g_session1);
                g_session1.map_rule_flags[0] = option;
                event_count = 0;
                overflow = 0;
                srand(1);
                ez2_str_effect_init(&lift_effect.base);
                ez2_str_effect_after_init(&lift_effect.base);
                joined(lift_text, sizeof lift_text);
                lift_state(lift_fields, sizeof lift_fields);

                if (strcmp(orig_text, lift_text) != 0 || strcmp(orig_fields, lift_fields) != 0) {
                    mismatches++;
                    if (reported++ < MAX_REPORTED)
                        printf("  option %d type %d level %d:\n    original: %s | %s\n    lift:     %s | %s\n", option, type, levels[li],
                               orig_text, orig_fields, lift_text, lift_fields);
                }
            }
        }
    }
    printf("Ez2str init: %d calls compared against the original (%u events recorded), %u mismatches, %u faults%s\n", compared, recorded,
           mismatches, faults, reported > MAX_REPORTED ? " (first 60 shown)" : "");
    return mismatches || faults ? 1 : 0;
}
