#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "effects/ez2str_effect_mgr.h"
#include "fileio/file.h"
#include "fileio/file_api.h"
#include "std/std.h"

unsigned int g_codePage = 949;
static struct Block { unsigned char *raw; size_t size; } blocks[1024];
static unsigned live_count, total_allocations, file_closes;
static int fail_allocation, missing_file;
static unsigned char bson[16384];
static size_t bson_size;
static unsigned trace_hash = 2166136261u;
static FILE *trace;

void *mem_alloc(size_t size)
{
    unsigned i;
    for (i = 0; i < 1024 && blocks[i].raw; ++i) {}
    assert(i < 1024);
    blocks[i].raw = malloc(size + 32);
    assert(blocks[i].raw);
    blocks[i].size = size;
    memset(blocks[i].raw, 0xa5, size + 32);
    ++live_count;
    ++total_allocations;
    return blocks[i].raw + 16;
}
void *mem_try_alloc(size_t size) { return fail_allocation ? NULL : mem_alloc(size); }
void mem_free(void *memory)
{
    unsigned i, j;
    if (!memory) return;
    for (i = 0; i < 1024 && (!blocks[i].raw || blocks[i].raw + 16 != memory); ++i) {}
    assert(i < 1024 && live_count);
    for (j = 0; j < 16; ++j) {
        assert(blocks[i].raw[j] == 0xa5);
        assert(blocks[i].raw[16 + blocks[i].size + j] == 0xa5);
    }
    free(blocks[i].raw);
    blocks[i].raw = NULL;
    --live_count;
}
void fatal_error(const char *message) { (void)message; abort(); }

struct File *file_construct(struct File *file) { memset(file, 0, sizeof *file); return file; }
unsigned char file_open(struct File *file, const char *name, int mode, unsigned char a, unsigned char b)
{
    assert(!strcmp(name, "ContentData\\EffectData\\EZ2STREffect.bson"));
    assert(mode == 0 && a == 1 && b == 0);
    if (missing_file) return 0;
    file->buffer = bson;
    file->size = (unsigned)bson_size;
    return 1;
}
void file_destruct(struct File *file) { (void)file; ++file_closes; }

#ifdef EZ2STR_MANAGER_REFERENCE
struct Ez2StrEffectMgr *g_ez2strMgr;
struct Ez2StrEffectMgr *ez2str_mgr_delete(struct Ez2StrEffectMgr *, unsigned char);
static void destroy_manager(void)
{
    if (g_ez2strMgr) ez2str_mgr_delete(g_ez2strMgr, 1);
    g_ez2strMgr = NULL;
}
#else
#define destroy_manager ez2str_effect_mgr_destroy
#endif

static void record(const void *data, size_t size)
{
    const unsigned char *bytes = data;
    size_t i;
    for (i = 0; i < size; ++i) trace_hash = (trace_hash ^ bytes[i]) * 16777619u;
    if (trace) assert(fwrite(data, 1, size, trace) == size);
}
static void record_string(const struct String *s)
{
    record(&s->size, sizeof s->size);
    record(string_cstr(s), s->size);
}
static void append(const void *data, size_t size)
{
    assert(bson_size + size <= sizeof bson);
    memcpy(bson + bson_size, data, size);
    bson_size += size;
}
static void field(unsigned char type, const char *name)
{
    append(&type, 1);
    append(name, strlen(name) + 1);
}
static size_t begin_object(void)
{
    unsigned zero = 0;
    size_t at = bson_size;
    append(&zero, sizeof zero);
    return at;
}
static void end_object(size_t at)
{
    unsigned char zero = 0;
    unsigned size;
    append(&zero, 1);
    size = (unsigned)(bson_size - at);
    memcpy(bson + at, &size, sizeof size);
}
static void text_field(const char *name, const char *value)
{
    unsigned size = (unsigned)strlen(value) + 1;
    field(2, name); append(&size, 4); append(value, size);
}
static void row(const char *name, int long_path, int incomplete)
{
    size_t at;
    double x = 2.5, y = -7.25;
    unsigned char yes = 1, no = 0;
    int quake = -1, delay = 42;
    field(3, name); at = begin_object();
    text_field("FilePath", long_path ? u8"effects/한국어/long_animation_name.str" : "a.str");
    field(1, "PosX"); append(&x, 8);
    field(1, "PosY"); append(&y, 8);
    field(8, "IsInfinite"); append(&yes, 1);
    field(8, "IsFloor"); append(&no, 1);
    text_field("SoundPath", long_path ? u8"sounds/한국어/long_sound_name.wav" : "a.wav");
    field(8, "IsSoundLoop"); append(&yes, 1);
    field(16, "QuakeDelay"); append(&quake, 4);
    if (!incomplete) { field(16, "StartDelayTime"); append(&delay, 4); }
    end_object(at);
}
static void document(int which)
{
    size_t root, table;
    bson_size = 0; root = begin_object();
    field(3, "EZ2STREffect"); table = begin_object();
    if (which == 0) {
        row("Zebra", 1, 0); row("ALPHA", 0, 0); row("alpha", 1, 0);
        row("middle_with_a_heap_backed_name", 1, 0);
    } else if (which == 1) row("Replacement", 1, 0);
    else if (which == 2) row("Invalid", 1, 1);
    end_object(table); end_object(root);
}
static void check_rows(struct Ez2StrEffectMgr *mgr, unsigned count)
{
    struct Vec ids;
    struct String missing;
    const struct String *name;
    unsigned i;
    assert(mgr->rows.size == count);
    ez2_str_effect_mgr_get_effect_ids(mgr, &ids);
    assert(VEC_COUNT(&ids, int) == count);
    record(&count, sizeof count);
    for (i = 0; i < count; ++i) {
        int id = ((int *)ids.first)[i];
        const struct EZ2STREntry *entry = ez2_str_effect_mgr_find_by_effect_id(mgr, id);
        assert(entry && entry->pos_x == 2.5f && entry->pos_y == -7.25f);
        assert(entry->flags == (EZ2STR_EX_INFINITE | EZ2STR_EX_SOUND_LOOP));
        assert(entry->quake_delay == -1 && entry->start_delay == 42);
        name = &((const struct EZ2STRRow *)((const char *)entry - offsetof(struct EZ2STRRow, entry)))->name;
        assert(ez2_str_effect_mgr_get_effect_id(mgr, name) == id);
        record(&id, sizeof id); record_string(name);
        record_string(&entry->file_path); record_string(&entry->sound_path);
    }
    mem_free(ids.first);
    string_init(&missing); string_assign_cstr(&missing, "missing");
    assert(ez2_str_effect_mgr_get_effect_id(mgr, &missing) == -1);
    string_free(&missing);
    assert(!ez2_str_effect_mgr_find_by_effect_id(mgr, -1));
}
int main(int argc, char **argv)
{
    struct Ez2StrEffectMgr *mgr;
    unsigned before, i;
    if (argc > 1) { trace = fopen(argv[1], "wb"); assert(trace); }
    destroy_manager(); assert(!live_count && !total_allocations);
    fail_allocation = 1;
    assert(!ez2str_effect_mgr_instance() && !live_count);
    fail_allocation = 0;
    for (i = 0; i < 3; ++i) {
        mgr = ez2str_effect_mgr_instance(); assert(mgr && !mgr->rows.size);
        before = total_allocations;
        assert(ez2str_effect_mgr_instance() == mgr && total_allocations == before);
        check_rows(mgr, 0);
        document(0); assert(ez2_str_effect_mgr_load(mgr)); check_rows(mgr, 3);
        document(2); assert(!ez2_str_effect_mgr_load(mgr)); check_rows(mgr, 3);
        bson_size = 1; assert(!ez2_str_effect_mgr_load(mgr)); check_rows(mgr, 3);
        missing_file = 1; assert(!ez2_str_effect_mgr_load(mgr)); check_rows(mgr, 3);
        missing_file = 0;
        document(1); assert(ez2_str_effect_mgr_load(mgr)); check_rows(mgr, 1);
        if (i == 1) { document(3); assert(ez2_str_effect_mgr_load(mgr)); check_rows(mgr, 0); }
        destroy_manager(); assert(!live_count);
        before = total_allocations;
        destroy_manager(); assert(!live_count && total_allocations == before);
    }
    assert(file_closes == 16);
    assert(trace_hash == 0x817beee3u);
    if (trace) assert(fclose(trace) == 0);
    printf("EZ2STR manager: BSON reload, queries, duplicate names, failures and cleanup passed; digest %08x.\n", trace_hash);
    return 0;
}

#include "ez2str_manager_unreached.h"
