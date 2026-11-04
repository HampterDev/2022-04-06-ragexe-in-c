#include "contents/adventurer_agency.h"
#include "ui/controls/edit.h"
#include "core/text_array.h"
#include "render/rp_pool.h"
#include "session/character_name.h"
#include "session/state.h"
#include "core/object_type_tables.h"
#include <ctype.h>
#include <stdlib.h>
#include "ui/social/party_board.h"
#include "core/msg_str.h"
#include "diag/perf.h"

struct UIRegisterPartyWnd;
struct UIWindow;
void ui_register_party_wnd_destroy(struct UIWindow *window);
void ui_register_party_wnd_store_info(struct UIWindow *self);
void ui_register_party_wnd_on_create(struct UIWindow *self, int width_unused, int height_unused);
void ui_register_party_wnd_on_draw(struct UIWindow *self);
intptr_t ui_register_party_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4);
const struct UIWindowOps ui_register_party_wnd_ops = {
    .destroy = ui_register_party_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_register_party_wnd_store_info,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_register_party_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_register_party_wnd_on_draw,
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
    .send_msg = ui_register_party_wnd_send_msg,
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

#define MSG_REGISTER_TITLE 0xd1c
#define MSG_LABEL_NAME 0xd57
#define MSG_LABEL_LEVEL 0x198
#define MSG_LABEL_PURPOSE 0xd29
#define MSG_LABEL_ROLES 0x1f8
#define MSG_LABEL_MEMO 0xd1f
#define MSG_CHANGE 0xd30
#define MSG_REGISTER 0xd2b
#define MSG_NOTICE 0xd2a
#define MSG_NOT_LEADER 0xdb2
#define MSG_LEVELS_REVERSED 0xdbc

#define CHAT_RED 0xff

struct UIRegisterPartyWnd *ui_register_party_wnd_construct(struct UIRegisterPartyWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_register_party_wnd_ops;
    self->purpose = 0;
    return self;
}

struct UIWindow *ui_register_party_wnd_new(void)
{
    return (struct UIWindow *)ui_register_party_wnd_construct(mem_alloc(sizeof(struct UIRegisterPartyWnd)));
}

void ui_register_party_wnd_destruct(struct UIRegisterPartyWnd *self)
{
    self->frame.base.ops = &ui_register_party_wnd_ops;
    ui_frame_wnd_destruct(&self->frame);
}

void ui_register_party_wnd_destroy(struct UIWindow *window)
{
    struct UIRegisterPartyWnd *self = (struct UIRegisterPartyWnd *)window;
    ui_register_party_wnd_destruct(self);
    mem_free(self);
}

BYTE party_is_number(struct String text)
{
    BYTE number = 0;

    if (text.size) {
        const char *p = string_cstr(&text), *end = p + text.size;

        while (p != end && isdigit(*p))
            p++;
        number = p == end;
    }
    string_free(&text);
    return number;
}

static struct UIWindow *new_static(void)
{
    struct UIWindow *text = mem_alloc(sizeof(struct UIStaticText));

    return text ? ui_static_text_construct(text) : NULL;
}

static void add_label(struct UIWindow *wnd, int width, int x, int y, int msg, int shadow)
{
    struct UIWindow *text = new_static();

    ui_window_create(text, width, 0xf);
    ui_window_move(text, x, y);
    ui_static_text_set_text(text, msg_str(msg), 0);
    ui_static_text_set_text_colour(text, 0, 0, 0);
    if (shadow)
        ((struct UIStaticText *)text)->white_shadow = 1;
    ui_window_add_child(wnd, text);
}

static struct UIWindow *add_level_box(struct UIWindow *wnd, int x, int level)
{
    struct UIWindow *box = mem_alloc(sizeof(struct UIEditCtrl));

    if (box)
        box = ui_edit_ctrl2_construct(box);
    ui_window_create(box, 0x19, 0x10);
    ui_window_move(box, x, 0x2f);
    ((struct UIEditCtrl *)box)->multiline = 1;
    UI_EDIT_MAX_CHARS(box) = 3;
    ui_edit_ctrl_set_back_colour(box, 0xe8, 0xe8, 0xe8);
    party_level_text(box, level);
    ui_window_add_child(wnd, box);
    return box;
}

static struct UIWindow *add_basic_button(struct UIRegisterPartyWnd *self, int msg, int id)
{
    struct UIWindow *wnd = &self->frame.base;
    struct UIWindow *button = mem_alloc(sizeof(struct UIBasicButton));

    if (button)
        button = &ui_basic_button_construct((struct UIBasicButton *)button)->button.base;
    ui_basic_button_set_label(button, ui_window_text_width(wnd, msg_str(msg), 0, 0, 0xc, 0, 0) + 0xa,
                           msg_str(msg));
    ui_window_move_base(button, 0x6c, 0xa5);
    button->id = id;
    UI_FRAME_DEFAULT_ID(&self->frame) = id;
    ui_window_add_child(wnd, button);
    return button;
}

void ui_register_party_wnd_on_create(struct UIWindow *window, int width_unused, int height_unused)
{
    struct UIRegisterPartyWnd *self = (struct UIRegisterPartyWnd *)window;
    static const int role_at[4][2] = { { 0x3c, 0x5f }, { 0x4e, 0x5f }, { 0x60, 0x5f }, { 0x72, 0x5f } };
    struct UIWindow *wnd = &self->frame.base;
    struct TextArray lines = {0};
    struct UIBitmapButton *close;
    int level;
    unsigned i;

    add_label(wnd, 0x28, 0xa, 0x17, MSG_LABEL_NAME, 0);
    self->name_text = new_static();
    ui_window_create(self->name_text, 0x77, 0xf);
    ui_window_move(self->name_text, 0x3c, 0x17);
    ui_static_text_set_text_ellipsis(self->name_text, session_character_name(&g_session1));
    ui_static_text_set_text_colour(self->name_text, 0, 0, 0);
    ui_window_add_child(wnd, self->name_text);
    add_label(wnd, 0x28, 0xa, 0x31, MSG_LABEL_LEVEL, 0);

    level = g_session1.charBaseLevel;
    self->min_level = add_level_box(wnd, 0x3c, level - PARTY_LEVEL_RANGE < 1 ? 1 : level - PARTY_LEVEL_RANGE);
    self->max_level = add_level_box(wnd, 0x60, level + PARTY_LEVEL_RANGE);

    add_label(wnd, wnd->width, 0xa, 0x49, MSG_LABEL_PURPOSE, 0);
    add_label(wnd, wnd->width, 0xa, 0x61, MSG_LABEL_ROLES, 1);
    party_make_job_slots(wnd, self->roles, role_at, 1);
    add_label(wnd, wnd->width - 0x20, 0xa, 0x79, MSG_LABEL_MEMO, 0);

    self->memo = mem_alloc(sizeof(struct UIEditCtrl));
    if (self->memo)
        self->memo = ui_edit_ctrl2_construct(self->memo);
    ui_window_create(self->memo, 0x8b, 0xe);
    UI_EDIT_MAX_CHARS(self->memo) = 0x18;
    ui_edit_ctrl_set_text_colour(self->memo, 0, 0, 0, -1);
    ui_window_move(self->memo, 0x3c, 0x77);
    ui_edit_ctrl_set_back_colour(self->memo, 0xe8, 0xe8, 0xe8);
    ui_window_add_child(wnd, self->memo);

    if (adventurer_agency_is_recruiting(g_adventurerAgency) == 1)
        self->change = add_basic_button(self, MSG_CHANGE, BOARD_ID_OWN_AD);
    else
        self->recruit = add_basic_button(self, MSG_REGISTER, BOARD_ID_RECRUIT);

    text_split_lines(msg_str(MSG_NOTICE), &lines, 0x28);
    for (i = 0; i < lines.size; i++) {
        struct UIWindow *text = new_static();

        ui_window_create(text, wnd->width - 0x12, 0xf);
        ui_window_move(text, 9, 0x88 + 0xc * (int)i);
        ui_static_text_set_text(text, text_array_at(&lines, i).data, 0);
        ui_static_text_set_font(text, 0, 0xb, 0, 0);
        ui_static_text_set_text_colour(text, 0xff, 0, 0);
        ui_window_add_child(wnd, text);
    }

    close = mem_alloc(sizeof *close);
    if (close)
        close = ui_bitmap_button_construct(close);
    ui_bitmap_button_set_bitmap(close, UI_SKIN "basic_interface\\btn_close.bmp", UI_BTN_UP, 0);
    ui_bitmap_button_set_bitmap(close, UI_SKIN "basic_interface\\btn_close_a.bmp", UI_BTN_HOVER, 0);
    ui_bitmap_button_set_bitmap(close, UI_SKIN "basic_interface\\btn_close_b.bmp", UI_BTN_DOWN, 0);
    ui_window_create(&close->button.base, close->bitmap_width, close->bitmap_height);
    ui_window_move_base(&close->button.base, 0xad, 0xa5);
    close->button.base.id = UI_ID_CLOSE;
    ui_window_add_child(wnd, &close->button.base);

    self->purpose_combo = party_make_purpose_combo(wnd, 0x3c, 0x46, 0x3f);
    ui_window_add_big_close_button(wnd, "", 0);
    text_array_clear(&lines);
}

void ui_register_party_wnd_on_draw(struct UIWindow *window)
{
    struct UIRegisterPartyWnd *self = (struct UIRegisterPartyWnd *)window;
    struct UIWindow *wnd = &self->frame.base;

    ui_window_clear_canvas(wnd, 0xffff00ff);
    ui_window_draw_bitmap(wnd, 0, 0,
                        res_mgr_get(g_resMgr(), ui_skin_path(AGENCY "bg_agency_register.bmp")), 0);
    ui_window_text_out(wnd, 0x56, 0x32, "~", 0, 0, 0xc, 0, 0, 0);
    ui_window_draw_title_bar(wnd, 1, msg_str(MSG_REGISTER_TITLE), 0);
}

void ui_register_party_wnd_store_info(struct UIWindow *window)
{
    struct UIRegisterPartyWnd *self = (struct UIRegisterPartyWnd *)window;
    if (!self->info)
        return;
    self->info->width = self->frame.base.width;
    self->info->height = self->frame.base.height;
    self->info->x = self->frame.base.x;
    self->info->y = self->frame.base.y;
}

void ui_register_party_wnd_check_levels(struct UIRegisterPartyWnd *self)
{
    int level = g_session1.charBaseLevel;
    struct String text, arg;
    const char *box;

    box = ui_edit_ctrl_get_text(self->min_level);
    string_init(&text);
    string_assign_cstr(&text, box);
    string_copy(&arg, &text);
    if (!party_is_number(arg))
        party_level_text(self->min_level, level - PARTY_LEVEL_RANGE < 1 ? 1 : level - PARTY_LEVEL_RANGE);
    box = ui_edit_ctrl_get_text(self->max_level);
    string_assign_cstr(&text, box);
    string_copy(&arg, &text);
    if (!party_is_number(arg))
        party_level_text(self->max_level, level + PARTY_LEVEL_RANGE);
    string_free(&text);
}

static int register_send(struct UIRegisterPartyWnd *self)
{
    struct AdvenPartyInfo ad;
    const char *text;
    int i, sent = 0;

    party_booking_init(&ad);
    for (i = 0; i < 4; i++)
        ad.roles[i] = self->roles[i]->checked;
    ad.min_level = atoi(ui_edit_ctrl_get_text(self->min_level));
    ad.max_level = atoi(ui_edit_ctrl_get_text(self->max_level));
    ad.purpose = self->purpose;
    text = ui_edit_ctrl_get_text(self->memo);
    if (!owned_text_assign(&ad.memo, text_view_cstr(text))) goto done;
    sent = adventurer_agency_register(g_adventurerAgency, &ad);
done:
    party_booking_clear(&ad);
    return sent;
}

static void chat_red(int msg)
{
    char line[0x100];

    memset(line, 0, sizeof line);
    snprintf(line, sizeof line, msg_str(msg));
    ui_window_mgr_send_msg_(&g_windowMgr, UIWM_MSG_CHAT_LINE, (intptr_t)line, CHAT_RED, 0, 0);
}

intptr_t ui_register_party_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                          intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4)
{
    struct UIRegisterPartyWnd *self = (struct UIRegisterPartyWnd *)window;
    switch (msg) {
    case UI_MSG_COMMAND:
        if (val1 != UI_ID_CLOSE) {
            if (val1 != BOARD_ID_OWN_AD && val1 != BOARD_ID_RECRUIT)
                return 0;
            if (g_session1.partyLeader != 0) {
                ui_register_party_wnd_check_levels(self);
                if (atoi(ui_edit_ctrl_get_text(self->min_level)) >
                    atoi(ui_edit_ctrl_get_text(self->max_level))) {
                    ui_window_mgr_send_msg_(&g_windowMgr, UIWM_MSG_CHAT_LINE, (intptr_t)msg_str(MSG_LEVELS_REVERSED),
                                          CHAT_RED, 0, 0);
                    return 0;
                }
                if (!register_send(self)) {
                    chat_red(0xd59);
                    return 0;
                }
            } else {
                chat_red(MSG_NOT_LEADER);
            }
        }
        ui_window_mgr_post_quit1(&g_windowMgr, PARTY_REGISTER_WND_ID);
        return 0;
    case UI_MSG_SET_INFO:
        self->info = (struct WindowInfo *)val1;
        if (!ui_info_on_screen(self->info)) {
            self->info->x = 0xc8;
            self->info->y = 0xc8;
            self->info->width = 0xdc;
            self->info->height = 0xc5;
        }
        ui_frame_wnd_move(&self->frame.base, self->info->x, self->info->y);
        return 0;
    case UI_MSG_COMBO_CHOSE:
        if (sender == self->purpose_combo)
            self->purpose = val1;
        return 0;
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
}

void ui_register_party_wnd_set_ad(struct UIRegisterPartyWnd *self, const struct AdvenPartyInfo *ad)
{
    int i;

    for (i = 0; i < 4; i++)
        ui_job_slot_button_set_checked(self->roles[i], ad->roles[i]);
    party_level_text(self->min_level, ad->min_level);
    party_level_text(self->max_level, ad->max_level);
    self->purpose = ad->purpose;
    ui_combo_box_def_wnd_select(self->purpose_combo, ad->purpose);
    ui_edit_ctrl_set_text((struct UIEditCtrl *)self->memo, owned_text_cstr(&ad->memo));
    ui_static_text_set_text(self->name_text, owned_text_cstr(&ad->name), 0);
    ui_window_invalidate_base(&self->frame.base);
}
