#include "contents/guild_emblem_name.h"
#include "std/std.h"
#include <stdio.h>
#include <string.h>

static void make_name(struct String *out, const struct String *world, const char *suffix)
{
    string_copy(out, world);
    string_append_cstr(out, suffix);
}

void guild_emblem_name(struct String *out, const struct String *world,
                       unsigned guild, int version)
{
    char suffix[32];

    snprintf(suffix, sizeof suffix, "_%u_%u.ebm", guild, (unsigned)version + 0x7fffffffu);
    make_name(out, world, suffix);
}

void guild_emblem_pattern(struct String *out, const struct String *world,
                          unsigned guild)
{
    char suffix[24];

    snprintf(suffix, sizeof suffix, "_%u_*.ebm", guild);
    make_name(out, world, suffix);
}
