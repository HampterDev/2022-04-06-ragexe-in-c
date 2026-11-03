#include "skills/skill_info.h"
#include "std/list.h"
#include "session/state.h"
#include <windows.h>
#include "core/fatal.h"
#include "contents/guild_info.h"
#include "ui/social/guild_member_sort.h"
#include "std/std.h"
#include "session/session_names.h"
#include "ui/ui_std.h"

struct GuildMember *guild_member_copy(struct GuildMember *dst,
                                                 const struct GuildMember *src)
{
    dst->aid = src->aid;
    dst->gid = src->gid;
    string_copy(&dst->name, &src->name);
    dst->hair_style = src->hair_style;
    dst->hair_color = src->hair_color;
    dst->job = src->job;
    dst->sex = src->sex;
    dst->position = src->position;
    string_copy(&dst->position_name, &src->position_name);
    dst->level = src->level;
    string_copy(&dst->memo, &src->memo);
    dst->exp_share = src->exp_share;
    dst->exp = src->exp;
    dst->online = src->online;
    dst->last_login = src->last_login;
    return dst;
}

struct GuildMember *guild_member_construct(struct GuildMember *self)
{
    string_init(&self->name);
    string_init(&self->position_name);
    string_init(&self->memo);
    return self;
}

void guild_member_destruct(struct GuildMember *self)
{
    string_free(&self->memo);
    string_free(&self->position_name);
    string_free(&self->name);
}

void guild_position_copy(struct GuildPosition *dst, const struct GuildPosition *src)
{
    dst->ranking = src->ranking;
    dst->position_id = src->position_id;
    string_copy(&dst->name, &src->name);
    dst->can_invite = src->can_invite;
    dst->can_expel = src->can_expel;
    dst->exp_tax = src->exp_tax;
    dst->can_use_storage = src->can_use_storage;
}

struct GuildPosition *guild_position_assign(struct GuildPosition *self,
                                                       const struct GuildPosition *src)
{
    self->ranking = src->ranking;
    self->position_id = src->position_id;
    string_assign(&self->name, string_cstr(&src->name), src->name.size);
    self->can_invite = src->can_invite;
    self->can_expel = src->can_expel;
    self->exp_tax = src->exp_tax;
    self->can_use_storage = src->can_use_storage;
    return self;
}

void guild_position_destruct(struct GuildPosition *self)
{
    string_free(&self->name);
}

void guild_position_edit_copy(void *dst, const void *src)
{
    struct GuildPositionEdit *to = dst;
    const struct GuildPositionEdit *from = src;

    to->position_id = from->position_id;
    guild_position_copy(&to->position, &from->position);
}

void guild_position_edit_destroy(void *value)
{
    struct GuildPositionEdit *edit = value;

    string_free(&edit->position.name);
}

static int wide_text_less(const char *a, const char *b)
{
    WCHAR wa[0x104], wb[0x104];

    memset(wa, 0, sizeof wa);
    memset(wb, 0, sizeof wb);
    MultiByteToWideChar(CP_UTF8, 0, a, -1, wa, 0x104);
    MultiByteToWideChar(CP_UTF8, 0, b, -1, wb, 0x104);
    return wcscmp(wa, wb) < 0;
}

static int job_name_less(const struct GuildMember *later, const struct GuildMember *earlier)
{
    const char *earlier_name = session_get_job_name(&g_session1, earlier->job, -1);
    const char *later_name = session_get_job_name(&g_session1, later->job, -1);

    return wide_text_less(later_name, earlier_name);
}

static int exp_less(const struct GuildMember *a, const struct GuildMember *b) { return a->exp < b->exp; }
static int exp_greater(const struct GuildMember *a, const struct GuildMember *b) { return a->exp > b->exp; }
static int job_less(const struct GuildMember *a, const struct GuildMember *b) { return job_name_less(a, b); }
static int job_not_less(const struct GuildMember *a, const struct GuildMember *b) { return !job_name_less(a, b); }
static int level_less(const struct GuildMember *a, const struct GuildMember *b) { return a->level < b->level; }
static int level_greater(const struct GuildMember *a, const struct GuildMember *b) { return a->level > b->level; }
static int name_less(const struct GuildMember *a, const struct GuildMember *b)
{
    return wide_text_less(string_cstr(&a->name), string_cstr(&b->name));
}
static int name_not_less(const struct GuildMember *a, const struct GuildMember *b)
{
    return !wide_text_less(string_cstr(&a->name), string_cstr(&b->name));
}
static int online_greater(const struct GuildMember *a, const struct GuildMember *b) { return a->online > b->online; }
static int position_less(const struct GuildMember *a, const struct GuildMember *b) { return a->position < b->position; }
static int position_greater(const struct GuildMember *a, const struct GuildMember *b)
{
    return a->position > b->position;
}
static int share_less(const struct GuildMember *a, const struct GuildMember *b) { return a->exp_share < b->exp_share; }
static int share_greater(const struct GuildMember *a, const struct GuildMember *b)
{
    return a->exp_share > b->exp_share;
}

#define MEMBER_SORT(name, before) \
    static int name##_less(const void *a, const void *b) { return before(a, b); } \
    void name(struct List *members) { list_sort(members, name##_less); }
MEMBER_SORT(guild_member_sort_exp_up, exp_less)
MEMBER_SORT(guild_member_sort_exp_down, exp_greater)
MEMBER_SORT(guild_member_sort_job_up, job_less)
MEMBER_SORT(guild_member_sort_job_down, job_not_less)
MEMBER_SORT(guild_member_sort_level_up, level_less)
MEMBER_SORT(guild_member_sort_level_down, level_greater)
MEMBER_SORT(guild_member_sort_name_up, name_less)
MEMBER_SORT(guild_member_sort_name_down, name_not_less)
MEMBER_SORT(guild_member_sort_online_first, online_greater)
MEMBER_SORT(guild_member_sort_position_up, position_less)
MEMBER_SORT(guild_member_sort_position_down, position_greater)
MEMBER_SORT(guild_member_sort_share_up, share_less)
MEMBER_SORT(guild_member_sort_share_down, share_greater)
