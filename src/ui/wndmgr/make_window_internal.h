#include "actor/map_type_table.h"
#include "contents/client_lua_calls.h"
#include "game/own_states.h"
#include "ui/misc/snap_mgr.h"
#include "session/state.h"
#ifndef UI_WNDMGR_MAKE_WINDOW_INTERNAL_H
#define UI_WNDMGR_MAKE_WINDOW_INTERNAL_H

#include "ui/wndmgr/wndmgr.h"
#include "ui/controls/controls.h"

extern int g_guildWndWidth, g_guildWndHeight;

int guild_wnd_info_is_valid_info(struct GuildWndInfo *self);
void guild_wnd_info_make_default(struct GuildWndInfo *self);
void wm_window_list_sort(struct List *windows);
struct UIAutoMessageWnd *ui_auto_message_wnd_construct(struct UIAutoMessageWnd *self);
struct UIWindow *ui_auto_message_wnd_new(void);
struct UICommandListWnd *ui_command_list_wnd_construct(struct UICommandListWnd *self);
struct UIWindow *ui_command_list_wnd_new(void);
struct UIMakeTargetListWnd *ui_make_target_list_wnd_construct(struct UIMakeTargetListWnd *self);
struct UIWindow *ui_make_target_list_wnd_new(void);
struct UIMakeTargetProcessWnd *ui_make_target_process_wnd_construct(struct UIMakeTargetProcessWnd *self);
struct UIWindow *ui_make_target_process_wnd_new(void);
struct UIPetTamingDeceiveWnd *ui_pet_taming_deceive_wnd_construct(struct UIPetTamingDeceiveWnd *self);
struct UIWindow *ui_pet_taming_deceive_wnd_new(void);
struct UISelectCartWnd *ui_select_cart_wnd_construct(struct UISelectCartWnd *self);
struct UIWindow *ui_select_cart_wnd_new(void);
struct UIStatusWnd *ui_status_wnd_construct(struct UIStatusWnd *self);
struct UIWindow *ui_status_wnd_new(void);
struct UIPreviewEquipWnd *ui_preview_equip_wnd_construct(struct UIPreviewEquipWnd *self);
struct UIWindow *ui_preview_equip_wnd_new(void);
struct SnapMgr;
#define WM_SKILL_LIST_HOMUN 0
#define WM_SKILL_LIST_MERCENARY 1

typedef struct UIWindow *(*WmNew)(void);
typedef struct UIWindow *(*WmNew1)(int arg);
void wm_move(struct UIWindow *wnd, int x, int y);
void wm_set_info(struct UIWindow *wnd, void *info);
void wm_show(struct UIWindow *wnd);
int wm_center_x(int width);
int wm_center_y(int height);
void wm_move_640(struct UIWindow *wnd, int x, int y480);
void wm_move_640_up(struct UIWindow *wnd, int x, int y480, int up);
void wm_move_center(struct UIWindow *wnd, int x, int y480);
struct UIWindow *wm_register(struct UIWindowMgr *self, int window_id, struct UIWindow *wnd);
enum {
    WM_AT_XY,
    WM_AT_640,
    WM_AT_CENTER,
    WM_AT_640_UP,
};
struct UIWindow *wm_open(struct UIWindowMgr *self, struct UIWindow **slot, WmNew new_window, WmNew1 new_window1,
                         int arg, int width, int height, int at, int x, int y, void *info);
#define WM_OPEN(slot, new_window, w, h, at, x, y) \
    wm_open(self, &self->slot, (new_window), NULL, 0, (w), (h), (at), (x), (y), NULL)
#define WM_OPEN_INFO(slot, new_window, w, h, at, x, y, info) \
    wm_open(self, &self->slot, (new_window), NULL, 0, (w), (h), (at), (x), (y), &self->info)
#define WM_OPEN_INFO1(slot, new_window, arg, w, h, at, x, y, info) \
    wm_open(self, &self->slot, NULL, (new_window), (arg), (w), (h), (at), (x), (y), &self->info)
int wm_times_half(int v);
void wm_move_on_centre(struct UIWindow *wnd);

#define WM_NOT_MINE ((struct UIWindow *)-1)
struct UIWindow *wm_make_character(struct UIWindowMgr *self, int window_id);
struct UIWindow *wm_make_hud(struct UIWindowMgr *self, int window_id);
struct UIWindow *wm_make_trade(struct UIWindowMgr *self, int window_id);
struct UIWindow *wm_make_social(struct UIWindowMgr *self, int window_id);

#endif
