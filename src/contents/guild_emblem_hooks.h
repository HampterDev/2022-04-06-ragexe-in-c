#ifndef CONTENTS_GUILD_EMBLEM_HOOKS_H
#define CONTENTS_GUILD_EMBLEM_HOOKS_H

/* The guild manager's side of g_emblemRules (resources/emblem.h): its bad-emblem list
   and emblem size limit, on g_guildMgr. */
char guild_emblem_is_bad(const char *name);
void guild_emblem_mark_bad(const char *name);
unsigned guild_emblem_max_size(void);

#endif
