#ifndef RESOURCES_EMBLEM_H
#define RESOURCES_EMBLEM_H

#include "resources/bitmap.h"

#include "resources/emblem_types.h"

struct EmblemRes *emblem_resource_init(struct EmblemRes *self);
void emblem_resource_finish(struct EmblemRes *self);
BYTE emblem_res_load(struct EmblemRes *self, const char *name);
BYTE emblem_res_animate(struct EmblemRes *self);

#include "contents/guild_mgr.h"

#endif
