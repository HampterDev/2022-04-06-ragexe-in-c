#include "ui/controls/edit.h"
#include "ui/controls/buttons.h"
#include "render/rp_pool.h"
#include "session/character_name.h"
#include "web/service_managers.h"
#include "session/state.h"
#include "core/object_type_tables.h"
#include "ui/window_access.h"
#include <stdlib.h>
#include "ui/social/party_board.h"
#include "ui/ui_std.h"
#include "core/msg_str.h"
#include "diag/perf.h"

struct UIJobSlotButton;
struct UIWindow;
void ui_job_slot_button_destroy(struct UIWindow *window);
void ui_job_slot_button_on_l_btn_up(struct UIWindow *self, int x, int y);
const struct UIButtonOps ui_job_slot_button_ops = {
    .base = {
        .destroy = ui_job_slot_button_destroy,
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
        .on_draw = ui_bitmap_button_on_draw,
        .on_focus = ui_window_on_focus_base,
        .on_focus_lost = ui_window_on_focus_lost_base,
        .on_l_btn_down = ui_button_on_l_btn_down,
        .on_l_btn_dbl_clk = ui_button_on_l_btn_dbl_clk,
        .on_mouse_move = ui_button_on_mouse_move,
        .on_mouse_hover = ui_button_on_mouse_hover,
        .track_mouse = ui_button_track_mouse,
        .on_mouse_shape = ui_button_on_mouse_shape,
        .on_l_btn_up = ui_job_slot_button_on_l_btn_up,
        .on_r_btn_down = ui_window_on_r_btn_down_base,
        .on_r_btn_up = ui_window_on_r_btn_up_base,
        .on_wheel = ui_window_on_wheel_base,
        .snap = ui_window_snap_base,
        .send_msg = ui_window_send_msg_base,
        .invalidate = ui_window_invalidate_base,
        .render = ui_window_render_base,
        .has_custom_backdrop = 0,
        .draw_backdrop = ui_window_draw_backdrop_base,
        .get_records = ui_window_get_records_base,
        .type = UI_TYPE_WINDOW,
        .on_hover_enter = ui_window_on_hover_enter_base,
        .on_hover_leave = ui_window_on_hover_leave_base,
        .make_quad = ui_window_make_quad_base,
        .hit_test = ui_window_hit_test_base,
        .render_overlay = ui_window_render_overlay_base,
    },
    .set_show = ui_button_set_show,
};

struct UIAdvenPartyBoardWnd;
void ui_adven_party_board_wnd_destroy(struct UIWindow *window);
void ui_adven_party_board_wnd_store_info(struct UIWindow *self);
void ui_adven_party_board_wnd_on_create(struct UIWindow *self, int width_unused, int height_unused);
void ui_adven_party_board_wnd_on_draw(struct UIWindow *self);
int ui_adven_party_board_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_adven_party_board_wnd_ops = {
    .destroy = ui_adven_party_board_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_adven_party_board_wnd_store_info,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_adven_party_board_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_adven_party_board_wnd_on_draw,
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
    .send_msg = ui_adven_party_board_wnd_send_msg,
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

#define MSG_BOARD_TITLE 0xdb0
#define MSG_SEARCH_EMPTY 0xd5b
#define MSG_SEARCH 0xd26
#define MSG_RESET 0xd27
#define MSG_RECRUIT 0xd1d
#define MSG_BOOKING_OFF 0xd2f
#define MSG_CLOSE 0xbe5
#define MSG_ROLE_TOOLTIP_DEF 0xd5c
#define MSG_ROLE_TOOLTIP_HEAL 0xda3
#define MSG_ROLE_TOOLTIP_ATK 0xd74
#define MSG_ROLE_TOOLTIP_BUF 0xdb4

#define SEARCH_EMPTY_COLOUR 0x646464
#define PAGE_LINE_Y 0x13f
#define BOTTOM_BUTTON_Y 0x13d
#define ROW_FIRST_Y 0x49
#define ROW_STEP 0x18

struct UIJobSlotButton *ui_job_slot_button_construct(struct UIJobSlotButton *self)
{
    ui_bitmap_button_construct(&self->bmp);
    self->bmp.button.base.ops = &ui_job_slot_button_ops.base;
    string_init(&self->on_bitmap);
    string_init(&self->off_bitmap);
    self->checked = 0;
    return self;
}

void ui_job_slot_button_destruct(struct UIJobSlotButton *self)
{
    self->bmp.button.base.ops = &ui_job_slot_button_ops.base;
    string_free(&self->off_bitmap);
    string_free(&self->on_bitmap);
    ui_bitmap_button_destruct(&self->bmp);
}

void ui_job_slot_button_destroy(struct UIWindow *window)
{
    struct UIJobSlotButton *self = (struct UIJobSlotButton *)window;
    ui_job_slot_button_destruct(self);
    mem_free(self);
}

void ui_job_slot_button_set_checked(struct UIJobSlotButton *self, BYTE checked)
{
    const struct String *shown, *pressed;

    self->checked = checked;
    ui_window_set_state(&self->bmp.button.base, UI_BTN_UP);
    if (checked == 1) {
        shown = &self->on_bitmap;
        pressed = &self->off_bitmap;
    } else {
        shown = &self->off_bitmap;
        pressed = &self->on_bitmap;
    }
    ui_bitmap_button_set_bitmap(&self->bmp, string_cstr(shown), UI_BTN_UP, 0);
    ui_bitmap_button_set_bitmap(&self->bmp, string_cstr(shown), UI_BTN_HOVER, 0);
    ui_bitmap_button_set_bitmap(&self->bmp, string_cstr(pressed), UI_BTN_DOWN, 0);
    ui_window_invalidate_base(&self->bmp.button.base);
}

void ui_job_slot_button_on_l_btn_up(struct UIWindow *window, int x, int y)
{
    struct UIJobSlotButton *self = (struct UIJobSlotButton *)window;
    ui_button_on_l_btn_up((struct UIWindow *)(&self->bmp.button), x, y);
    ui_job_slot_button_set_checked(self, self->checked == 0);
}

static void agency_bitmap(struct String *dest, const struct String *prefix, const char *name)
{
    string_assign(dest, string_cstr(prefix), prefix->size);
    string_append_cstr(dest, name);
    string_append_cstr(dest, ".bmp");
}

void party_make_job_slots(struct UIWindow *parent, struct UIJobSlotButton **slots, const int at[4][2], BYTE checked)
{
    static const char *const names[4][2] = {
        { "btn_job_def_on", "btn_job_def_off" },
        { "btn_job_heal_on", "btn_job_heal_off" },
        { "btn_job_atk_on", "btn_job_atk_off" },
        { "btn_job_buf_on", "btn_job_buf_off" },
    };
    struct String prefix;
    const char *tooltips[4];
    int i;

    string_init(&prefix);
    string_assign(&prefix, AGENCY, sizeof AGENCY - 1);
    tooltips[0] = msg_str(MSG_ROLE_TOOLTIP_DEF);
    tooltips[1] = msg_str(MSG_ROLE_TOOLTIP_HEAL);
    tooltips[2] = msg_str(MSG_ROLE_TOOLTIP_ATK);
    tooltips[3] = msg_str(MSG_ROLE_TOOLTIP_BUF);
    for (i = 0; i < 4; i++) {
        struct UIJobSlotButton *slot = mem_alloc(sizeof *slot);

        if (slot)
            slot = ui_job_slot_button_construct(slot);
        slots[i] = slot;
        ui_window_create(&slot->bmp.button.base, 0x10, 0x10);
        ui_window_move_base(&slots[i]->bmp.button.base, at[i][0], at[i][1]);
        agency_bitmap(&slots[i]->on_bitmap, &prefix, names[i][0]);
        agency_bitmap(&slots[i]->off_bitmap, &prefix, names[i][1]);
        ui_job_slot_button_set_checked(slots[i], checked);
        ui_bitmap_button_set_tool_tip(&slots[i]->bmp, tooltips[i]);
        ui_window_add_child(parent, &slots[i]->bmp.button.base);
    }
    string_free(&prefix);
}

struct UIWindow *party_make_purpose_combo(struct UIWindow *parent, int x, int y, int width)
{
    struct UIWindow *combo = mem_alloc(sizeof(struct UIComboBoxDef2Wnd));
    int i;

    if (combo)
        combo = ui_combo_box_def2_wnd_construct(combo);
    ((struct UIComboBoxDefWnd *)combo)->list_height = 0x7e;
    ui_window_create(combo, width, 0x12);
    ui_window_move(combo, x, y);
    ui_window_add_child(parent, combo);
    ui_combo_add(combo, msg_str(MSG_PURPOSE_ANY));
    for (i = 0; i < PURPOSE_COUNT - 1; i++)
        ui_combo_add(combo, msg_str(MSG_PURPOSE_FIRST + i));
    ui_combo_box_def_wnd_select(combo, 0);
    return combo;
}

void party_level_text(struct UIWindow *edit, int level)
{
    char text[10];

    memset(text, 0, sizeof text);
    _snprintf_s(text, sizeof text, (size_t)-1, "%d", level);
    ui_edit_ctrl_set_text((struct UIEditCtrl *)edit, text);
}

void party_button_caption(struct UIBmpTextButton *button, const char *text)
{
    ui_bmp_text_button_set_caption(button, text_view_cstr(text));
}

static struct UIWindow *board_wnd(struct UIAdvenPartyBoardWnd *self)
{
    return &self->frame.base;
}

static void button_enable(void *button, BYTE disabled)
{
    ((struct UIButton *)button)->disabled = disabled;
    ui_window_invalidate(button);
    ui_window_invalidate(button);
}

struct UIAdvenPartyBoardWnd *ui_adven_party_board_wnd_construct(struct UIAdvenPartyBoardWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_adven_party_board_wnd_ops;
    self->selected_row = 0;
    self->hover_row = 0;
    self->page = 1;
    self->pages = 1;
    self->first_open = 1;
    self->ad_count = 0;
    self->menu.first = NULL;
    self->menu.last = NULL;
    self->menu.end = NULL;
    self->purpose = 0;
    return self;
}

struct UIWindow *ui_adven_party_board_wnd_new(void)
{
    return (struct UIWindow *)ui_adven_party_board_wnd_construct(mem_alloc(sizeof(struct UIAdvenPartyBoardWnd)));
}

void ui_adven_party_board_wnd_destruct(struct UIAdvenPartyBoardWnd *self)
{
    self->frame.base.ops = &ui_adven_party_board_wnd_ops;
    ui_menu_entry_vec_free(&self->menu);
    ui_frame_wnd_destruct(&self->frame);
}

void ui_adven_party_board_wnd_destroy(struct UIWindow *window)
{
    struct UIAdvenPartyBoardWnd *self = (struct UIAdvenPartyBoardWnd *)window;
    ui_adven_party_board_wnd_destruct(self);
    mem_free(self);
}

int ui_adven_party_board_wnd_request_page(struct UIAdvenPartyBoardWnd *self, int page)
{
    if (!adventurer_agency_load_list(g_adventurerAgency, page)) return 0;
    self->page = page;
    button_enable(self->prev_page, 1);
    button_enable(self->next_page, 1);
    button_enable(self->refresh, 1);
    if (g_session1.partyLeader == 1)
        adventurer_agency_load_my_party(g_adventurerAgency);
    return 1;
}

void ui_adven_party_board_wnd_set_has_ad(struct UIAdvenPartyBoardWnd *self, BYTE has_ad)
{
    if (g_session1.partyLeader) {
        if (has_ad == 1) {
            ui_button_set_show((struct UIButton *)(self->own_row), 1);
            self->own_row->button.button.disabled = 0;
            ui_window_invalidate_base(&self->own_row->button.button.base);
            ui_button_set_show((struct UIButton *)(self->recruit), 0);
            self->recruit->bmp.button.disabled = 1;
            ui_window_invalidate_base(&self->recruit->bmp.button.base);
            ui_button_set_show((struct UIButton *)(self->booking_off), 1);
            ((struct UIButton *)self->booking_off)->disabled = 0;
            ui_window_invalidate_base(self->booking_off);
        } else {
            ui_button_set_show((struct UIButton *)(self->own_row), 0);
            self->own_row->button.button.disabled = 1;
            ui_window_invalidate_base(&self->own_row->button.button.base);
            ui_button_set_show((struct UIButton *)(self->recruit), 1);
            self->recruit->bmp.button.disabled = 0;
            ui_window_invalidate_base(&self->recruit->bmp.button.base);
            ui_bmp_text_button_set_caption(self->recruit, text_view_cstr(msg_str(MSG_RECRUIT)));
            ui_button_set_show((struct UIButton *)(self->booking_off), 0);
            ((struct UIButton *)self->booking_off)->disabled = 1;
            ui_window_invalidate_base(self->booking_off);
        }
    } else {
        ui_button_set_show((struct UIButton *)(self->own_row), 0);
        self->own_row->button.button.disabled = 1;
        ui_window_invalidate_base(&self->own_row->button.button.base);
        ui_button_set_show((struct UIButton *)(self->recruit), 1);
        self->recruit->bmp.button.disabled = 1;
        ui_window_invalidate_base(&self->recruit->bmp.button.base);
        party_button_caption(self->recruit, "");
    }
    ui_window_invalidate_children(board_wnd(self));
}

void ui_adven_party_board_wnd_set_own_ad(struct UIAdvenPartyBoardWnd *self, const struct AdvenPartyInfo *ad)
{
    if (ui_party_info_wnd_set_ad(self->own_row, ad))
        ui_adven_party_board_wnd_set_has_ad(self, 1);
}

static struct UIBitmapButton *add_bitmap_button(struct UIBitmapButton *button, const char *const bitmaps[4], int x, int y, int id)
{
    int state;

    for (state = UI_BTN_UP; state <= UI_BMP_DISABLED; state++)
        ui_bitmap_button_set_bitmap(button, bitmaps[state], state, 0);
    ui_window_create(&button->button.base, button->bitmap_width, button->bitmap_height);
    ui_window_move_base(&button->button.base, x, y);
    button->button.base.id = id;
    return button;
}

static struct UIBitmapButton *new_bitmap_button(void)
{
    struct UIBitmapButton *button = mem_alloc(sizeof *button);

    return button ? ui_bitmap_button_construct(button) : NULL;
}

static struct UIBmpTextButton *new_bmp_text_button(void)
{
    struct UIBmpTextButton *button = mem_alloc(sizeof *button);

    return button ? ui_bmp_text_button_construct(button) : NULL;
}

static struct UIPartyInfoWnd *new_row(void)
{
    struct UIPartyInfoWnd *row = mem_alloc(sizeof *row);

    return row ? ui_party_info_wnd_construct(row) : NULL;
}

static struct UIWindow *new_level_box(struct UIAdvenPartyBoardWnd *self, int level, int x)
{
    struct UIWindow *box = mem_alloc(sizeof(struct UIEditCtrl));

    if (box)
        box = ui_edit_ctrl2_construct(box);
    ui_window_create(box, 0x19, 0xe);
    ((struct UIEditCtrl *)box)->multiline = 1;
    UI_EDIT_MAX_CHARS(box) = 3;
    ui_edit_ctrl_set_text_colour(box, 0, 0, 0, -1);
    party_level_text(box, level);
    ui_window_move(box, x, 0x18);
    ui_edit_ctrl_set_back_colour(box, 0xf2, 0xf2, 0xf2);
    ui_window_set_show(box, 1);
    ui_window_add_child(board_wnd(self), box);
    return box;
}

void ui_adven_party_board_wnd_on_create(struct UIWindow *window, int width_unused, int height_unused)
{
    struct UIAdvenPartyBoardWnd *self = (struct UIAdvenPartyBoardWnd *)window;
    static const int role_at[4][2] = { { 0xa0, 0x17 }, { 0xb2, 0x17 }, { 0xc4, 0x17 }, { 0xd6, 0x17 } };
    static const char *const search_bmp[4] = {
        AGENCY "btn_search.bmp", AGENCY "btn_search_over.bmp", AGENCY "btn_search_press.bmp",
        AGENCY "btn_search_disable.bmp",
    };
    static const char *const reset_bmp[4] = {
        AGENCY "btn_initialize.bmp", AGENCY "btn_initialize_over.bmp", AGENCY "btn_initialize_press.bmp",
        AGENCY "btn_initialize_disable.bmp",
    };
    static const char *const refresh_bmp[4] = {
        AGENCY "btn_refresh.bmp", AGENCY "btn_refresh_over.bmp", AGENCY "btn_refresh_press.bmp",
        AGENCY "btn_refresh_disable.bmp",
    };
    static const char *const left_bmp[4] = {
        AGENCY "btn_list_left.bmp", AGENCY "btn_list_left.bmp", AGENCY "btn_list_left.bmp",
        AGENCY "btn_list_left_disable.bmp",
    };
    static const char *const right_bmp[4] = {
        AGENCY "btn_list_right.bmp", AGENCY "btn_list_right.bmp", AGENCY "btn_list_right.bmp",
        AGENCY "btn_list_right_disable.bmp",
    };
    static const char *const row_bmp[4] = {
        AGENCY "btn_list.bmp", AGENCY "btn_list_over.bmp", AGENCY "btn_list_press.bmp",
        AGENCY "btn_list_disable.bmp",
    };
    static const char *const own_bmp[4] = {
        AGENCY "btn_recruit_on.bmp", AGENCY "btn_recruit_on.bmp", AGENCY "btn_recruit_press.bmp",
        AGENCY "btn_recruit_disable.bmp",
    };
    static const char *const recruit_bmp[4] = {
        AGENCY "btn_recruit.bmp", AGENCY "btn_recruit.bmp", AGENCY "btn_recruit_press.bmp",
        AGENCY "btn_recruit_disable.bmp",
    };
    struct UIWindow *wnd = board_wnd(self);
    int level = g_session1.charBaseLevel;
    struct UIEditCtrl4 *search;
    struct UIBmpTextButton *text_button;
    struct UIBasicButton *close, *off;
    char pages[0x400];
    int i, w;

    self->min_level = new_level_box(self, level - PARTY_LEVEL_RANGE < 1 ? 1 : level - PARTY_LEVEL_RANGE, 0xa);
    self->max_level = new_level_box(self, level + PARTY_LEVEL_RANGE, 0x2e);

    party_make_job_slots(wnd, self->roles, role_at, 0);

    search = mem_alloc(sizeof *search);
    if (search)
        search = ui_edit_ctrl5_construct(search);
    self->search_text = &search->edit.base;
    ui_window_create(self->search_text, 0x74, 0xe);
    UI_EDIT_MAX_CHARS(self->search_text) = 0x96;
    ui_edit_ctrl_set_text_colour(self->search_text, 0, 0, 0, -1);
    ((struct UIEditCtrl4 *)self->search_text)->use_alt = 1;
    ui_edit_ctrl4_set_empty_text(self->search_text, msg_str(MSG_SEARCH_EMPTY));
    ((struct UIEditCtrl4 *)self->search_text)->alt_colour = SEARCH_EMPTY_COLOUR;
    ui_window_move_base(self->search_text, 0xee, 0x18);
    ui_edit_ctrl_set_back_colour(self->search_text, 0xff, 0xff, 0xff);
    ui_window_add_child(wnd, self->search_text);

    text_button = new_bmp_text_button();
    add_bitmap_button(&text_button->bmp, search_bmp, 0x162, 0x16, BOARD_ID_SEARCH);
    party_button_caption(text_button, msg_str(MSG_SEARCH));
    ui_window_add_child(wnd, &text_button->bmp.button.base);

    text_button = new_bmp_text_button();
    add_bitmap_button(&text_button->bmp, reset_bmp, 0x191, 0x16, BOARD_ID_RESET);
    party_button_caption(text_button, msg_str(MSG_RESET));
    ui_window_add_child(wnd, &text_button->bmp.button.base);

    self->refresh = &add_bitmap_button(new_bitmap_button(), refresh_bmp, 0x1c4, 0x16, BOARD_ID_REFRESH)->button.base;
    ui_window_add_child(wnd, self->refresh);
    self->prev_page = add_bitmap_button(new_bitmap_button(), left_bmp, 0xc7, PAGE_LINE_Y, BOARD_ID_PREV_PAGE);
    ui_window_add_child(wnd, &self->prev_page->button.base);
    self->next_page = add_bitmap_button(new_bitmap_button(), right_bmp, 0x109, PAGE_LINE_Y, BOARD_ID_NEXT_PAGE);
    ui_window_add_child(wnd, &self->next_page->button.base);

    for (i = 0; i < 10; i++) {
        struct UIPartyInfoWnd *row = new_row();

        self->rows[i] = row;
        add_bitmap_button(&row->button, row_bmp, 1, ROW_FIRST_Y + ROW_STEP * i, BOARD_ID_ROW);
        row->button.button.disabled = 1;
        ui_window_invalidate_base(&row->button.button.base);
        row->button.button.cmd_value = i;
        ui_window_add_child(wnd, &row->button.button.base);
    }

    memset(pages, 0, sizeof pages);
    _snprintf_s(pages, sizeof pages, (size_t)-1, "%d / %d", self->page, self->pages);

    self->own_row = new_row();
    add_bitmap_button(&self->own_row->button, own_bmp, 1, 0x31, BOARD_ID_OWN_AD);
    ui_button_set_show((struct UIButton *)(self->own_row), 0);
    ui_window_add_child(wnd, &self->own_row->button.button.base);

    self->recruit = new_bmp_text_button();
    add_bitmap_button(&self->recruit->bmp, recruit_bmp, 1, 0x31, BOARD_ID_RECRUIT);
    party_button_caption(self->recruit, msg_str(MSG_RECRUIT));
    ui_button_set_show((struct UIButton *)(self->recruit), 0);
    ui_window_add_child(wnd, &self->recruit->bmp.button.base);

    self->purpose_combo = party_make_purpose_combo(wnd, 0x4c, 0x16, 0x4c);

    self->page_text = mem_alloc(sizeof(struct UIStaticText));
    if (self->page_text)
        self->page_text = ui_static_text_construct(self->page_text);
    ui_window_create(self->page_text, wnd->width, 0x12);
    w = ui_window_text_width(wnd, "1/1", 0, 0, 0xc, 0, 0);
    ui_window_resize(self->page_text, w + 1, 0x10);
    ui_static_text_set_text(self->page_text, "1/1", 0);
    ui_window_move(self->page_text, wnd->width / 2 - w / 2, PAGE_LINE_Y);
    ui_window_add_child(wnd, self->page_text);

    ui_adven_party_board_wnd_request_page(self, 1);
    UI_FRAME_DEFAULT_ID(&self->frame) = BOARD_ID_SEARCH;

    close = mem_alloc(sizeof *close);
    if (close)
        close = ui_basic_button_construct(close);
    ui_basic_button_set_label(&close->button.base, 0x2a, msg_str(MSG_CLOSE));
    ui_window_move_base(&close->button.base, wnd->width - close->button.base.width - 0xa, BOTTOM_BUTTON_Y);
    close->button.base.id = UI_ID_CLOSE;
    ui_window_add_child(wnd, &close->button.base);

    off = mem_alloc(sizeof *off);
    if (off)
        off = ui_basic_button_construct(off);
    self->booking_off = &off->button.base;
    ui_basic_button_set_label(self->booking_off,
                           ui_window_text_width(wnd, msg_str(MSG_BOOKING_OFF), 0, 0, 0xc, 0, 0) + 0xa,
                           msg_str(MSG_BOOKING_OFF));
    ui_window_move_base(self->booking_off, close->button.base.x - self->booking_off->width - 5, BOTTOM_BUTTON_Y);
    self->booking_off->id = BOARD_ID_BOOKING_OFF;
    ui_window_add_child(wnd, self->booking_off);
    ui_button_set_show((struct UIButton *)(self->booking_off), 0);

    ui_window_add_big_close_button(wnd, "", 0);
}

void ui_adven_party_board_wnd_on_draw(struct UIWindow *window)
{
    struct UIAdvenPartyBoardWnd *self = (struct UIAdvenPartyBoardWnd *)window;
    struct UIWindow *wnd = board_wnd(self);

    ui_window_clear_canvas(wnd, 0xffff00ff);
    ui_window_draw_bitmap(wnd, 0, 0, res_mgr_get(g_resMgr(), ui_skin_path(AGENCY "bg_agencyList.bmp")), 0);
    ui_window_draw_title_bar(wnd, 1, msg_str(MSG_BOARD_TITLE), 0);
    ui_window_draw_button_bar(wnd, 2, 0, 1);
    ui_window_text_out(wnd, 0x24, 0x19, "~", 0, 0, 0xc, 0, 0, 0);
}

void ui_adven_party_board_wnd_store_info(struct UIWindow *window)
{
    struct UIAdvenPartyBoardWnd *self = (struct UIAdvenPartyBoardWnd *)window;
    if (!self->info)
        return;
    self->info->width = self->frame.base.width;
    self->info->height = self->frame.base.height;
    self->info->x = self->frame.base.x;
    self->info->y = self->frame.base.y;
}

static void open_register(const struct AdvenPartyInfo *ad)
{
    struct UIWindow *wnd;

    ui_window_mgr_post_quit1(&g_windowMgr, PARTY_APPLY_WND_ID);
    wnd = ui_window_open(PARTY_REGISTER_WND_ID);
    if (!wnd)
        return;
    if (ad) {
        ui_register_party_wnd_set_ad((struct UIRegisterPartyWnd *)wnd, ad);
    }
    ui_window_mgr_raise_window(&g_windowMgr, wnd);
}

static int board_search(struct UIAdvenPartyBoardWnd *self, int page)
{
    struct String text, arg;
    struct AdvenPartyInfo ad;
    int level, i, sent = 0;

    string_init(&text);
    string_assign_cstr(&text, ui_edit_ctrl_get_text(self->min_level));
    string_copy(&arg, &text);
    if (!party_is_number(arg)) {
        level = g_session1.charBaseLevel - PARTY_LEVEL_RANGE;
        party_level_text(self->min_level, level < 1 ? 1 : level);
    }
    string_assign_cstr(&text, ui_edit_ctrl_get_text(self->max_level));
    string_copy(&arg, &text);
    if (!party_is_number(arg))
        party_level_text(self->max_level, g_session1.charBaseLevel + PARTY_LEVEL_RANGE);

    party_booking_init(&ad);
    for (i = 0; i < 4; i++)
        ad.roles[i] = self->roles[i]->checked;
    if (ui_edit_ctrl_get_text_size(self->min_level) > 0)
        ad.min_level = atoi(ui_edit_ctrl_get_text(self->min_level));
    if (ui_edit_ctrl_get_text_size(self->max_level) > 0)
        ad.max_level = atoi(ui_edit_ctrl_get_text(self->max_level));
    if (self->purpose)
        ad.purpose = self->purpose;
    if (!owned_text_assign(&ad.memo, text_view_cstr(ui_edit_ctrl_get_text(self->search_text)))) goto done;
    if (!owned_text_assign(&ad.name, text_view_cstr(session_character_name(&g_session1)))) goto done;
    sent = adventurer_agency_search(g_adventurerAgency, &ad, page);
    if (sent) {
        self->searching = 1;
        self->page = page;
        button_enable(self->prev_page, 1);
        button_enable(self->next_page, 1);
        button_enable(self->refresh, 1);
    }
done:
    party_booking_clear(&ad);
    string_free(&text);
    return sent;
}

static void board_reset(struct UIAdvenPartyBoardWnd *self)
{
    int level = g_session1.charBaseLevel, i;

    party_level_text(self->min_level, level - PARTY_LEVEL_RANGE < 1 ? 1 : level - PARTY_LEVEL_RANGE);
    party_level_text(self->max_level, level + PARTY_LEVEL_RANGE);
    ui_combo_box_def_wnd_select(self->purpose_combo, 0);
    for (i = 0; i < 4; i++) {
        ui_window_set_state(&self->roles[i]->bmp.button.base, UI_BTN_UP);
        ui_job_slot_button_set_checked(self->roles[i], 0);
    }
    self->searching = 0;
    ui_edit_ctrl_set_text_default((struct UIEditCtrl *)self->search_text, "");
    ui_window_invalidate_children(board_wnd(self));
}

static void board_open_row(struct UIAdvenPartyBoardWnd *self, int index)
{
    struct AdvenPartyInfo *ad = &self->rows[index]->ad;
    struct UIWindow *wnd;

    if (ad->aid == g_session1.ownAid) {
        open_register(ad);
        return;
    }
    ui_window_mgr_post_quit1(&g_windowMgr, PARTY_REGISTER_WND_ID);
    wnd = ui_window_open(PARTY_APPLY_WND_ID);
    if (!wnd)
        return;
    ui_apply_for_party_wnd_set_ad((struct UIApplyForPartyWnd *)wnd, ad);
    ui_window_mgr_raise_window(&g_windowMgr, wnd);
}

static void board_command(struct UIAdvenPartyBoardWnd *self, int id, int val2)
{
    switch (id) {
    case UI_ID_CLOSE:
        ui_window_mgr_post_quit1(&g_windowMgr, PARTY_BOARD_WND_ID);
        break;
    case BOARD_ID_NEXT_PAGE:
        if (self->searching == 1)
            board_search(self, self->page + 1);
        else
            ui_adven_party_board_wnd_request_page(self, self->page + 1);
        break;
    case BOARD_ID_PREV_PAGE:
        if (self->searching == 1)
            board_search(self, self->page - 1);
        else
            ui_adven_party_board_wnd_request_page(self, self->page - 1);
        break;
    case BOARD_ID_RESET:
        board_reset(self);
        break;
    case BOARD_ID_SEARCH:
        board_search(self, 1);
        break;
    case BOARD_ID_ROW:
        board_open_row(self, val2);
        break;
    case BOARD_ID_BOOKING_OFF:
        adventurer_agency_cancel_own_booking(g_adventurerAgency);
        break;
    case BOARD_ID_REFRESH:
        if (ui_adven_party_board_wnd_request_page(self, 1))
            self->searching = 0;
        break;
    case BOARD_ID_OWN_AD:
        open_register(&self->own_row->ad);
        break;
    case BOARD_ID_RECRUIT:
        open_register(NULL);
        break;
    }
}

static void board_refresh(struct UIAdvenPartyBoardWnd *self)
{
    struct UIWindow *wnd = board_wnd(self);
    const struct PartyBookingList *ads = adventurer_agency_parties(g_adventurerAgency);
    char line[0x400];
    int i, half;

    for (i = 0; i < 10; i++)
        ui_party_info_wnd_reset(self->rows[i]);
    for (i = 0; i < 10 && (size_t)i < ads->count; ++i) {
        ui_party_info_wnd_set_ad(self->rows[i], &ads->items[i]);
        ui_window_invalidate_base(&self->rows[i]->button.button.base);
    }
    memset(line, 0, sizeof line);
    _snprintf_s(line, sizeof line, (size_t)-1, "%d / %d", self->page, self->pages);
    half = ui_window_text_width(wnd, line, 0, 0, 0xc, 0, 0);
    ui_window_resize(self->page_text, half + 1, 0x10);
    ui_static_text_set_text(self->page_text, line, 0);
    half /= 2;
    ui_window_move(self->page_text, wnd->width / 2 - half, PAGE_LINE_Y);
    ui_window_move_base(&self->prev_page->button.base, wnd->width / 2 - self->prev_page->button.base.width - half - 3, PAGE_LINE_Y);
    ui_window_move_base(&self->next_page->button.base, wnd->width / 2 + 3 + half, PAGE_LINE_Y);
    ui_adven_party_board_wnd_set_has_ad(self, adventurer_agency_is_recruiting(g_adventurerAgency));
    ui_adven_party_board_wnd_send_msg(&self->frame.base, NULL, BOARD_MSG_PAGE_BUTTONS, 0, 0, 0, 0);
    ui_window_invalidate_base(&self->frame.base);
    ui_window_invalidate_children(wnd);
}

static void board_page_buttons(struct UIAdvenPartyBoardWnd *self)
{
    if (self->page > self->pages)
        self->page = self->pages;
    button_enable(self->next_page, self->page >= self->pages);
    button_enable(self->prev_page, self->page == 1);
    ui_window_invalidate_base(&self->frame.base);
    if (g_session1.partyLeader == 0)
        ui_window_set_state(&self->own_row->button.button.base, 3);
    button_enable(self->refresh, 0);
}

int ui_adven_party_board_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                            int val1, int val2, int val3, int val4)
{
    struct UIAdvenPartyBoardWnd *self = (struct UIAdvenPartyBoardWnd *)window;
    switch (msg) {
    case UI_MSG_COMMAND:
        board_command(self, val1, val2);
        return 0;
    case UI_MSG_SET_INFO:
        self->info = (struct WindowInfo *)val1;
        if (!ui_info_on_screen(self->info)) {
            self->info->x = 0xc8;
            self->info->y = 0xc8;
            self->info->width = 0x1e0;
            self->info->height = 0x154;
        }
        ui_frame_wnd_move(&self->frame.base, self->info->x, self->info->y);
        return 0;
    case UI_MSG_COMBO_CHOSE:
        if (sender == self->purpose_combo)
            self->purpose = val1;
        return 0;
    case BOARD_MSG_REFRESH:
        board_refresh(self);
        return 0;
    case BOARD_MSG_PAGE_BUTTONS:
        board_page_buttons(self);
        return 0;
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
}

const char *party_purpose_name(int purpose)
{
    if ((unsigned)purpose > PURPOSE_COUNT - 1)
        return "";
    return msg_str(purpose == 0 ? MSG_PURPOSE_ANY : MSG_PURPOSE_FIRST + purpose - 1);
}
