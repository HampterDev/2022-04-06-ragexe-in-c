#include "contents/player_names.h"
#include "skills/skill_info.h"
#include "std/list.h"
#ifndef UI_SOCIAL_GUILD_MANAGE_H
#define UI_SOCIAL_GUILD_MANAGE_H
#include "ui/social/guild_member_sort.h"

#include "ui/social/guild_windows.h"
#include "session/session_party.h"
#include "contents/guild_mgr.h"
#include "resources/emblem_types.h"
#include "contents/guild_info.h"
#include "session/state.h"
#include "ui/window_manager_state.h"

#include "core/markup_tags.h"
#include "ui/social/social.h"
#include "ui/social/guild_wnd.h"
#include "ui/controls/controls.h"
#include "ui/window_mgr.h"
#include "ui/game.h"
#include "ui/ui_items.h"
#include "ui/runtime.h"
#include "ui/ui_std.h"

#define GUILD_TAX_MAX 0x32
#define GUILD_RELATION_ALLY 0
#define GUILD_RELATION_ENEMY 1

#define MODE_REQ_PARTY_INVITE 0x3b
#define MODE_REQ_GUILD_MEMBER_CHANGES 0x67
#define MODE_REQ_GUILD_POSITION_CHANGES 0x68
#define MODE_REQ_OPEN_GUILD_MEMBER_INFO 0x6d
#define MODE_REQ_GUILD_RELATION_END 0x7f
#define MODE_REQ_OPEN_WRITE_RODEX 0x10c
#define MODE_REQ_MOVE_GUILD_AGIT 0x13b
#define MODE_REQ_AGIT_INVESTMENT 0x13c

#define WND_ID_MENU 0x12
#define MENU_MSG_FIT 0x1e
#define MENU_MSG_ADD_ITEM 0x1f
#define MENU_MSG_SET_OWNER 0x53
#define GUILD_MSG_MENU_CHOICE 0x30
#define GUILD_MSG_OPEN_MENU 0x31
#define GUILD_MSG_REFRESH 0x3c
#define WND_ID_COMBO 0x1a

#define GUILD_ID_TEXT_BUTTON 0x109
#define GUILD_ID_POSITION_NAME 0x10b
#define GUILD_ID_POSITION_TAX 0x10c
#define GUILD_ID_EMBLEM_BROWSE 0x10e
#define GUILD_INFO_ID_DISBAND 0x1b9
#define GUILD_OK_REENABLE_MS 400
#define GUILD_OFF_SCREEN (-200)

#define GUILD_SORTED_COLOUR 0xebd8ce

#define GUILD_SORT_UP 1
#define GUILD_SORT_DOWN 2
#define GUILD_MEMBER_COLUMNS 6

#define GUILD_WND(self) (&(self)->base.frame.base)

static __inline void guild_place_ok(struct UIWindow *wnd, struct UIBitmapButton *ok, int have_changes)
{
    if (g_guildInfo.is_master && have_changes)
        ui_window_move_base(&ok->button.base, wnd->width - 0x3d, wnd->height - 0x17);
    else
        ui_window_move_base(&ok->button.base, GUILD_OFF_SCREEN, GUILD_OFF_SCREEN);
}

void guild_open_menu(struct Vec *entries, int owner_id);
struct UIBitmapButton *guild_add_ok_button(void);
void guild_update_scroll(struct UIScrollBar *bar, unsigned count, int per_row, int rows, int *top, int *scrollable);

BYTE ui_guild_member_manage_wnd_get_member(struct UIGuildMemberManageWnd *self, int index,
                                                 struct GuildMember *out);
BYTE ui_guild_member_manage_wnd_draw_row(struct UIGuildMemberManageWnd *self,
                                               struct GuildMember *member, int *row, int index);
struct GuildPosition *ui_guild_position_manage_wnd_get_position(struct UIGuildPositionManageWnd *self, int index, int *found);
int ui_guild_position_manage_wnd_hit_test(struct UIGuildPositionManageWnd *self, int x,
                                               int y);
void ui_guild_info_manage_wnd_fill_castles(struct UIGuildInfoManageWnd *self);

#endif
