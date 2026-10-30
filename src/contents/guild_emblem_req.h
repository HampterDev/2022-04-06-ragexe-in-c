#ifndef CONTENTS_GUILD_EMBLEM_REQ_H
#define CONTENTS_GUILD_EMBLEM_REQ_H

struct GuildEmblemReqBundle;

struct GuildEmblemReqBundle *emblem_bundle_create(void);
void emblem_bundle_destroy(struct GuildEmblemReqBundle *bundle);
void emblem_bundle_queue(struct GuildEmblemReqBundle *bundle, int guild, int version);
void emblem_bundle_process(struct GuildEmblemReqBundle *bundle);

#endif
