#include "session/hotkey_bindings.h"
#include "render/surface.h"
#include "session/option_pairs.h"
#include "session/hotkey_bindings.h"
#include "skills/skill_bundle.h"
#include "skills/skill_info.h"
#include "ui/browser_windows.h"
#include "replay/replay.h"
#include "navi/navi_manager.h"
#include "ui/misc/snap_mgr.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "ui/window_manager_state.h"
#ifndef UI_WNDMGR_WNDMGR_H
#define UI_WNDMGR_WNDMGR_H

#include "platform/client_config.h"

#include "ui/window_mgr.h"
#include "core/fatal.h"
#include "std/tree.h"
#include "ui/game.h"
#include "ui/skin.h"

#ifndef STD_H

#endif

struct WmChatLine {
    struct String text;
    COLORREF colour;
};

struct WmWindowEntry {
    int id;
    struct UIWindow *wnd;
};

struct WmClosedRect {
    int id;
    struct WindowInfo *rect;
};

struct WmBrowser {
    unsigned hwnd;
    struct WebBrowser *browser;
};

struct WmWhisperWnd {
    struct String name;
    struct UIWindow *wnd;
};
int wm_whisper_name_less(const void *a, const void *b);
void wm_whisper_wnd_destroy(void *value);

SIZE_ASSERT(GuildWndInfo, 0x14);

SIZE_ASSERT(ItemWndInfo, 0x28);
SIZE_ASSERT(ItemStoreWndInfo, 0x98);
SIZE_ASSERT(MiniMapWndInfo, 0x24);

extern const char *g_regUIRectInfoKey;
extern const char *g_regShortcutItemKey;
extern const char *g_regSkillUseLevelKey;
#define SAKRAY "사크라이"
#define SHORTCUT_SLOTS 36
#define SHORTCUT_PAGE_SLOTS 9

void ui_window_mgr_set_default_rect_info(struct UIWindowMgr *self);
void item_store_wnd_info_set_default(struct ItemStoreWndInfo *self);
void item_store_sub_info_set_default(struct WindowInfo *self, int index);
void mini_map_info_set_default(struct MiniMapWndInfo *self);

struct ReplayRecorder;
#define WM_REPLAY_RECORDING 2
void memorial_dun_info_reset(void);
struct UINavigationV4Wnd;
struct Roulette;
void roulette_reset(struct Roulette *self);

struct HotKey;
enum { WM_WEB_BROWSER_MAX = 5 };
void wm_destroy_browser(struct UIWindowMgr *self, struct WebBrowser *browser);
struct WmSnapRect {
    struct UIWindow *wnd;
    struct SnapInfo info;
};
struct MiniPartyWndAreaMgr;
extern struct MiniPartyWndAreaMgr *g_miniPartyAreaMgr;
struct MiniPartyWndAreaMgr *mini_party_area_mgr_construct(struct MiniPartyWndAreaMgr *self);
void mini_party_area_mgr_delete(struct MiniPartyWndAreaMgr *self);
#define WM_MINI_PARTY_AREA_MGR_SIZE sizeof(struct MiniPartyWndAreaMgr)

BYTE ui_window_mgr_is_untracked_id(struct UIWindowMgr *self, int window_id);
void ui_window_mgr_forget_window_id(struct UIWindowMgr *self, int window_id);
void ui_window_mgr_destroy_window(struct UIWindowMgr *self, struct UIWindow *wnd);
void ui_window_mgr_focus_first_edit(struct UIWindowMgr *self);
struct UIWindow *ui_window_mgr_find_by_serial(struct UIWindowMgr *self, int serial);
struct UIWindow *ui_window_mgr_window_at(struct UIWindowMgr *self, int x, int y);
void ui_window_mgr_show_shop_titles(struct UIWindowMgr *self, BYTE show);
void ui_window_mgr_send_key_down(struct UIWindowMgr *self);
int ui_window_mgr_classify_key(struct UIWindowMgr *self, int vk, int lparam);
BYTE ui_window_mgr_on_enter(struct UIWindowMgr *self);
int ui_window_mgr_get_hot_key_behavior(struct UIWindowMgr *self);
int ui_window_mgr_modal_loop(struct UIWindowMgr *self, struct UIWindow *wnd);
int ui_window_mgr_do_modal(struct UIWindowMgr *self, struct UIWindow *wnd);
void ui_window_mgr_invalidate_all(struct UIWindowMgr *self);
void ui_window_mgr_invalidate_once_per_frame(struct UIWindowMgr *self);
void ui_window_mgr_send_key_left(struct UIWindowMgr *self);
BYTE ui_window_mgr_is_barter_open(struct UIWindowMgr *self);
struct UIWindow *ui_window_mgr_open_sub_chat(struct UIWindowMgr *self, int key, int x, int y,
                                                    int width, int height);
BYTE ui_window_mgr_is_first_edit(struct UIWindowMgr *self, struct UIWindow *edit);
BYTE ui_window_mgr_is_last_edit(struct UIWindowMgr *self, struct UIWindow *edit);
BYTE ui_window_mgr_is_edit_in_open_window(struct UIWindowMgr *self);
void ui_window_mgr_close_all_but_chat(struct UIWindowMgr *self);
void ui_window_mgr_clear_slots(struct UIWindowMgr *self);
int ui_window_mgr_process_input(struct UIWindowMgr *self);
void ui_window_mgr_on_process(struct UIWindowMgr *self);
BYTE ui_window_mgr_handle_key(struct UIWindowMgr *self, int vk, int first_press);
BYTE ui_window_mgr_hot_key_action(struct UIWindowMgr *self, int kind);
void ui_window_mgr_send_chat_macro(int index);
BYTE ui_window_mgr_show_option_window(struct UIWindowMgr *self, int window_id, int show);
void ui_window_mgr_window_hotkey(struct UIWindowMgr *self, int key);
BYTE actor_homun_ai_running(void *actor);
BYTE actor_merc_ai_running(void *actor);
void ui_window_mgr_close_all_plates(struct UIWindowMgr *self);
int ui_window_mgr_is_dragging_member_plate(struct UIWindowMgr *self);
void ui_window_mgr_restore_plates(struct UIWindowMgr *self);
void ui_window_mgr_init_window_flags(struct UIWindowMgr *self);
void ui_window_mgr_close_drag_plate(struct UIWindowMgr *self);
BYTE ui_window_mgr_is_hot_key_of_window(struct UIWindowMgr *self);
void ui_window_mgr_note_window_opened(struct UIWindowMgr *self, int window_id);
struct WindowInfo *ui_window_mgr_get_closed_rect(struct UIWindowMgr *self, int window_id);
int ui_scale_x640(int v);
int ui_center_x640(int x);
int ui_center_y480(int y);
int scale_y_from480(int n);

void ui_window_mgr_toggle_hide_ui(struct UIWindowMgr *self);

void ui_window_mgr_request_add_friend(struct UIWindowMgr *self, const char *name);

void ui_window_mgr_modeless_message_box(struct UIWindowMgr *self, const char *text, int type, int arg3, int arg5, const char *title, int width, int height, const char *picture);

void ui_window_mgr_create_party_plates(struct UIWindowMgr *self);
void ui_window_mgr_command_plate_of(struct UIWindowMgr *self, int aid, int a, int b);
void ui_window_mgr_move_dragged_plate(struct UIWindowMgr *self, int x, int y);

void ui_window_mgr_reset_shown_infos(struct UIWindowMgr *self, const BYTE *opts);

void ui_window_mgr_init_shortcut_item_info_from_reg(struct UIWindowMgr *self);
void ui_window_mgr_init_skill_use_level_from_reg(struct UIWindowMgr *self);
void ui_window_mgr_write_shortcut_item_info_to_reg(struct UIWindowMgr *self);

void ui_window_mgr_render_windows(struct UIWindowMgr *self);

void ui_window_mgr_notify_whisper_wnd(struct UIWindowMgr *self, const char *name);
void ui_window_mgr_set_chat_line(struct UIWindowMgr *self, const char *text);
int ui_window_mgr_add_whisper_chat_to_whisper_wnd(struct UIWindowMgr *self, const char *name, const char *line,
                                                       unsigned colour, int cid);

void session_refresh_merchant_mirror_item_wnd(struct UIWindowMgr *self);
struct UIWindow *ui_minimap_zoom_wnd_new(void);
BYTE ui_minimap_zoom_wnd_claims_mouse(struct UIWindow *self);
void ui_minimap_zoom_wnd_on_l_btn_down(struct UIWindow *self, int x, int y);
void ui_window_mgr_add_to_name_waiting_list(struct UIWindowMgr *self, struct UIWindow *wnd);
struct BitmapRes;
void ui_window_mgr_draw_picture(struct UIWindowMgr *self, int x, int y, struct BitmapRes *bitmap);
void ui_window_mgr_draw_serial_number(struct UIWindowMgr *self);
BYTE ui_window_mgr_drop_on(struct UIWindowMgr *self, int x, int y, struct DragInfo *drag);
void ui_window_mgr_drop_window_refs(struct UIWindowMgr *self, struct UIWindow *wnd);
void ui_window_mgr_edit_focus_to_chat_wnd(struct UIWindowMgr *self);
void ui_window_mgr_focus_next_edit(struct UIWindowMgr *self);
void ui_window_mgr_focus_prev_edit(struct UIWindowMgr *self);
int ui_window_mgr_get_window_ids(struct UIWindowMgr *self, int *ids);
void ui_window_mgr_hide_chat_wnd(struct UIWindowMgr *self);
void ui_window_mgr_navigation_help(struct UIWindowMgr *self);
void ui_window_mgr_refresh_item_windows(struct UIWindowMgr *self);
void ui_window_mgr_remove_snap_rect(struct UIWindowMgr *self, struct UIWindow *wnd);
void ui_window_mgr_replay_close_window(struct UIWindowMgr *self, int window_id);
void ui_window_mgr_send_key_up(struct UIWindowMgr *self);
void ui_window_mgr_send_key_right(struct UIWindowMgr *self);
void ui_window_mgr_set_active_top_window(struct UIWindowMgr *self, struct UIWindow *wnd);
void ui_window_mgr_set_shop_titles_shown(struct UIWindowMgr *self, BYTE show);
void ui_window_mgr_show_windows(struct UIWindowMgr *self, int window_id, BYTE show);
void ui_window_mgr_tick(struct UIWindowMgr *self);
void ui_window_mgr_render(struct UIWindowMgr *self);
void ui_window_mgr_set_focus_edit_first(struct UIWindowMgr *self);
void ui_window_mgr_set_focus_edit_last(struct UIWindowMgr *self);

int ach_tracing_info_is_on_screen(POINT *self);
void ach_tracing_info_set_default(POINT *self);
int basic_info_wnd_info_is_on_screen(struct BasicInfoWndInfo *self);
void basic_info_wnd_info_set_default(struct BasicInfoWndInfo *self);
int equip_wnd_info_is_valid(struct WndInfoTall *self);
void equip_wnd_info_set_default(struct WndInfoTall *self);
int graphic_setting_wnd_info_is_valid(struct WindowInfo *self);
void graphic_setting_wnd_info_make_default(struct WindowInfo *self);
int homun_info_wnd_info_is_on_screen(struct WindowInfo *self);
void homun_info_wnd_info_set_default(struct WindowInfo *self);
int mer_info_wnd_info_is_on_screen(struct WindowInfo *self);
void mer_info_wnd_info_set_default(struct WindowInfo *self);
int mini_map_info_is_on_screen(struct MiniMapWndInfo *self);
void navi_window_info_clamp(struct NavigationWndInfo *self);
int npc_menu_info_is_valid(struct WindowInfo *self);
void npc_menu_info_set_default(struct WindowInfo *self);
int party_setting_wnd_info_is_valid(struct WindowInfo *self);
void party_setting_wnd_info_make_default(struct WindowInfo *self);
BYTE preview_equip_info_is_on_screen(POINT *self);
void preview_equip_info_set_default(POINT *self);
int short_cut_wnd_info_is_valid(struct WndInfoShow *self);
void short_cut_wnd_info_make_default(struct WndInfoShow *self);
int status_wnd_info_is_valid(struct StatusWndInfo *self);
void status_wnd_info_make_default(struct StatusWndInfo *self);

#endif
