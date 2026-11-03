#include "contents/guild_emblem_hooks.h"
#include "contents/guild_mgr.h"

char guild_emblem_is_bad(const char *name)
{
    return guild_mgr_is_bad_emblem(g_guildMgr, string_from_cstr(name));
}

void guild_emblem_mark_bad(const char *name)
{
    guild_mgr_mark_bad_emblem(g_guildMgr, string_from_cstr(name));
}

unsigned guild_emblem_max_size(void)
{
    return g_guildMgr->emblem_max_size;
}
