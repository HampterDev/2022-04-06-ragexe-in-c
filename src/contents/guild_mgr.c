#include "fileio/resource_api.h"
#include "core/memory.h"
#include "contents/guild_mgr.h"
#include "contents/client_res.h"
#include "fileio/res_mgr.h"
#include "replay/replay.h"

struct GuildMgr *g_guildMgr;

struct GuildEmblem {
    unsigned id;
    int emblem_version;
};

static int name_less(const void *a, const void *b)
{
    return string_compare(a, b) < 0;
}

static void destroy_name(void *value)
{
    string_free(value);
}

void guild_mgr_clear_versions(struct GuildMgr *self)
{
    tree_clear(&self->guilds, NULL);
}

static struct GuildMgr *guild_mgr_construct(struct GuildMgr *self)
{
    tree_init(&self->guilds);
    self->requests = 0;
    tree_init(&self->bad_emblems);
    self->download.guild_id = 0;
    self->download.version = 0;
    self->download.data = NULL;
    self->download.size = self->download.capacity = 0;
    self->emblem_max_size = GUILD_EMBLEM_MAX_SIZE;
    self->requests = emblem_bundle_create();
    return self;
}

static void guild_mgr_destruct(struct GuildMgr *self)
{
    if (self->requests) {
        emblem_bundle_destroy(self->requests);
        self->requests = 0;
    }
    guild_emblem_download_destroy(&self->download);
    tree_free(&self->bad_emblems, destroy_name);
    tree_free(&self->guilds, NULL);
}

void guild_mgr_update_requests(struct GuildMgr *self)
{
    emblem_bundle_process(self->requests);
}

char guild_mgr_create(void)
{
    struct GuildMgr *mgr;

    if (g_guildMgr)
        return 0;
    mgr = (struct GuildMgr *)mem_try_alloc(sizeof *mgr);
    if (mgr)
        guild_mgr_construct(mgr);
    g_guildMgr = mgr;
    if (!mgr)
        return 0;
    if (!mgr->requests) {
        guild_mgr_destruct(mgr);
        mem_free(mgr);
        g_guildMgr = 0;
        return 0;
    }
    return 1;
}

void guild_mgr_destroy(void)
{
    if (!g_guildMgr)
        return;
    guild_mgr_destruct(g_guildMgr);
    mem_free(g_guildMgr);
    g_guildMgr = 0;
}

static struct GuildEmblem *find_guild(const struct GuildMgr *self, unsigned guild)
{
    return tree_find(&self->guilds, &guild, tree_less_uint);
}

int guild_mgr_emblem_version(struct GuildMgr *self, unsigned guild)
{
    struct GuildEmblem *node = find_guild(self, guild);

    return node ? node->emblem_version : 0;
}

void guild_mgr_set_emblem_version(struct GuildMgr *self, unsigned guild, int version)
{
    struct GuildEmblem *node = tree_get(&self->guilds, &guild, sizeof guild, sizeof *node, tree_less_uint);

    node->emblem_version = version;
}

struct String *guild_mgr_emblem_file_name(struct GuildMgr *self, struct String *out,
                                                        unsigned guild)
{
    struct GuildEmblem *node = find_guild(self, guild);

    if (node && node->emblem_version > 0) {
        struct String path, name;

        guild_mgr_emblem_path(self, &path, guild, node->emblem_version);
        string_copy(&name, &path);
        if (!res_mgr_get_string(g_resMgr(), name) && !g_replayPlaying)
            emblem_bundle_queue(self->requests, guild, guild_mgr_emblem_version(self, guild));
        *out = path;
        return out;
    }
    string_init(out);
    return out;
}

static void file_part(struct String *name, const struct String *path)
{
    const char *text = string_cstr(path);
    size_t at = path->size;

    string_copy(name, path);
    while (at) {
        if (text[--at] == '\\') {
            string_substring(name, path, (unsigned)at + 1, path->size);
            return;
        }
    }
}

void guild_mgr_mark_bad_emblem(struct GuildMgr *self, struct String path)
{
    struct String name;

    file_part(&name, &path);
    if (!tree_find(&self->bad_emblems, &name, name_less)) {
        struct String copy;

        string_copy(&copy, &name);
        tree_add(&self->bad_emblems, &copy, sizeof copy, name_less, NULL);
    }
    string_free(&name);
    string_free(&path);
}

char guild_mgr_is_bad_emblem(struct GuildMgr *self, struct String path)
{
    struct String name;
    char bad;

    file_part(&name, &path);
    bad = tree_find(&self->bad_emblems, &name, name_less) != NULL;
    string_free(&name);
    string_free(&path);
    return bad;
}
