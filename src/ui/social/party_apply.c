#include "contents/adventurer_agency.h"
#include "world/tooltip.h"
#include "core/text_array.h"
#include "render/rp_pool.h"
#include "core/object_type_tables.h"
#include "world/mode_manager.h"
#include <string.h>
#include "ui/social/party_booking.h"
#include "core/msg_str.h"
#include "diag/perf.h"
#include "session/session_party.h"

struct UIApplyForPartyWnd;
struct UIWindow;
void ui_apply_for_party_wnd_destroy(struct UIWindow *window);
void ui_apply_for_party_wnd_store_info(struct UIWindow *self);
void ui_apply_for_party_wnd_on_create(struct UIWindow *self, int width_unused, int height_unused);
void ui_apply_for_party_wnd_on_draw(struct UIWindow *self);
int ui_apply_for_party_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_apply_for_party_wnd_ops = {
    .destroy = ui_apply_for_party_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_apply_for_party_wnd_store_info,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_apply_for_party_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_apply_for_party_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_frame_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_frame_wnd_on_mouse_move,
    .on_mouse_hover = ui_apply_for_party_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_frame_wnd_on_l_btn_up,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_apply_for_party_wnd_send_msg,
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

#define APPLY_FOR_PARTY_WND_ID 0x159

#define APPLY_ID_NAME 0x175
#define APPLY_ID_APPLY 0x107
#define APPLY_ID_CLOSE 0xc1

#define MSG_APPLY_TITLE 0xd28
#define MSG_APPLY_LABEL_NAME 0xd57
#define MSG_APPLY_LABEL_LEVEL 0x198
#define MSG_APPLY_LABEL_PURPOSE 0xd29
#define MSG_APPLY_LABEL_ROLES 0x1f8
#define MSG_APPLY_LABEL_MEMO 0xd1f
#define MSG_APPLY_NOTICE 0xd2a
#define MSG_APPLY_BUTTON_NAME 0xd2e
#define MSG_APPLY_BUTTON_APPLY 0xd2d
#define MSG_APPLY_BUTTON_CLOSE 0xbe5
#define MSG_ROLE_TOOLTIP_DEF 0xd5c
#define MSG_ROLE_TOOLTIP_HEAL 0xda3
#define MSG_ROLE_TOOLTIP_ATK 0xd74
#define MSG_ROLE_TOOLTIP_BUF 0xdb4
#define MSG_NAME_ON_SESSION_LIST 0x66

#define UIWM_MSG_WHISPER 0xe

struct UIApplyForPartyWnd *ui_apply_for_party_wnd_construct(struct UIApplyForPartyWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_apply_for_party_wnd_ops;
    party_booking_init(&self->ad);
    return self;
}

struct UIWindow *ui_apply_for_party_wnd_new(void)
{
    return (struct UIWindow *)ui_apply_for_party_wnd_construct(mem_alloc(sizeof(struct UIApplyForPartyWnd)));
}

static void ui_apply_for_party_wnd_destruct(struct UIApplyForPartyWnd *self)
{
    self->frame.base.ops = &ui_apply_for_party_wnd_ops;
    adventurer_agency_cancel_detail(g_adventurerAgency);
    party_booking_clear(&self->ad);
    ui_frame_wnd_destruct(&self->frame);
}

void ui_apply_for_party_wnd_destroy(struct UIWindow *window)
{
    struct UIApplyForPartyWnd *self = (struct UIApplyForPartyWnd *)window;
    ui_apply_for_party_wnd_destruct(self);
    mem_free(self);
}

static void add_text_button(struct UIApplyForPartyWnd *self, int msg, int x, int id)
{
    struct UIWindow *wnd = &self->frame.base;
    struct UIWindow *button = mem_alloc(sizeof(struct UIBasicButton));
    const char *label;

    if (button)
        button = (struct UIWindow *)ui_basic_button_construct((struct UIBasicButton *)button);
    label = msg_str(msg);
    ui_basic_button_set_label(button, ui_window_text_width(wnd, msg_str(msg), 0, 0, 0xc, 0, 0) + 0xa, label);
    ui_window_move_base(button, x, 0xa5);
    button->id = id;
    ui_window_add_child(wnd, button);
}

void ui_apply_for_party_wnd_on_create(struct UIWindow *window, int width_unused, int height_unused)
{
    struct UIApplyForPartyWnd *self = (struct UIApplyForPartyWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct TextArray lines = {0};
    unsigned i;

    add_text_button(self, MSG_APPLY_BUTTON_NAME, 0x33, APPLY_ID_NAME);
    add_text_button(self, MSG_APPLY_BUTTON_APPLY, 0x6c, APPLY_ID_APPLY);
    add_text_button(self, MSG_APPLY_BUTTON_CLOSE, 0xad, APPLY_ID_CLOSE);

    text_split_lines(msg_str(MSG_APPLY_NOTICE), &lines, 0x28);
    for (i = 0; i < lines.size; i++) {
        struct UIWindow *text = mem_alloc(sizeof(struct UIStaticText));

        if (text)
            text = ui_static_text_construct(text);
        ui_window_create(text, wnd->width - 0x12, 0xf);
        ui_window_move(text, 9, 0x88 + 0xc * (int)i);
        ui_static_text_set_text(text, text_array_at(&lines, i).data, 0);
        ui_static_text_set_font(text, 0, 0xb, 0, 0);
        ui_static_text_set_text_colour(text, 0xff, 0, 0);
        ui_window_add_child(wnd, text);
    }
    ui_window_add_big_close_button(wnd, "", 0);
    text_array_clear(&lines);
}

static void draw_label(struct UIWindow *wnd, int x, int y, const char *text)
{
    ui_window_text_out(wnd, x, y, text, 0, 0, 0xc, 0, 0, 0);
}

void ui_apply_for_party_wnd_on_draw(struct UIWindow *window)
{
    struct UIApplyForPartyWnd *self = (struct UIApplyForPartyWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct String name = {0};
    char levels[10] = { 0 };

    ui_window_clear_canvas(wnd, 0xffff00ff);
    ui_window_draw_bitmap(wnd, 0, 0,
                        res_mgr_get(g_resMgr(), ui_skin_path(AGENCY "bg_agency_register.bmp")), 0);
    ui_window_draw_title_bar(wnd, 1, msg_str(MSG_APPLY_TITLE), 0);
    draw_label(wnd, 0xa, 0x19, msg_str(MSG_APPLY_LABEL_NAME));
    ui_window_fit_plain_text(wnd, &name, owned_text_view(&self->ad.name), 0x77);
    draw_label(wnd, 0x3c, 0x19, string_cstr(&name));
    draw_label(wnd, 0xa, 0x31, msg_str(MSG_APPLY_LABEL_LEVEL));
    snprintf(levels, 0xa, "%d~%d", self->ad.min_level, self->ad.max_level);
    draw_label(wnd, 0x3c, 0x31, levels);
    draw_label(wnd, 0xa, 0x49, msg_str(MSG_APPLY_LABEL_PURPOSE));
    draw_label(wnd, 0x3c, 0x49, party_purpose_name(self->ad.purpose));
    draw_label(wnd, 0xa, 0x61, msg_str(MSG_APPLY_LABEL_ROLES));
    party_draw_roles(wnd, self->ad.roles, 0x3c, 0x5f);
    draw_label(wnd, 0xa, 0x79, msg_str(MSG_APPLY_LABEL_MEMO));
    draw_label(wnd, 0x3c, 0x79, owned_text_cstr(&self->ad.memo));
    string_free(&name);
}

void ui_apply_for_party_wnd_on_mouse_hover(struct UIWindow *window, int x, int y)
{
    struct UIApplyForPartyWnd *self = (struct UIApplyForPartyWnd *)window;
    unsigned hover_x = (unsigned)x;
    struct String tip;
    int sx, sy;

    if ((unsigned)(y - 0x5f) > 0x10)
        return;
    string_init(&tip);
    if (hover_x > 0x3c && hover_x < 0x4c) {
        const char *text = msg_str(MSG_ROLE_TOOLTIP_DEF);

        hover_x = 0x3c;
        string_assign_cstr(&tip, text);
    } else if (hover_x > 0x4e && hover_x < 0x5e) {
        const char *text = msg_str(MSG_ROLE_TOOLTIP_HEAL);

        hover_x = 0x4e;
        string_assign_cstr(&tip, text);
    } else if (hover_x > 0x60 && hover_x < 0x70) {
        hover_x = 0x60;
        string_assign_cstr(&tip, msg_str(MSG_ROLE_TOOLTIP_ATK));
    } else if (hover_x > 0x72 && hover_x < 0x82) {
        hover_x = 0x72;
        string_assign_cstr(&tip, msg_str(MSG_ROLE_TOOLTIP_BUF));
    }
    if (tip.size) {
        sx = (int)hover_x;
        sy = 0x4b;
        ui_window_to_screen(&self->frame.base, &sx, &sy);
        mode_show_tooltip(client_current_mode(), string_cstr(&tip), sx, sy, -1, 0);
    }
    string_free(&tip);
}

void ui_apply_for_party_wnd_store_info(struct UIWindow *window)
{
    struct UIApplyForPartyWnd *self = (struct UIApplyForPartyWnd *)window;
    struct UIWindow *wnd = &self->frame.base;

    if (!self->info)
        return;
    self->info->width = wnd->width;
    self->info->height = wnd->height;
    self->info->x = wnd->x;
    self->info->y = wnd->y;
}

static void apply_info_reset(struct WindowInfo *info)
{
    info->x = 0xc8;
    info->y = 0xc8;
    info->width = 0xdc;
    info->height = 0xbe;
}

void party_booking_send_name(const struct AdvenPartyInfo *ad)
{
    struct String id_text, id_tmp;
    const char *label = NULL;
    const char *kind;

    if (ui_window_mgr_is_friend(&g_windowMgr, owned_text_cstr(&ad->name)))
        label = msg_str(MSG_NAME_ON_SESSION_LIST);
    kind = session_is_guild_member_aid(&g_session1, ad->aid) ? "GuildMember" : label;
    string_init(&id_text);
    string_free(&id_text);
    string_move(&id_text, session_scramble_number(&g_session1, &id_tmp, ad->gid));
    string_free(&id_tmp);
    ui_window_mgr_send_msg_(&g_windowMgr, UIWM_MSG_WHISPER, (int)owned_text_cstr(&ad->name), (int)kind,
                        (int)string_cstr(&id_text), 0);
    string_free(&id_text);
}

int ui_apply_for_party_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                          int val1, int val2, int val3, int val4)
{
    struct UIApplyForPartyWnd *self = (struct UIApplyForPartyWnd *)window;
    if (msg == UI_MSG_COMMAND) {
        if (val1 == APPLY_ID_CLOSE)
            ui_window_mgr_post_quit1(&g_windowMgr, APPLY_FOR_PARTY_WND_ID);
        else if (val1 == APPLY_ID_APPLY)
            adventurer_agency_load_party_info(g_adventurerAgency, self->ad.aid);
        else if (val1 == APPLY_ID_NAME)
            party_booking_send_name(&self->ad);
        return 0;
    }
    if (msg == UI_MSG_SET_INFO) {
        self->info = (struct WindowInfo *)val1;
        if (!ui_info_on_screen(self->info))
            apply_info_reset(self->info);
        ui_frame_wnd_move(&self->frame.base, self->info->x, self->info->y);
        return 0;
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
}

int ui_apply_for_party_wnd_set_ad(struct UIApplyForPartyWnd *self, const struct AdvenPartyInfo *ad)
{
    if (!party_booking_assign(&self->ad, ad))
        return 0;
    ui_window_invalidate_base(&self->frame.base);
    return 1;
}
