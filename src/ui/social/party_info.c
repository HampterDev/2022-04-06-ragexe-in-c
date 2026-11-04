#include "ui/controls/buttons.h"
#include "render/rp_pool.h"
#include "session/state.h"
#include "core/object_type_tables.h"
#include "ui/social/party_booking.h"
#include "diag/perf.h"

struct UIPartyInfoWnd;
struct UIWindow;
void ui_party_info_wnd_destroy(struct UIWindow *window);
void ui_party_info_wnd_on_draw(struct UIWindow *self);
void ui_party_info_wnd_on_l_btn_up(struct UIWindow *self, int x, int y);
void ui_party_info_wnd_on_r_btn_up(struct UIWindow *self, int x, int y);
const struct UIButtonOps ui_party_info_wnd_ops = {
    .base = {
        .destroy = ui_party_info_wnd_destroy,
        .resize = ui_window_resize_base,
        .is_frame = 0,
        .refreshes_children = 0,
        .move = ui_window_move_base,
        .is_focusable = ui_window_is_focusable_base,
        .get_backdrop_box = ui_window_get_backdrop_box_base,
        .is_click_through = ui_window_is_click_through_base,
        .is_hit_testable = ui_window_is_hit_testable_base,
        .drag_and_drop = ui_window_drag_and_drop_base,
        .store_info = ui_window_store_info_base,
        .save_pos = ui_window_save_pos_base,
        .move_delta = ui_window_move_delta_base,
        .set_show = ui_window_set_show_base,
        .on_create = ui_window_on_create_base,
        .on_destroy = ui_window_on_destroy_base,
        .on_process = ui_window_on_process_base,
        .on_draw = ui_party_info_wnd_on_draw,
        .on_focus = ui_window_on_focus_base,
        .on_focus_lost = ui_window_on_focus_lost_base,
        .on_l_btn_down = ui_button_on_l_btn_down,
        .on_l_btn_dbl_clk = ui_button_on_l_btn_dbl_clk,
        .on_mouse_move = ui_button_on_mouse_move,
        .on_mouse_hover = ui_button_on_mouse_hover,
        .track_mouse = ui_button_track_mouse,
        .on_mouse_shape = ui_button_on_mouse_shape,
        .on_l_btn_up = ui_party_info_wnd_on_l_btn_up,
        .on_r_btn_down = ui_window_on_r_btn_down_base,
        .on_r_btn_up = ui_party_info_wnd_on_r_btn_up,
        .on_wheel = ui_window_on_wheel_base,
        .snap = ui_window_snap_base,
        .send_msg = ui_window_send_msg_base,
        .invalidate = ui_window_invalidate_base,
        .render = ui_window_render_base,
        .has_custom_backdrop = 0,
        .draw_backdrop = ui_window_draw_backdrop_base,
        .type = UI_TYPE_WINDOW,
        .on_hover_enter = ui_window_on_hover_enter_base,
        .on_hover_leave = ui_window_on_hover_leave_base,
        .make_quad = ui_window_make_quad_base,
        .hit_test = ui_window_hit_test_base,
        .render_overlay = ui_window_render_overlay_base,
    },
    .set_show = ui_button_set_show,
};

struct UIPartyInfoWnd *ui_party_info_wnd_construct(struct UIPartyInfoWnd *self)
{
    int i;

    ui_bitmap_button_construct(&self->button);
    self->button.button.base.ops = &ui_party_info_wnd_ops.base;
    string_init(&self->memo);
    string_init(&self->levels);
    for (i = 0; i < 4; i++)
        self->roles[i] = 0;
    string_init(&self->name);
    party_booking_init(&self->ad);
    self->button.button.skinned = 0;
    return self;
}

static void ui_party_info_wnd_destruct(struct UIPartyInfoWnd *self)
{
    self->button.button.base.ops = &ui_party_info_wnd_ops.base;
    party_booking_clear(&self->ad);
    string_free(&self->name);
    string_free(&self->levels);
    string_free(&self->memo);
    ui_bitmap_button_destruct(&self->button);
}

void ui_party_info_wnd_destroy(struct UIWindow *window)
{
    struct UIPartyInfoWnd *self = (struct UIPartyInfoWnd *)window;
    ui_party_info_wnd_destruct(self);
    mem_free(self);
}

static void *skin_res(const char *path)
{
    return res_mgr_get(g_resMgr(), ui_skin_path(path));
}

void ui_party_info_wnd_on_draw(struct UIWindow *window)
{
    struct UIPartyInfoWnd *self = (struct UIPartyInfoWnd *)window;
    struct UIWindow *wnd = &self->button.button.base;
    struct String cut = {0};
    void *bitmap = NULL;

    ui_window_clear_canvas(wnd, 0xffff00ff);
    if (!self->button.button.skinned)
        return;
    if (wnd->state >= 0 && wnd->state <= 2)
        bitmap = skin_res(string_cstr(&self->button.bitmaps[wnd->state]));
    if (self->button.button.disabled && self->button.bitmaps[3].size)
        bitmap = skin_res(string_cstr(&self->button.bitmaps[3]));
    if (bitmap)
        ui_window_draw_bitmap(wnd, 0, 0, bitmap, 0);

    ui_window_text_out(wnd, 0xb, 5, string_cstr(&self->levels), 0, 0, 0xc, 0, 0, 0);
    ui_window_text_out(wnd, 0x40, 5, party_purpose_name(self->purpose), 0, 0, 0xc, 0, 0, 0);
    party_draw_roles(wnd, self->ad.roles, 0x6d, 3);
    ui_window_fit_plain_text(wnd, &cut, string_view(&self->name), 0x77);
    ui_window_text_out(wnd, 0xc0, 5, string_cstr(&cut), (int)cut.size, 0, 0xc, 0, 0, 0);
    ui_window_text_out(wnd, 0x13b, 5, string_cstr(&self->memo), (int)self->memo.size, 0, 0xc, 0,
                     0, 0);
    string_free(&cut);
}

void ui_party_info_wnd_on_l_btn_up(struct UIWindow *window, int x, int y)
{
    struct UIPartyInfoWnd *self = (struct UIPartyInfoWnd *)window;
    if (!self->button.button.disabled)
        ui_button_on_l_btn_up((struct UIWindow *)(&self->button.button), x, y);
}

void ui_party_info_wnd_on_r_btn_up(struct UIWindow *window, int x, int y)
{
    struct UIPartyInfoWnd *self = (struct UIPartyInfoWnd *)window;
    if (self->button.button.disabled)
        return;
    if (self->ad.aid == g_session1.ownAid || self->ad.gid == g_session1.ownGid)
        return;
    party_booking_send_name(&self->ad);
}

int ui_party_info_wnd_set_ad(struct UIPartyInfoWnd *self, const struct AdvenPartyInfo *ad)
{
    char levels[10];

    if (!party_booking_assign(&self->ad, ad))
        return 0;
    self->button.button.disabled = 0;
    self->button.button.skinned = 1;
    string_assign(&self->memo, owned_text_cstr(&ad->memo), ad->memo.size);
    memset(levels, 0, sizeof levels);
    snprintf(levels, sizeof levels, "%d~%d", ad->min_level, ad->max_level);
    string_assign_cstr(&self->levels, levels);
    self->roles[0] = ad->roles[0];
    self->roles[2] = ad->roles[1];
    self->roles[1] = ad->roles[2];
    self->roles[3] = ad->roles[3];
    self->purpose = ad->purpose;
    string_assign(&self->name, owned_text_cstr(&ad->name), ad->name.size);

    return 1;
}

void ui_party_info_wnd_reset(struct UIPartyInfoWnd *self)
{
    self->button.button.skinned = 0;
    string_clear(&self->memo);
    string_clear(&self->levels);
    self->roles[0] = 1;
    self->roles[1] = 1;
    self->roles[3] = 1;
    self->purpose = 0;
    string_clear(&self->name);
    self->button.button.disabled = 1;
    ui_window_invalidate_base(&self->button.button.base);
}
