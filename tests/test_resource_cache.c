#include "fileio/res_mgr.h"
#include "resources/resource_factories.h"
#include "diag/perf.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "fileio/resource_errors.h"
#include "fileio/resource_path.h"
#include <mmsystem.h>

static unsigned errors;
void resource_error_report(const char *name, const char *path)
{
    assert(name && *name && path && *path);
    ++errors;
}

struct perf_counters g_perf;
static struct ResMgr manager;
static unsigned created, destroyed, cleaned, updated;
static int fail_factory, fail_load, recursive_load, inside_load;
static struct Res *release_during_cleanup, *remove_on_update;
static unsigned update_order[8];

struct TestResource {
    struct Res base;
    unsigned id;
    void *payload;
};

static void before_delete(struct Res *res)
{
    struct Res *other = release_during_cleanup;
    assert(res_mgr_find(&manager, res->typeIndex, res->name.name) != res);
    ++cleaned;
    release_during_cleanup = NULL;
    if (other)
        res_mgr_release(&manager, other);
}

static unsigned payloads;
static void destroy_resource(struct Res *res)
{
    struct TestResource *self = (struct TestResource *)res;
    before_delete(res);
    if (self->payload) {
        free(self->payload);
        --payloads;
    }
    ++destroyed;
    free(res);
}

static void update_resource(struct Res *res)
{
    assert(updated < sizeof update_order / sizeof *update_order);
    update_order[updated++] = ((struct TestResource *)res)->id;
    if (res == remove_on_update) {
        remove_on_update = NULL;
        res_mgr_release(&manager, res);
    }
}

static int is_resident(const struct Res *res) { (void)res; return 1; }
static int load_resource(struct Res *res, const char *path)
{
    ((struct TestResource *)res)->payload = malloc(16);
    assert(((struct TestResource *)res)->payload);
    ++payloads;
    assert(strchr(path, '\\'));
    if (recursive_load && !inside_load) {
        inside_load = 1;
        assert(res_mgr_get(&manager, path));
        inside_load = 0;
    }
    return !fail_load;
}

static const struct ResourceOps test_ops = {
    .destroy = destroy_resource,
    .load = load_resource,
    .update = update_resource,
    .is_resident = is_resident,
};

static struct Res *create_resource(void)
{
    struct TestResource *res;
    if (fail_factory)
        return NULL;
    res = calloc(1, sizeof *res);
    assert(res);
    res_construct(&res->base);
    res->base.ops = &test_ops;
    res->id = ++created;
    return &res->base;
}

#define FACTORY(name) struct Res *name(void) { return create_resource(); }
FACTORY(animated_model_resource_create)
FACTORY(attribute_resource_create)
FACTORY(granny_model_resource_create)
FACTORY(model_resource_v1_create)
FACTORY(model_resource_v2_create)
FACTORY(world_resource_create)
FACTORY(animation_resource_create)
FACTORY(bitmap_resource_create)
FACTORY(effect_resource_create)
FACTORY(emblem_resource_create)
FACTORY(gif_resource_create)
FACTORY(ground_resource_create)
FACTORY(imf_resource_create)
FACTORY(palette_resource_create)
FACTORY(rma_resource_create)
FACTORY(sprite_resource_create)
FACTORY(wave_resource_create)

void *mem_alloc(size_t size) { void *p = malloc(size); assert(p); return p; }
void mem_free(void *p)
{  free(p); }

static const char *file_text;
static int file_size, file_available, file_readable;
static unsigned reads, closes, file_destructs;
struct File *file_construct(struct File *file) { memset(file, 0, sizeof *file); return file; }
unsigned char file_open(struct File *file, const char *name, int mode, unsigned char pak, unsigned char hint)
{
    (void)mode; (void)pak; (void)hint;
    assert(strcmp(name, "ResNameTable.txt") == 0);
    file->size = file_size;
    return (unsigned char)file_available;
}
unsigned char file_read(struct File *file, void *out, int size)
{
    (void)file;
    ++reads;
    if (!file_readable)
        return 0;
    assert(size >= 0);
    if (size)
        memcpy(out, file_text, (size_t)size);
    return 1;
}
void file_close(struct File *file) { (void)file; ++closes; }
void file_destruct(struct File *file) { (void)file; ++file_destructs; }

static void test_storage(void)
{
    enum { COUNT = 4096 };
    struct ResourceList cache = {0}, residents = {0};
    struct Res *values = calloc(COUNT, sizeof *values);
    struct Res duplicate;
    struct Hash missing = {0};
    int i;
    assert(values);
    for (i = COUNT - 1; i >= 0; --i) {
        values[i].name.hash = 0x80000000u + (unsigned)i % 7;
        sprintf(values[i].name.name, "%06d", i);
        assert(resource_cache_insert(&cache, values + i));
    }
    assert(cache.count == COUNT);
    for (i = 0; i < COUNT; ++i) {
        assert(resource_cache_find(&cache, &values[i].name) == values + i);
        if (i)
            assert(hash_less(&cache.items[i - 1]->name, &cache.items[i]->name));
    }
    duplicate = values[1337];
    assert(!resource_cache_insert(&cache, &duplicate));
    assert(resource_cache_find(&cache, &duplicate.name) == values + 1337);
    assert(!resource_cache_find(&cache, &missing));
    assert(!resource_list_take(&cache, COUNT));
    resource_list_remove(&cache, values + 1337);
    assert(!resource_cache_find(&cache, &duplicate.name));
    assert(resource_cache_insert(&cache, values + 1337));
    resource_list_append(&residents, values + 9);
    resource_list_append(&residents, values + 2);
    resource_list_append(&residents, values + 7);
    resource_list_remove(&residents, values + 2);
    assert(resource_list_take(&residents, 0) == values + 9);
    assert(resource_list_take(&residents, 0) == values + 7);
    assert(!resource_list_take(&residents, 0));
    resource_list_release(&residents);
    resource_list_release(&cache);
    resource_list_release(&cache);
    assert(!cache.items && !cache.count && !cache.capacity);
    free(values);
}

static void test_aliases(void)
{
    struct ResourceAliases aliases = {0};
    char name[32] = "original", value[32] = "owned";
    const char *held, *unknown = "missing", *variant = "prefix123x@tail";
    const char fields[] = "first#one#first#ignored#\r\nsecond#junk\r\ntwo#empty##\n#third#three#trailing#unfinished";
    const char embedded[] = "nul\0extra#value\0ignored#";
    unsigned i;
    assert(resource_alias_add(&aliases, name, value));
    held = resource_alias_find(&aliases, name);
    name[0] = value[0] = 'X';
    assert(strcmp(held, "owned") == 0);
    for (i = 0; i < 4096; ++i) {
        sprintf(name, "%06u", i);
        assert(resource_alias_add(&aliases, name, "value"));
    }
    assert(strcmp(held, "owned") == 0);
    assert(!resource_alias_add(&aliases, "original", "replacement"));
    assert(resource_alias_resolve(&aliases, "original") == held);
    assert(resource_alias_resolve(&aliases, unknown) == unknown);
    assert(!resource_alias_resolve(&aliases, NULL));
    resource_alias_add(&aliases, "prefixx@tail", "fallback");
    assert(strcmp(resource_alias_resolve(&aliases, variant), "fallback") == 0);
    resource_alias_add(&aliases, variant, "exact");
    assert(strcmp(resource_alias_resolve(&aliases, variant), "exact") == 0);
    resource_alias_add(&aliases, "a@x@tail", "last-at");
    assert(strcmp(resource_alias_resolve(&aliases, "a@123x@tail"), "last-at") == 0);
    resource_alias_parse(&aliases, fields, sizeof fields - 1);
    assert(aliases.count == 4);
    assert(strcmp(resource_alias_find(&aliases, "first"), "one") == 0);
    assert(strcmp(resource_alias_find(&aliases, "second"), "two") == 0);
    assert(strcmp(resource_alias_find(&aliases, "empty"), "") == 0);
    assert(strcmp(resource_alias_find(&aliases, "third"), "three") == 0);
    assert(!resource_alias_find(&aliases, "trailing"));
    resource_alias_parse(&aliases, embedded, sizeof embedded - 1);
    assert(aliases.count == 1);
    assert(strcmp(resource_alias_find(&aliases, "nul"), "value") == 0);
    resource_alias_parse(&aliases, NULL, 0);
    resource_alias_release(&aliases);
    resource_alias_release(&aliases);
    assert(!aliases.items && !aliases.count && !aliases.capacity);
}

static void test_lifecycle(void)
{
    struct Res *a, *b, *c;
    unsigned before, first_id, second_id, third_id;
    int bmp = resource_type_index("bmp");
    res_mgr_construct(&manager);
    assert(!res_mgr_get(&manager, NULL));
    assert(!res_mgr_get(&manager, ""));
    assert(!res_mgr_get(&manager, "unknown.xyz"));
    assert(!res_mgr_find(&manager, -1, "x"));
    assert(!res_mgr_find(&manager, RESOURCE_TYPE_COUNT, "x"));
    assert(!res_mgr_find(&manager, bmp, NULL));
    a = res_mgr_get(&manager, "A.BMP");
    assert(a && strcmp(a->name.name, "texture\\a.bmp") == 0);
    assert(res_mgr_get(&manager, "A.BMP") == a);
    assert(res_mgr_get(&manager, "texture\\a.bmp") == a);
    assert(created == 1 && manager.resources[bmp].count == 1);
    b = res_mgr_get(&manager, "b.bmp");
    c = res_mgr_get(&manager, "c.wav");
    first_id = ((struct TestResource *)a)->id;
    second_id = ((struct TestResource *)b)->id;
    third_id = ((struct TestResource *)c)->id;
    remove_on_update = a;
    res_mgr_call_residents(&manager);
    assert(updated == 3 && update_order[0] == first_id && update_order[1] == second_id && update_order[2] == third_id);
    assert(manager.resident.count == 2 && destroyed == 1);
    b->lockCount = 1;
    res_mgr_purge_by_extension(&manager, "bmp");
    assert(manager.resources[bmp].count == 1);
    res_mgr_purge_unlocked(&manager);
    assert(manager.resident.count == 1 && manager.resident.items[0] == b);
    b->lockCount = 0;
    b->lastUse = timeGetTime() - g_resPurgeAgeMs - 100;
    res_mgr_purge_stale(&manager);
    assert(!manager.resident.count && destroyed == created);
    a = res_mgr_get(&manager, "A.BMP");
    b = res_mgr_get(&manager, "b.bmp");
    before = destroyed;
    release_during_cleanup = b;
    res_mgr_release(&manager, a);
    assert(destroyed == before + 2 && !manager.resident.count);
    res_mgr_release(&manager, NULL);
    assert(res_mgr_get(&manager, "purge_a.bmp"));
    assert(res_mgr_get(&manager, "purge_b.bmp"));
    assert(res_mgr_get(&manager, "purge_c.bmp"));
    before = destroyed;
    release_during_cleanup = manager.resources[bmp].items[1];
    res_mgr_purge_by_extension(&manager, "bmp");
    assert(destroyed == before + 3 && !manager.resident.count);
    before = created;
    recursive_load = 1;
    a = res_mgr_get(&manager, "recursive.bmp");
    recursive_load = 0;
    assert(a && created == before + 2);
    assert(manager.resources[bmp].count == 1 && manager.resident.count == 1);
    assert(res_mgr_get(&manager, "recursive.bmp") == a);
    assert(destroyed + 1 == created);
    b = res_mgr_get(&manager, "delete_all.bmp");
    release_during_cleanup = a;
    if (manager.resources[bmp].items[0] == a)
        release_during_cleanup = b;
    res_mgr_delete_all(&manager);
    assert(destroyed == created && !manager.resident.count);
    before = cleaned;
    fail_load = 1;
    assert(!res_mgr_get(&manager, "failed.bmp"));
    fail_load = 0;
    assert(errors == 1 && destroyed == created && cleaned == before + 1 && !payloads);
    fail_factory = 1;
    assert(!res_mgr_get(&manager, "unallocated.bmp"));
    fail_factory = 0;
    assert(errors == 1 && destroyed == created);
    assert(res_mgr_get(&manager, "failed.bmp"));
    res_mgr_destruct(&manager);
    assert(destroyed == created && !payloads);
    assert(!manager.resident.items && !manager.aliases.items);
    for (bmp = 0; bmp < RESOURCE_TYPE_COUNT; ++bmp)
        assert(!manager.resources[bmp].items);
}

static void test_alias_file(void)
{
    unsigned before;
    res_mgr_construct(&manager);
    file_text = "a.bmp#b.bmp#";
    file_size = (int)strlen(file_text);
    file_available = file_readable = 1;
    res_mgr_read_res_name_table(&manager);
    assert(strcmp(res_mgr_get_alias(&manager, "a.bmp"), "b.bmp") == 0);
    file_readable = 0;
    res_mgr_read_res_name_table(&manager);
    assert(!manager.aliases.count);
    before = reads;
    file_size = -1;
    res_mgr_read_res_name_table(&manager);
    assert(reads == before && !manager.aliases.count);
    file_available = 0;
    res_mgr_read_res_name_table(&manager);
    assert(reads == before && closes == 3 && file_destructs == 4);
    file_available = file_readable = 1;
    file_size = 0;
    res_mgr_read_res_name_table(&manager);
    assert(!manager.aliases.count);
    res_mgr_destruct(&manager);
}

static void test_resource_paths(void)
{
    char name[300], path[252], normalized[16];
    char *map;
    unsigned before = created;
    int bmp = resource_type_index("bmp");
    struct Res *res;
    res_mgr_construct(&manager);
    assert(resource_normalize_name(normalized, sizeof normalized, "A\x81\x82.BMP"));
    assert(strcmp(normalized, "a\x81\x82.bmp") == 0);
    assert(!resource_normalize_name(normalized, 3, "ABC"));
    assert(!*normalized);
    assert(!resource_normalize_name(NULL, 0, "a"));
    assert(!resource_normalize_name(normalized, sizeof normalized, NULL));
    assert(resource_build_path(path, sizeof path, "file.bmp", "texture\\"));
    assert(strcmp(path, "texture\\file.bmp") == 0);
    assert(resource_build_path(path, sizeof path, "texture\\file.bmp", "texture\\"));
    assert(strcmp(path, "texture\\file.bmp") == 0);
    assert(resource_build_path(path, sizeof path, "file.gat", ""));
    assert(strcmp(path, "file.gat") == 0);
    memset(name, 'x', 239);
    strcpy(name + 239, ".bmp");
    assert(resource_build_path(path, sizeof path, name, "texture\\"));
    assert(strlen(path) == sizeof path - 1);
    res = res_mgr_get(&manager, name);
    assert(res && created == before + 1);
    assert(res_mgr_is_available(&manager, name));
    assert(res_mgr_find(&manager, bmp, path) == res);
    memset(name, 'x', 240);
    strcpy(name + 240, ".bmp");
    assert(!resource_build_path(path, sizeof path, name, "texture\\"));
    assert(!*path);
    assert(!res_mgr_get(&manager, name));
    assert(!res_mgr_is_available(&manager, name));
    memset(name, 'x', sizeof name - 1);
    name[sizeof name - 1] = 0;
    assert(!res_mgr_get(&manager, name));
    assert(!res_mgr_find(&manager, bmp, name));
    resource_alias_add(&manager.aliases, "oversized.bmp", name);
    assert(!res_mgr_get(&manager, "oversized.bmp"));
    assert(created == before + 1);
    map = res_mgr_resolve_map_name(&manager, "town");
    assert(strcmp(map, "town") == 0);
    free(map);
    resource_alias_add(&manager.aliases, "town.gat", "other.gat");
    map = res_mgr_resolve_map_name(&manager, "town");
    assert(strcmp(map, "other") == 0);
    resource_alias_release(&manager.aliases);
    assert(strcmp(map, "other") == 0);
    free(map);
    resource_alias_add(&manager.aliases, "town.gat", "without_suffix");
    map = res_mgr_resolve_map_name(&manager, "town");
    assert(strcmp(map, "without_suffix") == 0);
    free(map);
    resource_alias_add(&manager.aliases, "empty.gat", ".gat_tail");
    map = res_mgr_resolve_map_name(&manager, "empty");
    assert(!*map);
    free(map);
    map = res_mgr_resolve_map_name(&manager, NULL);
    assert(!*map);
    free(map);
    res_mgr_destruct(&manager);
    assert(created == destroyed);
}

/* ResNameTable keeps CP949 names: a UTF-8 name without an alias of its own uses its
   CP949 spelling's alias. 한글 and 가 in CP949 and UTF-8. */
static void test_cp949_aliases(void)
{
    struct Res *res;
    res_mgr_construct(&manager);
    /* 한글: only its CP949 spelling has an alias. */
    resource_alias_add(&manager.aliases, "\xc7\xd1\xb1\xdb.bmp", "alias.bmp");
    /* 국: both spellings have one, and the UTF-8 name keeps its own. */
    resource_alias_add(&manager.aliases, "\xea\xb5\xad.bmp", "own.bmp");
    resource_alias_add(&manager.aliases, "\xb1\xb9.bmp", "other.bmp");
    res = res_mgr_get(&manager, "\xed\x95\x9c\xea\xb8\x80.bmp");
    assert(res && strcmp(res->name.name, "texture\\alias.bmp") == 0);
    assert(res_mgr_get(&manager, "\xc7\xd1\xb1\xdb.bmp") == res);
    res = res_mgr_get(&manager, "\xea\xb5\xad.bmp");
    assert(res && strcmp(res->name.name, "texture\\own.bmp") == 0);
    /* 가 has no alias; the file layer finds its CP949 spelling. */
    res = res_mgr_get(&manager, "\xea\xb0\x80.bmp");
    assert(res && strcmp(res->name.name, "texture\\\xea\xb0\x80.bmp") == 0);
    res_mgr_destruct(&manager);
    assert(created == destroyed);
}

int main(void)
{
    test_storage();
    test_aliases();
    test_lifecycle();
    test_alias_file();
    test_resource_paths();
    test_cp949_aliases();
    puts("resource cache: growth, aliases, loading, residents, recursive cleanup, purge and failures passed");
    return 0;
}
