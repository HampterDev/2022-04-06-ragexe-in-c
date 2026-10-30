#include "ui/controls/listbox.h"
#include "render/rp_pool.h"
#include "iteminfo/item_api.h"
#include "ui/record_tags.h"
#include "session/map_names.h"
#include "ui/window_manager_state.h"
#include "core/object_type_tables.h"
#include "game/context.h"
#include <string.h>
#include "ui/quests/quests.h"
#include "ui/quests/npc_menu.h"
#include "ui/window_mgr.h"
#include "ui/game.h"
#include "ui/ui_items.h"
#include "ui/controls/controls.h"
#include "core/msg_str.h"
#include "diag/perf.h"
#include "session/session_names.h"
#include "ui/wndmgr/wndmgr.h"

static int g_npcMenuCount = 0;

#define NPC_MENU_OK_ID 0xb0
#define NPC_MENU_CANCEL_ID 0xb1

#define WARP_TITLE_MSG 0xd5

struct Vec;
struct UIChooseWnd;
struct UIWindow;
void ui_choose_wnd_destroy(struct UIWindow *window);
void ui_choose_wnd_store_info(struct UIWindow *self);
void ui_choose_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_choose_wnd_on_draw(struct UIWindow *self);
int ui_choose_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_choose_wnd_ops = {
    .destroy = ui_choose_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_choose_wnd_store_info,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_choose_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_choose_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_frame_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_frame_wnd_on_mouse_move,
    .on_mouse_hover = ui_frame_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_frame_wnd_on_l_btn_up,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_choose_wnd_send_msg,
    .invalidate = ui_window_invalidate_base,
    .render = ui_window_render_base,
    .has_custom_backdrop = 0,
    .draw_backdrop = ui_window_draw_backdrop_base,
    .get_records = ui_choose_wnd_save_state,
    .type = UI_TYPE_FRAME,
    .on_hover_enter = ui_window_on_hover_enter_base,
    .on_hover_leave = ui_window_on_hover_leave_base,
    .make_quad = ui_window_make_quad_base,
    .hit_test = ui_window_hit_test_base,
    .render_overlay = ui_window_render_overlay_base,
};
struct UIChooseWarpWnd;
void ui_choose_warp_wnd_destroy(struct UIWindow *window);
void ui_choose_warp_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_choose_warp_wnd_on_draw(struct UIWindow *self);
int ui_choose_warp_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_choose_warp_wnd_ops = {
    .destroy = ui_choose_warp_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_window_store_info_base,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_choose_warp_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_choose_warp_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_frame_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_frame_wnd_on_mouse_move,
    .on_mouse_hover = ui_frame_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_frame_wnd_on_l_btn_up,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_choose_warp_wnd_send_msg,
    .invalidate = ui_window_invalidate_base,
    .render = ui_window_render_base,
    .has_custom_backdrop = 0,
    .draw_backdrop = ui_window_draw_backdrop_base,
    .get_records = ui_choose_warp_wnd_save_state,
    .type = UI_TYPE_FRAME,
    .on_hover_enter = ui_window_on_hover_enter_base,
    .on_hover_leave = ui_window_on_hover_leave_base,
    .make_quad = ui_window_make_quad_base,
    .hit_test = ui_window_hit_test_base,
    .render_overlay = ui_window_render_overlay_base,
};

static struct UIListBox *npc_menu_create_list(struct UIWindow *wnd, int width, int height)
{
    struct UIListBox *list = mem_alloc(sizeof(struct UIListBox));
    int list_w = width - 0x18, list_h = height - 0x37;

    if (list)
        list = ui_list_box_construct(list);
    if (list_w < 0x14)
        list_w = 0x14;
    if (list_h < 0x1e)
        list_h = 0x1e;
    ui_list_box_create_window(list, 0xc, 0x1a, list_w, list_h, 0);
    list->r = list->g = list->b = 0xf0;
    list->base.id = NPC_MENU_OK_ID;
    ui_window_add_child(wnd, &list->base);
    return list;
}

#define NPC_MENU_BUTTON(name) UI_SKIN name ".bmp", UI_SKIN name "_a.bmp", UI_SKIN name "_b.bmp"

static void npc_menu_add_button(struct UIWindow *wnd, const char *normal, const char *hover, const char *pressed,
                              int x, int y, int id)
{
    struct UIBitmapButton *button = mem_alloc(sizeof(struct UIBitmapButton));

    if (button)
        button = ui_bitmap_button_construct(button);
    ui_bitmap_button_set_bitmap(button, normal, 0, 0);
    ui_bitmap_button_set_bitmap(button, hover, 1, 0);
    ui_bitmap_button_set_bitmap(button, pressed, 2, 0);
    ui_window_create(&button->button.base, button->bitmap_width, button->bitmap_height);
    ui_window_move_base(&button->button.base, x, y);
    button->button.base.id = id;
    ui_window_add_child(wnd, &button->button.base);
}

static void npc_menu_add_buttons(struct UIFrameWnd *frame)
{
    struct UIWindow *wnd = &frame->base;

    npc_menu_add_button(wnd, NPC_MENU_BUTTON("btn_ok"), wnd->width - 0x5c, wnd->height - 0x18, NPC_MENU_OK_ID);
    UI_FRAME_DEFAULT_ID(frame) = NPC_MENU_OK_ID;
    npc_menu_add_button(wnd, NPC_MENU_BUTTON("btn_cancel"), wnd->width - 0x2e, wnd->height - 0x18,
                      NPC_MENU_CANCEL_ID);
}

struct UIChooseWnd *ui_choose_wnd_construct(struct UIChooseWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_choose_wnd_ops;
    self->line_values.first = self->line_values.last = self->line_values.end = NULL;
    self->list = NULL;
    self->line_index = -1;
    self->info = NULL;
    return self;
}

struct UIWindow *ui_choose_wnd_new(void)
{
    return (struct UIWindow *)ui_choose_wnd_construct(mem_alloc(sizeof(struct UIChooseWnd)));
}

static void ui_choose_wnd_destruct(struct UIChooseWnd *self)
{
    self->frame.base.ops = &ui_choose_wnd_ops;
    self->line_values.last = self->line_values.first;
    vec_free(&self->line_values);
    ui_frame_wnd_destruct(&self->frame);
}

void ui_choose_wnd_destroy(struct UIWindow *window)
{
    struct UIChooseWnd *self = (struct UIChooseWnd *)window;
    ui_choose_wnd_destruct(self);
    mem_free(self);
}

void ui_choose_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UIChooseWnd *self = (struct UIChooseWnd *)window;
    self->list = npc_menu_create_list(&self->frame.base, width, height);
    npc_menu_add_buttons(&self->frame);
}

void ui_choose_wnd_on_draw(struct UIWindow *window)
{
    struct UIChooseWnd *self = (struct UIChooseWnd *)window;
    ui_window_draw_sys_box(&self->frame.base, 0, 0, 0, 0);
}

struct UIChooseWarpWnd *ui_choose_warp_wnd_construct(struct UIChooseWarpWnd *self)
{
    struct Vec *dest = &self->destinations;

    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_choose_warp_wnd_ops;
    vec_init(dest);
    vec_reserve(dest, 6, sizeof(struct String));
    return self;
}

struct UIWindow *ui_choose_warp_wnd_new(void)
{
    return (struct UIWindow *)ui_choose_warp_wnd_construct(mem_alloc(sizeof(struct UIChooseWarpWnd)));
}

static void ui_choose_warp_wnd_destruct(struct UIChooseWarpWnd *self)
{
    self->frame.base.ops = &ui_choose_warp_wnd_ops;
    string_vec_free(&self->destinations);
    ui_frame_wnd_destruct(&self->frame);
}

void ui_choose_warp_wnd_destroy(struct UIWindow *window)
{
    struct UIChooseWarpWnd *self = (struct UIChooseWarpWnd *)window;
    ui_choose_warp_wnd_destruct(self);
    mem_free(self);
}

void ui_choose_warp_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UIChooseWarpWnd *self = (struct UIChooseWarpWnd *)window;
    struct UIWindow *title = mem_alloc(sizeof(struct UIStaticText));

    if (title)
        title = ui_static_text_construct(title);
    ui_window_create(title, self->frame.base.width - 0x1e, 0xf);
    ui_window_move(title, 0xc, 9);
    ui_static_text_set_text(title, msg_str(WARP_TITLE_MSG), 0);
    ui_static_text_set_font(title, 0, 12, 0, 0);
    ui_window_add_child(&self->frame.base, title);

    self->list = npc_menu_create_list(&self->frame.base, width, height);
    npc_menu_add_buttons(&self->frame);
}

void ui_choose_warp_wnd_on_draw(struct UIWindow *window)
{
    struct UIChooseWarpWnd *self = (struct UIChooseWarpWnd *)window;
    ui_window_draw_sys_box(&self->frame.base, 0, 0, 0, 0);
}

#define NPC_MENU_MSG_SCROLL_A 0x12
#define NPC_MENU_MSG_SCROLL_B 0x13
#define NPC_MENU_MSG_ADD_LINE 0x1b
#define NPC_MENU_MSG_SET_NPC 0x1c
#define NPC_MENU_MSG_SET_VALUE 0x3b
#define NPC_MENU_MSG_ABORT 0xa6

#define REQ_NPC_MENU_CHOICE 0x17
#define REQ_NPC_CLOSE 0x28
#define REQ_WARP_CHOICE 0x4a

#define NPC_MENU_WND_ID 0x11
#define WARP_WND_ID 0x27
#define MSG_NPC_MENU_BLOCKED 0xaaa
#define MSG_NPC_MENU_HIDDEN 0x29a
#define MSG_WARP_RANDOM 0xd4
#define MSG_WARP_CANCEL 0xda
#define NPC_MENU_OK_DELAY_MS 300

#define NPC_MENU_INFO(self) ((struct WindowInfo *)(self)->info)

#define NPC_MENU_RESTORED_MSG 0x20

static int npc_menu_load_records(struct UIWindow *self, const char *buf, unsigned size, short begin, short end,
                               short line)
{
    struct UIRecordHeader hdr;
    unsigned pos = 6;

    memcpy(&hdr, buf, sizeof hdr);
    if (hdr.tag != begin)
        return -1;
    while (size > pos) {
        memcpy(&hdr, buf + pos, sizeof hdr);
        pos += 6;
        if (hdr.tag == end)
            return 1;
        if (hdr.tag == line) {
            char text[0x104];

            memset(text, 0, sizeof text);
            memcpy(text, buf + pos, hdr.len);
            pos += hdr.len;
            ui_window_send_msg(self, NULL, NPC_MENU_MSG_ADD_LINE, (int)text, 0, 0, 0);
        } else {
            pos += hdr.len;
        }
    }
    return 0;
}

void ui_choose_wnd_store_info(struct UIWindow *window)
{
    struct UIChooseWnd *self = (struct UIChooseWnd *)window;
    struct WindowInfo *info = NPC_MENU_INFO(self);

    if (!info)
        return;
    info->width = self->frame.base.width;
    info->height = self->frame.base.height;
    info->x = self->frame.base.x;
    info->y = self->frame.base.y;
}

int ui_choose_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1, int val2,
                                   int val3, int val4)
{
    struct UIChooseWnd *self = (struct UIChooseWnd *)window;
    if (g_windowMgr.rodex_compose_wnd) {
        ui_chat_line(msg_str(MSG_NPC_MENU_BLOCKED), 0x64ffff);
        return 0;
    }
    switch (msg) {
    case UI_MSG_COMMAND:
        ui_window_record_command(&self->frame.base, val1, val2, val3, val4);
        if (val1 == NPC_MENU_OK_ID) {
            DWORD now = GetTickCount();

            if (now - g_session1.npcDialogTick < NPC_MENU_OK_DELAY_MS)
                return 0;
            g_session1.npcDialogTick = now;
            if (self->line_values.first == self->line_values.last) {
                ui_choose_wnd_send_msg(&self->frame.base, NULL, UI_MSG_COMMAND, NPC_MENU_CANCEL_ID, 0, 0, 0);
                return 0;
            }
            if (self->list->selected == -1)
                return 0;
            game_send_message(REQ_NPC_MENU_CHOICE,
                          VEC_AT(&self->line_values, int, self->list->selected) + 1, self->npc_id, 0, 0);
            ui_window_mgr_post_quit1(&g_windowMgr, NPC_MENU_WND_ID);
        } else if (val1 == NPC_MENU_CANCEL_ID) {
            game_send_message(REQ_NPC_CLOSE, 0, 0, 0, 0);
        }
        return 0;
    case NPC_MENU_MSG_SCROLL_A:
    case NPC_MENU_MSG_SCROLL_B:
        ui_window_send_msg(&self->list->base, NULL, msg, 0, 0, 0, 0);
        return 0;
    case NPC_MENU_MSG_ADD_LINE: {
        const char *text = (const char *)val1;
        int tag_len = 0, item_id = 0;

        if (!text)
            return 0;
        self->line_index++;
        if (strstr(text, msg_str(MSG_NPC_MENU_HIDDEN)))
            return 0;
        if (parse_item_id_tag(text, text + strlen(text), &tag_len, &item_id)) {
            struct String name;

            string_init(&name);
            if (item_id) {
                struct ItemInfo item;
                const char *buf;
                struct String tmp;

                item_info_construct(&item);
                item_info_set_item_id(&item, item_id);
                buf = item_info_get_name(&item);
                string_init(&tmp);
                string_assign_cstr(&tmp, buf);
                string_free(&name);
                string_move(&name, &tmp);
                item_info_destruct(&item);
            }
            ui_list_box_append_text(self->list, string_cstr(&name));
            string_free(&name);
        } else {
            ui_list_box_append_text(self->list, text);
        }
        vec_push(&self->line_values, &self->line_index, sizeof self->line_index);
        return 0;
    }
    case NPC_MENU_MSG_SET_NPC:
        g_npcMenuCount++;
        self->npc_id = val1;
        return 0;
    case UI_MSG_SET_INFO:
        self->info = (void *)val1;
        if (!npc_menu_info_is_valid(NPC_MENU_INFO(self)))
            npc_menu_info_set_default(NPC_MENU_INFO(self));
        ui_frame_wnd_move(&self->frame.base, NPC_MENU_INFO(self)->x, NPC_MENU_INFO(self)->y);
        return 0;
    case UI_MSG_LOAD_RECORDS:
        {
            int record_bytes = ui_window_records_validate((const void *)val1, (size_t)(unsigned)val2);
            if (record_bytes <= 0) return 0;
            val2 = record_bytes;
        }
        if (npc_menu_load_records(&self->frame.base, (const char *)val1, (unsigned)val2, NPC_MENU_STATE_BEGIN,
                                NPC_MENU_STATE_END, NPC_MENU_STATE_LINE) >= 0
            && g_windowMgr.say_dialog_wnd)
            ui_window_send_msg(g_windowMgr.say_dialog_wnd, NULL, NPC_MENU_RESTORED_MSG, 0, 0, 0, 0);
        return 0;
    case NPC_MENU_MSG_ABORT: {
        struct Mode *mode = game_current_base_mode();

        if (mode)
            mode_send_message(mode, REQ_NPC_MENU_CHOICE, -1, self->npc_id, 0, 0);
        return 0;
    }
    default:
        return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
    }
}

int ui_choose_warp_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1,
                                       int val2, int val3, int val4)
{
    struct UIChooseWarpWnd *self = (struct UIChooseWarpWnd *)window;
    switch (msg) {
    case UI_MSG_COMMAND:
        ui_window_record_command(&self->frame.base, val1, val2, val3, val4);
        if (val1 == NPC_MENU_OK_ID) {
            int sel = self->list->selected;
            struct Mode *mode;

            if (sel == -1)
                return 0;
            mode = game_current_base_mode();
            mode_send_message(mode, REQ_WARP_CHOICE,
                          (int)string_cstr(&VEC_AT(&self->destinations, struct String, sel)),
                          self->skill_id, 0, 0);
            ui_window_mgr_post_quit1(&g_windowMgr, WARP_WND_ID);
        } else if (val1 == NPC_MENU_CANCEL_ID) {
            game_send_message(REQ_WARP_CHOICE, (int)"cancel", self->skill_id, 0, 0);
            ui_window_mgr_post_quit1(&g_windowMgr, WARP_WND_ID);
        }
        return 0;
    case NPC_MENU_MSG_SCROLL_A:
    case NPC_MENU_MSG_SCROLL_B:
        ui_window_send_msg(&self->list->base, NULL, msg, 0, 0, 0, 0);
        return 0;
    case NPC_MENU_MSG_ADD_LINE: {
        const char *text = (const char *)val1;
        struct String s;
        BYTE match;

        if (!text)
            return 0;
        string_init(&s);
        string_assign_cstr(&s, text);
        match = string_equal_cstr(&s, "Random");
        string_free(&s);
        if (match) {
            ui_list_box_append_text(self->list, msg_str(MSG_WARP_RANDOM));
        } else {
            string_assign_cstr(&s, text);
            match = string_equal_cstr(&s, "cancel");
            string_free(&s);
            if (match)
                ui_list_box_append_text(self->list, msg_str(MSG_WARP_CANCEL));
            else
                ui_list_box_append_text(self->list, session_map_display_name(session_get_rsw_name(text)));
        }
        string_init(&s);
        string_assign_cstr(&s, text);
        string_vec_push_move(&self->destinations, &s);
        string_free(&s);
        return 0;
    }
    case NPC_MENU_MSG_SET_VALUE:
        self->skill_id = val1;
        return 0;
    case UI_MSG_LOAD_RECORDS:
        {
            int record_bytes = ui_window_records_validate((const void *)val1, (size_t)(unsigned)val2);
            if (record_bytes <= 0) return 0;
            val2 = record_bytes;
        }
        if (npc_menu_load_records(&self->frame.base, (const char *)val1, (unsigned)val2, WARP_STATE_BEGIN,
                                WARP_STATE_END, WARP_STATE_LINE) == 1)
            ui_choose_warp_wnd_send_msg(&self->frame.base, NULL, NPC_MENU_MSG_ADD_LINE, (int)"cancel", 0, 0, 0);
        return 0;
    default:
        return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
    }
}
