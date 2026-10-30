#include "render/rp_pool.h"
#include "ui/misc/snap_mgr.h"
#include "ui/window_manager_state.h"
#include "core/object_type_tables.h"
#include "ui/social/party_windows.h"
#include "core/msg_str.h"
#include "diag/perf.h"
#include "session/session_items.h"
#include "session/session_state.h"
#include "world/world.h"

struct UIMiniPartyWnd;
struct UIWindow;
void ui_mini_party_wnd_destroy(struct UIWindow *window);
void ui_mini_party_wnd_move(struct UIWindow *self, int x, int y);
void ui_mini_party_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_mini_party_wnd_on_draw(struct UIWindow *self);
void ui_mini_party_wnd_on_l_btn_down(struct UIWindow *self, int x, int y);
void ui_mini_party_wnd_on_l_btn_up(struct UIWindow *self, int x, int y);
int ui_mini_party_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_mini_party_wnd_ops = {
    .destroy = ui_mini_party_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_mini_party_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_window_store_info_base,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_mini_party_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_mini_party_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_mini_party_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_frame_wnd_on_mouse_move,
    .on_mouse_hover = ui_frame_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_mini_party_wnd_on_l_btn_up,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_mini_party_wnd_send_msg,
    .invalidate = ui_window_invalidate_base,
    .render = ui_window_render_base,
    .has_custom_backdrop = 0,
    .draw_backdrop = ui_window_draw_backdrop_base,
    .get_records = ui_frame_wnd_get_records,
    .type = UI_TYPE_FRAME,
    .on_hover_enter = ui_window_on_hover_enter_base,
    .on_hover_leave = ui_window_on_hover_leave_base,
    .make_quad = ui_window_make_quad_base,
    .hit_test = ui_window_hit_test_base,
    .render_overlay = ui_window_render_overlay_base,
};

#define MSG_NO_CELL_THERE 0xc49

struct UIMiniPartyWnd *ui_mini_party_wnd_construct(struct UIMiniPartyWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 1);
    self->frame.base.ops = &ui_mini_party_wnd_ops;
    self->member.found = 1;
    string_init(&self->member.name);
    string_init(&self->member.map_name);
    self->member.dead = 0;
    self->cell = -1;
    self->info = NULL;
    self->edge_button = NULL;
    self->mode = 0;
    self->widths.first = self->widths.last = self->widths.end = NULL;
    string_init(&self->background);
    string_assign(&self->background, PLATE_BG, 0x30);
    return self;
}

void ui_mini_party_wnd_destruct(struct UIMiniPartyWnd *self)
{
    self->frame.base.ops = &ui_mini_party_wnd_ops;
    string_free(&self->background);
    vec_free(&self->widths);
    party_member_destroy(&self->member);
    ui_frame_wnd_destruct(&self->frame);
}

void ui_mini_party_wnd_destroy(struct UIWindow *window)
{
    struct UIMiniPartyWnd *self = (struct UIMiniPartyWnd *)window;
    ui_mini_party_wnd_destruct(self);
    mem_free(self);
}

void ui_mini_party_wnd_move(struct UIWindow *window, int x, int y)
{
    struct UIMiniPartyWnd *self = (struct UIMiniPartyWnd *)window;
    self->frame.base.x = x;
    self->frame.base.y = y;
}

struct PartyMember *ui_mini_party_wnd_get_member(struct UIMiniPartyWnd *self, struct PartyMember *out)
{
    const struct PartyMember *m = &self->member;

    out->found = m->found;
    out->aid = m->aid;
    out->char_id = m->char_id;
    string_copy(&out->name, &m->name);
    string_copy(&out->map_name, &m->map_name);
    out->role = m->role;
    out->state = m->state;
    out->color = m->color;
    out->job = m->job;
    out->level = m->level;
    out->dead = m->dead;
    return out;
}

void ui_mini_party_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UIMiniPartyWnd *self = (struct UIMiniPartyWnd *)window;
    struct UIMiniPartyInfoWnd *info = mem_alloc(sizeof(struct UIMiniPartyInfoWnd));
    int *w;

    self->info = info ? ui_mini_party_info_wnd_construct(info) : NULL;
    self->info->owner = self;
    ui_window_create(&self->info->frame.base, 0xae, 0x34);
    ui_frame_wnd_move(&self->info->frame.base, 0, 0);
    ui_window_add_child(&self->frame.base, &self->info->frame.base);
    vec_resize(&self->widths, 3, sizeof(int));
    w = (int *)self->widths.first;
    w[0] = 0xae;
    w[1] = 0x65;
    w[2] = 0x40;
}

void ui_mini_party_wnd_on_draw(struct UIWindow *window)
{
    struct UIMiniPartyWnd *self = (struct UIMiniPartyWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    int i, count;

    list_clear(&wnd->draws, NULL);

    count = self->member.state != 1 ? 3 : 1;
    for (i = 0; i < count; i++) {
        struct String name;

        string_copy(&name, &self->background);
        ui_window_queue_bitmap(wnd, 0, 0, name, 0xffffffff, 0, 0);
    }
}

void ui_mini_party_wnd_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIMiniPartyWnd *self = (struct UIMiniPartyWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    int sx, sy;

    if (g_windowMgr.last_id != wnd->id) {
        ui_window_mgr_touch_window_id(&g_windowMgr, wnd->id);
        g_windowMgr.last_id = wnd->id;
    }
    ui_window_mgr_set_capture(&g_windowMgr, wnd);
    sx = x;
    sy = y;
    ui_window_to_screen(wnd, &sx, &sy);
    self->frame.press_x = sx;
    self->frame.press_y = sy;
    self->frame.saved_x = wnd->x;
    self->frame.saved_y = wnd->y;
    wnd->alpha_time = timeGetTime();
    snap_mgr_save_group_positions(&g_snapMgr, wnd);
}

void ui_mini_party_wnd_on_l_btn_up(struct UIWindow *window, int x, int y)
{
    struct UIMiniPartyWnd *self = (struct UIMiniPartyWnd *)window;
    struct MiniPartyWndAreaMgr *grid = mini_party_area_mgr_instance();
    int cell = mini_party_area_mgr_cell_at(grid, self->frame.base.x, self->frame.base.y);

    if (cell != -1) {
        struct UIMiniPartyWnd *other = (struct UIMiniPartyWnd *)ui_window_mgr_find_by_key(&g_windowMgr, cell);
        int ok = 1;

        if (other) {
            struct PartyMember m;

            other->cell = self->cell;
            ui_mini_party_wnd_get_member(other, &m);
            session_set_plate_cell(&g_session1, m.aid, self->cell);
            party_member_destroy(&m);
        } else {
            grid = mini_party_area_mgr_instance();
            if (cell < 0 || cell >= grid->rows * grid->cols) {
                ui_message_box(msg_str(MSG_NO_CELL_THERE));
                ok = 0;
            }
        }
        if (ok) {
            self->cell = cell;
            session_set_plate_cell(&g_session1, self->member.aid, cell);
        }
    }
    ui_frame_wnd_on_l_btn_up((struct UIWindow *)(&self->frame), x, y);
    ui_window_mgr_plates_changed(&g_windowMgr);
}

void ui_mini_party_wnd_set_display_mode(struct UIMiniPartyWnd *self, int mode)
{
    struct UIWindow *wnd = &self->frame.base;
    const char *bg;
    size_t bg_len;
    int right;

    self->mode = mode;
    switch (mode) {
    case 0:
        bg = PLATE_BG;
        bg_len = 0x30;
        right = 0x23;
        break;
    case 1:
        bg = PLATE_BG_MINI;
        bg_len = 0x35;
        right = 0x17;
        break;
    case 2:
        bg = PLATE_BG_MAXMINI;
        bg_len = 0x38;
        right = 0x23;
        break;
    default:
        ui_window_invalidate_base(wnd);
        return;
    }
    ui_window_resize_base(wnd, ((int *)self->widths.first)[mode], wnd->height);
    if (self->info)
        ui_window_resize_base(&self->info->frame.base, wnd->width, wnd->height);
    string_assign(&self->background, bg, bg_len);
    ui_window_move(self->edge_button, ((int *)self->widths.first)[self->mode] - right, 0x19);
    ui_window_invalidate_base(wnd);
}

int ui_mini_party_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1,
                                      int val2, int val3, int val4)
{
    struct UIMiniPartyWnd *self = (struct UIMiniPartyWnd *)window;
    struct UIWindow *info = (struct UIWindow *)self->info;

    switch (msg) {
    case 0x17: {
        const struct PartyMember *m = (const struct PartyMember *)val1;

        if (!m || !m->found)
            return 0;
        self->member.found = m->found;
        self->member.aid = m->aid;
        self->member.char_id = m->char_id;
        string_assign(&self->member.name, string_cstr(&m->name), m->name.size);
        string_assign(&self->member.map_name, string_cstr(&m->map_name), m->map_name.size);
        self->member.role = m->role;
        self->member.state = m->state;
        self->member.color = m->color;
        self->member.job = m->job;
        self->member.level = m->level;
        self->member.dead = m->dead;
        if (info)
            ui_mini_party_info_wnd_send_msg(info, NULL, 0x17, 0, 0, 0, 0);
        return 0;
    }
    case UI_MSG_COMMAND:
        if (val1 == 0x22 && self->member.aid == val2 && self->member.state == 0 && info) {
            ui_mini_party_info_wnd_send_msg(info, sender, UI_MSG_COMMAND, 0x22, val2, val3, val4);
            ui_window_invalidate_children(&self->frame.base);
        }
        return 0;
    case 0xe: {
        int *w = (int *)self->widths.first;
        int old = self->mode, mode = old;

        if (val1 != 7)
            return 0;
        if (val2 <= w[2]) {
            mode = self->mode = 2;
        } else if (val2 <= w[1]) {
            mode = self->mode = 1;
        } else if (val2 <= w[0]) {
            mode = self->mode = 0;
        }
        if (old != mode) {
            int count = session_get_party_member_count(&g_session1), i;

            for (i = 0; i < count; i++) {
                struct PartyMember m;
                struct UIMiniPartyWnd *plate;

                party_member_at(&m, i);
                if (m.found) {
                    plate = ui_window_mgr_get_plate(&g_windowMgr, m.aid);
                    if (plate) {
                        ui_mini_party_wnd_set_display_mode(plate, self->mode);
                        ui_window_invalidate_base(&plate->frame.base);
                    }
                }
                party_member_destroy(&m);
            }
        }
        return 0;
    }
    case 0x30:
        ui_mini_party_info_wnd_send_msg(info, sender, msg, val1, val2, val3, val4);
        return 0;
    case 0x31:
        if (info)
            ui_mini_party_info_wnd_send_msg(info, sender, msg, val1, val2, val3, val4);
        return 0;
    default:
        return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
    }
}
