#include <string.h>
#include "contents/guild_info.h"
#include "std/std.h"

void guild_relation_copy(struct GuildRelation *dst, const struct GuildRelation *src)
{
    dst->id = src->id;
    dst->kind = src->kind;
    string_copy(&dst->name, &src->name);
}

void guild_relation_destruct(struct GuildRelation *self)
{
    string_free(&self->name);
}

void guild_expelled_copy(struct GuildExpelled *dst, const struct GuildExpelled *src)
{
    dst->gid = src->gid;
    string_copy(&dst->name, &src->name);
    string_copy(&dst->account_name, &src->account_name);
    string_copy(&dst->reason, &src->reason);
}

void guild_expelled_destruct(struct GuildExpelled *self)
{
    string_free(&self->reason);
    string_free(&self->account_name);
    string_free(&self->name);
}

void guild_total_info_row_destruct(struct GuildTotalInfoRow *self)
{
    string_free(&self->name);
}

void guild_position_name_destroy(void *value)
{
    struct GuildPositionName *entry = value;

    string_free(&entry->name);
}
