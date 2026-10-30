#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "game/gamemode.h"
#include "game/emblem_snapshot.h"
#include "web/name_card_snapshot.h"
#include "contents/guild_emblem_api.h"
#include "contents/guild_info.h"
#include "fileio/file_api.h"
#include "replay/replay.h"
#include "replay/snapshot.h"
#include "fixtures/emblems_wire.h"
#include "fixtures/name_cards_wire.h"
static struct Block
{
    unsigned char *base;
    size_t size;
} blocks[256];
static int allocations, attempts, fail_at;
static int block_index(void *p)
{
    int i;
    for (i = 0; i < 256; ++i)
        if (blocks[i].base && blocks[i].base + 8 == p)
            return i;
    assert(0);
    return -1;
}
static void check_guards(void)
{
    int i, j;
    for (i = 0; i < 256; ++i)
        if (blocks[i].base)
            for (j = 0; j < 8; ++j)
            {
                assert(blocks[i].base[j] == 0xa7);
                assert(blocks[i].base[8 + blocks[i].size + j] == 0xa7);
            }
}
void *allocate(size_t size)
{
    int i;
    if (++attempts == fail_at)
        return NULL;
    for (i = 0; i < 256 && blocks[i].base; ++i)
    {
    }
    assert(i < 256 && size <= SIZE_MAX - 16);
    blocks[i].base = malloc(size + 16);
    assert(blocks[i].base);
    blocks[i].size = size;
    memset(blocks[i].base, 0xa7, size + 16);
    ++allocations;
    return blocks[i].base + 8;
}
void release(void *p)
{
    int i;
    if (!p)
        return;
    check_guards();
    i = block_index(p);
    free(blocks[i].base);
    blocks[i].base = NULL;
    --allocations;
}
void *resize(void *p, size_t size)
{
    void *result = allocate(size);
    if (result && p)
    {
        size_t old_size = blocks[block_index(p)].size;
        memcpy(result, p, size < old_size ? size : old_size);
        release(p);
    }
    return result;
}

static FILE *cache_open(const char *path, const char *mode);
static size_t cache_write(const void *data, size_t size, size_t count, FILE *stream);
static int cache_close(FILE *stream);
#define fopen cache_open
#define fwrite cache_write
#define fclose cache_close
#define malloc allocate
#define realloc resize
#define free release
#include "../src/replay/snapshot.c"
#include "../src/game/replay_snapshots.c"
#undef malloc
#undef realloc
#undef free
#undef fopen
#undef fwrite
#undef fclose

void *mem_alloc(size_t size)
{
    int keep_attempts = attempts, keep_failure = fail_at;
    void *p;
    fail_at = 0;
    p = allocate(size);
    attempts = keep_attempts;
    fail_at = keep_failure;
    assert(p);
    return p;
}
void mem_free(void *p)
{
    release(p);
}
void fatal_error(const char *s)
{
    (void)s;
    abort();
}
struct RagConnection *rag_connection_instance(void) { abort(); }
char rag_connection_send_packet(struct RagConnection *c, int n, const void *p)
{
    (void)c;
    (void)n;
    (void)p;
    abort();
}
struct GuildMgr *g_guildMgr = (struct GuildMgr *)1;
struct GuildInfo g_guildInfo;
char g_clientDir[] = "C:\\Client";
static struct GameMode mode;
static struct World world;
static struct Player player;
static struct GameActor actors[3];
static struct ListNode head;
static struct { struct ListNode links; void *value; } nodes[3];
static int cache_mode, no_create, read_failure, write_failure, constructs, destructs, opens, writes,
    guild_calls, last_version;
static FILE cache_streams[2];
static int cache_live[2], cache_closed;
static unsigned char written[2][8];
static size_t written_size[2];
static struct String owned(const char *text)
{
    struct String s;
    string_init(&s);
    string_assign_cstr(&s, text);
    return s;
}
int guild_mgr_emblem_version(struct GuildMgr *manager, unsigned guild)
{
    assert(manager == g_guildMgr);
    return guild == 10 ? 1 : guild == 20 ? 2 : 0;
}
struct String *guild_mgr_emblem_file_name(struct GuildMgr *manager, struct String *out,
                                             unsigned guild)
{
    assert(manager == g_guildMgr && (guild == 10 || guild == 20));
    *out = owned(guild == 10 ? "world_10_2147483648.ebm" : "world_20_2147483649.ebm");
    return out;
}
void game_actor_set_guild(struct GameActor *actor, int guild, int version)
{
    assert(actor == (struct GameActor *)&player);
    ++guild_calls;
    last_version = version;
    if (guild > 0)
        actor->guild_id = guild;
}
struct File *file_construct(struct File *file)
{
    memset(file, 0, sizeof *file);
    ++constructs;
    return file;
}
unsigned char file_open(struct File *file, const char *path, int mode_value,
                         unsigned char resolve, unsigned char disk)
{
    static unsigned char first[] = {1, 2, 3, 4}, second[] = {0xaa, 0xbb};
    int first_file = strstr(path, "world_10_") != NULL;
    (void)resolve;
    (void)disk;
    ++opens;
    assert(!strncmp(path, g_clientDir, strlen(g_clientDir)) && strstr(path, "\\_tmpEmblem\\"));
    assert(first_file || strstr(path, "world_20_"));
    file->position = first_file ? 0 : 1;
    assert(mode_value == 0);
    if (cache_mode == 1)
        return 0;
    file->buffer = first_file ? first : second;
    file->size = first_file ? 4 : 2;
    return 1;
}
unsigned char file_read(struct File *file, void *out, int size)
{
    assert(!cache_mode && size == file->size);
    if (read_failure)
        return 0;
    memcpy(out, file->buffer, size);
    return 1;
}
static FILE *cache_open(const char *path, const char *mode)
{
    int index = strstr(path, "world_10_") ? 0 : 1;
    assert(cache_mode == 1 && !strcmp(mode, "wb"));
    assert(!strncmp(path, g_clientDir, strlen(g_clientDir)) && strstr(path, "\\_tmpEmblem\\"));
    assert(index == 0 || strstr(path, "world_20_"));
    ++opens;
    if (no_create)
        return NULL;
    assert(!cache_live[index]);
    cache_live[index] = 1;
    return &cache_streams[index];
}
static size_t cache_write(const void *data, size_t size, size_t count, FILE *stream)
{
    size_t index = (size_t)(stream - cache_streams);
    assert(index < 2 && cache_live[index] && size == 1 && count <= 8);
    ++writes;
    if (write_failure)
        return 0;
    written_size[index] = count;
    if (count)
        memcpy(written[index], data, count);
    return count;
}
static int cache_close(FILE *stream)
{
    size_t index = (size_t)(stream - cache_streams);
    assert(index < 2 && cache_live[index]);
    cache_live[index] = 0;
    ++cache_closed;
    return 0;
}
void file_close(struct File *file) { (void)file; }
void file_destruct(struct File *file)
{
    (void)file;
    ++destructs;
}
static void reset_effects(void)
{
    assert(!cache_live[0] && !cache_live[1]);
    cache_closed = 0;
    constructs = destructs = opens = writes = guild_calls = 0;
    g_guildInfo.guild_id = -77;
    player.guild_id = 10;
}
static void untouched(void)
{
    struct ReplayBuffer names = {0};
    assert(!constructs && !destructs && !opens && !writes && !guild_calls &&
           g_guildInfo.guild_id == -77);
    assert(name_cards_snapshot_save(&mode.actor_names, &names));
    assert(names.size == sizeof expected_names &&
           !memcmp(names.data, expected_names, sizeof expected_names));
    replay_buffer_destroy(&names);
}
static void bad(const void *data, size_t size)
{
    reset_effects();
    assert(game_emblems_load(&mode, data, size) == -1);
    untouched();
}
static void setup(void)
{
    size_t i;
    mode.world = &world;
    world.player = &player;
    world.actors.head = &head;
    head.next = &nodes[0].links;
    for (i = 0; i < 3; ++i)
    {
        nodes[i].value = &actors[i];
        nodes[i].links.next = i == 2 ? &head : &nodes[i + 1].links;
        actors[i].guild_id = i == 0 ? 20 : i == 1 ? 10 : 0;
    }
    name_card_holder_init(&mode.actor_names);
    name_card_holder_set_all(&mode.actor_names, 11, owned("alpha_actor"), owned("Alpha party"),
                      owned("Alpha guild"), owned("Officer"), owned("achievement"),
                      owned("NPC title"), 3);
    name_card_holder_set_all(&mode.actor_names, 0x80000002u, owned("beta_actor_with_long_name"), owned(""),
                      owned("Beta guild"), owned("Captain"), owned(""), owned(""), 0);
    reset_effects();
}
static void wire_and_bounds(void)
{
    struct ReplayBuffer out = {0};
    unsigned char bytes[sizeof expected_emblems + 1];
    size_t i;
    assert(game_emblems_save(&mode, &out));
    assert(out.size == sizeof expected_emblems && !memcmp(out.data, expected_emblems, out.size));
    assert(constructs == 2 && destructs == 2);
    replay_buffer_destroy(&out);
    cache_mode = 1;
    for (i = 0; i < sizeof expected_emblems; ++i)
        bad(expected_emblems, i);
    bad(NULL, sizeof expected_emblems);
    bad(expected_emblems, (size_t)INT_MAX + 1);
    memcpy(bytes, expected_emblems, sizeof expected_emblems);
    bytes[2] = 1;
    bad(bytes, sizeof expected_emblems);
    memcpy(bytes, expected_emblems, sizeof expected_emblems);
    memset(bytes + 12, 0xff, 4);
    bad(bytes, sizeof expected_emblems);
    memcpy(bytes, expected_emblems, sizeof expected_emblems);
    bytes[sizeof expected_emblems - 4] = 1;
    bad(bytes, sizeof expected_emblems);
    reset_effects();
    assert(game_emblems_load(&mode, expected_emblems, sizeof expected_emblems) ==
           sizeof expected_emblems);
    assert(guild_calls == 1 && last_version == 1 && g_guildInfo.guild_id == 10 && writes == 2 &&
           cache_closed == 2 && constructs == destructs);
    assert(written_size[0] == 4 && !memcmp(written[0], "\1\2\3\4", 4));
    assert(written_size[1] == 2 && written[1][0] == 0xaa && written[1][1] == 0xbb);
    cache_mode = 2;
    reset_effects();
    assert(game_emblems_load(&mode, expected_emblems, sizeof expected_emblems) > 0 && !writes &&
           constructs == destructs);
    cache_mode = 1;
    no_create = 1;
    reset_effects();
    assert(game_emblems_load(&mode, expected_emblems, sizeof expected_emblems) > 0 && !writes &&
           constructs == destructs);
    no_create = 0;
    write_failure = 1;
    reset_effects();
    assert(game_emblems_load(&mode, expected_emblems, sizeof expected_emblems) > 0 && writes == 2 &&
           cache_closed == 2 && constructs == destructs && guild_calls == 1);
    write_failure = 0;
}
static void record_cases(void)
{
    const char *invalid[] = {"../outside", "dir\\name", "C:bad",     "bad:name", "NUL", "CON.ebm",
                             "COM1.ebm",   "LPT9.foo",  "trailing.", "space ",   "",    "bad?name"};
    struct ReplayBuffer out = {0};
    struct SnapshotWriter writer = {0};
    unsigned char value[64];
    size_t i, n;
    for (i = 0; i < sizeof invalid / sizeof invalid[0]; ++i)
    {
        n = strlen(invalid[i]);
        write_word(value, (uint32_t)n);
        memcpy(value + 4, invalid[i], n);
        snapshot_write(&writer, EMBLEMS_BEGIN, NULL, 0);
        snapshot_write(&writer, EMBLEM_FILE, value, n + 4);
        snapshot_write(&writer, EMBLEMS_END, NULL, 0);
        assert(snapshot_finish(&writer, &out));
        bad(out.data, out.size);
        replay_buffer_destroy(&out);
    }
    write_word(value, 4);
    memcpy(value + 4, "a\0bc", 4);
    snapshot_write(&writer, EMBLEMS_BEGIN, NULL, 0);
    snapshot_write(&writer, EMBLEM_FILE, value, 8);
    snapshot_write(&writer, EMBLEMS_END, NULL, 0);
    assert(snapshot_finish(&writer, &out));
    bad(out.data, out.size);
    replay_buffer_destroy(&out);
    for (n = 0; n < 7; ++n)
        if (n != 4)
        {
            snapshot_write(&writer, EMBLEMS_BEGIN, NULL, 0);
            snapshot_write(&writer, ACTOR_NAMES, expected_names, sizeof expected_names);
            snapshot_write(&writer, PLAYER_GUILD, value, n);
            snapshot_write(&writer, EMBLEMS_END, NULL, 0);
            assert(snapshot_finish(&writer, &out));
            bad(out.data, out.size);
            replay_buffer_destroy(&out);
        }
    snapshot_write(&writer, EMBLEMS_BEGIN, NULL, 0);
    snapshot_write(&writer, 0x7fff, "unknown", 7);
    snapshot_write(&writer, EMBLEMS_END, NULL, 0);
    assert(snapshot_finish(&writer, &out));
    reset_effects();
    assert(game_emblems_load(&mode, out.data, out.size) == (int)out.size);
    untouched();
    replay_buffer_destroy(&out);
}
static void allocation_failures(void)
{
    struct ReplayBuffer out = {0};
    int count, i, base = allocations;
    unsigned char *old;
    cache_mode = 0;
    reset_effects();
    attempts = 0;
    assert(game_emblems_save(&mode, &out));
    count = attempts;
    replay_buffer_destroy(&out);
    assert(allocations == base);
    for (i = 1; i <= count; ++i)
    {
        out.data = allocate(3);
        out.size = 3;
        memcpy(out.data, "old", 3);
        old = out.data;
        attempts = 0;
        fail_at = i;
        reset_effects();
        assert(!game_emblems_save(&mode, &out));
        fail_at = 0;
        assert(out.data == old && out.size == 3 && !memcmp(out.data, "old", 3) &&
               constructs == destructs);
        replay_buffer_destroy(&out);
        assert(allocations == base);
    }
    read_failure = 1;
    assert(!game_emblems_save(&mode, &out) && !out.data && constructs == destructs);
    read_failure = 0;
}
static int io_fail, empty_input, short_input;
int replay_read_chunk(struct ReplayRecorder *recorder, int id, struct ReplayBuffer *out)
{
    assert(recorder && id == 0x15);
    if (io_fail)
        return 0;
    if (empty_input)
        return 1;
    out->size = sizeof expected_emblems - short_input;
    out->data = allocate(out->size);
    assert(out->data);
    memcpy(out->data, expected_emblems, out->size);
    return 1;
}
static void restoration(void)
{
    struct ReplayRecorder recorder = {0};
    cache_mode = 2;
    reset_effects();
    assert(game_emblems_restore(&mode, &recorder));
    assert(guild_calls == 1);
    reset_effects();
    io_fail = 1;
    assert(!game_emblems_restore(&mode, &recorder));
    untouched();
    io_fail = 0;
    short_input = 1;
    assert(!game_emblems_restore(&mode, &recorder));
    untouched();
    short_input = 0;
    empty_input = 1;
    assert(game_emblems_restore(&mode, &recorder));
    untouched();
    empty_input = 0;
    assert(!game_emblems_restore(NULL, &recorder) && !game_emblems_restore(&mode, NULL));
}
int main(void)
{
    setup();
    wire_and_bounds();
    record_cases();
    allocation_failures();
    restoration();
    name_card_holder_destroy(&mode.actor_names);
    assert(!allocations);
    puts("emblem snapshots: archived bytes, real name codec, deduplicated files, complete "
         "validation before side effects, safe cache paths, optional cache failures, output "
         "rollback and replay restoration passed");
    return 0;
}
