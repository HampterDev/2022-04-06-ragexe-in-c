#ifndef GUILD_EMBLEM_NAME_H
#define GUILD_EMBLEM_NAME_H

struct String;

void guild_emblem_name(struct String *out, const struct String *world,
                       unsigned guild, int version);
void guild_emblem_pattern(struct String *out, const struct String *world,
                          unsigned guild);

#endif
