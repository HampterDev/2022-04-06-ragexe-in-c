/* Effect trace harness: runs every RagEffect ID through the client's real dispatch
   (rag_effect_process*.c), launchers (rag_skills_*.c, rag_songs.c, rag_msg_sprites.c,
   rag_unreferenced*.c, rag_process_moves.c), rag_prim_launch.c, effect_prim.c,
   rag_effect.c, rag_effect_init and the STR player (rag_str_anim.c, rag_str_playback.c,
   rag_str_bindings.c, str_path.c, str_animation.c), the effect data loader with the
   embedded data/effects/effects.json (effect_data.c) and the launch entry
   game_actor_launch_effect (actor/effect_launch.c), compiled with the client's flags
   against the stubs below.

   rand() in those files is renamed to effect_trace_rand by tests/effect_trace_rand.h
   (cl /FI), a seeded copy of the MSVC CRT generator that counts its calls.

   Each run is one effect ID with one input set (seed, master facing, launch offset and
   rotation, hit count, duration, min-effects flag, msg_params, master kind, camera,
   logic-step schedule). After every rag_effect_on_process call the harness hashes the
   side-effect log, the changed bytes of the RagEffect and the master, every object new in
   the world list (primitives in full) and the rand() call count. Pointers are replaced by
   names: objects by their creation order, textures and resources by file name, other
   heap blocks by allocation order. The hash of all runs is asserted; per-ID digests and
   a readable trace are written on request:

       test_effect_trace.exe [--digests FILE] [--trace FILE] [--ids FIRST LAST]

   --ids runs only that ID range and skips the total check. A pointer that escaped the
   naming would make the digest differ between two runs, as ASLR moves the image, heap and
   stack. */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <float.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "actor/types.h"
#include "actor/actor_ops.h"
#include "actor/object_ops.h"
#include "actor/render_ops.h"
#include "core/object_type.h"
#include "effects/durations.h"
#include "effects/effect_data.h"
#include "effects/effect_mgr.h"
#include "effects/rag/rag_effect.h"
#include "effects/skills/ez2str_effect.h"
#include "game/own_states.h"
#include "render/math3d.h"
#include "render/renderer.h"
#include "render/texture_cache.h"
#include "render/texture_types.h"
#include "resources/animation.h"
#include "resources/ezeffect.h"
#include "session/state.h"
#include "world/state.h"
#include "world/view_types.h"
#include "game/context.h"

#define EXPECTED_DIGEST 0x2842b5a04b52ece4ull
#define EXPECTED_EZ2STR_DIGEST 0x1b0232b840610959ull

#define FRAME_CAP 400
#define CALL_CAP 1200
#define LAST_ID (EFFECT_DURATION_COUNT - 1)

static int g_current_id = -1, g_current_set = -1;

static void fail(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "effect trace (id %d, set %d): ", g_current_id, g_current_set);
    vfprintf(stderr, fmt, args);
    fputc('\n', stderr);
    va_end(args);
    exit(1);
}

/* ---- rand(): the MSVC CRT generator, seeded per run, counting calls ---- */

static uint32_t g_holdrand = 1;
static unsigned long g_rand_calls;

int effect_trace_rand(void)
{
    ++g_rand_calls;
    g_holdrand = g_holdrand * 214013u + 2531011u;
    return (int)((g_holdrand >> 16) & 0x7fff);
}

/* ---- hashing ---- */

#define FNV64_BASIS 0xcbf29ce484222325ull
#define FNV64_PRIME 0x100000001b3ull

static uint64_t g_run_hash;
static FILE *g_trace;
static int g_tracing;

static uint64_t hash_mix(uint64_t h, const void *data, size_t n)
{
    const unsigned char *p = data;
    size_t i = 0;
    for (; i + 4 <= n; i += 4) {
        uint32_t w;
        memcpy(&w, p + i, 4);
        h = (h ^ w) * FNV64_PRIME;
    }
    for (; i < n; ++i)
        h = (h ^ p[i]) * FNV64_PRIME;
    return h;
}

static void hash_u32(uint32_t v)
{
    g_run_hash = hash_mix(g_run_hash, &v, sizeof v);
}

static uint32_t fnv32(const char *s)
{
    uint32_t h = 2166136261u;
    while (*s)
        h = (h ^ (unsigned char)*s++) * 16777619u;
    return h;
}

/* One line of the readable serialisation: hashed always, written when tracing. */
static void line(const char *fmt, ...)
{
    char text[1024];
    va_list args;
    int n;
    va_start(args, fmt);
    n = vsnprintf(text, sizeof text, fmt, args);
    va_end(args);
    if (n < 0)
        fail("format error");
    if ((size_t)n >= sizeof text)
        n = (int)sizeof text - 1;
    g_run_hash = hash_mix(g_run_hash, text, (size_t)n);
    hash_u32(0x0a);
    if (g_tracing) {
        fputs(text, g_trace);
        fputc('\n', g_trace);
    }
}

static const char *fstr(float f, char *buf)
{
    sprintf(buf, "%.9g", (double)f);
    return buf;
}

/* ---- heap: run-scoped blocks with a fixed fill and an allocation order ---- */

struct Block {
    struct Block *prev, *next;
    size_t size;
    unsigned ordinal;
    unsigned magic;
    unsigned pad;
};
#define BLOCK_MAGIC 0x7ace51abu
#define FILL_BYTE 0xcd

static struct Block g_run_blocks = { &g_run_blocks, &g_run_blocks, 0, 0, 0, 0 };
static struct Block g_kept_blocks = { &g_kept_blocks, &g_kept_blocks, 0, 0, 0, 0 };
static int g_in_run;
static unsigned g_alloc_ordinal;
static uintptr_t g_heap_low = UINTPTR_MAX, g_heap_high;

void *mem_try_alloc(size_t bytes)
{
    struct Block *b = malloc(sizeof *b + (bytes ? bytes : 1));
    struct Block *list = g_in_run ? &g_run_blocks : &g_kept_blocks;
    if (!b)
        fail("out of memory");
    b->size = bytes;
    b->ordinal = g_in_run ? g_alloc_ordinal++ : 0;
    b->magic = BLOCK_MAGIC;
    b->next = list;
    b->prev = list->prev;
    list->prev->next = b;
    list->prev = b;
    memset(b + 1, FILL_BYTE, bytes);
    if ((uintptr_t)b < g_heap_low)
        g_heap_low = (uintptr_t)b;
    if ((uintptr_t)(b + 1) + bytes + 1 > g_heap_high)
        g_heap_high = (uintptr_t)(b + 1) + bytes + 1;
    return b + 1;
}

void *mem_alloc(size_t bytes)
{
    return mem_try_alloc(bytes);
}

void mem_free(void *memory)
{
    struct Block *b;
    if (!memory)
        return;
    b = (struct Block *)memory - 1;
    if (b->magic != BLOCK_MAGIC)
        fail("mem_free of a block the harness did not allocate");
    b->magic = 0;
    b->prev->next = b->next;
    b->next->prev = b->prev;
    free(b);
}

static const struct Block *block_of(const void *p)
{
    const struct Block *b;
    uintptr_t a = (uintptr_t)p;
    if (a < g_heap_low || a >= g_heap_high)
        return NULL;
    /* One past the end counts: a full Vec's last and end point there. */
    for (b = g_run_blocks.next; b != &g_run_blocks; b = b->next)
        if (a >= (uintptr_t)(b + 1) && a <= (uintptr_t)(b + 1) + b->size)
            return b;
    return NULL;
}

static void free_run_blocks(void)
{
    while (g_run_blocks.next != &g_run_blocks) {
        struct Block *b = g_run_blocks.next;
        b->prev->next = b->next;
        b->next->prev = b->prev;
        b->magic = 0;
        free(b);
    }
}

__declspec(noreturn) void fatal_error(const char *message)
{
    fail("fatal_error: %s", message);
}

/* ---- named stub objects: textures and resources live for the whole process ---- */

enum { NAMED_TEXTURE = 1, NAMED_RES, NAMED_STR };

struct Named {
    const void *ptr;
    int kind;
    char name[MAX_PATH];
};

#define NAMED_SLOTS 16384
static struct Named *g_named[NAMED_SLOTS];      /* by pointer */
static struct Named *g_named_by_name[NAMED_SLOTS * 3];
static int g_named_count;

static size_t ptr_slot(const void *p)
{
    return ((uintptr_t)p >> 4) * 2654435761u % NAMED_SLOTS;
}

static const struct Named *named_of(const void *p)
{
    size_t i = ptr_slot(p);
    while (g_named[i]) {
        if (g_named[i]->ptr == p)
            return g_named[i];
        i = (i + 1) % NAMED_SLOTS;
    }
    return NULL;
}

static struct Named *named_find(int kind, const char *name)
{
    size_t slots = sizeof g_named_by_name / sizeof g_named_by_name[0];
    size_t i = (fnv32(name) ^ (unsigned)kind * 0x9e3779b9u) % slots;
    while (g_named_by_name[i]) {
        if (g_named_by_name[i]->kind == kind && !strcmp(g_named_by_name[i]->name, name))
            return g_named_by_name[i];
        i = (i + 1) % slots;
    }
    return NULL;
}

static struct Named *named_add(int kind, const char *name, const void *ptr)
{
    size_t slots = sizeof g_named_by_name / sizeof g_named_by_name[0];
    struct Named *n = calloc(1, sizeof *n);
    size_t i;
    if (!n || ++g_named_count >= NAMED_SLOTS / 2)
        fail("too many named stub objects");
    n->ptr = ptr;
    n->kind = kind;
    if (strlen(name) >= sizeof n->name)
        fail("name too long: %s", name);
    strcpy(n->name, name);
    i = ptr_slot(ptr);
    while (g_named[i])
        i = (i + 1) % NAMED_SLOTS;
    g_named[i] = n;
    i = (fnv32(name) ^ (unsigned)kind * 0x9e3779b9u) % slots;
    while (g_named_by_name[i])
        i = (i + 1) % slots;
    g_named_by_name[i] = n;
    return n;
}

/* ---- world objects of the current run ---- */

struct ObjRef {
    const void *ptr;
    char kind;      /* M master, P player, E RagEffect, p primitive, o other */
    int index;
    size_t bytes;
};

#define MAX_OBJS 200000
static struct ObjRef g_objs[MAX_OBJS];
static int g_obj_count, g_effect_count, g_prim_count, g_other_count;
static int g_serialised_count;          /* g_objs[0..g_serialised_count) were serialised as new */
static struct List g_objects;
static struct ListNode *g_scanned;      /* last node of g_objects already registered */

static const struct GameObjectOps *g_rag_ops, *g_prim_ops;
static struct GameActorOps g_actor_ops, g_foreign_ops;

static uint8_t g_master_store[sizeof(struct Player)];
static uint8_t g_player_store[sizeof(struct Player)];
#define MASTER ((struct GameActor *)g_master_store)
#define PLAYER_ACTOR ((struct GameActor *)g_player_store)

static const struct ObjRef *obj_of(const void *p)
{
    int i;
    for (i = g_obj_count - 1; i >= 0; --i)
        if (g_objs[i].ptr == p)
            return &g_objs[i];
    return NULL;
}

static void register_obj(const void *p, char kind, size_t bytes)
{
    struct ObjRef *r;
    if (g_obj_count >= MAX_OBJS)
        fail("too many objects in one run");
    r = &g_objs[g_obj_count++];
    r->ptr = p;
    r->kind = kind;
    r->bytes = bytes;
    r->index = kind == 'E' ? g_effect_count++ : kind == 'p' ? g_prim_count++ : kind == 'o' ? g_other_count++ : 0;
}

static char kind_of_object(const struct GameObject *o)
{
    if (o->ops == g_rag_ops)
        return 'E';
    if (o->ops == g_prim_ops)
        return 'p';
    return 'o';
}

/* Registers the objects added to the world list since the last scan, in list order. */
static void scan_new_objects(void)
{
    struct ListNode *n;
    if (!g_objects.head)
        return;
    for (n = g_scanned->next; n != g_objects.head; n = n->next) {
        struct GameObject *o = *LIST_VALUE(n, struct GameObject *);
        const struct Block *b = block_of(o);
        if (!obj_of(o))
            register_obj(o, kind_of_object(o), b ? b->size : sizeof(struct RenderObject));
        g_scanned = n;
    }
}

static void ptr_name(const void *p, char *out, size_t cap)
{
    const struct ObjRef *r;
    const struct Named *n;
    const struct Block *b;
    if (!p) {
        snprintf(out, cap, "0");
        return;
    }
    r = obj_of(p);
    if (!r) {
        scan_new_objects();
        r = obj_of(p);
    }
    if (r) {
        if (r->kind == 'M' || r->kind == 'P')
            snprintf(out, cap, "%c", r->kind);
        else
            snprintf(out, cap, "%c%d", r->kind, r->index);
        return;
    }
    if ((n = named_of(p)) != NULL) {
        snprintf(out, cap, "%s:%s", n->kind == NAMED_TEXTURE ? "tex" : n->kind == NAMED_STR ? "str" : "res",
                 n->name);
        return;
    }
    n = named_of((const uint8_t *)p - offsetof(struct EzEffectRes, anim));
    if (n && n->kind == NAMED_STR) {
        snprintf(out, cap, "anim:%s", n->name);
        return;
    }
    if (p == g_rag_ops) { snprintf(out, cap, "ops:effect"); return; }
    if (p == g_prim_ops) { snprintf(out, cap, "ops:prim"); return; }
    if (p == &g_actor_ops) { snprintf(out, cap, "ops:actor"); return; }
    if (p == &g_foreign_ops) { snprintf(out, cap, "ops:other"); return; }
    if ((b = block_of(p)) != NULL) {
        snprintf(out, cap, "a%u+%u", b->ordinal, (unsigned)((uintptr_t)p - (uintptr_t)(b + 1)));
        return;
    }
    snprintf(out, cap, "?");
}

static size_t g_fix_offset = (size_t)-1;

static uint32_t ptr_id(const void *p)
{
    char name[MAX_PATH + 16];
    if (!p)
        return 0;
    ptr_name(p, name, sizeof name);
    if (name[0] == '?')
        fail("unnamed pointer %p in a serialised field at offset 0x%x", p, (unsigned)g_fix_offset);
    return fnv32(name) | 0x80000000u;   /* never a user-mode address of this 32-bit process */
}

/* ---- serialisation of objects into flat buffers ---- */

static uint8_t *g_buf;
static size_t g_buf_cap, g_buf_len;

static void buf_reserve(size_t n)
{
    if (g_buf_len + n > g_buf_cap) {
        size_t cap = g_buf_cap ? g_buf_cap : 1 << 20;
        while (cap < g_buf_len + n)
            cap *= 2;
        g_buf = realloc(g_buf, cap);
        if (!g_buf)
            fail("out of memory");
        g_buf_cap = cap;
    }
}

static size_t buf_put(const void *p, size_t n)
{
    size_t at = g_buf_len;
    buf_reserve(n);
    memcpy(g_buf + g_buf_len, p, n);
    g_buf_len += n;
    return at;
}

static void buf_u32(uint32_t v)
{
    buf_put(&v, sizeof v);
}

/* Overwrites the pointer stored at buf[at + offset] with its name id. */
static void fix_ptr(size_t at, size_t offset)
{
    void *p;
    uint32_t id;
    memcpy(&p, g_buf + at + offset, sizeof p);
    g_fix_offset = offset;
    id = ptr_id(p);
    g_fix_offset = (size_t)-1;
    memset(g_buf + at + offset, 0, sizeof p);
    memcpy(g_buf + at + offset, &id, sizeof id);
}

#define FIX(at, type, field) fix_ptr((at), offsetof(type, field))

static void fix_render_object(size_t at, size_t base)
{
    fix_ptr(at, base + offsetof(struct RenderObject, ops));
    fix_ptr(at, base + offsetof(struct RenderObject, body_pal_name.data));
    fix_ptr(at, base + offsetof(struct RenderObject, spr));
    fix_ptr(at, base + offsetof(struct RenderObject, act));
    fix_ptr(at, base + offsetof(struct RenderObject, vars.head));
}

static void put_list_ids(const struct List *list)
{
    const struct ListNode *n;
    buf_u32(list->size);
    if (!list->head)
        return;
    for (n = list->head->next; n != list->head; n = n->next)
        buf_u32(ptr_id(*LIST_VALUE(n, void *const)));
}

static void put_floats(const struct Vec *v)
{
    const float *f;
    if (!v->first) {
        buf_u32(0xffffffffu);
        return;
    }
    buf_u32((uint32_t)((const float *)v->last - (const float *)v->first));
    for (f = v->first; f != v->last; ++f)
        buf_put(f, sizeof *f);
}

static void fix_triangles(size_t at, size_t base, int count)
{
    int i;
    for (i = 0; i < count; ++i)
        fix_ptr(at, base + i * sizeof(struct PrimTriangle) + offsetof(struct PrimTriangle, texture));
}

/* The RagEffect; the 512 triangles only when asked (launchers never write them). */
static void put_effect(const struct RagEffect *e, int with_triangles)
{
    size_t tri = offsetof(struct RagEffect, triangles), tri_end = tri + sizeof e->triangles;
    size_t at = buf_put(e, tri);
    size_t at2;
    fix_render_object(at, 0);
    FIX(at, struct RagEffect, master);
    FIX(at, struct RagEffect, prims.head);
    FIX(at, struct RagEffect, str_res);
    FIX(at, struct RagEffect, str_anim);
    if (with_triangles) {
        size_t t = buf_put(e->triangles, sizeof e->triangles);
        fix_triangles(t, 0, PRIM_TRIANGLES);
    }
    at2 = buf_put((const uint8_t *)e + tri_end, sizeof *e - tri_end);
    fix_ptr(at2, offsetof(struct RagEffect, last_prim) - tri_end);
    put_list_ids(&e->prims);
}

static struct EffectPrim *g_prim_template;

static void put_prim(const struct EffectPrim *p)
{
    size_t tri = offsetof(struct EffectPrim, triangles), tri_end = tri + sizeof p->triangles;
    size_t at = buf_put(p, tri);
    const struct Block *b;
    fix_render_object(at, 0);
    FIX(at, struct EffectPrim, master);
    FIX(at, struct EffectPrim, textures);
    FIX(at, struct EffectPrim, alpha_key_frames.first);
    FIX(at, struct EffectPrim, alpha_key_frames.last);
    FIX(at, struct EffectPrim, alpha_key_frames.end);
    FIX(at, struct EffectPrim, alpha_key_values.first);
    FIX(at, struct EffectPrim, alpha_key_values.last);
    FIX(at, struct EffectPrim, alpha_key_values.end);
    FIX(at, struct EffectPrim, alpha_key_ceilings.first);
    FIX(at, struct EffectPrim, alpha_key_ceilings.last);
    FIX(at, struct EffectPrim, alpha_key_ceilings.end);
    FIX(at, struct EffectPrim, alpha_key_floors.first);
    FIX(at, struct EffectPrim, alpha_key_floors.last);
    FIX(at, struct EffectPrim, alpha_key_floors.end);
    if (!memcmp(p->triangles, g_prim_template->triangles, sizeof p->triangles)) {
        buf_u32(0x7e1a7e1au);
    } else {
        size_t t = buf_put(p->triangles, sizeof p->triangles);
        fix_triangles(t, 0, PRIM_TRIANGLES);
    }
    buf_put((const uint8_t *)p + tri_end, sizeof *p - tri_end);
    if (p->textures && (b = block_of(p->textures)) != NULL) {
        size_t i, n = b->size / sizeof *p->textures;
        buf_u32((uint32_t)n);
        for (i = 0; i < n; ++i) {
            uint32_t raw;
            memcpy(&raw, &p->textures[i], sizeof raw);
            buf_u32(raw == 0xcdcdcdcdu ? raw : ptr_id(p->textures[i]));
        }
    }
    put_floats(&p->alpha_key_frames);
    put_floats(&p->alpha_key_values);
    put_floats(&p->alpha_key_ceilings);
    put_floats(&p->alpha_key_floors);
}

static void put_actor(const struct GameActor *a)
{
    size_t at = buf_put(a, sizeof(struct Pc));
    fix_render_object(at, 0);
    FIX(at, struct GameActor, effects.head);
    FIX(at, struct GameActor, path.cells);
    FIX(at, struct GameActor, extra_hit_queue.head);
    FIX(at, struct GameActor, hit_queue.head);
    FIX(at, struct GameActor, attached.head);
    put_list_ids(&a->effects);
}

static void put_other(const struct ObjRef *r)
{
    size_t at = buf_put(r->ptr, sizeof(struct RenderObject));
    fix_render_object(at, 0);
}

static void put_object(const struct ObjRef *r)
{
    if (r->kind == 'E')
        put_effect(r->ptr, 1);
    else if (r->kind == 'p')
        put_prim(r->ptr);
    else if (r->kind == 'M' || r->kind == 'P')
        put_actor(r->ptr);
    else
        put_other(r);
}

/* Hashes the changed words of a snapshot against the previous one; the first snapshot
   is hashed whole. */
struct Snapshot {
    uint8_t *data;
    size_t len, cap;
};

static void snapshot_diff(const char *who, struct Snapshot *s)
{
    size_t i, block, changed = 0, n = g_buf_len;
    char text[200];
    int shown = 0;
    if (!s->data || s->len != n) {
        g_run_hash = hash_mix(g_run_hash, g_buf, n);
        line("%s state %u bytes", who, (unsigned)n);
    } else {
        for (block = 0; block < n; block += 256) {
            size_t len = n - block < 256 ? n - block : 256;
            if (!memcmp(s->data + block, g_buf + block, len))
                continue;
            for (i = block; i < block + len; i += 4) {
                uint32_t a = 0, b = 0;
                memcpy(&a, s->data + i, i + 4 <= n ? 4 : n - i);
                memcpy(&b, g_buf + i, i + 4 <= n ? 4 : n - i);
                if (a == b)
                    continue;
                hash_u32((uint32_t)i);
                hash_u32(b);
                ++changed;
                if (g_tracing && shown < 12) {
                    snprintf(text, sizeof text, " +%x:%08x>%08x", (unsigned)i, a, b);
                    fputs(text, g_trace);
                    ++shown;
                }
            }
        }
        if (g_tracing && changed) {
            fprintf(g_trace, "%s %s changed %u words\n", shown < (int)changed ? " ..." : "", who,
                    (unsigned)changed);
        }
    }
    if (s->cap < n) {
        s->data = realloc(s->data, n);
        if (!s->data)
            fail("out of memory");
        s->cap = n;
    }
    memcpy(s->data, g_buf, n);
    s->len = n;
}

/* ---- the input sets ---- */

struct InputSet {
    unsigned seed;
    float facing;
    Vec3 offset;
    float rotation;
    int hit_count;          /* -1: no 0x2c message */
    int duration;           /* -1: the table duration; else a 0x50 message */
    int min_effects;        /* map_rule_flags[0] */
    int master_is_pc;
    int master_is_player;
    float camera;
    unsigned schedule;      /* 0: one logic step per frame; else the seed of a 0..3 step pattern */
    int has_params;
    float params[4];
    int effect_on;
};

static const struct InputSet k_sets[] = {
    { 1u,      0.0f, {   0.0f,  0.0f,   0.0f },   0.0f, -1,  -1, 0, 1, 0,   0.0f,  0u, 0, { 0 }, 1 },
    { 0x2f6bu, 90.0f, {  30.0f,  0.0f, -45.0f },   0.0f,  3,  -1, 1, 1, 1,  45.0f,  7u, 1, { 1.0f, 2.0f, 3.0f, 4.0f }, 1 },
    { 77u,   225.0f, { -60.0f, 12.0f,  20.0f }, 135.0f, 51,  37, 0, 0, 0, 300.0f, 11u, 0, { 0 }, 0 },
    { 31337u, 315.0f, {   5.0f,  0.0f,   5.0f }, 270.0f,  1, 120, 2, 1, 0, 180.0f, 23u, 1, { -0.5f, 100.0f, 0.0f, 7.0f }, 1 },
    { 4242u, 180.0f, {   0.0f,  0.0f,  80.0f },  90.0f,  0,  -1, 1, 0, 1,  90.0f,  0u, 0, { 0 }, 1 },
};
#define SET_COUNT ((int)(sizeof k_sets / sizeof k_sets[0]))

static const struct InputSet *g_set;
static struct RagEffect *g_effect;      /* the effect under test */

/* ---- client globals and stubs ---- */

struct Session g_session1;
static struct Renderer g_renderer_store;
struct Renderer *g_renderer = &g_renderer_store;
struct TexCache g_textureCache;
struct EffectMgr *g_effectMgr;
int g_isSkipFrame;
DWORD g_fogColor;
float g_fogNear, g_fogFar;
int gfx_shadows_supported(void) { return 0; }
int g_lightmap = 1;
static struct World g_world;
static int g_res_mgr_token, g_map_attr_token, g_mode_token;

struct ResMgr *g_resMgr(void) { return (struct ResMgr *)&g_res_mgr_token; }
struct World *game_current_world(void) { return &g_world; }
struct List *game_current_object_list(void) { return &g_objects; }
struct C3dAttr *game_current_map_attr(void) { return (struct C3dAttr *)&g_map_attr_token; }
const char *game_current_map_name(void) { return "prt_fild08"; }
int game_map_preserves_party_effects(void) { return 0; }
struct Mode *client_current_mode(void) { return (struct Mode *)&g_mode_token; }
static struct View g_view;
struct GameMode *game_current_mode(void) { return (struct GameMode *)&g_mode_token; }
struct View *game_current_view(void) { g_view.longitude = g_set->camera; return &g_view; }
int session_is_mercenary_job(struct Session *self, int id) { (void)self; return id >= 6017 && id <= 6046; }
struct GameActor *world_get_game_actor_by_aid(struct World *world, unsigned aid)
{
    (void)world; (void)aid;
    fail("world_get_game_actor_by_aid is not part of effect processing");
    return NULL;
}

struct Player *game_current_player(void)
{
    return (struct Player *)(g_set->master_is_player ? g_master_store : g_player_store);
}

struct GameActor *game_find_actor(int aid)
{
    line("find_actor %d", aid);
    if (aid == MASTER->aid)
        return MASTER;
    if (aid == PLAYER_ACTOR->aid && !g_set->master_is_player)
        return PLAYER_ACTOR;
    return NULL;
}

float game_camera_heading(void)
{
    return g_set->camera;
}

static const float k_view[12] = {
    0.9f, 0.0f, 0.43588989f,
    0.0f, 0.8f, -0.6f,
    -0.43588989f, 0.6f, 0.78f,
    -120.0f, 35.0f, 420.0f,
};

int game_copy_view_matrix(float matrix[12])
{
    memcpy(matrix, k_view, sizeof k_view);
    return 1;
}

/* As in effect_prim_draw.c, whose other functions draw; the projection is the real
   rp_project.c. */
struct TexDrawVertex *prim_project_vertex(struct TexDrawVertex *out, const float *matrix, Vec3 pos)
{
    out->x = 0.0f;
    out->y = 0.0f;
    out->z = 0.0f;
    out->rhw = 0.0f;
    out->diffuse = 0;
    out->specular = 0xff000000;
    out->u = 0.0f;
    out->v = 0.0f;
    renderer_project_vertex(g_renderer, &pos, matrix, out);
    return out;
}

float c3d_attr_get_height(struct C3dAttr *self, float x, float z)
{
    char a[32], b[32];
    line("height %s %s %s", self == game_current_map_attr() ? "map" : "?", fstr(x, a), fstr(z, b));
    return z * 0.0625f - x * 0.03125f - 7.0f;
}

int game_send_message(int message, int a, int b, int c, int d)
{
    if (message == 0x87) {
        /* The quake with strength, duration and amplitude passes them as float pointers. */
        char x[32], y[32], z[32];
        line("game_msg 0x87 %s %s %s %d", fstr(*(const float *)(uintptr_t)a, x), fstr(*(const float *)(uintptr_t)b, y),
             fstr(*(const float *)(uintptr_t)c, z), d);
        return 0;
    }
    line("game_msg 0x%x %d %d %d %d", message, a, b, c, d);
    return 0;
}

int mode_send_message(struct Mode *mode, int message, int a, int b, int c, int d)
{
    line("mode_msg %s 0x%x %d %d %d %d", mode == client_current_mode() ? "mode" : "?", message, a, b, c, d);
    return 0;
}

void sound_play_wave(const char *name, float x, float y, float z, int max_distance, int full_distance, float volume)
{
    char a[32], b[32], c[32], d[32];
    line("wave f%d %s %s %s %s %d %d %s", g_effect ? g_effect->frame : -1, name, fstr(x, a), fstr(y, b),
         fstr(z, c), max_distance, full_distance, fstr(volume, d));
}

void error_msg(const char *text, int flags)
{
    line("error_msg %d %s", flags, text);
}

int client_lua_call(struct ClientLua *lua, const char *function, const char *format, ...)
{
    va_list args;
    const char *f;
    char text[512];
    size_t used;
    int outputs = 0, last_int = 0;
    used = (size_t)snprintf(text, sizeof text, "lua %s %s", function, format);
    va_start(args, format);
    for (f = format; *f; ++f) {
        if (*f == '>') {
            outputs = 1;
            continue;
        }
        if (!outputs) {
            if (*f == 'd')
                used += (size_t)snprintf(text + used, sizeof text - used, " %d", last_int = va_arg(args, int));
            else if (*f == 's')
                used += (size_t)snprintf(text + used, sizeof text - used, " %s", va_arg(args, const char *));
            else
                fail("lua input format %c", *f);
        } else if (*f == 'b') {
            *va_arg(args, unsigned char *) = 0;
        } else if (*f == 'd') {
            *va_arg(args, int *) = 0;
        } else if (*f == 's') {
            /* Headgear STR names, for the EZ2STR file choice; every other string is unset. */
            static char hat_name[64];
            snprintf(hat_name, sizeof hat_name, "hat\\hat_%d.str", last_int);
            *va_arg(args, const char **) = !strcmp(function, "GetHatEfResName") ? hat_name : NULL;
        } else {
            fail("lua output format %c", *f);
        }
        if (used >= sizeof text)
            used = sizeof text - 1;
    }
    va_end(args);
    line("%s", text);
    (void)lua;
    return 1;
}

/* Textures: one object per name. */
struct Texture *texture_cache_get(struct TexCache *cache, const char *name, int a, int b)
{
    struct Named *n;
    (void)cache;
    line("texture %s %d %d", name ? name : "(null)", a, b);
    if (!name)
        return NULL;
    n = named_find(NAMED_TEXTURE, name);
    if (!n) {
        struct Texture *t = calloc(1, sizeof *t);
        if (!t)
            fail("out of memory");
        strncpy(t->name, name, sizeof t->name - 1);
        t->reqWidth = t->reqHeight = 64;
        t->base.width = t->base.height = 64;
        n = named_add(NAMED_TEXTURE, name, t);
    }
    return (struct Texture *)n->ptr;
}

/* Resources: STR files get a synthetic animation made from their name, so that their
   length and layers differ between files; everything else is a bare Res. */
struct StubRes {
    struct Res res;
    unsigned frames;
};

static uint32_t g_str_rng;

static uint32_t str_rand(void)
{
    g_str_rng ^= g_str_rng << 13;
    g_str_rng ^= g_str_rng >> 17;
    g_str_rng ^= g_str_rng << 5;
    return g_str_rng;
}

static float str_value(int range)
{
    return (float)((int)(str_rand() % (unsigned)(2 * range + 1)) - range) * 0.5f;
}

static void make_str(struct EzEffectRes *res, const char *name)
{
    struct StrAnim *anim = &res->anim;
    int layer, key;
    g_str_rng = fnv32(name) | 1u;
    anim->frame_count = 0;
    anim->layer_count = 1 + (int)(str_rand() % 4);
    for (layer = 0; layer < anim->layer_count; ++layer) {
        struct StrLayer *l = &anim->layers[layer];
        struct StrKey *keys;
        int frame = (int)(str_rand() % 5);
        l->texture_count = 1 + (int)(str_rand() % 3);
        l->key_count = 1 + (int)(str_rand() % 4);
        keys = calloc((size_t)l->key_count, sizeof *keys);
        if (!keys)
            fail("out of memory");
        for (key = 0; key < l->key_count; ++key) {
            struct StrKeyState *s = &keys[key].s;
            int i;
            keys[key].frame = frame;
            keys[key].type = key == 0 ? 0 : (int)(str_rand() % 2);
            s->x = 320.0f + str_value(40);
            s->y = 240.0f + str_value(40);
            s->u = str_value(2);
            s->v = str_value(2);
            s->us = 1.0f + str_value(1);
            s->vs = 1.0f + str_value(1);
            for (i = 0; i < 4; ++i) {
                s->second_uv[i] = str_value(2);
                s->corner_x[i] = str_value(60);
                s->corner_y[i] = str_value(60);
                s->color[i] = 128.0f + str_value(100);
            }
            s->aniframe = (float)(str_rand() % 3);
            s->anitype = (int)(str_rand() % 6);
            s->delay = str_value(4);
            s->angle = str_value(90);
            s->src_blend = (int)(str_rand() % 10);
            s->dest_blend = (int)(str_rand() % 10);
            s->mtpreset = (int)(str_rand() % 3);
            frame += 1 + (int)(str_rand() % 20);
        }
        l->keys = keys;
        if (frame > anim->frame_count)
            anim->frame_count = frame;
    }
}

static int ends_with(const char *s, const char *tail)
{
    size_t a = strlen(s), b = strlen(tail);
    return a >= b && !_stricmp(s + a - b, tail);
}

void *res_mgr_get(struct ResMgr *self, const char *name)
{
    struct Named *n;
    int str;
    (void)self;
    line("res_get %s", name ? name : "(null)");
    if (!name || !name[0])
        return NULL;
    str = ends_with(name, ".str");
    n = named_find(str ? NAMED_STR : NAMED_RES, name);
    if (!n) {
        void *res;
        if (str) {
            struct EzEffectRes *e = calloc(1, sizeof *e);
            if (!e)
                fail("out of memory");
            make_str(e, name);
            res = e;
        } else {
            struct StubRes *r = calloc(1, sizeof *r);
            if (!r)
                fail("out of memory");
            r->frames = 2 + fnv32(name) % 7;
            res = r;
        }
        n = named_add(str ? NAMED_STR : NAMED_RES, name, res);
    }
    return (void *)n->ptr;
}

/* Asked by ez2_str_particle_open for the min_ spelling: names with an odd hash exist. */
unsigned char res_mgr_is_available(struct ResMgr *self, const char *name)
{
    unsigned char available = (unsigned char)(fnv32(name) & 1);
    (void)self;
    line("res_available %s %u", name, available);
    return available;
}

static const char *res_name(const void *res)
{
    const struct Named *n = named_of(res);
    if (!n)
        fail("lock of an unknown resource");
    return n->name;
}

void res_lock(struct Res *self)
{
    line("res_lock %s", res_name(self));
    self->lockCount++;
}

void res_unlock(struct Res *self)
{
    line("res_unlock %s", res_name(self));
    self->lockCount--;
}

unsigned actres_frame_count(const struct ActRes *self, unsigned action)
{
    const struct StubRes *r = (const struct StubRes *)self;
    line("act_frames %s %u", res_name(self), action);
    return r->frames + action % 3;
}

static struct SprClip g_clips[2] = { { 3, -110, 0, 0, { 255, 255, 255, 255 }, 1.0f, 1.0f, 0, 0 },
                                     { -4, -60, 1, 0, { 255, 255, 255, 255 }, 1.0f, 1.0f, 0, 0 } };
static struct Motion g_motion;

struct Motion *act_res_get_motion(const struct ActRes *self, unsigned action, unsigned motion)
{
    line("act_motion %s %u %u", res_name(self), action, motion);
    g_motion.clips = g_clips;
    g_motion.clip_count = 2;
    return &g_motion;
}

struct SprClip *motion_get_clip(const struct Motion *self, unsigned index)
{
    line("motion_clip %u", index);
    return index < self->clip_count ? &self->clips[index] : NULL;
}

/* Render objects. */
struct RenderObject *render_object_construct(struct RenderObject *self)
{
    memset(self, 0, sizeof *self);
    return self;
}

void render_object_dc_render_object(struct RenderObject *self)
{
    char a[64];
    ptr_name(self, a, sizeof a);
    line("render_object_destruct %s", a);
}

void *render_object_get_act_res(struct RenderObject *self)
{
    return self->act;
}

void render_object_load_spr_res(struct RenderObject *self, const char *name)
{
    if (self->spr)
        res_unlock((struct Res *)self->spr);
    self->spr = res_mgr_get(g_resMgr(), name);
    if (self->spr)
        res_lock((struct Res *)self->spr);
}

void render_object_load_act_res(struct RenderObject *self, const char *name)
{
    if (self->act)
        res_unlock((struct Res *)self->act);
    self->act = res_mgr_get(g_resMgr(), name);
    res_lock((struct Res *)self->act);
}

void render_object_set_argb(struct RenderObject *self, int a, int r, int g, int b)
{
    unsigned char *argb = (unsigned char *)&self->argb;
    char name[64];
    ptr_name(self, name, sizeof name);
    line("set_argb %s %d %d %d %d", name, a, r, g, b);
    if (a != -1)
        argb[3] = (unsigned char)a;
    if (r != -1)
        argb[2] = (unsigned char)r;
    if (g != -1)
        argb[1] = (unsigned char)g;
    if (b != -1)
        argb[0] = (unsigned char)b;
}

void render_object_play_motion_range(struct RenderObject *self, unsigned char action, unsigned char motion,
                                     unsigned char last, unsigned char close_action, unsigned char close_motion,
                                     int timed, int duration, int loop)
{
    char name[64];
    ptr_name(self, name, sizeof name);
    line("play_motion %s %u %u %u %u %u %d %d %d", name, action, motion, last, close_action, close_motion, timed,
         duration, loop);
}

static void send_msg(struct GameObject *object, struct GameObject *source, RenderMsgArg msg, RenderMsgArg par1,
                     RenderMsgArg par2, RenderMsgArg par3, RenderMsgArg par4)
{
    char a[64], b[64];
    ptr_name(object, a, sizeof a);
    ptr_name(source, b, sizeof b);
    line("send %s<-%s 0x%llx %lld %lld %lld %lld", a, b, (long long)msg, (long long)par1, (long long)par2,
         (long long)par3, (long long)par4);
}

void render_object_send_msg(struct GameObject *object, struct GameObject *source, RenderMsgArg msg,
                            RenderMsgArg par1, RenderMsgArg par2, RenderMsgArg par3, RenderMsgArg par4)
{
    send_msg(object, source, msg, par1, par2, par3, par4);
}

int render_object_return_zero(struct RenderObject *object) { (void)object; return 0; }
void render_object_set_action_base(struct RenderObject *object, int action, int type)
{
    char name[64];
    ptr_name(object, name, sizeof name);
    line("set_action %s %d %d", name, action, type);
}
void render_object_update_motion_base(struct RenderObject *object)
{
    char name[64];
    ptr_name(object, name, sizeof name);
    line("update_motion %s", name);
}
void game_object_set_render_info(struct RenderObject *object, const struct ScreenBounds *bounds, float x, float y)
{
    (void)object; (void)bounds; (void)x; (void)y;
    fail("render info is not part of processing");
}

static void actor_destroy(struct GameObject *object) { (void)object; fail("destroy of a stub actor"); }
static unsigned char actor_process(struct GameObject *object) { (void)object; fail("process of a stub actor"); return 0; }
static void actor_render(struct GameObject *object, const float *view) { (void)object; (void)view; fail("render of a stub actor"); }
static void actor_set_job_model_stub(struct GameActor *actor, int job, int sex)
{
    char name[64];
    ptr_name(actor, name, sizeof name);
    line("set_job_model %s %d %d", name, job, sex);
}

static int actor_state(struct RenderObject *object)
{
    char name[64];
    ptr_name(object, name, sizeof name);
    line("effect_state %s", name);
    return 0;
}


void effect_prim_render(struct GameObject *object, const float *view)
{
    (void)object; (void)view;
    fail("primitives are not rendered");
}

unsigned char effect_prim_on_process(struct GameObject *object)
{
    (void)object;
    fail("primitives are not processed");
    return 0;
}

struct TexDrawPrim *renderer_new_prim(struct Renderer *self) { (void)self; fail("render"); return NULL; }
void renderer_add_prim(struct Renderer *self, struct RPQuad *prim, int flags) { (void)self; (void)prim; (void)flags; fail("render"); }
void *str_layer_get_texture(struct StrLayer *layer, int index) { (void)layer; (void)index; fail("render"); return NULL; }
BYTE session_is_screen_flipped(struct Session *self) { (void)self; return 0; }
void ez2_str_particle_rewind(struct Ez2StrParticle *self) { (void)self; line("ez2_rewind"); }

/* Actors. */
int game_actor_get_height_offset(struct GameActor *self)
{
    char name[64];
    ptr_name(self, name, sizeof name);
    line("height_offset %s", name);
    return 17;
}

int game_actor_get_job(struct GameActor *actor)
{
    return actor->job;
}

void game_actor_set_job(struct GameActor *self, int job)
{
    char name[64];
    ptr_name(self, name, sizeof name);
    line("set_job %s %d", name, job);
    self->job = job;
}

void *object_as_type(void *object, enum ObjectType type)
{
    char name[64];
    void *result = NULL;
    ptr_name(object, name, sizeof name);
    if (type == OBJECT_PC && object == (void *)MASTER && g_set->master_is_pc)
        result = object;
    line("as_type %s %d %s", name, (int)type, result ? "yes" : "no");
    return result;
}

static void *construct_copy(void *self, size_t bytes, const char *what, const void *src)
{
    char name[64];
    ptr_name(src, name, sizeof name);
    memset(self, 0, bytes);
    ((struct RenderObject *)self)->ops = &g_foreign_ops.base.base;
    line("construct %s %u from %s", what, (unsigned)bytes, name);
    return self;
}

struct BlurPC *blur_pc_construct(struct BlurPC *self, struct Pc *master)
{
    return construct_copy(self, sizeof *self, "blur_pc", master);
}
struct HalluNPC *hallu_npc_construct(struct HalluNPC *self, struct GameActor *src)
{
    return construct_copy(self, sizeof *self, "hallu_npc", src);
}
struct HalluPC2 *hallu_pc2_construct(struct HalluPC2 *self, struct Pc *master)
{
    return construct_copy(self, sizeof *self, "hallu_pc2", master);
}
struct RenderObject *hallu_pc_construct(void *mem, struct RenderObject *actor)
{
    return construct_copy(mem, sizeof(struct HalluPC), "hallu_pc", actor);
}
struct HalluPlant *hallu_plant_construct(struct HalluPlant *self)
{
    return construct_copy(self, sizeof *self, "hallu_plant", NULL);
}

struct Effect *effect_mgr_spawn(struct EffectMgr *self, int aid, int type, Vec3 pos, int target_aid, int level,
                                unsigned char reuse, int instance_key)
{
    char a[32], b[32], c[32];
    (void)self;
    line("effect_mgr_spawn %d 0x%x %s %s %s %d %d %u %d", aid, type, fstr(pos.x, a), fstr(pos.y, b), fstr(pos.z, c),
         target_aid, level, reuse, instance_key);
    return NULL;
}

/* Session. */
const char *session_get_attr_wave_name(int attr)
{
    static char name[64];
    snprintf(name, sizeof name, "attr_wave_%d.wav", attr);
    return name;
}

const char *session_get_efst_image(struct Session *self, int efst, int pass)
{
    static char names[16][64];
    static unsigned next;
    char *name;
    (void)self;
    line("efst_image %d %d", efst, pass);
    if ((efst + pass) % 3 == 0)
        return NULL;
    name = names[next++ % 16];
    snprintf(name, 64, "efst_%d_%d.tga", efst, pass);
    return name;
}

int session_is_party_member_aid(struct Session *self, int aid, unsigned char only_if_clear)
{
    (void)self;
    line("party_member %d %u", aid, only_if_clear);
    return 0;
}

int session_is_pc_job(struct Session *self, int job)
{
    (void)self;
    return job >= 0 && job < 4000;
}

const char *skill_get_id_name(int skill_id)
{
    static char name[64];
    snprintf(name, sizeof name, "SKILL_%d", skill_id);
    return name;
}

static const struct OwnStateEntry k_own_states[3] = {
    { 0x2a, 0, 1, 1000, 500 }, { 5, 0, 2, 2000, 100 }, { 30, 0, 3, 3000, 3000 },
};
size_t own_states_count(void) { return 3; }
const struct OwnStateEntry *own_states_at(size_t position) { return &k_own_states[position % 3]; }

/* ---- the driver ---- */

extern unsigned g_launchedEffectIds[32];

static uint32_t g_schedule_state;

static int next_skip(void)
{
    static const int pattern[8] = { 1, 1, 1, 0, 2, 1, 0, 3 };
    if (!g_set->schedule)
        return 1;
    g_schedule_state = g_schedule_state * 1103515245u + 12345u;
    return pattern[(g_schedule_state >> 16) & 7];
}

static void reset_actor(struct GameActor *a, int aid, Vec3 pos, float roty)
{
    memset(a, 0, sizeof(struct Player));
    a->ops = &g_actor_ops.base.base;
    a->pos = pos;
    a->roty = roty;
    a->aid = aid;
    a->job = 4;
    a->sex = 1;
    a->argb = 0xffffffffu;
    a->visible = 1;
    a->revealed = 1;
    a->is_pc = 1;
    a->scale = 1.0f;
    a->level = 99;
    list_init(&a->effects);
}

static void paint_stack(void)
{
    volatile unsigned char stack[32768];
    memset((void *)stack, 0x5a, sizeof stack);
}

static struct Snapshot g_effect_snap, g_master_snap, g_session_snap;
static unsigned long g_total_steps, g_total_prims, g_total_runs;

static void serialise_new_objects(void)
{
    scan_new_objects();
    for (; g_serialised_count < g_obj_count; ++g_serialised_count) {
        const struct ObjRef *r = &g_objs[g_serialised_count];
        char text[160];
        g_buf_len = 0;
        put_object(r);
        g_run_hash = hash_mix(g_run_hash, g_buf, g_buf_len);
        if (r->kind == 'p') {
            const struct EffectPrim *p = r->ptr;
            char tex[MAX_PATH + 16], spr[MAX_PATH + 16];
            ++g_total_prims;
            ptr_name(p->textures ? p->textures[0] : NULL, tex, sizeof tex);
            ptr_name(p->base.spr, spr, sizeof spr);
            line("new p%d kind %d duration %d tex0 %s spr %s hash %016llx", r->index, p->kind, p->duration,
                 p->textures ? tex : "-", spr, (unsigned long long)hash_mix(FNV64_BASIS, g_buf, g_buf_len));
        } else {
            snprintf(text, sizeof text, "new %c%d %u bytes hash %016llx", r->kind, r->index, (unsigned)g_buf_len,
                     (unsigned long long)hash_mix(FNV64_BASIS, g_buf, g_buf_len));
            line("%s", text);
        }
    }
}

static void serialise_step(void)
{
    g_buf_len = 0;
    put_effect(g_effect, 0);
    snapshot_diff("E0", &g_effect_snap);
    g_buf_len = 0;
    put_actor(MASTER);
    snapshot_diff("M", &g_master_snap);
    g_buf_len = 0;
    buf_put(g_session1.stateIconSlots, sizeof g_session1.stateIconSlots);
    snapshot_diff("session", &g_session_snap);
    hash_u32((uint32_t)g_rand_calls);
}

static void serialise_final(void)
{
    int i;
    line("final rand %lu objects %d", g_rand_calls, g_obj_count);
    for (i = 0; i < g_obj_count; ++i) {
        g_buf_len = 0;
        put_object(&g_objs[i]);
        g_run_hash = hash_mix(g_run_hash, g_buf, g_buf_len);
    }
}

static void free_snapshots(void)
{
    g_effect_snap.len = g_master_snap.len = g_session_snap.len = 0;
}

static uint64_t run_effect(int id, const struct InputSet *set, int set_index)
{
    Vec3 master_pos = { 140.5f, -2.25f, 160.75f };
    Vec3 player_pos = { 95.0f, -1.0f, 210.0f };
    int calls = 0, frames = 0, done = 0;

    g_set = set;
    g_current_id = id;
    g_current_set = set_index;
    g_run_hash = FNV64_BASIS;
    g_in_run = 1;
    g_alloc_ordinal = 0;
    g_obj_count = g_effect_count = g_prim_count = g_other_count = 0;
    g_holdrand = set->seed ^ ((unsigned)id * 2654435761u);
    g_rand_calls = 0;
    g_schedule_state = set->schedule * 2246822519u + (unsigned)id;
    g_effect = NULL;
    free_snapshots();
    memset(g_launchedEffectIds, 0, sizeof(unsigned) * 32);
    memset(&g_session1.stateIconSlots, 0, sizeof g_session1.stateIconSlots);
    g_session1.map_rule_flags[0] = set->min_effects;
    g_session1.effectOn = set->effect_on;

    list_init(&g_objects);
    g_scanned = g_objects.head;
    reset_actor(MASTER, 2001, master_pos, set->facing);
    reset_actor(PLAYER_ACTOR, 1000, player_pos, 45.0f);
    if (set->master_is_player)
        MASTER->aid = g_session1.ownAid;
    register_obj(MASTER, 'M', sizeof(struct Player));
    if (!set->master_is_player)
        register_obj(PLAYER_ACTOR, 'P', sizeof(struct Player));
    g_serialised_count = g_obj_count;

    if (g_tracing)
        fprintf(g_trace, "== id %d set %d\n", id, set_index);
    line("launch %d", id);
    g_effect = game_actor_launch_effect(MASTER, id, set->offset, set->rotation);
    if (!g_effect) {
        line("not launched");
    } else {
        if (set->hit_count >= 0)
            rag_effect_send_msg(g_effect, NULL, 0x2c, set->hit_count, 0, 0);
        if (set->has_params)
            rag_effect_send_msg(g_effect, NULL, 0x2e, (RenderMsgArg)(uintptr_t)set->params, 0, 0);
        if (set->duration >= 0)
            rag_effect_send_msg(g_effect, NULL, RAG_MSG_SET_DURATION, set->duration, 0, 0);
        serialise_new_objects();
        serialise_step();
        while (!done) {
            int skip = next_skip(), steps = skip > 0 ? skip : 1, k;
            MASTER->drawn = 1;
            g_isSkipFrame = skip;
            ++frames;
            for (k = 0; k < steps && !done; ++k) {
                unsigned long rand_before = g_rand_calls;
                unsigned char going;
                ++calls;
                ++g_total_steps;
                line("step %d skip %d frame %d", calls, skip, g_effect->frame);
                paint_stack();
                going = g_effect->base.ops->on_process((struct GameObject *)g_effect);
                line("ret %d frame %d rand %lu", going, g_effect->frame, g_rand_calls - rand_before);
                serialise_new_objects();
                serialise_step();
                if (!going || g_effect->frame >= FRAME_CAP || calls >= CALL_CAP)
                    done = 1;
            }
        }
        serialise_final();
        g_effect->base.ops->destroy((struct GameObject *)g_effect);
        g_effect = NULL;
        line("destroyed after %d steps, %d frames", calls, frames);
    }
    free_run_blocks();
    g_objects.head = NULL;
    g_in_run = 0;
    ++g_total_runs;
    return g_run_hash;
}

static LONG WINAPI crashed(EXCEPTION_POINTERS *info)
{
    fprintf(stderr, "effect trace (id %d, set %d): exception 0x%08lx at %p\n", g_current_id, g_current_set,
            info->ExceptionRecord->ExceptionCode, info->ExceptionRecord->ExceptionAddress);
    fflush(stderr);
    return EXCEPTION_EXECUTE_HANDLER;
}

static void prepare(void)
{
    struct RagEffect *effect;

    SetUnhandledExceptionFilter(crashed);
    /* The client's floating-point setting (START-073, src/core/winmain.c): 24-bit x87
       precision. */
    _controlfp(0x20000, 0x30000);
    math_init_degree_tables();
    g_actor_ops.base.base.destroy = actor_destroy;
    g_actor_ops.base.base.on_process = actor_process;
    g_actor_ops.base.base.send_msg = send_msg;
    g_actor_ops.base.base.render = actor_render;
    g_actor_ops.base.base.get_direction = game_object_dir_of;
    g_actor_ops.base.get_effect_state = actor_state;
    g_actor_ops.set_job_model = actor_set_job_model_stub;
    g_foreign_ops = g_actor_ops;
    g_ragEffectProcess = rag_effect_on_process;

    g_renderer_store.proj[0] = 700.0f;
    g_renderer_store.proj[1] = 700.0f;
    g_renderer_store.proj[2] = 1.0f;
    g_renderer_store.aspect = 1.3333334f;
    g_renderer_store.scaleX = g_renderer_store.scaleY = 1.0f;
    g_renderer_store.width = 640;
    g_renderer_store.height = 480;
    g_renderer_store.halfW = g_renderer_store.centreX = 320;
    g_renderer_store.halfH = g_renderer_store.centreY = 240;
    g_session1.ownAid = 1000;

    effect = mem_alloc(sizeof *effect);
    rag_effect_construct(effect);
    g_rag_ops = effect->base.ops;
    g_prim_template = mem_alloc(sizeof *g_prim_template);
    effect_prim_construct(g_prim_template);
    g_prim_ops = g_prim_template->base.ops;

    /* The effect data built into the executable, whatever the environment says. */
    _putenv("RAGEXE_EFFECT_DATA=");
    if (!effect_data_load())
        fail("the embedded effect data does not load");
}

/* ---- the EZ2STR file choice: ez2_str_effect_open_for_type (ez2str_effect_files.c) ---- */

#define EZ2STR_LAST_TYPE 2450

static uint64_t ez2str_choice(int type, int min_effects, unsigned seed)
{
    struct Ez2StrEffect effect;
    struct Ez2StrParticle particle;
    char res[MAX_PATH + 16];

    g_run_hash = FNV64_BASIS;
    g_in_run = 1;
    g_alloc_ordinal = 0;
    g_obj_count = 0;
    g_holdrand = seed ^ ((unsigned)type * 2654435761u);
    g_rand_calls = 0;
    g_session1.map_rule_flags[0] = min_effects;
    memset(&effect, 0, sizeof effect);
    memset(&particle, 0, sizeof particle);
    effect.base.type = type;
    string_init(&effect.str_name);
    string_init(&particle.path);
    string_init(&particle.texture_set);
    if (type == 1412)
        string_assign_cstr(&effect.str_name, "custom\\named.str");
    if (g_tracing)
        fprintf(g_trace, "== ez2str type %d min %d seed %u\n", type, min_effects, seed);
    ez2_str_effect_open_for_type(&effect, &particle, type);
    ptr_name(particle.res, res, sizeof res);
    line("ez2str res %s path %s set %s rand %lu", res, string_cstr(&particle.path),
         string_cstr(&particle.texture_set), g_rand_calls);
    if (particle.res)
        res_unlock(&particle.res->res);
    free_run_blocks();
    g_in_run = 0;
    return g_run_hash;
}

int main(int argc, char **argv)
{
    static const int extra_ids[3] = { 2385, 10000, 60000 };
    static const unsigned ez2str_seeds[2] = { 5u, 6u };
    const char *digest_path = NULL, *trace_path = NULL;
    int first = 0, last = LAST_ID + 3, partial = 0, id, i, s, m;
    FILE *digests = NULL;
    uint64_t total = FNV64_BASIS, ez2str_total = FNV64_BASIS;
    unsigned long ids = 0, choices = 0;

    for (i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--digests") && i + 1 < argc)
            digest_path = argv[++i];
        else if (!strcmp(argv[i], "--trace") && i + 1 < argc)
            trace_path = argv[++i];
        else if (!strcmp(argv[i], "--ids") && i + 2 < argc) {
            first = atoi(argv[++i]);
            last = atoi(argv[++i]);
            partial = 1;
        } else
            fail("usage: test_effect_trace [--digests FILE] [--trace FILE] [--ids FIRST LAST]");
    }
    if (digest_path && !(digests = fopen(digest_path, "w")))
        fail("cannot write %s", digest_path);
    if (trace_path && !(g_trace = fopen(trace_path, "w")))
        fail("cannot write %s", trace_path);
    g_tracing = g_trace != NULL;

    prepare();
    /* Every ID of the duration table, then three past it. */
    for (i = 0; i <= LAST_ID + 3; ++i) {
        uint64_t id_hash = FNV64_BASIS;
        if (i < first || i > last)
            continue;
        id = i <= LAST_ID ? i : extra_ids[i - LAST_ID - 1];
        for (s = 0; s < SET_COUNT; ++s) {
            uint64_t h = run_effect(id, &k_sets[s], s);
            id_hash = hash_mix(id_hash, &h, sizeof h);
        }
        ++ids;
        total = hash_mix(total, &id, sizeof id);
        total = hash_mix(total, &id_hash, sizeof id_hash);
        if (digests)
            fprintf(digests, "%d %016llx\n", id, (unsigned long long)id_hash);
    }
    g_current_id = g_current_set = -1;
    g_set = &k_sets[0];
    for (id = 0; id <= EZ2STR_LAST_TYPE; ++id) {
        uint64_t type_hash = FNV64_BASIS;
        if (partial && (id < first || id > last))
            continue;
        g_current_id = id;
        for (m = 0; m < 3; ++m) {
            for (s = 0; s < 2; ++s) {
                uint64_t h = ez2str_choice(id, m, ez2str_seeds[s]);
                type_hash = hash_mix(type_hash, &h, sizeof h);
                ++choices;
            }
        }
        ez2str_total = hash_mix(ez2str_total, &id, sizeof id);
        ez2str_total = hash_mix(ez2str_total, &type_hash, sizeof type_hash);
        if (digests)
            fprintf(digests, "ez2str %d %016llx\n", id, (unsigned long long)type_hash);
    }
    if (digests)
        fclose(digests);
    if (g_trace)
        fclose(g_trace);
    printf("Effect trace: %lu IDs x %d input sets, %lu steps, %lu primitives; digest %016llx.\n", ids, SET_COUNT,
           g_total_steps, g_total_prims, (unsigned long long)total);
    printf("EZ2STR file choice: %lu calls (types 0-%d, min-effects 0/1/2, two seeds); digest %016llx.\n", choices,
           EZ2STR_LAST_TYPE, (unsigned long long)ez2str_total);
    if (!partial && (total != EXPECTED_DIGEST || ez2str_total != EXPECTED_EZ2STR_DIGEST)) {
        fprintf(stderr, "effect trace: digests %016llx and %016llx, expected %016llx and %016llx\n",
                (unsigned long long)total, (unsigned long long)ez2str_total, (unsigned long long)EXPECTED_DIGEST,
                (unsigned long long)EXPECTED_EZ2STR_DIGEST);
        return 1;
    }
    return 0;
}
