#include "ui/window_manager_state.h"
#include "core/memory.h"
#include <string.h>
#include "ui/wndmgr/wndmgr.h"
#include "ui/social/party_windows.h"
#include "platform/clientinfo.h"
#include "ui/chat/sub_chat_map.h"
#include "ui/ui_std.h"

struct UIWindowMgr g_windowMgr;

static void destroy_string(void *value) { string_free(value); }

static void destroy_chat_line(void *value)
{
    struct WmChatLine *line = value;

    string_free(&line->text);
}

struct UIWindowMgr *window_mgr_construct(struct UIWindowMgr *self)
{
    tree_init(&self->windows_by_id);
    list_init(&self->id_list);
    tree_init(&self->sub_chat_wnds);
    self->close_capture_root = 0;
    list_init(&self->windows);
    list_init(&self->quit_list);
    list_init(&self->name_waiting);
    tree_init(&self->snap_rects);
    self->mouse_x_last = -1;
    self->mouse_y_last = -1;
    string_init(&self->wallpaper_name);
    list_init(&self->pending_chat_lines);
    list_init(&self->chat_history);
    list_init(&self->whisper_history);
    string_init(&self->chat_line);
    tree_init(&self->whisper_wnds);
    list_init(&self->hidden_wnds);
    self->basic_info_info.tall = 1;
    self->message_boxes.first = NULL;
    self->message_boxes.last = NULL;
    self->message_boxes.end = NULL;
    tree_init(&self->closed_rects);
    list_init(&self->plates);
    list_init(&self->plate_ids);

    ui_window_mgr_clear_slots(self);
    self->backdrop = NULL;
    self->screen_width = 0;
    self->screen_height = 0;
    string_clear(&self->chat_line);
    self->chat_wnd_status = 1;
    self->chat_wnd_height = 0x62;
    self->chat_wnd_x = 0;
    self->chat_wnd_y = 0;
    self->chat_wnd_show = 1;
    self->minimap_zoom = 1.0f;
    self->minimap_show_navigation = 1;
    self->minimap_argb = 0xaaffffff;
    self->draw_compass = 1;
    self->whisper_history_pos = 0;
    self->chat_history_pos = 0;
    self->invalidated = 0;
    self->chat_wnd_stick_on = 1;
    self->on_st_sub_chat = 0;
    self->on_bt_sub_chat = 0;
    self->candidate_box = g_languageIndex == 2;
    self->skill_describe_show = 0;
    self->logo_x = 0;
    self->logo_y = g_serviceType == 0 ? -25 : 0;
    self->grade_width = 0xcb;
    self->service_piece_x = self->service_piece_y = 0;
    self->tape_y = self->tape_top = 0;
    self->rating_x = self->rating_y = 0;
    self->unpicked_piece_x = self->unpicked_piece_y = 0;
    self->login_drag_piece = 0;
    self->grade_height = 0x73;
    self->grade_x = 0;
    self->grade_y = 0;

    list_clear(&self->id_list, NULL);
    self->last_id = -1;
    self->chat_bar_shown = 0;
    self->chat_bar_toggleable = 1;
    self->hide_ui_state = 0;
    self->navigation_v4_wnd = NULL;
    self->hover_serial = 0;
    self->hover_prev = 0;
    self->round_box_tooltip_show = 0;
    self->drag_plate = NULL;
    self->item_store_wnd_num = 0;
    self->plate_width = 0x97;
    self->plate_height = 0x25;
    self->show_shop_titles = 1;
    return self;
}

void ui_window_mgr_destroy(struct UIWindowMgr *self)
{
    struct TreeNode *it;

    if (g_miniPartyAreaMgr == NULL) {
        void *mem = mem_try_alloc(WM_MINI_PARTY_AREA_MGR_SIZE);

        g_miniPartyAreaMgr = mem ? mini_party_area_mgr_construct(mem) : NULL;
    }
    if (g_miniPartyAreaMgr) {
        mini_party_area_mgr_delete(g_miniPartyAreaMgr);
        g_miniPartyAreaMgr = NULL;
    }
    ui_window_mgr_close_all_windows(self);
    if (self->backdrop) {
        surface_destroy(self->backdrop);
        self->backdrop = NULL;
    }

    for (it = tree_first(&self->closed_rects); it != self->closed_rects.head; it = tree_next(it))
        mem_free(TREE_VALUE(it, struct WmClosedRect)->rect);
    tree_clear(&self->closed_rects, NULL);

    list_free(&self->plate_ids, NULL);
    list_free(&self->plates, NULL);
    tree_free(&self->closed_rects, NULL);
    vec_free(&self->message_boxes);
    list_free(&self->hidden_wnds, NULL);
    tree_free(&self->whisper_wnds, wm_whisper_wnd_destroy);
    string_free(&self->chat_line);
    list_free(&self->whisper_history, destroy_string);
    list_free(&self->chat_history, destroy_string);
    list_free(&self->pending_chat_lines, destroy_chat_line);
    string_free(&self->wallpaper_name);
    tree_free(&self->snap_rects, NULL);
    list_free(&self->name_waiting, NULL);
    list_free(&self->quit_list, NULL);
    list_free(&self->windows, NULL);
    tree_free(&self->sub_chat_wnds, NULL);
    list_free(&self->id_list, NULL);
    tree_free(&self->windows_by_id, NULL);
}
