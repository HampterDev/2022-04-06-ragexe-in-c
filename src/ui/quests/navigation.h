#include "contents/private_airplane.h"
#include "ui/chat/markup_links.h"
#include "render/texture_types.h"
#include "render/texture_cache.h"
#include "resources/animation.h"
#include "ui/navigation_view.h"
#include "ui/worldmap/route_view.h"
#include "navi/navi_manager.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#ifndef UI_QUESTS_NAVIGATION_H
#define UI_QUESTS_NAVIGATION_H

#include "core/markup_tags.h"
#include "ui/window.h"
#include "ui/controls/controls.h"
#include "ui/window_mgr.h"
#include "ui/game.h"

#define NAVI_DIR UI_SKIN "navigation_interface3\\"

struct UIListBox_NaviSearch {
    struct UIListBox list;
    struct Vec kinds;
};
#define NAVI_LINE_MAP 1
#define NAVI_LINE_NPC 2
#define NAVI_LINE_MOB 3
#define NAVI_LINE_START 100
#define NAVI_LINE_STEP 200
#define NAVI_LINE_TARGET 300

void ui_list_box_navi_search_clear_items(struct UIListBox *base_list);

static __inline struct UIListBox_NaviSearch *navi_list_construct(struct UIListBox_NaviSearch *self)
{
    ui_list_box_construct(&self->list);
    self->list.base.ops = &ui_list_box_navi_search_ops.base;
    self->kinds.first = self->kinds.last = self->kinds.end = NULL;
    return self;
}
static __inline void navi_list_add(struct UIListBox_NaviSearch *list, int kind, const char *text)
{
    vec_push(&list->kinds, &kind, sizeof kind);
    ui_list_box_add_item(&list->list, text);
    ui_window_invalidate_base(&list->list.base);
}

struct UINavigationV4Wnd {
    struct UIFrameWnd frame;
    struct UIWindow *close_button;
    struct UIBitmapButton *mini_close;
    struct UIWindow *mini_button;
    struct UIBitmapButton *help;
    struct UIBitmapButton *maximize;
    struct UIBitmapButton *road_icon;
    struct UIBitmapButton *back;
    struct UIWindow *minimize_box;
    struct UIBitmapButton *route_list;
    struct UIComboBoxBmpWnd *kind_box;
    struct UIBitmapEditCtrl *search;
    struct UIBitmapButton *search_btn;
    struct UIWindow *count_text;
    struct UIListBox_NaviSearch *results;
    struct UIListBox_NaviSearch *entries;
    struct UIWindow *service;
    struct UIBitmapButton *share;
    struct UIBitmapButton *find;
    struct UIBitmapButton *airship;
    struct NavigationWndInfo *info;
    DWORD hint_until;
    int anim_period;
};

#define NAVI_MSG_ROAD_ICON 0x28

#define NAVI_ID_ROAD_ICON 0xb5
#define NAVI_ID_MAXIMIZE 0xc0
#define NAVI_ID_MINIMIZE 0xc2
#define NAVI_ID_SERVICE 0xcd
#define NAVI_ID_BACK 0x114
#define NAVI_ID_SEARCH 0x139
#define NAVI_ID_RESULTS 0x148
#define NAVI_ID_HELP 0x14f
#define NAVI_ID_SHARE 0x15a
#define NAVI_ID_FIND 0x1a6
#define NAVI_ID_ENTRIES 0x1b1
#define NAVI_ID_ROUTE_LIST 0x1b2
#define NAVI_ID_AIRSHIP 0x1eb
#define NAVI_ID_SEARCH_EDIT 0x1fc
#define NAVI_ROAD_ICON_WND_ID 0x132
#define NAVI_ROUTE_WND_ID 0x13a
#define NAVI_HELP_WND_ID 0xe5
#define NAVI_AIRSHIP_WND_ID 6

#define NAVI_VIEW_SEARCH 0
#define NAVI_VIEW_ROUTE 1
#define NAVI_VIEW_TARGET 2

struct UINavigationV4Wnd *ui_navigation_v4_wnd_construct(struct UINavigationV4Wnd *self);
struct UIWindow *ui_navigation_v4_wnd_new(void);
void ui_navigation_v4_wnd_relayout(struct UINavigationV4Wnd *self);
void ui_navigation_v4_wnd_layout_list_page(struct UINavigationV4Wnd *self);
void ui_navigation_v4_wnd_layout_route(struct UINavigationV4Wnd *self);
void ui_navigation_v4_wnd_fill_results(struct UINavigationV4Wnd *self);
void ui_navigation_v4_wnd_fill_entries(struct UINavigationV4Wnd *self, int group);
void ui_navigation_v4_wnd_draw_picture(struct UINavigationV4Wnd *self, int x, int y, int size);
struct BitmapRes;
void ui_navigation_v4_wnd_draw_guided_npc(struct UINavigationV4Wnd *self, int x, int y, int w, int h,
                                                struct BitmapRes *background);
void ui_navigation_v4_wnd_set_road_icon(struct UINavigationV4Wnd *self, int icon);
void ui_navigation_v4_wnd_put_link(struct UINavigationV4Wnd *self, struct String map, int x, int y);
void ui_navigation_v4_wnd_pick_entry(struct UINavigationV4Wnd *self, int entry);
struct String *ui_navigation_v4_wnd_result_name(struct UINavigationV4Wnd *self, struct String *out, int kind,
                                                         void *obj);
struct String *ui_navigation_v4_wnd_member_text(struct UINavigationV4Wnd *self, struct String *out, int kind,
                                                         void *obj);
struct String *ui_navigation_v4_wnd_mob_text(struct UINavigationV4Wnd *self, struct String *out, void *mob);

static __inline void ui_canvas_draw_image(struct DcBitmap *canvas, int x, int y, int w, int h, struct Texture *tex,
                                          unsigned tex_w, unsigned tex_h, float sx, float sy, DWORD argb)
{
    dc_bitmap_draw_texels_scaled(canvas, x, y, w, h, tex, tex_w, tex_h, sx, sy, argb, 0, 0, 0, 0, 0);
}
#define UI_TEXTURE_W(t) ((unsigned)((const struct Texture *)(t))->reqWidth)
#define UI_TEXTURE_H(t) ((unsigned)((const struct Texture *)(t))->reqHeight)

struct UIPrivateAirplaneWnd *ui_private_airplane_wnd_new(void);
int ui_private_airplane_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                            int val1, int val2, int val3, int val4);

#include "ui/quests/world_view.h"
struct Session;

int ui_navigation_v4_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1,
                                         int val2, int val3, int val4);

BYTE ui_navigation_v4_wnd_minimize_checked(struct UINavigationV4Wnd *self);

#endif
