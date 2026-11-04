#include "world/tooltip.h"
#include "render/rp_pool.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "core/object_type_tables.h"
#include "core/object_type.h"
#include "core/memory.h"
#include "game/context.h"
#include "ui/window_access.h"
#include "ui/social/party_windows.h"
#include "core/msg_str.h"
#include "diag/perf.h"
#include "session/csession.h"
#include "session/session_items.h"
#include "session/session_state.h"
#include "ui/character/gauge.h"
#include "ui/wndmgr/wndmgr.h"
#include "platform/clock.h"

struct UIMessengerGroupWnd;
struct UIWindow;
void ui_messenger_group_wnd_destroy(struct UIWindow *window);
void ui_messenger_group_wnd_store_info(struct UIWindow *self);
void ui_messenger_group_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_messenger_group_wnd_on_l_btn_down(struct UIWindow *self, int x, int y);
void ui_messenger_group_wnd_on_l_btn_dbl_clk(struct UIWindow *self, int x, int y);
void ui_messenger_group_wnd_on_mouse_move(struct UIWindow *self, int x, int y);
void ui_messenger_group_wnd_on_mouse_hover(struct UIWindow *self, int x, int y);
void ui_messenger_group_wnd_on_l_btn_up(struct UIWindow *self, int x, int y);
void ui_messenger_group_wnd_on_r_btn_up(struct UIWindow *self, int x, int y);
void ui_messenger_group_wnd_on_wheel(struct UIWindow *self, int delta);
const struct UIWindowOps ui_messenger_group_wnd_ops = {
    .destroy = ui_messenger_group_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_messenger_group_wnd_store_info,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_messenger_group_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_messenger_group_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_messenger_group_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_messenger_group_wnd_on_l_btn_dbl_clk,
    .on_mouse_move = ui_messenger_group_wnd_on_mouse_move,
    .on_mouse_hover = ui_messenger_group_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_messenger_group_wnd_on_l_btn_up,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_messenger_group_wnd_on_r_btn_up,
    .on_wheel = ui_messenger_group_wnd_on_wheel,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_messenger_group_wnd_send_msg,
    .invalidate = ui_window_invalidate_base,
    .render = ui_window_render_base,
    .has_custom_backdrop = 0,
    .draw_backdrop = ui_window_draw_backdrop_base,
    .type = UI_TYPE_FRAME,
    .on_hover_enter = ui_window_on_hover_enter_base,
    .on_hover_leave = ui_window_on_hover_leave_base,
    .make_quad = ui_window_make_quad_base,
    .hit_test = ui_window_hit_test_base,
    .render_overlay = ui_window_render_overlay_base,
};

struct UIMessengerGroupWnd *ui_messenger_group_wnd_construct(struct UIMessengerGroupWnd *self)
{
    int i;

    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_messenger_group_wnd_ops;
    list_init(&self->rows);
    self->menu.first = self->menu.last = self->menu.end = NULL;
    self->layouts.first = self->layouts.last = self->layouts.end = NULL;
    self->info = NULL;
    self->scrollbar = NULL;
    self->first = 0;
    self->cols = 1;
    self->visible = 5;
    self->scrollable = 0;
    self->row_height = 0x17;
    self->max_button = NULL;
    self->mini_button = NULL;
    self->mode = 0;
    for (i = 0; i < PARTY_WND_BUTTONS; i++)
        self->party_hides[i] = 0;
    return self;
}

struct UIWindow *ui_messenger_group_wnd_new(void)
{
    return (struct UIWindow *)ui_messenger_group_wnd_construct(mem_alloc(sizeof(struct UIMessengerGroupWnd)));
}

static void destroy_member(void *value)
{
    party_member_destroy(value);
}

void ui_messenger_group_wnd_destruct(struct UIMessengerGroupWnd *self)
{
    self->frame.base.ops = &ui_messenger_group_wnd_ops;
    vec_free(&self->layouts);
    ui_menu_entry_vec_free(&self->menu);
    list_free(&self->rows, destroy_member);
    ui_frame_wnd_destruct(&self->frame);
}

void ui_messenger_group_wnd_destroy(struct UIWindow *window)
{
    struct UIMessengerGroupWnd *self = (struct UIMessengerGroupWnd *)window;
    ui_messenger_group_wnd_destruct(self);
    mem_free(self);
}

static struct PartyMember *member_none(struct PartyMember *out)
{
    out->found = 0;
    string_init(&out->name);
    string_init(&out->map_name);
    out->dead = 0;
    return out;
}

struct PartyMember *ui_messenger_group_wnd_empty_member(struct PartyMember *out)
{
    return member_none(out);
}

struct PartyMember *ui_messenger_group_wnd_get_member(struct UIMessengerGroupWnd *self,
                                                                 struct PartyMember *out, int index)
{
    struct ListNode *node;
    int i = 0;

    for (node = self->rows.head->next; node != self->rows.head; node = node->next, i++)
        if (i == index)
            return party_member_info_copy(out, LIST_VALUE(node, struct PartyMember));
    return member_none(out);
}

struct PartyMember *ui_messenger_group_wnd_member_at(struct UIMessengerGroupWnd *self,
                                                                struct PartyMember *out, int x, int y)
{
    struct ListNode *node;
    int row, i = 0;

    self->hover = -1;
    if (x < 2 || x > self->frame.base.width - 2)
        return member_none(out);
    if (y < 0x11 || y >= self->visible * self->row_height + 0x11)
        return ui_messenger_group_wnd_empty_member(out);
    row = (y - 0x11) / self->row_height + self->first;
    self->hover = row;
    for (node = self->rows.head->next; node != self->rows.head; node = node->next, i++)
        if (i == row)
            return party_member_info_copy(out, LIST_VALUE(node, struct PartyMember));
    self->hover = -1;
    return member_none(out);
}

void ui_messenger_group_wnd_create_plate(int cell,
                                                const struct PartyMember *member)
{
    struct UIMiniPartyWnd *plate = mem_alloc(sizeof(struct UIMiniPartyWnd));

    plate = plate ? ui_mini_party_wnd_construct(plate) : NULL;
    ui_window_create(&plate->frame.base, 0xae, 0x34);
    plate->cell = cell;
    ui_window_mgr_add_plate(&g_windowMgr, plate);
    ui_mini_party_wnd_send_msg(&plate->frame.base, NULL, PARTY_MSG_REFRESH, (intptr_t)member, 0, 0, 0);
    session_add_plate_record(&g_session1, member->aid, cell);
}

void ui_messenger_group_wnd_drop_plate(struct UIMessengerGroupWnd *self, int x, int y, int row)
{
    struct UIWindow *wnd = &self->frame.base;
    struct PartyMember m;
    struct PlateRecord rec;
    struct UIWindow *anchor;
    int top, cell;

    ui_window_mgr_close_drag_plate(&g_windowMgr);
    if (x > wnd->x && x < wnd->x + wnd->width && y > wnd->y && y < wnd->y + wnd->height)
        return;
    ui_messenger_group_wnd_get_member(self, &m, row);
    session_get_plate_record(&g_session1, &rec, m.aid);
    anchor = ui_window_find(0x133);
    if (rec.found != 1) {
        top = (int)((float)g_renderer->height * 0.3f);
        if (anchor)
            top = anchor->y + anchor->height;
        if (x < g_windowMgr.plate_width + 0x17 && y > top) {
            cell = (y - top) / (g_windowMgr.plate_height + 3);
            if (ui_window_mgr_find_by_key(&g_windowMgr, cell))
                cell = ui_window_mgr_find_free_plate_cell(&g_windowMgr);
        } else {
            cell = ui_window_mgr_find_free_plate_cell(&g_windowMgr);
        }
        if (x < 0 || x < g_windowMgr.screen_width || y < 0 || y < g_windowMgr.screen_height)
            cell = ui_window_mgr_find_free_plate_cell(&g_windowMgr);
        ui_messenger_group_wnd_create_plate(cell, &m);
    }
    party_member_destroy(&m);
}

struct UIWindow *party_wnd_docked(struct UIMessengerGroupWnd *self)
{
    if (self->tab == 0)
        return g_windowMgr.friend_option_wnd;
    return g_windowMgr.party_setting_wnd ? g_windowMgr.party_setting_wnd : g_windowMgr.party_invitation_to_wnd;
}

void party_wnd_dock(struct UIMessengerGroupWnd *self, struct UIWindow *wnd)
{
    int x = 0, y = 0;

    ui_window_to_screen(&self->frame.base, &x, &y);
    ui_window_move(wnd, x + 2 + self->frame.base.width, y);
}

void party_wnd_title(struct UIMessengerGroupWnd *self, char *buf)
{
    const char *labels[2];

    labels[0] = msg_str(0x66);
    labels[1] = msg_str(0x67);
    if (self->tab == 1) {
        if (g_session1.partyName.size)
            sprintf(buf, "%s(%s)", labels[1], string_cstr(&g_session1.partyName));
        else
            sprintf(buf, "%s", labels[1]);
    } else {
        sprintf(buf, "%s(%d/%d)", labels[self->tab], self->rows.size, PARTY_WND_ROWS);
    }
}

static const int button_ids[PARTY_WND_BUTTONS] = {
    0x218, 0xe0, 0xe1, 0xe2, 0xe3, 0x171, 0x171, 0x1b5, 0x1b6, 0x1b7, 0x164,
};
static const BYTE button_disabled[PARTY_WND_BUTTONS] = { 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0 };
static const BYTE button_party_hides[PARTY_WND_BUTTONS] = { 0, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0 };
static const int friend_tips[PARTY_WND_BUTTONS - 1] = {
    0x62, 0x63, 0x64, 0x163, 0x15f, 0x65a, 0x65b, 0x806, 0x807, 0x808,
};
static const int party_tips[PARTY_WND_BUTTONS] = {
    0xdba, 0x63, 0x64, 0x65, 0x61, 0x65a, 0x65b, 0x806, 0x807, 0x808, 0xf8f,
};

static void button_bitmap(struct UIBitmapButton *button, const struct String *base, const char *number,
                          const char *suffix, int state)
{
    struct String path;

    string_init(&path);
    string_reserve(&path, strlen(number) + base->size);
    string_append(&path, string_cstr(base), base->size);
    string_append_cstr(&path, number);
    string_append_cstr(&path, suffix);
    ui_bitmap_button_set_bitmap(button, string_cstr(&path), state, 0);
    string_free(&path);
}

void ui_messenger_group_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UIMessengerGroupWnd *self = (struct UIMessengerGroupWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    const char *labels[2], *tips[2][PARTY_WND_BUTTONS];
    int widths[2], checked[2] = { 0, 1 };
    char title[0x40], number[0x10];
    struct String base;
    int i, x;

    party_layouts_init(&self->layouts);
    self->mini_button = ui_window_add_mini_button(wnd, "", 0);

    self->max_button = mem_alloc(sizeof(struct UIBitmapButton));
    self->max_button = self->max_button ? ui_bitmap_button_construct(self->max_button) : NULL;
    ui_bitmap_button_set_bitmap(self->max_button, RENEWAL_PARTY "btn_titlebar_maximum_out.bmp", 0, 0);
    ui_bitmap_button_set_bitmap(self->max_button, RENEWAL_PARTY "btn_titlebar_maximum_over.bmp", 1, 0);
    ui_bitmap_button_set_bitmap(self->max_button, RENEWAL_PARTY "btn_titlebar_maximum_press.bmp", 2, 0);
    ui_window_create(&self->max_button->button.base, self->max_button->bitmap_width,
                     self->max_button->bitmap_height);
    ui_window_move_base(&self->max_button->button.base, wnd->width - 0x1c, 3);
    self->max_button->button.base.id = 0x203;
    ui_window_add_child(wnd, &self->max_button->button.base);

    for (i = 0; i < PARTY_WND_ROWS; i++) {
        self->gauges[i] = ui_pc_gage_new();
        ui_window_create(&self->gauges[i]->gage.base, 0x3c, 5);
        ui_window_move_base(&self->gauges[i]->gage.base, -100, -100);
        ui_window_add_child(wnd, &self->gauges[i]->gage.base);
    }
    for (i = 0; i < PARTY_WND_ROWS; i++) {
        struct UIJobIconButton *icon = mem_alloc(sizeof *icon);

        self->job_icons[i] = icon ? ui_job_icon_button_construct(icon) : NULL;
        ui_window_create(&self->job_icons[i]->bmp.button.base, 0x32, 0x32);
        ui_bitmap_button_set_bitmap(&self->job_icons[i]->bmp, RENEWAL_PARTY "icon_jobs_4046.bmp", 0, 0);
        ui_bitmap_button_set_bitmap(&self->job_icons[i]->bmp, RENEWAL_PARTY "icon_jobs_4046.bmp", 1, 0);
        ui_bitmap_button_set_bitmap(&self->job_icons[i]->bmp, RENEWAL_PARTY "icon_jobs_4046.bmp", 2, 0);
        ui_window_add_child(wnd, &self->job_icons[i]->bmp.button.base);
    }

    self->scrollbar = mem_alloc(sizeof(struct UIScrollBar));
    self->scrollbar = self->scrollbar ? ui_scroll_bar_construct(self->scrollbar) : NULL;
    ui_scroll_bar_create(self->scrollbar, wnd->width - 0xf, 0x11, 1, wnd->height - 0x3a);
    ui_window_add_child(wnd, (struct UIWindow *)self->scrollbar);

    self->tab = 1;
    labels[0] = msg_str(0x66);
    labels[1] = msg_str(0x67);
    widths[0] = 0x30;
    widths[1] = ui_window_text_width(wnd, msg_str(0x67), 0, 0, 0xc, 0, 0) + 0xd;
    for (i = 0, x = 4; x < 0x7c; i++, x += 0x3c) {
        struct UIRadioBtn *radio = mem_alloc(sizeof(struct UIRadioBtn));

        self->tabs[i] = radio = radio ? ui_radio_btn_construct(radio) : NULL;
        ui_window_create(&radio->base, widths[i], 0xc);
        ui_window_move_base(&radio->base, x, wnd->height - 0x10);
        ui_radio_btn_set_label(&radio->base, labels[i]);
        ui_radio_btn_send_msg(&radio->base, wnd, PARTY_MSG_SET_STATE, checked[i], 0, 0, 0);
        ui_window_add_child(wnd, &radio->base);
    }

    party_wnd_title(self, title);
    self->title = mem_alloc(sizeof(struct UIStaticText));
    self->title = self->title ? ui_static_text_construct(self->title) : NULL;
    ui_window_create(self->title, wnd->width - 0x39, 0xf);
    ui_static_text_set_text_ellipsis(self->title, title);
    ui_window_move(self->title, 0x10, 2);
    ((struct UIStaticText *)self->title)->white_shadow = 1;
    ui_window_add_child(wnd, self->title);
    self->close_button = ui_window_add_close_button(wnd, "Alt+Z", 0);

    string_init(&base);
    string_assign(&base, UI_SKIN "basic_interface\\mesbtn_0", 0x27);
    for (i = 0; i < PARTY_WND_BUTTONS - 1; i++)
        tips[0][i] = msg_str(friend_tips[i]);
    tips[0][PARTY_WND_BUTTONS - 1] = title;
    for (i = 0; i < PARTY_WND_BUTTONS; i++)
        tips[1][i] = msg_str(party_tips[i]);
    for (i = 0; i < PARTY_WND_BUTTONS; i++)
        self->party_hides[i] = button_party_hides[i];
    for (i = 0; i < PARTY_WND_BUTTONS; i++) {
        struct UIBitmapButton *button;

        snprintf(number, sizeof number, "%d", i + 1);
        button = mem_alloc(sizeof(struct UIBitmapButton));
        self->buttons[i] = button = button ? ui_bitmap_button_construct(button) : NULL;
        if (i == 0 || i == 2) {
            button_bitmap(button, &base, number, "_disable.bmp", 0);
            button_bitmap(button, &base, number, "_disable.bmp", 1);
            button_bitmap(button, &base, number, "_disable.bmp", 2);
        } else {
            button_bitmap(button, &base, number, ".bmp", 0);
            button_bitmap(button, &base, number, "_a.bmp", 1);
            button_bitmap(button, &base, number, "_b.bmp", 2);
        }
        ui_window_create(&button->button.base, button->bitmap_width, button->bitmap_height);
        ui_window_move_base(&button->button.base, 5 + 0x1a * i, wnd->height - 0x28);
        button->button.base.id = button_ids[i];
        ui_bitmap_button_set_tool_tip(button, tips[self->tab != 0][i]);
        button->button.disabled = button_disabled[i];
        ui_window_invalidate_base(&button->button.base);
        ui_window_add_child(wnd, &button->button.base);
    }
    string_free(&base);

    self->resizer = mem_alloc(sizeof(struct UIResizer));
    self->resizer = self->resizer ? ui_resizer_construct(self->resizer) : NULL;
    ui_window_create((struct UIWindow *)self->resizer, 0x10, 0x10);
    ui_resizer_set_edge(self->resizer, 7);
    ui_resizer_set_bitmap(self->resizer, UI_SKIN "btn_resize.bmp");
    ui_window_move_base(&self->resizer->base, wnd->width - 0xf, wnd->height - 0xf);
    ui_resizer_set_colours(self->resizer, 0xffff00ff, 0xffff00ff);
    ui_window_add_child(wnd, (struct UIWindow *)self->resizer);

    ui_messenger_group_wnd_send_msg(wnd, NULL, PARTY_MSG_REFRESH, 0, 0, 0, 0);
}

void ui_messenger_group_wnd_on_l_btn_dbl_clk(struct UIWindow *window, int x, int y)
{
    struct UIMessengerGroupWnd *self = (struct UIMessengerGroupWnd *)window;
    if (!g_session1.lockMouse)
        ui_messenger_group_wnd_send_msg(&self->frame.base, NULL, UI_MSG_COMMAND, 0xe0, 0, 0, 0);
}

void ui_messenger_group_wnd_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIMessengerGroupWnd *self = (struct UIMessengerGroupWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct PartyMember m, m2;
    struct UIDragMiniPartyWnd *plate;
    struct ListNode *node;
    struct GameMode *mode;
    int old, i;

    if (y < 0x11) {
        struct UIWindow *docked;

        ui_frame_wnd_on_l_btn_down((struct UIWindow *)(&self->frame), x, y);
        docked = party_wnd_docked(self);
        if (docked) {
            docked->alpha_target = 0x80;
            docked->alpha_time = platform_ticks_ms();
        }
        return;
    }
    old = self->hover;
    ui_messenger_group_wnd_member_at(self, &m, x, y);
    if (old != self->hover)
        ui_window_invalidate_base(wnd);
    if (!m.found || m.state != 0)
        goto out;
    mode = game_current_mode();
    if (!mode)
        goto out;
    game_mode_click_actor(mode, m.aid);
    ui_messenger_group_wnd_get_member(self, &m2, self->hover);
    if (self->tab == 1 && g_session1.lockMouse != 1
        && ui_window_mgr_has_plate(&g_windowMgr, m2.aid) != 1) {
        game_mode_set_drag_source(mode, 0x1d, self->hover);
        if (!g_windowMgr.drag_plate) {
            plate = mem_try_alloc(sizeof(struct UIDragMiniPartyWnd));
            if (plate) {
                struct PartyMember arg;

                party_member_info_copy(&arg, &m2);
                plate = ui_drag_mini_party_wnd_construct(plate, arg);
            }
            ui_window_create(&plate->frame.base, wnd->width, self->row_height);
            ui_frame_wnd_move(&plate->frame.base, -1000, -1000);
            ui_window_mgr_set_drag_plate(&g_windowMgr, &plate->frame.base);
            plate->row = self->hover;
            plate->layout = self->mode;
            ui_drag_mini_party_wnd_move_gauge(plate, 0x2f, (int)((float)self->row_height * 0.76f));
            ui_drag_mini_party_wnd_set_grab(plate, x,
                                       y - ((self->hover - self->first * self->cols) / self->cols * self->row_height
                                            + 0x11));
            i = 0;
            for (node = self->rows.head->next; node != self->rows.head; node = node->next, i++) {
                const struct PartyMember *member = LIST_VALUE(node, struct PartyMember);
                void *actor;

                if (i < self->first * self->cols || !member->found || member->aid != m2.aid)
                    continue;
                if (member->aid == g_session1.ownAid)
                    actor = game_current_player();
                else
                    actor = game_find_actor(member->aid);
                if (actor)
                    ui_drag_mini_party_wnd_set_hp(plate, self->gauges[i]->hp, self->gauges[i]->max_hp);
                else
                    ui_drag_mini_party_wnd_remove_gauge(plate);
                break;
            }
        }
    }
    party_member_destroy(&m2);
out:
    party_member_destroy(&m);
}

void ui_messenger_group_wnd_on_l_btn_up(struct UIWindow *window, int x, int y)
{
    struct UIMessengerGroupWnd *self = (struct UIMessengerGroupWnd *)window;
    struct UIWindow *docked;

    ui_frame_wnd_on_l_btn_up((struct UIWindow *)(&self->frame), x, y);
    docked = party_wnd_docked(self);
    if (docked) {
        docked->alpha_target = 0xff;
        docked->alpha_time = platform_ticks_ms();
    }
    ui_window_mgr_close_drag_plate(&g_windowMgr);
}

void ui_messenger_group_wnd_on_mouse_move(struct UIWindow *window, int x, int y)
{
    struct UIMessengerGroupWnd *self = (struct UIMessengerGroupWnd *)window;
    struct UIWindow *docked;

    ui_frame_wnd_on_mouse_move((struct UIWindow *)(&self->frame), x, y);
    docked = party_wnd_docked(self);
    if (docked)
        party_wnd_dock(self, docked);
}

void ui_messenger_group_wnd_on_mouse_hover(struct UIWindow *window, int x, int y)
{
    struct UIMessengerGroupWnd *self = (struct UIMessengerGroupWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct String map;
    struct String fitted = {0};
    struct PartyMember m;
    char text[0x100], level[0x40];
    struct Mode *mode;
    int old, tx, ty;

    if (y < 0x11)
        ui_frame_wnd_on_mouse_hover((struct UIWindow *)(&self->frame), x, y);
    if (self->tab == 0)
        return;
    mode = game_current_base_mode();
    if (!mode)
        return;
    old = self->hover;
    ui_messenger_group_wnd_member_at(self, &m, x, y);
    if (old != self->hover)
        ui_window_invalidate_base(wnd);
    if (m.found) {
        memset(text, 0, sizeof(text));
        if (m.map_name.size && m.state == 0) {
            string_init(&map);
            string_assign_cstr(&map,
                               session_map_display_name(string_cstr(&m.map_name)));
            sprintf(text, "%s(%s)", string_cstr(&m.name), string_cstr(&map));
            string_free(&map);
        } else {
            sprintf(text, "%s", string_cstr(&m.name));
        }
        ui_window_fit_text(wnd, &fitted, (int)((double)wnd->width * 0.8 - 80.0), text, 0, 0xc, 1);
        if (string_find(&fitted, text, 0) == STRING_NPOS) {
            sprintf(level, "Lv.%d", m.level);
            tx = PARTY_LAYOUT(&self->layouts, self->mode).x + 0x21
                 + ui_window_text_width(wnd, level, (int)strlen(level), 0, 0xc, 1, 0);
            ty = self->hover / self->cols * self->row_height + 0x16;
            ui_window_to_screen(wnd, &tx, &ty);
            mode_show_tooltip(mode, text, tx, ty, 0xffffffff, 0);
        }
        string_free(&fitted);
    }
    party_member_destroy(&m);
}

void ui_messenger_group_wnd_on_r_btn_up(struct UIWindow *window, int x, int y)
{
    struct UIMessengerGroupWnd *self = (struct UIMessengerGroupWnd *)window;
    struct PartyMember m;
    int old;

    if (g_session1.lockMouse)
        return;
    old = self->hover;
    ui_messenger_group_wnd_member_at(self, &m, x, y);
    if (old != self->hover)
        ui_window_invalidate_base(&self->frame.base);
    if (self->hover != -1)
        ui_messenger_group_wnd_send_msg(&self->frame.base, NULL, PARTY_MSG_OPEN_MENU, 0, 0, 0, 0);
    party_member_destroy(&m);
}

void ui_messenger_group_wnd_on_wheel(struct UIWindow *window, int delta)
{
    struct UIMessengerGroupWnd *self = (struct UIMessengerGroupWnd *)window;
    int step;

    if (!self->scrollable)
        return;
    step = self->visible > 3 ? 3 : self->visible - 1;
    if (delta >= 0)
        step = -step;
    ui_messenger_group_wnd_send_msg(&self->frame.base, (struct UIWindow *)self->scrollbar, PARTY_MSG_SCROLL, step, 0, 0, 0);
}

void ui_messenger_group_wnd_update_scroll(struct UIMessengerGroupWnd *self)
{
    int rows, max;

    if (!self->scrollbar)
        return;
    rows = (int)(self->rows.size / (unsigned)self->cols) + (self->rows.size % (unsigned)self->cols != 0);
    max = rows - self->visible;
    if (max < 0)
        max = 0;
    self->scrollable = max != 0;
    if (max) {
        ui_scroll_bar_set_range(self->scrollbar, max, self->visible, rows);
        if (self->first < 0)
            self->first = 0;
        if (self->first > rows - self->visible)
            self->first = rows - self->visible;
        ui_scroll_bar_set_pos(self->scrollbar, self->first);
    } else {
        self->first = 0;
        ui_scroll_bar_set_range(self->scrollbar, 0, self->visible, rows);
    }
    ui_window_invalidate_base(&self->scrollbar->base);
    ui_messenger_group_wnd_update_gauges(self);
}

void ui_messenger_group_wnd_layout_buttons(struct UIMessengerGroupWnd *self)
{
    struct UIWindow **b = (struct UIWindow **)self->buttons;
    int skipped = 0, shift = 0, i;

    for (i = 0; i < PARTY_WND_BUTTONS; i++) {
        if (self->tab == 1 && self->party_hides[i]) {
            ui_window_move(b[i], -100, -100);
            skipped++;
        } else {
            ui_window_move(b[i], (i - skipped) * 0x1a + 5, self->frame.base.height - 0x28);
        }
    }
    if (self->tab == 1) {
        shift = skipped * -0x1a;
        if (session_get_party_member_count(&g_session1)) {
            ui_window_move(b[10], shift + 0xd5, self->frame.base.height - 0x28);
            ui_window_move(b[9], shift + 0xa1, self->frame.base.height - 0x28);
            ui_window_move(b[8], shift + 0xbb, self->frame.base.height - 0x28);
            ui_window_move(b[7], -100, -100);
        } else {
            ui_window_move(b[10], -100, -100);
            ui_window_move(b[7], shift + 0xa1, self->frame.base.height - 0x28);
            ui_window_move(b[8], -100, -100);
            ui_window_move(b[9], -100, -100);
        }
    } else {
        ui_window_move(b[7], -100, -100);
        ui_window_move(b[8], -100, -100);
        ui_window_move(b[9], -100, -100);
        ui_window_move(b[10], -100, -100);
    }
    if (g_session1.lockMouse) {
        ui_window_move(b[5], shift + 0x87, self->frame.base.height - 0x28);
        ui_window_move(b[6], -100, -100);
    } else {
        ui_window_move(b[6], shift + 0x87, self->frame.base.height - 0x28);
        ui_window_move(b[5], -100, -100);
    }
}

void ui_messenger_group_wnd_update_gauges(struct UIMessengerGroupWnd *self)
{
    struct Mode *mode = game_current_base_mode();
    struct ListNode *node;
    int shown = 0, i = 0;

    if (!mode || self->tab == 0)
        return;
    for (node = self->rows.head->next; node != self->rows.head; node = node->next, i++) {
        const struct PartyMember *member = LIST_VALUE(node, struct PartyMember);
        struct UIPcGage *gauge = self->gauges[i];

        if (i < self->first * self->cols) {
            if (gauge->gage.base.x != -100)
                ui_window_move_base(&gauge->gage.base, -100, -100);
            continue;
        }
        if (member->found) {
            void *actor;
            int y;

            if (member->aid == g_session1.ownAid)
                actor = game_current_player();
            else
                actor = object_as_type(game_find_actor(member->aid), OBJECT_PC);
            y = shown / self->cols * self->row_height + 0x11;
            if (gauge->gage.base.y != y)
                ui_window_move_base(&gauge->gage.base, 0x2f, (int)((float)self->row_height * 0.76f + (float)y));
            if (actor && member->state == 0)
                ui_gage_set_back_colour(&gauge->gage, 0x444444);
            else if (gauge->gage.base.x != -100)
                ui_window_move_base(&gauge->gage.base, -100, -100);
        }
        shown++;
        if (shown > self->visible * self->cols && gauge->gage.base.x != -100)
            ui_window_move_base(&gauge->gage.base, -100, -100);
    }
}

void ui_messenger_group_wnd_store_info(struct UIWindow *window)
{
    struct UIMessengerGroupWnd *self = (struct UIMessengerGroupWnd *)window;
    struct UIWindow *wnd = &self->frame.base;

    if (!self->info)
        return;
    self->info->width = wnd->width;
    self->info->height = wnd->height;
    self->info->x = wnd->x;
    self->info->y = wnd->y;
    self->info->tab = self->tab;
    self->info->mode = self->mode;
}
