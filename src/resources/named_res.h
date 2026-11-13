#ifndef RESOURCES_NAMED_RES_H
#define RESOURCES_NAMED_RES_H

#include <stddef.h>
#include "std/string.h"

struct Res;

/* A resource an object names (its palette, its imf): the name, and the resource looked up by it the
   first time it is asked for after the name was set (assign or take, even to the same name), held
   locked until the name is set again or the holder frees it. A name the resource manager has
   nothing for answers NULL until it is set again, and an empty name answers NULL without a lookup.

   Each holds at most one resource. Locked, the resource stays through the resource manager's
   purges, which skip locked resources, for as long as its holder lives; the holders are actors,
   freed when they leave and all of them when the world's objects are cleared on a map change, so
   what these hold is bounded by the actors alive. Uses the resource manager's API only
   (fileio/resource_api.h). */
struct NamedRes {
    struct String name;
    struct Res *res;
    unsigned char looked_up;
};

void named_res_init(struct NamedRes *self);
void named_res_free(struct NamedRes *self);
void named_res_assign(struct NamedRes *self, const char *text, size_t size);
/* Takes the text of name, which is left empty. */
void named_res_take(struct NamedRes *self, struct String *name);
const char *named_res_text(const struct NamedRes *self);
size_t named_res_size(const struct NamedRes *self);
struct Res *named_res_get(struct NamedRes *self);

#endif
