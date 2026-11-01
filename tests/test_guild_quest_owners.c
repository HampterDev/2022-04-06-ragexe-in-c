#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "contents/guild_mgr.h"
#include "contents/renew_quest.h"
#include "contents/guild_emblem_transport.h"
#include "fileio/resource_api.h"
#include "ui/ui_std.h"
#undef malloc
#undef realloc
#undef free

static struct Block { unsigned char *raw; size_t size; } blocks[4096];
static unsigned live_blocks, try_count, fail_try, sends, saved, changed;
static DWORD now, send_delay;
static unsigned trace_hash = 2166136261u;
static FILE *trace;
static int last_guild, last_version;

static void record(unsigned tag, unsigned value)
{
    unsigned values[2] = {tag, value};
    unsigned char *bytes = (unsigned char *)values;
    size_t i;
    if (trace) assert(fwrite(bytes, 1, sizeof values, trace) == sizeof values);
    for (i = 0; i < sizeof values; ++i) trace_hash = (trace_hash ^ bytes[i]) * 16777619u;
}

static unsigned block_index(void *memory)
{
    unsigned i;
    for (i = 0; i < 4096; ++i)
        if (blocks[i].raw && blocks[i].raw + 16 == memory) return i;
    assert(0);
    return 0;
}

void *test_heap_alloc(size_t size)
{
    unsigned i;
    for (i = 0; i < 4096 && blocks[i].raw; ++i) {}
    assert(i < 4096 && size <= SIZE_MAX - 32);
    blocks[i].raw = malloc(size + 32);
    assert(blocks[i].raw);
    blocks[i].size = size;
    memset(blocks[i].raw, 0xa5, size + 32);
    ++live_blocks;
    return blocks[i].raw + 16;
}

void test_heap_free(void *memory)
{
    unsigned i, j;
    if (!memory) return;
    i = block_index(memory);
    for (j = 0; j < 16; ++j) {
        assert(blocks[i].raw[j] == 0xa5);
        assert(blocks[i].raw[16 + blocks[i].size + j] == 0xa5);
    }
    free(blocks[i].raw);
    blocks[i].raw = NULL;
    assert(live_blocks);
    --live_blocks;
}

void *test_heap_realloc(void *memory, size_t size)
{
    void *result = test_heap_alloc(size);
    if (memory) {
        size_t old_size = blocks[block_index(memory)].size;
        memcpy(result, memory, size < old_size ? size : old_size);
        test_heap_free(memory);
    }
    return result;
}

void *mem_alloc(size_t size) { return test_heap_alloc(size); }
void *mem_try_alloc(size_t size) { return ++try_count == fail_try ? NULL : test_heap_alloc(size); }
void mem_free(void *memory) { test_heap_free(memory); }
void fatal_error(const char *message) { fprintf(stderr, "%s\n", message); abort(); }
void error_msg(const char *message, int flags) { (void)flags; fatal_error(message); }

DWORD guild_quest_test_time(void) { record(1, now); return now; }
int guild_emblem_request(int guild, int version)
{
    ++sends;
    last_guild = guild;
    last_version = version;
    record(2, guild);
    record(3, version);
    now += send_delay;
    return 1;
}

void guild_emblem_save(const unsigned char *data, unsigned size, unsigned guild, int version)
{
    assert(size == 4 && !memcmp(data, "data", 4) && guild == 7 && version == 9);
    ++saved;
    record(4, size);
}
void guild_emblem_changed(unsigned guild) { assert(guild == 7); ++changed; }

struct ResMgr *g_resMgr(void) { abort(); }
void *res_mgr_get_string(struct ResMgr *manager, struct String name) { abort(); }
struct String *guild_mgr_emblem_path(struct GuildMgr *manager, struct String *out,
    unsigned guild, int version) { abort(); }
struct ItemInfo;
struct String *item_info_get_display_name(const struct ItemInfo *item, struct String *out) { abort(); }

#ifdef GUILD_QUEST_OWNER_REFERENCE
const void *vtbl_guild_emblem_req_bundle[] = {emblem_bundle_delete};
static struct GuildEmblemReqBundle *bundle_create(void)
{
    struct GuildEmblemReqBundle *bundle = mem_try_alloc(sizeof *bundle);
    return bundle ? emblem_bundle_construct(bundle) : NULL;
}
static void bundle_destroy(struct GuildEmblemReqBundle *bundle)
{
    if (bundle) emblem_bundle_delete(bundle, 1);
}
static void clear_versions(struct GuildMgr *manager) { std_tree_clear(&manager->guilds, guild_map_erase); }
#else
#define bundle_create emblem_bundle_create
#define bundle_destroy emblem_bundle_destroy
#define clear_versions guild_mgr_clear_versions
#endif

static void assert_send(struct GuildEmblemReqBundle *bundle, int guild, int version)
{
    unsigned before = sends;
    emblem_bundle_process(bundle);
    assert(sends == before + 1 && last_guild == guild && last_version == version);
}

static void quiet(struct GuildEmblemReqBundle *bundle)
{
    unsigned before = sends;
    emblem_bundle_process(bundle);
    assert(sends == before);
}

static void test_queue(void)
{
    struct GuildEmblemReqBundle *bundle = bundle_create();
    int i;
    assert(bundle);
    now = 100;
    quiet(bundle);
    for (i = 1; i <= 16; ++i) {
        emblem_bundle_queue(bundle, i, 1);
        emblem_bundle_queue(bundle, i, 1);
    }
    for (i = 1; i <= 10; ++i) assert_send(bundle, i, 1);
    for (i = 17; i <= 40; ++i) emblem_bundle_queue(bundle, i, 1);
    for (i = 11; i <= 40; ++i) assert_send(bundle, i, 1);
    quiet(bundle);
    emblem_bundle_queue(bundle, 1, 1);
    emblem_bundle_queue(bundle, 1, 2);
    assert_send(bundle, 1, 2);
    now = 60099;
    emblem_bundle_queue(bundle, 1, 1);
    quiet(bundle);
    now = 60100;
    emblem_bundle_queue(bundle, 1, 1);
    assert_send(bundle, 1, 1);
    emblem_bundle_queue(bundle, 0, 3);
    emblem_bundle_queue(bundle, -1, -7);
    quiet(bundle);
    assert_send(bundle, -1, -7);
    bundle_destroy(bundle);
    assert(!live_blocks);

    bundle = bundle_create();
    for (i = 1; i <= 24; ++i) {
        now = 1000 + i * 1000;
        emblem_bundle_queue(bundle, i, i);
        assert_send(bundle, i, i);
    }
    now = 75000;
    for (i = 25; i <= 70; ++i) {
        emblem_bundle_queue(bundle, i, i);
        assert_send(bundle, i, i);
    }
    emblem_bundle_queue(bundle, 16, 16);
    quiet(bundle);
    emblem_bundle_queue(bundle, 1, 1);
    assert_send(bundle, 1, 1);
    bundle_destroy(bundle);
    assert(!live_blocks);

    bundle = bundle_create();
    now = 100; send_delay = 50;
    emblem_bundle_queue(bundle, 77, 4);
    assert_send(bundle, 77, 4);
    send_delay = 0;
    now = 60149;
    emblem_bundle_queue(bundle, 77, 4); quiet(bundle);
    now = 60150;
    emblem_bundle_queue(bundle, 77, 4); assert_send(bundle, 77, 4);
    bundle_destroy(bundle);

    bundle = bundle_create(); now = 0xfffffff0u;
    emblem_bundle_queue(bundle, 9, 1); assert_send(bundle, 9, 1);
    emblem_bundle_queue(bundle, 9, 1); assert_send(bundle, 9, 1);
    bundle_destroy(bundle);
    bundle_destroy(NULL);
    assert(!live_blocks);
}

static struct String text(const char *bytes)
{
    struct String result;
    string_init(&result);
    string_assign_cstr(&result, bytes);
    return result;
}

static void test_guild(void)
{
    int cycle, i;
    for (i = 1; i <= 2; ++i) {
        try_count = 0; fail_try = i;
        assert(!guild_mgr_create() && !g_guildMgr && !live_blocks);
        guild_mgr_destroy();
    }
    fail_try = 0;
    for (cycle = 0; cycle < 3; ++cycle) {
        assert(guild_mgr_create() && g_guildMgr);
        assert(!guild_mgr_create());
        assert(g_guildMgr->emblem_max_size == GUILD_EMBLEM_MAX_SIZE);
        assert(!guild_mgr_emblem_version(g_guildMgr, 1));
        for (i = 0; i < 97; ++i) guild_mgr_set_emblem_version(g_guildMgr, (i * 31) % 97, i - 50);
        for (i = 0; i < 97; ++i) {
            int version = guild_mgr_emblem_version(g_guildMgr, (i * 31) % 97);
            assert(version == i - 50); record(5, version);
        }
        guild_mgr_set_emblem_version(g_guildMgr, 7, 9);
        now = 500; emblem_bundle_queue(g_guildMgr->requests, 7, guild_mgr_emblem_version(g_guildMgr, 7));
        guild_mgr_update_requests(g_guildMgr);
        assert(last_guild == 7 && last_version == 9);
        guild_mgr_mark_bad_emblem(g_guildMgr, text("path\\long_failed_emblem_name.ebm"));
        guild_mgr_mark_bad_emblem(g_guildMgr, text("other\\long_failed_emblem_name.ebm"));
        assert(g_guildMgr->bad_emblems.size == 1);
        assert(guild_mgr_is_bad_emblem(g_guildMgr, text("long_failed_emblem_name.ebm")));
        assert(!guild_mgr_is_bad_emblem(g_guildMgr, text("good.ebm")));
        assert(guild_mgr_download_chunk(g_guildMgr, 0, 7, 9, NULL, 0));
        assert(guild_mgr_download_chunk(g_guildMgr, 1, 7, 9, "data", 4));
        assert(guild_mgr_download_chunk(g_guildMgr, 2, 7, 9, NULL, 0));
        clear_versions(g_guildMgr);
        assert(!g_guildMgr->guilds.size && !guild_mgr_emblem_version(g_guildMgr, 7));
        assert(g_guildMgr->bad_emblems.size == 1 && g_guildMgr->download.size == 4);
        guild_mgr_set_emblem_version(g_guildMgr, 7, 12);
        guild_mgr_destroy(); guild_mgr_destroy();
        assert(!g_guildMgr && !live_blocks);
    }
    assert(saved == 3 && changed == 3);
}

static void fill_quests(void)
{
    unsigned i, j;
    for (i = 1; i <= 25; ++i) {
        struct RenewQuestB *b = renew_quest_b_at(&g_questTable->quests_b, &i);
        struct String line = text("한국어 퀘스트 긴 설명 ownership test");
        for (j = 0; j < 7; ++j) {
            string_assign(&b->base.text[j], string_cstr(&line), line.size);
        }
        string_assign(&b->reward_exp, string_cstr(&line), line.size);
        string_assign(&b->reward_job_exp, string_cstr(&line), line.size);
        string_vec_assign(&b->description, &line, &line + 1);
        {
            struct RenewQuestReward reward = {i, 2};
            vec_push(&b->rewards, &reward, sizeof reward);
        }
        b->base.npc_x = i; b->plain.deadline.year = 2030 + i;
        record(7, b->plain.deadline.year);
        string_free(&line);
    }
}

static void test_quest(void)
{
    int cycle;
    try_count = 0; fail_try = 1;
    assert(!renew_quest_create() && !g_questTable && !live_blocks);
    renew_quest_destroy(); fail_try = 0;
    for (cycle = 0; cycle < 3; ++cycle) {
        assert(renew_quest_create() && !renew_quest_create());
        assert(g_questTable->hunt_name_length == 24);
        fill_quests();
        assert(g_questTable->quests_b.size == 25);
        fill_quests();
        renew_quest_destroy(); renew_quest_destroy();
        assert(!g_questTable && !live_blocks);
    }
}

int main(int argc, char **argv)
{
    if (argc > 1) { trace = fopen(argv[1], "wb"); assert(trace); }
    test_queue(); test_guild(); test_quest();
    if (trace) assert(!fclose(trace));
    printf("Guild/quest owners: queues, retry timing, maps, values and failure cleanup passed; digest %08x.\n", trace_hash);
    return 0;
}

#include "guild_quest_owners_unreached.h"
