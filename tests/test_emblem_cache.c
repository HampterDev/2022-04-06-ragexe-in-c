#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "session/state.h"
#include "session/session_state.h"
#include "contents/guild_info.h"
#include "contents/guild_emblem_name.h"
#include "contents/guild_emblem_api.h"
#include "core/app.h"
#include "core/file_util.h"
#include "std/std.h"

char g_clientDir[0x88] = "C:\\emblem-test";
struct GuildInfo g_guildInfo;
struct GuildMgr *g_guildMgr;
static char opened[512];
static unsigned writes, closes, updates;
static int current_version;
static const unsigned char payload[] = {0x78, 0x9c, 0, 0xff, 1};
static FILE *const stream = (FILE *)1;
struct Session g_session1;
struct Renderer *g_renderer;
int g_wobbleFramesLeft;
BYTE g_wobbleMode;
struct StoreSearchItem;
const char *session_character_name(const struct Session *s) { (void)s; abort(); }
int session_is_homunculus_skill(struct Session *s, int id) { (void)s; (void)id; abort(); }
void session_refresh_basic_info(struct Session *s) { (void)s; abort(); }
BYTE clientinfo_is_admin_aid(int aid) { (void)aid; abort(); }
void store_search_items_destroy(struct StoreSearchItem *a, struct StoreSearchItem *b)
{ (void)a; (void)b; abort(); }

void *mem_alloc(size_t n) { void *p = malloc(n); assert(p); return p; }
void mem_free(void *p) { free(p); }
void fatal_error(const char *message) { (void)message; abort(); }
int emblem_test_access(const char *path, int mode)
{ assert(!strcmp(path, "C:\\emblem-test\\_tmpEmblem\\") && mode == 0); return 0; }
FILE *emblem_test_open(const char *path, const char *mode)
{ assert(!strcmp(mode, "wb")); strcpy(opened, path); return stream; }
size_t emblem_test_write(const void *data, size_t size, size_t count, FILE *file)
{
    assert(file == stream && size == sizeof payload && count == 1);
    assert(!memcmp(data, payload, size)); ++writes; return count;
}
int emblem_test_close(FILE *file) { assert(file == stream); ++closes; return 0; }
void dir_list_names_list(struct List *files, const char *directory, const char *pattern)
{
    assert(files->size == 0 && !strcmp(directory, "C:\\emblem-test\\_tmpEmblem\\"));
    assert(!strcmp(pattern, "world_7_*.ebm"));
}
void guild_mgr_set_emblem_version(struct GuildMgr *manager, unsigned guild, int version)
{ assert(manager == g_guildMgr && guild == 7); ++updates; current_version = version; }

int main(void)
{
    static struct Session session;
    const int versions[] = {0, 1, INT_MAX, INT_MIN, -1};
    unsigned i;
    string_init(&session.worldName);
    string_assign_cstr(&session.worldName, "world");
    g_guildInfo.guild_id = 7;
    for (i = 0; i < sizeof versions / sizeof versions[0]; ++i) {
        struct String name;
        char expected[512];
        guild_emblem_name(&name, &session.worldName, 7, versions[i]);
        snprintf(expected, sizeof expected, "%s\\_tmpEmblem\\%s", g_clientDir, string_cstr(&name));
        session_save_emblem(&session, payload, sizeof payload, 7, versions[i]);
        if (strcmp(opened, expected)) {
            fprintf(stderr, "Cache name mismatch: wrote %s, lookup expects %s\n", opened, expected);
            return 1;
        }
        assert(current_version == versions[i] && updates == i + 1);
        assert(writes == i + 1 && closes == i + 1);
        string_free(&name);
    }
    g_guildInfo.guild_id = 8;
    session_save_emblem(&session, payload, sizeof payload, 7, 1);
    assert(updates == 5 && writes == 6 && closes == 6);
    string_free(&session.worldName);
    puts("PASS: actual legacy emblem writer uses the lookup cache name across version boundaries, preserves payload bytes, cleanup and own-guild version updates.");
    return 0;
}
