#ifndef UI_SOCIAL_GUILD_MEMBER_SORT_H
#define UI_SOCIAL_GUILD_MEMBER_SORT_H

struct List;
typedef void (*GuildMemberSort)(struct List *members);

void guild_member_sort_exp_up(struct List *members);
void guild_member_sort_exp_down(struct List *members);
void guild_member_sort_job_up(struct List *members);
void guild_member_sort_job_down(struct List *members);
void guild_member_sort_level_up(struct List *members);
void guild_member_sort_level_down(struct List *members);
void guild_member_sort_name_up(struct List *members);
void guild_member_sort_name_down(struct List *members);
void guild_member_sort_online_first(struct List *members);
void guild_member_sort_position_up(struct List *members);
void guild_member_sort_position_down(struct List *members);
void guild_member_sort_share_up(struct List *members);
void guild_member_sort_share_down(struct List *members);

struct GuildMember *guild_member_construct(struct GuildMember *self);

#endif
