#ifndef UI_SOCIAL_GUILD_REQUESTS_H
#define UI_SOCIAL_GUILD_REQUESTS_H
#include "contents/guild_info.h"
void guild_request_page(enum GuildPage page);

struct UIGuildBanishedMemberWnd *ui_guild_banished_member_wnd_construct(struct UIGuildBanishedMemberWnd *self);
struct UIWindow *ui_guild_banished_member_wnd_new(void);
struct UIGuildTotalInfoWnd *ui_guild_total_info_wnd_construct(struct UIGuildTotalInfoWnd *self);
struct UIWindow *ui_guild_total_info_wnd_new(void);

#endif
