#include "contents/guild_info.h"
#ifndef UI_SOCIAL_GUILD_WND_H
#define UI_SOCIAL_GUILD_WND_H

#include "ui/window.h"

struct GuildWndInfo;

struct UIGuildWnd {
    struct UIFrameWnd frame;
    struct GuildWndInfo *info;
    int tab_labels[6];
};
struct UIGuildWndOps {
    struct UIWindowOps base;
    int tab;
};

struct UIGuildWnd *ui_guild_wnd_construct(struct UIGuildWnd *self);
void ui_guild_wnd_init_tab_text(struct UIGuildWnd *self);
void ui_guild_wnd_on_draw(struct UIWindow *self);
void ui_guild_wnd_on_l_btn_down(struct UIWindow *self, int x, int y);
int ui_guild_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2,
                                  int val3, int val4);

enum GuildTab {
    GUILD_TAB_INFO = 0,
    GUILD_TAB_MEMBERS = 1,
    GUILD_TAB_POSITIONS = 2,
    GUILD_TAB_SKILLS = 3,
    GUILD_TAB_EXPELLED = 4,
    GUILD_TAB_NOTICE = 5,
    GUILD_TAB_TOTAL_INFO = 6,
};
#define GUILD_WND_ID 0x3b
#define GUILD_TAB_WND_ID_FIRST 0x3c

#define MODE_REQ_GUILD_INFO 0x6b

struct String *guild_name_cut_nameless(struct String *out, const struct String *name);

struct GuildLayout {
    int expelled_cols[2];
    int expelled_row_height;
    int total_cols[4];
    int total_row_height;
    int member_position_cols[12];
    int position_row_height;
};
SIZE_ASSERT(GuildLayout, 0x54);
extern const struct GuildLayout g_guildLayout;

struct GuildListView {
    struct UIScrollBar *bar;
    int top;
    int selected;
    int per_row;
    int rows;
    int scrollable;
};
int guild_list_view_hit_test(const struct GuildListView *list, const struct UIWindow *wnd,
                             const struct ListNode *head, int x, int y, int row_height);
void ui_guild_wnd_draw_tabs(struct UIGuildWnd *self, int tab);

void ui_guild_wnd_destroy(struct UIWindow *window);

void ui_guild_wnd_store_info(struct UIWindow *window);

void ui_guild_wnd_destruct(struct UIGuildWnd *self);

#endif
