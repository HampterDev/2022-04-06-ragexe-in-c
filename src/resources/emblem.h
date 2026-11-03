#ifndef RESOURCES_EMBLEM_H
#define RESOURCES_EMBLEM_H

#include "resources/bitmap.h"

#include "resources/emblem_types.h"

struct EmblemRes *emblem_resource_init(struct EmblemRes *self);
void emblem_resource_finish(struct EmblemRes *self);
BYTE emblem_res_load(struct EmblemRes *self, const char *name);
BYTE emblem_res_animate(struct EmblemRes *self);

/* The guild emblem rules emblem_res_load follows. Defined in resources/emblem_res.c;
   client_install_hooks() (core/client_init.c) installs the guild manager's
   guild_emblem_is_bad, guild_emblem_mark_bad and guild_emblem_max_size
   (contents/guild_emblem_hooks.c). */
struct EmblemRules {
    /* Nonzero when emblem `name` failed to load before; it then loads as a blank
       24x24 frame. NULL: no emblem is known to be bad. */
    char (*is_bad)(const char *name);
    /* Records that emblem `name` holds no valid image. NULL: nothing is recorded. */
    void (*mark_bad)(const char *name);
    /* The largest emblem the server sends, in bytes; emblem_res_load unpacks at most
       6/5 of it. NULL: 0, so no emblem loads. */
    unsigned (*max_size)(void);
};

extern struct EmblemRules g_emblemRules;

#endif
