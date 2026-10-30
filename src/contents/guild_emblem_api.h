#ifndef CONTENTS_GUILD_EMBLEM_API_H
#define CONTENTS_GUILD_EMBLEM_API_H
#include <stddef.h>
struct GuildMgr;
struct String;
extern struct GuildMgr *g_guildMgr;
void guild_mgr_clear_versions(struct GuildMgr *manager);
int guild_mgr_emblem_version(struct GuildMgr *manager, unsigned guild);
void guild_mgr_set_emblem_version(struct GuildMgr *manager, unsigned guild, int version);
struct String *guild_mgr_emblem_file_name(struct GuildMgr *manager, struct String *out,
                                             unsigned guild);
int guild_mgr_download_chunk(struct GuildMgr *manager, int part, unsigned guild, int version,
                             const void *data, size_t size);
#endif
