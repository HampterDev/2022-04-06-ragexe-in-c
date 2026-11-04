#include "game/mode_util.h"
#ifndef CONTENTS_GUILD_MGR_H
#define CONTENTS_GUILD_MGR_H

#include "contents/guild_emblem_api.h"
#include "base.h"
#include "std/std.h"
#include "contents/guild_emblem_req.h"

struct GuildEmblemDownload {
    unsigned guild_id;
    int version;
    unsigned char *data;
    size_t size, capacity;
};

struct GuildMgr {
    struct Tree guilds;
    struct GuildEmblemReqBundle *requests;
    struct Tree bad_emblems;
    struct GuildEmblemDownload download;
    unsigned emblem_max_size;
};

#define GUILD_EMBLEM_MAX_SIZE 0xc800

void guild_emblem_download_destroy(struct GuildEmblemDownload *download);
void guild_mgr_update_requests(struct GuildMgr *self);
char guild_mgr_create(void);
void guild_mgr_destroy(void);

void guild_mgr_mark_bad_emblem(struct GuildMgr *self, struct String path);
char guild_mgr_is_bad_emblem(struct GuildMgr *self, struct String path);

struct String *guild_mgr_emblem_path(struct GuildMgr *self, struct String *out, unsigned guild,
                                                   int version);
void guild_emblem_save(const unsigned char *data, unsigned size, unsigned guild, int version);
void guild_emblem_changed(unsigned guild);

#endif
