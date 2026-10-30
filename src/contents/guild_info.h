#ifndef CONTENTS_GUILD_INFO_H
#define CONTENTS_GUILD_INFO_H

#include "base.h"
#include "std/container_types.h"
#include "std/list.h"

struct GuildMember {
    int aid;
    int gid;
    struct String name;
    int hair_style;
    int hair_color;
    int job;
    int sex;
    int position;
    struct String position_name;
    int level;
    struct String memo;
    float exp_share;
    int exp;
    int online;
    int last_login;
};

struct GuildPosition {
    int ranking;
    int position_id;
    struct String name;
    int can_invite;
    int can_expel;
    int exp_tax;
    int can_use_storage;
};

struct GuildRelation {
    int id;
    int kind;
    struct String name;
};

struct GuildExpelled {
    int gid;
    struct String name;
    struct String account_name;
    struct String reason;
};

struct GuildTotalInfoRow {
    int id;
    struct String name;
    int values[4];
};

struct GuildMemberChange {
    int aid;
    int gid;
    int position;
};

struct GuildPositionName {
    int position;
    struct String name;
};

struct GuildMemberEdit {
    int gid;
    struct GuildMemberChange change;
};

struct GuildPositionEdit {
    int position_id;
    struct GuildPosition position;
};

enum GuildPage {
    GUILD_PAGE_INFO = 0,
    GUILD_PAGE_MEMBERS = 1,
    GUILD_PAGE_POSITIONS = 2,
    GUILD_PAGE_SKILLS = 3,
    GUILD_PAGE_EXPELLED = 4,
    GUILD_PAGE_TOTAL_INFO = 5,
    GUILD_PAGE_COUNT
};

struct GuildInfo {
    struct String name;
    struct String master_name;
    struct String notice_subject;
    struct String notice_body;
    int level;
    int member_count;
    int online_count;
    int max_members;
    int average_level;
    struct String castle_name;
    int exp;
    int exp_next;
    int tax_points;
    int honor;
    int virtue;
    int menu_flags;
    int guild_id;
    int can_invite;
    int can_expel;
    int is_master;
    int page_asked[GUILD_PAGE_COUNT];
    int master_gid;
    struct List positions;
    struct List members;
    struct List relations;
    struct List expelled;
    struct List total_info;
    struct Tree position_names;
    struct Tree member_position_edits;
    struct Tree position_edits;
    struct Vec castles;
};

extern struct GuildInfo g_guildInfo;

struct GuildInfo *guild_info_construct(struct GuildInfo *self);
void guild_info_destruct(struct GuildInfo *self);
void guild_info_reset(struct GuildInfo *self);
void guild_info_set_master(struct GuildInfo *self, int gid);
void guild_info_resolve_name(struct GuildInfo *self, int gid, const char *name);

void guild_info_add_member(struct GuildInfo *self, const struct GuildMember *member);
int guild_info_remove_member(struct GuildInfo *self, int gid);
int guild_info_remove_member_by_name(struct GuildInfo *self, const char *name);
void guild_info_clear_members(struct GuildInfo *self);
int guild_info_member_aid_by_name(struct GuildInfo *self, const char *name);
void guild_info_set_member_position(struct GuildInfo *self, int aid, int gid, int position);
void guild_info_set_member_status(struct GuildInfo *self, int online, int gid,
                                             short sex, short hair_style, short hair_color);
void guild_info_set_member_online(struct GuildInfo *self, int aid, int online);
void guild_info_count_online(struct GuildInfo *self);
void guild_info_update_member_shares(struct GuildInfo *self);

void guild_info_update_position(struct GuildInfo *self, struct GuildPosition position);
void guild_info_remove_relation(struct GuildInfo *self, int id, int kind);
int guild_info_count_relations(struct GuildInfo *self, int kind);
char guild_info_has_kind0_relation(struct GuildInfo *self, int guild_id);
void guild_info_add_expelled(struct GuildInfo *self, const struct GuildExpelled *entry);

void guild_info_add_castle(struct GuildInfo *self, int id);
void guild_info_remove_castle(struct GuildInfo *self, int id);
void guild_info_clear_castles(struct GuildInfo *self);
int guild_info_castle_count(struct GuildInfo *self);
int guild_info_castle_at(struct GuildInfo *self, int index);

void guild_relation_copy(struct GuildRelation *dst, const struct GuildRelation *src);
void guild_relation_destruct(struct GuildRelation *self);
void guild_expelled_copy(struct GuildExpelled *dst, const struct GuildExpelled *src);
void guild_expelled_destruct(struct GuildExpelled *self);
void guild_total_info_row_destruct(struct GuildTotalInfoRow *self);
void guild_position_name_destroy(void *value);

struct GuildMember *guild_member_copy(struct GuildMember *dst,
                                                 const struct GuildMember *src);

void guild_member_destruct(struct GuildMember *self);
struct GuildPosition *guild_position_assign(struct GuildPosition *self,
                                                      const struct GuildPosition *from);
void guild_position_copy(struct GuildPosition *dst, const struct GuildPosition *src);
void guild_position_destruct(struct GuildPosition *self);
void guild_position_edit_copy(void *dst, const void *src);
void guild_position_edit_destroy(void *value);

#endif
