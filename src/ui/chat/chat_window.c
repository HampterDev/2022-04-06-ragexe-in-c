#include "ui/controls/listbox.h"
#include "ui/controls/edit.h"
#include "ui/chat/readable_links.h"
#include "render/rp_pool.h"
#include "session/state.h"
#include "session/chat_tabs_layout.h"
#include "ui/window_manager_state.h"
#include "core/object_type_tables.h"
#include "ui/window_access.h"
#include <string.h>
#include "ui/chat/chat_window.h"
#include "ui/chat/markup_links.h"
#include "ui/chat/chat_window_parts.h"
#include "core/msg_str.h"
#include "diag/perf.h"
#include "render/screen_draw.h"
#include "ui/ui_std.h"
#include "ui/wndmgr/wndmgr.h"

struct UINewChatWnd;
struct UIWindow;
void ui_new_chat_wnd_destroy(struct UIWindow *window);
void ui_new_chat_wnd_resize(struct UIWindow *self, int width, int height);
void ui_new_chat_wnd_move(struct UIWindow *self, int x, int y);
void ui_new_chat_wnd_store_info(struct UIWindow *self);
void ui_new_chat_wnd_set_show(struct UIWindow *self, int visible);
void ui_new_chat_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_new_chat_wnd_on_draw(struct UIWindow *self);
void ui_new_chat_wnd_on_l_btn_down(struct UIWindow *self, int x, int y);
void ui_new_chat_wnd_on_mouse_move(struct UIWindow *self, int x, int y);
void ui_new_chat_wnd_on_l_btn_up(struct UIWindow *self, int x, int y);
void ui_new_chat_wnd_draw_backdrop(struct UIWindow *window);
const struct UIWindowOps ui_new_chat_wnd_ops = {
    .destroy = ui_new_chat_wnd_destroy,
    .resize = ui_new_chat_wnd_resize,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_new_chat_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_new_chat_wnd_store_info,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_new_chat_wnd_set_show,
    .on_create = ui_new_chat_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_new_chat_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_new_chat_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_new_chat_wnd_on_mouse_move,
    .on_mouse_hover = ui_frame_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_new_chat_wnd_on_l_btn_up,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_new_chat_wnd_send_msg,
    .invalidate = ui_window_invalidate_base,
    .render = ui_window_render_base,
    .has_custom_backdrop = 1,
    .draw_backdrop = ui_new_chat_wnd_draw_backdrop,
    .get_records = ui_frame_wnd_get_records,
    .type = UI_TYPE_FRAME,
    .on_hover_enter = ui_window_on_hover_enter_base,
    .on_hover_leave = ui_window_on_hover_leave_base,
    .make_quad = ui_window_make_quad_base,
    .hit_test = ui_window_hit_test_base,
    .render_overlay = ui_window_render_overlay_base,
};

#define CHAT_SELECTED_TAB_ARGB 0x66000000
#define CHAT_OTHER_TAB_ARGB 0x99000000
#define CHAT_TITLE_ICONS_HEIGHT 0xe
#define CHAT_LAYOUT_FILE "SaveData\\ChatWndInfo_U.lua"
#define CHAT_LINE_GREY 0x8e8e8e
#define CHAT_LINE_WHITE 0xffffff
#define CHAT_TAB_PADDING 0xa
#define CHAT_CREATE_STEP_PIXELS 42
#define CHAT_CREATE_MAX_STEP 4
#define MSG_CHAT_BAR_DIALOG 0xc73
#define MSG_CHAT_BAR_SYS_1 0xc75
#define MSG_CHAT_BAR_SYS_2 0xc74

struct UINewChatWnd *ui_new_chat_wnd_construct(struct UINewChatWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_new_chat_wnd_ops;
    memset(&self->boxes, 0, sizeof self->boxes);
    self->edit = NULL;
    self->whisper = NULL;
    self->focus_edit = NULL;
    self->resizer = NULL;
    self->max_tabs = CHAT_MAX_TABS;
    g_windowMgr.chat_wnd_stick_on = 1;
    SIZE_STEP(self) = 0;
    self->combo_kind = 0;
    self->tab = 0;
    self->saved_step = 0;
    self->tab_height = 0x10;
    self->tab_width = 0x47;
    self->bar_height = 0x18;
    switch (g_serviceType) {
    case 1:
    case 7:
    case 11:
        self->tab_width = 0x6e;
        break;
    case 10:
        self->tab_width = 0x47;
        break;
    case 18:
        self->tab_width = 0x5a;
        break;
    }
    return self;
}

struct UIWindow *ui_new_chat_wnd_new(void)
{
    return (struct UIWindow *)ui_new_chat_wnd_construct(mem_alloc(sizeof(struct UINewChatWnd)));
}

void ui_new_chat_wnd_destroy(struct UIWindow *window)
{
    struct UINewChatWnd *self = (struct UINewChatWnd *)window;
    int saved;
    self->frame.base.ops = &ui_new_chat_wnd_ops;
    saved = chat_tabs_save_layout(&g_session1.chatWndTabs, &g_session1.subChatWnds,
                                 g_session1.canDragWnd == 1, CHAT_LAYOUT_FILE);
    vec_free(&self->boxes);
    ui_frame_wnd_destruct(&self->frame);
    mem_free(self);
    if (!saved)
        MessageBoxA(NULL, "Could not save your chat layout.\n"
                    "Your latest layout changes will not be restored next time.",
                    "Chat layout", MB_OK | MB_ICONWARNING | MB_TASKMODAL);
}

void ui_new_chat_wnd_resize(struct UIWindow *window, int width, int height)
{
    struct UINewChatWnd *self = (struct UINewChatWnd *)window;
    struct UIWindow *wnd = &self->frame.base;

    ui_window_resize_base(wnd, width, height);
    self->backdrop.width = wnd->width - 6;
    self->backdrop.height = wnd->height - self->tab_height;
}

void ui_new_chat_wnd_move(struct UIWindow *window, int x, int y)
{
    struct UINewChatWnd *self = (struct UINewChatWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    int sx = 0, sy = 0;

    ui_frame_wnd_move((struct UIWindow *)(&self->frame), x, y);
    ui_window_to_screen(wnd, &sx, &sy);
    self->backdrop.x = sx + 3;
    self->backdrop.y = self->tab_height + sy;
    if (g_windowMgr.sub_chat_mini_wnd)
        ui_window_send_msg(g_windowMgr.sub_chat_mini_wnd, wnd, UI_MSG_COMMAND, MINI_ID_PLACE, wnd->width + x, wnd->height + y, 0);
}

void ui_new_chat_wnd_store_info(struct UIWindow *window)
{
    struct UINewChatWnd *self = (struct UINewChatWnd *)window;
    g_windowMgr.chat_wnd_status = SIZE_STEP(self);
    g_windowMgr.chat_wnd_height = self->frame.base.height;
    g_windowMgr.chat_wnd_x = self->frame.base.x;
    g_windowMgr.chat_wnd_y = self->frame.base.y;
}

void ui_new_chat_wnd_set_show(struct UIWindow *window, int visible)
{
    struct UINewChatWnd *self = (struct UINewChatWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct UIWindow **box;
    int i;

    wnd->visible = visible;
    if (visible) {
        ui_window_invalidate_base(wnd);
        ui_window_mgr_update_snap_rect(&g_windowMgr, wnd);
    } else {
        ui_window_mgr_remove_snap_rect(&g_windowMgr, wnd);
    }
    for (box = self->boxes.first; box != self->boxes.last; box++)
        ui_window_set_show(*box, wnd->visible);
    for (i = 0; (unsigned)i < g_windowMgr.sub_chat_wnds.size; i++) {
        struct SubChatWnd *sub = tree_get(&g_windowMgr.sub_chat_wnds, &i, sizeof i, sizeof *sub, tree_less_int);

        ui_window_set_show(sub->wnd, wnd->visible);
    }
}

void ui_new_chat_wnd_place_title_buttons(struct UINewChatWnd *self)
{
    struct UIWindow *mini = &self->mini->button.base;
    struct UIWindow *tab_option = &self->tab_option->button.base;
    struct UIWindow *stick = &self->stick->button.base;
    struct UIWindow *battle = &self->battle_option->button.base;
    int x;

    if (!CHAT_TABS_EDITABLE && SIZE_STEP(self)) {
        ui_window_move_base(mini, OFF_SCREEN, OFF_SCREEN);
        ui_window_move_base(tab_option, OFF_SCREEN, OFF_SCREEN);
        ui_window_move_base(stick, OFF_SCREEN, OFF_SCREEN);
        ui_window_move_base(battle, self->edit_icon_x - battle->width - 2, 4);
        return;
    }
    if (!SIZE_STEP(self)) {
        ui_window_move_base(mini, OFF_SCREEN, OFF_SCREEN);
        ui_window_move_base(tab_option, OFF_SCREEN, OFF_SCREEN);
        ui_window_move_base(stick, OFF_SCREEN, OFF_SCREEN);
        ui_window_move_base(battle, OFF_SCREEN, OFF_SCREEN);
        return;
    }
    x = self->edit_icon_x - mini->width - 2;
    ui_window_move_base(mini, x, 4);
    x += -2 - tab_option->width;
    ui_window_move_base(tab_option, x, 4);
    x += -2 - stick->width;
    ui_window_move_base(stick, x, 4);
    x -= battle->width + 2;
    ui_window_move_base(battle, x, 4);
}

void ui_new_chat_wnd_place_bar(struct UINewChatWnd *self)
{
    struct UIWindow *wnd = &self->frame.base;
    int i;

    if (g_windowMgr.chat_bar_shown) {
        int bar_y = wnd->height - self->bar_height;

        ui_window_move_base(&self->whisper->edit.base, 6, bar_y + 4);
        ui_window_move_base(&self->edit->edit.base, 0x6e, bar_y + 4);
        ui_window_move_base(&self->resizer->base, 3, wnd->height - self->resizer->base.height);
        ui_window_move_base(&self->bar_buttons[0]->button.base, 0x61, bar_y + 3);
        ui_window_move_base(&self->bar_buttons[1]->button.base, 0x24a, bar_y + 7);
        ui_window_move_base(&self->bar_buttons[2]->button.base, 0x23c, bar_y + 7);
    } else {
        ui_window_move_base(&self->whisper->edit.base, OFF_SCREEN, OFF_SCREEN);
        ui_window_move_base(&self->edit->edit.base, OFF_SCREEN, OFF_SCREEN);
        ui_window_move_base(&self->resizer->base, OFF_SCREEN, OFF_SCREEN);
        for (i = 0; i < 3; i++)
            ui_window_move_base(&self->bar_buttons[i]->button.base, OFF_SCREEN, OFF_SCREEN);
    }
    ui_window_invalidate_base(wnd);
}

void ui_new_chat_wnd_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UINewChatWnd *self = (struct UINewChatWnd *)window;
    struct UIWindow *wnd = &self->frame.base;

    if ((unsigned)(y - 1) > 0x13) {
        ui_window_mgr_set_focus_edit(&g_windowMgr, self->focus_edit);
        ui_frame_wnd_on_l_btn_down((struct UIWindow *)(&self->frame), x, y);
        return;
    }
    if (SIZE_STEP(self) <= 0)
        return;
    if (y < CHAT_TITLE_ICONS_HEIGHT && x > self->edit_icon_x && x < self->edit_icon_x + 9) {
        RECT rect;
        int i;

        g_session1.canDragWnd = g_session1.canDragWnd == 0;
        ui_new_chat_wnd_place_title_buttons(self);
        for (i = 0; (unsigned)i < g_windowMgr.sub_chat_wnds.size; i++) {
            struct SubChatWnd *sub = tree_get(&g_windowMgr.sub_chat_wnds, &i, sizeof i, sizeof *sub, tree_less_int);

            ui_sub_chat_his_wnd_place_buttons((struct UISubChatHisWnd *)sub->wnd);
            ui_window_invalidate(sub->wnd);
        }
        rect.left = self->edit_icon_x;
        rect.top = 4;
        rect.right = self->edit_icon_x + 0xf;
        rect.bottom = 0xd;
        ui_window_fill_canvas(wnd, CHAT_COLOUR_KEY, &rect);
        ui_window_invalidate_base(wnd);
    }
    if (g_windowMgr.chat_bar_toggleable && y < CHAT_TITLE_ICONS_HEIGHT && x > self->bar_icon_x && x < self->bar_icon_x + 0xe) {
        g_windowMgr.chat_bar_shown = g_windowMgr.chat_bar_shown == 0;
        if (g_windowMgr.new_chat_wnd)
            ui_new_chat_wnd_place_bar((struct UINewChatWnd *)g_windowMgr.new_chat_wnd);
        ui_window_invalidate_base(wnd);
    }
}

void ui_new_chat_wnd_on_mouse_move(struct UIWindow *window, int x, int y)
{
    struct UINewChatWnd *self = (struct UINewChatWnd *)window;
    if (ui_window_mgr_get_capture(&g_windowMgr) == &self->frame.base && g_session1.canDragWnd == 1)
        ui_frame_wnd_on_mouse_move((struct UIWindow *)(&self->frame), x, y);
}

void ui_new_chat_wnd_on_l_btn_up(struct UIWindow *window, int x, int y)
{
    struct UINewChatWnd *self = (struct UINewChatWnd *)window;
    ui_frame_wnd_on_l_btn_up((struct UIWindow *)(&self->frame), x, y);
}

void ui_new_chat_wnd_draw_backdrop(struct UIWindow *window)
{
    struct UINewChatWnd *self = (struct UINewChatWnd *)window;

    int x = 3, y = 0;
    int i;

    ui_window_to_screen(&self->frame.base, &x, &y);
    if (!SIZE_STEP(self))
        return;
    for (i = 0; i < (int)VEC_COUNT(&CHAT_TABS(self)->names, struct String); i++) {
        int other = i != self->tab;

        draw_box_screen(x + 1 + self->tab_width * i, y + 1 + other, self->tab_width - 1, self->tab_height - other,
                      other ? CHAT_OTHER_TAB_ARGB : CHAT_SELECTED_TAB_ARGB);
    }
    draw_box_screen(self->backdrop.x + 1, self->backdrop.y + 1, self->backdrop.width - 2, self->backdrop.height - 1,
                  self->backdrop.argb);
}

static void place_bar_buttons(struct UINewChatWnd *self)
{
    int bar_y = self->frame.base.height - self->bar_height;
    int place[3][2];
    int i;

    place[0][0] = 0x61;
    place[0][1] = bar_y + 3;
    place[1][0] = 0x24a;
    place[1][1] = bar_y + 7;
    place[2][0] = 0x23c;
    place[2][1] = bar_y + 7;
    for (i = 0; i < 3; i++)
        ui_window_move_base(&self->bar_buttons[i]->button.base, place[i][0], place[i][1]);
}

void ui_new_chat_wnd_set_size_step(struct UINewChatWnd *self, int step)
{
    struct UIWindow *wnd = &self->frame.base;
    struct UIWindow **box;
    int old_height, height, i;

    if (g_windowMgr.combo_box_wnd)
        ui_window_mgr_post_quit1(&g_windowMgr, 0x1a);
    if (ui_window_mgr_get_capture(&g_windowMgr) == wnd)
        return;
    step = step <= 15 ? step : 0;
    if (step < 0)
        step = 15;
    SIZE_STEP(self) = step;
    ui_window_invalidate_base(wnd);
    old_height = wnd->height;
    if (step) {
        height = self->bar_height + (step + 3) * 14 + self->tab_height;
        ui_new_chat_wnd_resize(wnd, wnd->width, height);
        ui_new_chat_wnd_move(wnd, wnd->x, wnd->y - height + old_height);
        for (i = 0, box = self->boxes.first; box != self->boxes.last; box++, i++) {
            ui_window_resize(*box, wnd->width - 8, height - self->tab_height - self->bar_height - 4);
            ui_window_invalidate(*box);
            if (i == self->tab)
                ui_window_move(*box, 5, self->tab_height + 2);
            else
                ui_window_move(*box, OFF_SCREEN - wnd->width, OFF_SCREEN);
        }
        ui_window_invalidate_base(wnd);
        ui_window_move_base(&self->whisper->edit.base, 6, wnd->height - self->bar_height + 4);
        ui_window_move_base(&self->edit->edit.base, 0x6e, wnd->height - self->bar_height + 4);
        ui_window_move_base(&self->resizer->base, 3, wnd->height - self->resizer->base.height);
        ui_window_move_base(&self->stick->button.base, self->tab_width * 2 + 4, 4);
        ui_window_move_base(&self->battle_option->button.base, self->tab_width * 2 + 0x13, 4);
        ui_window_move(self->tabs, 3, 0);
    } else {
        height = self->bar_height;
        ui_new_chat_wnd_resize(wnd, wnd->width, height);
        ui_new_chat_wnd_move(wnd, wnd->x, wnd->y - height + old_height);
        for (box = self->boxes.first; box != self->boxes.last; box++)
            ui_window_move(*box, OFF_SCREEN - wnd->width, OFF_SCREEN);
        ui_window_move_base(&self->whisper->edit.base, 6, wnd->height - self->bar_height + 4);
        ui_window_move_base(&self->edit->edit.base, 0x6e, wnd->height - self->bar_height + 4);
        ui_window_move_base(&self->resizer->base, 3, wnd->height - self->resizer->base.height);
        ui_window_move_base(&self->stick->button.base, OFF_SCREEN, OFF_SCREEN);
        ui_window_move_base(&self->battle_option->button.base, OFF_SCREEN, OFF_SCREEN);
        ui_window_move(self->tabs, OFF_SCREEN, OFF_SCREEN);
    }
    place_bar_buttons(self);
    ui_new_chat_wnd_place_title_buttons(self);
    ui_new_chat_wnd_place_bar(self);
}

static struct UIBitmapButton *title_button(const char *name)
{
    static const char *const suffix[3] = { "_a.bmp", "_b.bmp", "_b.bmp" };
    struct UIBitmapButton *button;
    struct String path;
    int state;

    string_init(&path);
    string_assign_cstr(&path, name);
    button = mem_alloc(sizeof(struct UIBitmapButton));
    if (button)
        button = ui_bitmap_button_construct(button);
    for (state = UI_BTN_UP; state <= UI_BTN_DOWN; state++) {
        struct String file;

        string_copy(&file, &path);
        string_append_cstr(&file, suffix[state]);
        ui_bitmap_button_set_bitmap(button, string_cstr(&file), state, 0);
        string_free(&file);
    }
    ui_window_create(&button->button.base, button->bitmap_width, button->bitmap_height);
    string_free(&path);
    return button;
}

void ui_new_chat_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UINewChatWnd *self = (struct UINewChatWnd *)window;
    static const char *const bar_bitmaps[3][3] = {
        { "dialog_btn0", "dialog_btn1", "dialog_btn2" },
        { "sys_base_off", "sys_base_on", "sys_base_on" },
        { "sys_base_off", "sys_base_on", "sys_base_on" },
    };
    static const int bar_ids[3] = { CHAT_ID_BAR_DIALOG, CHAT_ID_BAR_SYS_1, CHAT_ID_BAR_SYS_2 };
    struct UIWindow *wnd = &self->frame.base;
    struct UIWindow *tabs, *grip;
    struct UIChatEditCtrl *edit;
    struct UIWhisperEditCtrl *whisper;
    struct UIResizer *resizer;
    struct ChatTab *node;
    const char *tooltips[3];
    int place[3][2];
    int bar_y, key, i, cursor_key;
    int x, y;

    if (height <= self->bar_height) {
        SIZE_STEP(self) = 0;
    } else {
        int step = (height - (SIZE_STEP(self) + self->tab_height + 0x4a)) / CHAT_CREATE_STEP_PIXELS + 1;

        SIZE_STEP(self) = step > CHAT_CREATE_MAX_STEP ? CHAT_CREATE_MAX_STEP : step;
    }
    self->tab_width = 0;
    self->edit_icon_x = width - 0x14;

    tabs = mem_alloc(sizeof(struct UITabControl));
    if (tabs)
        tabs = ui_tab_control_construct(tabs);
    self->tabs = tabs;
    ui_tab_control_setup(tabs, UI_TABS_TOP, self->tab_height, CHAT_TAB_PADDING, 1, 1, 1, 0, 0, CHAT_LINE_WHITE,
                       CHAT_LINE_GREY, CHAT_LINE_WHITE, CHAT_LINE_WHITE);
    ui_window_move(self->tabs, 3, 0);
    ui_window_add_child(wnd, self->tabs);
    CHAT_TABS(self)->selected = 0;

    self->stick = title_button(BASIC_INTERFACE "stickoff");
    ui_window_move_base(&self->stick->button.base, self->tab_width * 2 + 4, 4);
    self->stick->button.base.id = CHAT_ID_STICK;
    ui_window_add_child(wnd, &self->stick->button.base);

    self->battle_option = title_button(BASIC_INTERFACE "battle_option" "2");
    ui_window_move_base(&self->battle_option->button.base, self->tab_width * 2 + 0x13, 5);
    self->battle_option->button.base.id = CHAT_ID_BATTLE_OPTION;
    ui_window_add_child(wnd, &self->battle_option->button.base);

    self->tab_option = title_button(BASIC_INTERFACE "battle_option");
    ui_window_move_base(&self->tab_option->button.base, self->tab_width * 2 + 0x22, 5);
    self->tab_option->button.base.id = CHAT_ID_TAB_OPTION;
    ui_window_add_child(wnd, &self->tab_option->button.base);

    self->mini = title_button(BASIC_INTERFACE "wnd_mini");
    self->mini->button.base.id = CHAT_ID_MINI;
    ui_window_add_child(wnd, &self->mini->button.base);

    for (node = chat_tabs_first(&g_session1.chatWndTabs); node;
         node = chat_tabs_after(&g_session1.chatWndTabs, cursor_key)) {
        struct UIChatHisBox *box;

        cursor_key = node->key;
        ui_tab_control_add_tab(self->tabs, owned_text_cstr(&node->value.name));
        box = mem_alloc(sizeof(struct UIChatHisBox));
        if (box)
            box = ui_chat_his_box_construct(box);
        ui_list_box_create_window(&box->list, 0, 0, wnd->width - 8, 0x46, 0);
        ui_window_move_base((struct UIWindow *)box, OFF_SCREEN - wnd->width, OFF_SCREEN);
        ((struct UIWindow *)box)->id = CHAT_ID_BOX;
        ui_window_add_child(wnd, (struct UIWindow *)box);
        vec_push(&self->boxes, &box, sizeof box);
    }
    if (g_session1.chatWndTabs.size)
        self->tab_width = (unsigned)self->tabs->width / g_session1.chatWndTabs.size;
    key = 0;
    for (node = chat_tabs_first(&g_session1.subChatWnds); node;
         node = chat_tabs_after(&g_session1.subChatWnds, cursor_key)) {
        cursor_key = node->key;
        ui_window_mgr_open_sub_chat(&g_windowMgr, key++, node->value.x, node->value.y, node->value.width,
                                node->value.height);
    }

    edit = mem_alloc(sizeof(struct UIChatEditCtrl));
    if (edit)
        edit = ui_chat_edit_ctrl_construct(edit);
    self->edit = edit;
    ui_window_create(&edit->edit.base, 0x1cc, 0x10);
    ui_window_move_base(&self->edit->edit.base, 0x6e, wnd->height - self->bar_height + 4);
    self->edit->edit.max_chars = 0xff;
    ui_edit_ctrl_set_back_colour(&self->edit->edit.base, 0xf0, 0xf0, 0xf0);
    ui_window_add_child(wnd, &self->edit->edit.base);
    self->focus_edit = &self->edit->edit.base;
    ui_window_mgr_set_focus_edit(&g_windowMgr, &self->edit->edit.base);
    self->edit->edit.base.id = CHAT_ID_BOX;

    whisper = mem_alloc(sizeof(struct UIWhisperEditCtrl));
    if (whisper)
        whisper = ui_whisper_edit_ctrl_construct(whisper);
    self->whisper = whisper;
    ui_window_create(&whisper->edit.base, 0x5a, 0x10);
    ui_window_move_base(&self->whisper->edit.base, 6, wnd->height - self->bar_height + 4);
    self->whisper->edit.max_chars = 0x18;
    ui_edit_ctrl_set_back_colour(&self->whisper->edit.base, 0xf0, 0xf0, 0xf0);
    ui_window_add_child(wnd, &self->whisper->edit.base);
    self->frame.default_id = 0xb0;

    resizer = mem_alloc(sizeof(struct UIResizer));
    if (resizer)
        resizer = ui_resizer_construct(resizer);
    self->resizer = resizer;
    ui_resizer_set_bitmap(resizer, BASIC_INTERFACE "dialog_mid.bmp");
    ui_window_create(&self->resizer->base, self->resizer->base.width, self->resizer->base.height);
    ui_resizer_set_edge(self->resizer, UI_EDGE_LEFT);
    ui_window_move_base(&self->resizer->base, 2, wnd->height - self->resizer->base.height);
    ui_window_add_child(wnd, &self->resizer->base);

    bar_y = wnd->height - self->bar_height;
    place[0][0] = 0x61;
    place[0][1] = bar_y + 3;
    place[1][0] = 0x24a;
    place[1][1] = bar_y + 7;
    place[2][0] = 0x23c;
    place[2][1] = bar_y + 7;
    tooltips[0] = msg_str(MSG_CHAT_BAR_DIALOG);
    tooltips[1] = msg_str(MSG_CHAT_BAR_SYS_1);
    tooltips[2] = msg_str(MSG_CHAT_BAR_SYS_2);
    for (i = 0; i < 3; i++) {
        struct UIBitmapButton *button = mem_alloc(sizeof(struct UIBitmapButton));
        int state;

        if (button)
            button = ui_bitmap_button_construct(button);
        self->bar_buttons[i] = button;
        for (state = UI_BTN_UP; state <= UI_BTN_DOWN; state++) {
            struct String base, file;

            string_init(&base);
            string_assign_cstr(&base, BASIC_INTERFACE);
            string_copy(&file, &base);
            string_append_cstr(&file, bar_bitmaps[i][state]);
            string_append_cstr(&file, ".bmp");
            ui_bitmap_button_set_bitmap(self->bar_buttons[i], string_cstr(&file), state, 0);
            string_free(&file);
            string_free(&base);
        }
        ui_window_create(&self->bar_buttons[i]->button.base, self->bar_buttons[i]->bitmap_width,
                         self->bar_buttons[i]->bitmap_height);
        ui_window_move_base(&self->bar_buttons[i]->button.base, place[i][0], place[i][1]);
        self->bar_buttons[i]->button.base.id = bar_ids[i];
        ui_bitmap_button_set_tool_tip(self->bar_buttons[i], tooltips[i]);
        ui_window_add_child(wnd, &self->bar_buttons[i]->button.base);
    }

    grip = mem_alloc(sizeof(struct UIResizer));
    if (grip)
        grip = &ui_resizer_construct((struct UIResizer *)grip)->base;
    ui_window_create(grip, wnd->width - 0x1a, 6);
    ui_resizer_set_edge((struct UIResizer *)grip, UI_EDGE_TOP);
    ui_window_move_base(grip, 3, self->tab_height);
    ui_resizer_set_colours((struct UIResizer *)grip, CHAT_COLOUR_KEY, CHAT_COLOUR_KEY);
    ui_window_add_child(wnd, grip);

    x = 0;
    y = 0;
    ui_window_to_screen(wnd, &x, &y);
    self->backdrop.x = x + 3;
    self->backdrop.argb = CHAT_BACK_ARGB;
    self->backdrop.y = y + self->tab_height;
    self->backdrop.width = wnd->width - 6;
    self->backdrop.edge_argb = 0;
    self->backdrop.draw_edge = 0;
    self->backdrop.height = wnd->height - self->tab_height - self->bar_height;
    ui_window_mgr_no_op(&g_windowMgr);
    if (!g_windowMgr.chat_bar_toggleable) {
        g_windowMgr.chat_bar_shown = 1;
        ui_new_chat_wnd_place_bar(self);
        ui_window_mgr_set_focus_edit(&g_windowMgr, self->focus_edit);
    }
}

void ui_new_chat_wnd_on_draw(struct UIWindow *window)
{
    struct UINewChatWnd *self = (struct UINewChatWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct BitmapRes *bitmap;

    ui_window_clear_canvas(wnd, CHAT_COLOUR_KEY);
    ui_window_fill_rect(wnd, 4, wnd->height - self->bar_height, wnd->width - 8, 1, CHAT_LINE_GREY);
    if (CHAT_LINES_SHOWN && wnd->height > self->bar_height + self->tab_height) {
        int tabs_end = (int)VEC_COUNT(&CHAT_TABS(self)->names, struct String) * self->tab_width;
        int th = self->tab_height;

        ui_window_fill_rect(wnd, 3, th, 1, wnd->height - th, CHAT_LINE_WHITE);
        ui_window_fill_rect(wnd, tabs_end + 3, th, wnd->width - tabs_end - 10, 1, CHAT_LINE_WHITE);
        ui_window_fill_rect(wnd, wnd->width - 6, th, 1, 1, CHAT_LINE_GREY);
        ui_window_fill_rect(wnd, wnd->width - 5, th + 1, 1, 1, CHAT_LINE_GREY);
        ui_window_fill_rect(wnd, wnd->width - 4, th + 2, 1, wnd->height - th - 2, CHAT_LINE_GREY);
    }
    if (g_windowMgr.chat_bar_shown) {
        bitmap = ui_skin_bitmap(BASIC_INTERFACE "dialog_bg.bmp");
        ui_window_draw_bitmap(wnd, 0, wnd->height - bitmap->height, bitmap, 0);
    }
    if (SIZE_STEP(self) <= 0)
        return;
    if (g_windowMgr.chat_bar_toggleable) {
        bitmap = ui_skin_bitmap(g_windowMgr.chat_bar_shown ? BASIC_INTERFACE "chatmode_on.bmp" : BASIC_INTERFACE "chatmode_off.bmp");
        if (bitmap) {
            self->bar_icon_x = self->battle_option->button.base.x - bitmap->width - 2;
            ui_window_draw_bitmap(wnd, self->bar_icon_x, 4, bitmap, 1);
        }
    }
    bitmap = ui_skin_bitmap(CHAT_TABS_EDITABLE ? BASIC_INTERFACE "unlock_dragwnd.bmp"
                                               : BASIC_INTERFACE "lock_dragwnd.bmp");
    if (bitmap)
        ui_window_draw_bitmap(wnd, self->edit_icon_x, 4, bitmap, 1);
}

void ui_new_chat_wnd_show_tab(struct UINewChatWnd *self, BYTE place_boxes)
{
    struct UIWindow *options;
    int i;

    if (place_boxes && SIZE_STEP(self)) {
        for (i = 0; i < CHAT_BOX_COUNT(self); i++) {
            if (self->tab == i) {
                ui_window_move((CHAT_BOX(self, i)), 5, self->tab_height + 2);
                ui_window_invalidate((CHAT_BOX(self, i)));
            } else {
                ui_window_move((CHAT_BOX(self, i)), OFF_SCREEN - self->frame.base.width, OFF_SCREEN);
            }
        }
    }
    options = ui_window_find(WND_ID_BATTLE_MSG_OPTION);
    if (options)
        ui_window_send_msg(options, &self->frame.base, UI_MSG_COMMAND, BATTLE_OPTION_FOR_TAB, self->tab, 1, 2);
    ui_window_invalidate(self->tabs);
    ui_window_invalidate_base(&self->frame.base);
}

void ui_new_chat_wnd_on_enter(struct UINewChatWnd *self)
{
    if (g_windowMgr.chat_bar_shown && g_windowMgr.chat_bar_toggleable) {
        if (!*ui_edit_ctrl_get_text(&self->edit->edit.base)) {
            g_windowMgr.chat_bar_shown = 0;
            ui_new_chat_wnd_place_bar(self);
            ui_window_mgr_set_active_top_window(&g_windowMgr, NULL);
        }
    } else {
        g_windowMgr.chat_bar_shown = 1;
        ui_new_chat_wnd_place_bar(self);
        ui_window_mgr_set_focus_edit(&g_windowMgr, self->focus_edit);
        ui_edit_ctrl_set_text_default((struct UIEditCtrl *)self->focus_edit, "");
        if (!g_windowMgr.chat_bar_toggleable)
            ui_new_chat_wnd_set_size_step(self, SIZE_STEP(self));
        ui_window_mgr_set_active_top_window(&g_windowMgr, self->focus_edit);
    }
    if (self->edit->popup)
        ui_item_tag_ctrl_clear_tags((struct UIItemTagCtrl *)self->edit->popup);
}

struct UIWindow *ui_chat_window_edit(struct UIWindow *chat)
{
    return (struct UIWindow *)((struct UINewChatWnd *)chat)->edit;
}

struct UIWindow *ui_chat_window_whisper_edit(struct UIWindow *chat)
{
    return (struct UIWindow *)((struct UINewChatWnd *)chat)->whisper;
}

struct UIWindow *ui_chat_window_focus_edit(struct UIWindow *chat)
{
    return ((struct UINewChatWnd *)chat)->focus_edit;
}

struct UIWindow *ui_chat_window_tabs(struct UIWindow *chat)
{
    return ((struct UINewChatWnd *)chat)->tabs;
}
