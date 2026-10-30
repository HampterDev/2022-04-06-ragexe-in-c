#include "core/memory.h"
#include "contents/clan_info.h"

static void destroy_relation(void *value)
{
    struct ClanRelation *relation = value;

    string_free(&relation->name);
}

void clan_info_clear(struct ClanInfo *self)
{
    string_clear(&self->name);
    string_clear(&self->master);
    self->level = 0;
    self->online_count = 0;
    self->member_count = 0;
    string_clear(&self->map_name);
    self->emblem = 0;
    self->in_clan = 0;
    list_clear(&self->relations, destroy_relation);
}

struct ClanInfo *clan_info_construct(struct ClanInfo *self)
{
    string_init(&self->name);
    string_init(&self->master);
    string_init(&self->map_name);
    list_init(&self->relations);
    self->constructed = 0;
    clan_info_clear(self);
    self->constructed = 1;
    return self;
}

void clan_info_destruct(struct ClanInfo *self)
{
    list_free(&self->relations, destroy_relation);
    string_free(&self->map_name);
    string_free(&self->master);
    string_free(&self->name);
}

void clan_info_delete(struct ClanInfo *self)
{
    clan_info_destruct(self);
    mem_free(self);
}

struct ClanInfo *g_clanInfo;

char clan_info_create(void)
{
    struct ClanInfo *clan;

    if (g_clanInfo)
        return 0;
    clan = (struct ClanInfo *)mem_try_alloc(sizeof *clan);
    if (clan)
        clan_info_construct(clan);
    g_clanInfo = clan;
    if (!clan)
        return 0;
    if (!clan->constructed) {
        clan_info_delete(clan);
        g_clanInfo = 0;
        return 0;
    }
    return 1;
}

void clan_info_destroy(void)
{
    if (!g_clanInfo)
        return;
    clan_info_delete(g_clanInfo);
    g_clanInfo = 0;
}
