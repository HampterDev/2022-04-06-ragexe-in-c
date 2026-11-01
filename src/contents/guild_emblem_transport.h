#ifndef CONTENTS_GUILD_EMBLEM_TRANSPORT_H
#define CONTENTS_GUILD_EMBLEM_TRANSPORT_H
#include <stddef.h>
int guild_emblem_request(int guild, int version);
int guild_emblem_upload(const void *data, size_t size);
#endif
