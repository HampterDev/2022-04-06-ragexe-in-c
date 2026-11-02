#include "game/actor_actions.h"
#include "game/drag.h"
#include "contents/player_names.h"
#include "session/map_names.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#ifndef UI_SOCIAL_PARTY_WINDOWS_H
#define UI_SOCIAL_PARTY_WINDOWS_H

#include "world/actor_lookup.h"
#include "game/context.h"
#include "actor/types.h"
#include "core/markup_tags.h"
#include "ui/social/social.h"
#include "ui/character/job_icon_button.h"
#include "ui/ui_std.h"
#include <string.h>
#include "session/session_items.h"
#include "session/party_types.h"
#include "ui/character/gauge.h"

static __inline void party_member_destroy(struct PartyMember *m)
{
    string_free(&m->map_name);
    string_free(&m->name);
}

struct PartyRowLayout {
    int x, y;
    int row_height;
};
SIZE_ASSERT(PartyRowLayout, 12);
#define PARTY_LAYOUT(v, i) (((struct PartyRowLayout *)(v)->first)[i])

static __inline void party_layouts_init(struct Vec *v)
{
    struct PartyRowLayout *l;

    vec_resize(v, 2, sizeof *l);
    l = (struct PartyRowLayout *)v->first;
    l[0].x = 5;
    l[0].y = 5;
    l[0].row_height = 0x22;
    l[1].x = 4;
    l[1].y = 1;
    l[1].row_height = 0x1b;
}

extern int g_miniPartyCellWidth;
extern int g_miniPartyCellHeight;

struct MiniPartyWndAreaMgr {
    RECT area;
    int cols, rows;
    struct Vec taken;
    struct Vec free;
};

struct MiniPartySortArea {
    RECT area;
    int cols, rows;
    struct List cells;
};
OFFSET_ASSERT(MiniPartySortArea, cells, 0x18);

struct MiniPartyWndAreaMgr *mini_party_area_mgr_instance(void);
int mini_party_area_mgr_find_free_cell(struct MiniPartyWndAreaMgr *self);
POINT *mini_party_area_mgr_cell_pos(struct MiniPartyWndAreaMgr *self, POINT *out, int cell);
POINT *mini_party_sort_area_cell_pos(struct MiniPartySortArea *self, POINT *out, int cell);
int mini_party_area_mgr_cell_at(struct MiniPartyWndAreaMgr *self, int x, int y);
void mini_party_area_mgr_rebuild(struct MiniPartyWndAreaMgr *self);

struct UIDragMiniPartyWnd {
    struct UIFrameWnd frame;
    struct PartyMember member;
    struct UIPcGage *gauge;
    int row;
    int grab_x, grab_y;
    int layout;
    struct Vec layouts;
};

struct UIDragMiniPartyWnd *ui_drag_mini_party_wnd_construct(struct UIDragMiniPartyWnd *self,
                                                                   struct PartyMember member);
void ui_drag_mini_party_wnd_move_gauge(struct UIDragMiniPartyWnd *self, int x, int y);
void ui_drag_mini_party_wnd_remove_gauge(struct UIDragMiniPartyWnd *self);
void ui_drag_mini_party_wnd_set_hp(struct UIDragMiniPartyWnd *self, int hp, int max_hp);
void ui_drag_mini_party_wnd_set_grab(struct UIDragMiniPartyWnd *self, int x, int y);

RECT *ui_window_mgr_mini_party_area(struct UIWindowMgr *self, RECT *out);
void mini_party_sort_area_get_cells(struct MiniPartySortArea *self, struct List *out);

struct UIMiniPartyInfoWnd;

struct UIMiniPartyWnd {
    struct UIFrameWnd frame;
    struct PartyMember member;
    int cell;
    struct UIMiniPartyInfoWnd *info;
    struct UIWindow *edge_button;
    int mode;
    struct Vec widths;
    struct String background;
};

struct UIMiniPartyInfoWnd {
    struct UIFrameWnd frame;
    struct Vec menu;
    struct UIPcGage *gauge;
    struct UIMiniPartyWnd *owner;
};

struct UIMiniPartyWnd *ui_mini_party_wnd_construct(struct UIMiniPartyWnd *self);
struct PartyMember *ui_mini_party_wnd_get_member(struct UIMiniPartyWnd *self, struct PartyMember *out);
void ui_mini_party_wnd_set_display_mode(struct UIMiniPartyWnd *self, int mode);
intptr_t ui_mini_party_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, intptr_t val1,
                                      intptr_t val2, intptr_t val3, intptr_t val4);
void ui_mini_party_wnd_on_l_btn_up(struct UIWindow *window, int x, int y);
void ui_mini_party_wnd_on_l_btn_down(struct UIWindow *window, int x, int y);
struct UIMiniPartyInfoWnd *ui_mini_party_info_wnd_construct(struct UIMiniPartyInfoWnd *self);
intptr_t ui_mini_party_info_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, intptr_t val1,
                                          intptr_t val2, intptr_t val3, intptr_t val4);

#define RENEWAL_PARTY UI_SKIN "renewalparty\\"
#define PLATE_BG RENEWAL_PARTY "img_partymini_bg.tga"
#define PLATE_BG_MINI RENEWAL_PARTY "img_partymini_bg_mini.tga"
#define PLATE_BG_MAXMINI RENEWAL_PARTY "img_partymini_bg_maxmini.tga"

int ui_window_mgr_find_free_plate_cell(struct UIWindowMgr *self);
void ui_window_mgr_add_plate(struct UIWindowMgr *self, struct UIMiniPartyWnd *plate);
struct UIMiniPartyWnd *ui_window_mgr_get_plate(struct UIWindowMgr *self, int id);
void ui_window_mgr_remove_plate(struct UIWindowMgr *self, int id);
void ui_window_mgr_plates_changed(struct UIWindowMgr *self);
static __inline struct PartyMember *party_member_at(struct PartyMember *out, int index)
{
    return session_party_member_at(&g_session1, out, index);
}

static __inline struct UIPcGage *party_actor_gauge(const void *actor)
{
    return (struct UIPcGage *)((const struct Pc *)actor)->hp_gage_wnd;
}

struct PartyWndInfo {
    int x, y;
    int width, height;
    int show;
    int tab;
    int mode;
};

#define PARTY_WND_ROWS 40
#define PARTY_WND_BUTTONS 11

struct UIMessengerGroupWnd {
    struct UIFrameWnd frame;
    struct PartyWndInfo *info;
    struct UIScrollBar *scrollbar;
    struct UIRadioBtn *tabs[2];
    struct UIWindow *close_button;
    struct UIBitmapButton *buttons[PARTY_WND_BUTTONS];
    struct UIResizer *resizer;
    struct UIWindow *title;
    struct List rows;
    struct Vec menu;
    int scrollable;
    int first;
    int cols;
    int visible;
    int hover;
    int row_height;
    struct UIPcGage *gauges[PARTY_WND_ROWS];
    struct UIJobIconButton *job_icons[PARTY_WND_ROWS];
    int mode;
    struct Vec layouts;
    struct UIWindow *mini_button;
    struct UIBitmapButton *max_button;
    BYTE party_hides[PARTY_WND_BUTTONS];
    int tab;
};

#define PARTY_MSG_SCROLL 7
#define PARTY_MSG_PAGE_UP 9
#define PARTY_MSG_PAGE_DOWN 0xa
#define PARTY_MSG_SET_STATE 0xd
#define PARTY_MSG_RESIZE 0xe
#define PARTY_MSG_REFRESH 0x17
#define PARTY_MSG_MENU_CHOICE 0x30
#define PARTY_MSG_OPEN_MENU 0x31
#define PARTY_RESIZE_SIZE 7

struct PartyMember *ui_messenger_group_wnd_empty_member(struct PartyMember *out);
struct PartyMember *ui_messenger_group_wnd_get_member(struct UIMessengerGroupWnd *self,
                                                                 struct PartyMember *out, int index);
struct PartyMember *ui_messenger_group_wnd_member_at(struct UIMessengerGroupWnd *self,
                                                                struct PartyMember *out, int x, int y);
void ui_messenger_group_wnd_create_plate(int cell,
                                                const struct PartyMember *member);
void ui_messenger_group_wnd_update_scroll(struct UIMessengerGroupWnd *self);
void ui_messenger_group_wnd_layout_buttons(struct UIMessengerGroupWnd *self);
void ui_messenger_group_wnd_update_gauges(struct UIMessengerGroupWnd *self);
void party_wnd_title(struct UIMessengerGroupWnd *self, char *buf);
struct UIWindow *party_wnd_docked(struct UIMessengerGroupWnd *self);
void party_wnd_dock(struct UIMessengerGroupWnd *self, struct UIWindow *wnd);

BYTE ui_window_mgr_has_plate(struct UIWindowMgr *self, int id);
void ui_window_mgr_set_drag_plate(struct UIWindowMgr *self, struct UIWindow *plate);
void ui_window_mgr_sort_plates(struct UIWindowMgr *self);

#include <stdlib.h>

void ui_messenger_group_wnd_on_draw(struct UIWindow *window);
intptr_t ui_messenger_group_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, intptr_t val1,
                                           intptr_t val2, intptr_t val3, intptr_t val4);

struct UIMessengerGroupWnd *ui_messenger_group_wnd_construct(struct UIMessengerGroupWnd *self);
struct UIWindow *ui_messenger_group_wnd_new(void);
void ui_messenger_group_wnd_drop_plate(struct UIMessengerGroupWnd *self, int x, int y, int row);

#endif
