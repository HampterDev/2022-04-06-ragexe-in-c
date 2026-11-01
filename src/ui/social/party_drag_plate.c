#include "render/rp_pool.h"
#include "session/state.h"
#include "core/object_type_tables.h"
#include "ui/window_access.h"
#include "game/context.h"
#include "ui/social/party_windows.h"
#include "diag/perf.h"
#include "ui/character/gauge.h"

struct UIDragMiniPartyWnd *ui_drag_mini_party_wnd_construct(struct UIDragMiniPartyWnd *self,
                                                                   struct PartyMember member)
{
    struct PartyMember *m = &self->member;

    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_drag_mini_party_wnd_ops;
    m->found = member.found;
    m->aid = member.aid;
    m->char_id = member.char_id;
    string_copy(&m->name, &member.name);
    string_copy(&m->map_name, &member.map_name);
    m->role = member.role;
    m->state = member.state;
    m->color = member.color;
    m->job = member.job;
    m->level = member.level;
    m->dead = member.dead;
    self->gauge = NULL;
    self->layout = 0;
    self->layouts.first = self->layouts.last = self->layouts.end = NULL;
    self->frame.base.state = 0;
    party_member_destroy(&member);
    return self;
}

void ui_drag_mini_party_wnd_destruct(struct UIDragMiniPartyWnd *self)
{
    self->frame.base.ops = &ui_drag_mini_party_wnd_ops;
    vec_free(&self->layouts);
    party_member_destroy(&self->member);
    ui_frame_wnd_destruct(&self->frame);
}

void ui_drag_mini_party_wnd_destroy(struct UIWindow *window)
{
    struct UIDragMiniPartyWnd *self = (struct UIDragMiniPartyWnd *)window;
    ui_drag_mini_party_wnd_destruct(self);
    mem_free(self);
}

void ui_drag_mini_party_wnd_move_gauge(struct UIDragMiniPartyWnd *self, int x, int y)
{
    ui_window_move_base(&self->gauge->gage.base, x, y);
}

void ui_drag_mini_party_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UIDragMiniPartyWnd *self = (struct UIDragMiniPartyWnd *)window;
    party_layouts_init(&self->layouts);
    if (!self->gauge) {
        self->gauge = ui_pc_gage_new();
        ui_window_create(&self->gauge->gage.base, 0x3c, 5);
        ui_window_move_base(&self->gauge->gage.base, 0x36, 0x19);
        ui_window_add_child(&self->frame.base, &self->gauge->gage.base);
    }
}

static struct BitmapRes *party_bitmap(const char *path)
{
    return (struct BitmapRes *)res_mgr_get(g_resMgr(), ui_skin_path(path));
}

void ui_drag_mini_party_wnd_on_draw(struct UIWindow *window)
{
    struct UIDragMiniPartyWnd *self = (struct UIDragMiniPartyWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct PartyMember *m = &self->member;
    struct String base, map;
    struct String fitted = {0};
    char text[0x100], counts[0x80], path[0x180];
    struct PartyRowLayout *l;
    struct BitmapRes *bmp;
    int die, len;

    ui_window_clear_canvas(wnd, 0xffffffff);
    string_init(&base);
    string_assign(&base, RENEWAL_PARTY, 0x1c);
    if (!m->found) {
        string_free(&base);
        return;
    }

    die = 0;
    if (m->dead == 1) {
        die = 1;
    }
    else if (g_session1.ownAid == m->aid) {
        if (game_player_is_dead())
            die = 1;
    }
    sprintf(path, die ? "%sicon_jobs_%d_die.bmp" : "%sicon_jobs_%d.bmp", string_cstr(&base), m->job);
    bmp = party_bitmap(path);
    l = &PARTY_LAYOUT(&self->layouts, self->layout);
    ui_window_draw_bitmap(wnd, l->x, l->y + (m->role != 0 ? 0 : 2), bmp, 1);
    if (m->role == 0) {
        sprintf(path, "%sico_partyCrown.bmp", string_cstr(&base));
        bmp = party_bitmap(path);
        ui_window_draw_bitmap(wnd, PARTY_LAYOUT(&self->layouts, self->layout).x + 2, 0, bmp, 1);
    }

    {
        const char *map_name = session_map_display_name(string_cstr(&m->map_name));

        string_init(&map);
        string_assign_cstr(&map, map_name);
    }
    sprintf(text, "%d %s(%s)", m->level, string_cstr(&m->name), string_cstr(&map));
    ui_window_fit_text(wnd, &fitted, wnd->width - 0x43, text, 0, 0xc, 1);
    l = &PARTY_LAYOUT(&self->layouts, self->layout);
    ui_window_text_out(wnd, l->x + 0x1c, l->y + 3, string_cstr(&fitted), 0, 0, 0xc, 0xb07f72, 1, 0);

    if (self->gauge) {
        sprintf(counts, "%d/%d", self->gauge->hp, self->gauge->max_hp);
        len = (int)strlen(counts);
        ui_window_text_out(wnd, self->gauge->gage.base.x + 0x42, self->gauge->gage.base.y - 4, counts, len, 0, 0xa, 0, 0,
                         0);
    }

    memset(text, 0, sizeof(text));
    if (m->state != 0)
        sprintf(text, "%sicon_party_off.bmp", string_cstr(&base));
    else if (m->aid == g_session1.ownAid)
        sprintf(text, "%sicon_party_me.bmp", string_cstr(&base));
    else
        sprintf(text, "%sicon_party_on.bmp", string_cstr(&base));
    bmp = party_bitmap(text);
    if (self->gauge)
        ui_window_draw_bitmap(wnd, (int)((float)wnd->width * 0.8f), self->gauge->gage.base.y - 5, bmp, 1);

    string_free(&fitted);
    string_free(&map);
    string_free(&base);
}

void ui_drag_mini_party_wnd_on_l_btn_up(struct UIWindow *window, int x, int y)
{
    struct UIDragMiniPartyWnd *self = (struct UIDragMiniPartyWnd *)window;
    struct UIWindow *party = ui_window_find(0x22);

    if (party)
        ui_messenger_group_wnd_drop_plate((struct UIMessengerGroupWnd *)party, x + self->frame.base.x, y + self->frame.base.y, self->row);
}

void ui_drag_mini_party_wnd_remove_gauge(struct UIDragMiniPartyWnd *self)
{
    ui_window_remove_child(&self->frame.base, &self->gauge->gage.base, 1);
    self->gauge = NULL;
}

void ui_drag_mini_party_wnd_set_hp(struct UIDragMiniPartyWnd *self, int hp, int max_hp)
{
    ui_pc_gage_set_hp(self->gauge, hp, max_hp);
}

void ui_drag_mini_party_wnd_set_grab(struct UIDragMiniPartyWnd *self, int x, int y)
{
    self->grab_x = x;
    self->grab_y = y;
}

const struct UIWindowOps ui_drag_mini_party_wnd_ops = {
    .destroy = ui_drag_mini_party_wnd_destroy,
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
    .on_create = ui_drag_mini_party_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_drag_mini_party_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_frame_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_frame_wnd_on_mouse_move,
    .on_mouse_hover = ui_frame_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_drag_mini_party_wnd_on_l_btn_up,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_frame_wnd_send_msg,
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
