#include "contents/pet_evolution.h"
#include "ui/controls/edit.h"
#include "render/rp_pool.h"
#include "contents/manner_filter.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "core/object_type_tables.h"
#include "core/object_type.h"
#include "game/context.h"
#include "ui/window_access.h"
#include "core/markup_tags.h"
#include "ui/items/items.h"
#include "ui/items/pet_windows.h"
#include <mbstring.h>
#include <stdlib.h>
#include "core/msg_str.h"
#include "core/skin_path.h"
#include "core/text_util.h"
#include "diag/perf.h"
#include "session/session_items.h"
#include "session/session_jobs.h"
#include "session/session_names.h"
#include "session/session_state.h"
#include "session/session_talk.h"

#define UI_WND_PET_INFO 0x58
#define UI_WND_PET_MENU 0x104
#define UI_WND_PET_EVOLUTION 0x105

#define PET_ID_RENAME UI_ID_OK
#define PET_ID_COMMAND 0x104
#define PET_ID_FEED_ANSWER 0x1e0
#define PET_FEED_YES 0xb3

#define UI_MSG_MENU_ADD 0x1f
#define UI_MSG_MENU_OWNER 0x53
#define UI_MSG_PET_MENU 0x27
#define PET_MENU_FEED 0x1cb
#define PET_MENU_PERFORM 0x1cc
#define PET_MENU_UNEQUIP 0x1cd
#define PET_MENU_RETURN_TO_EGG 0x1ce
#define PET_MENU_EVOLVE 0x1cf
#define PET_MENU_AUTO_FEED 0x1d0
#define PET_MENU_ROW_HEIGHT 16
#define PET_MENU_WIDTH 0x6e

#define UI_MSG_PET_EVOLUTION_EGGS 0x8e

#define REQ_PET_COMMAND 0x96
#define PET_COMMAND_FEED 1
#define PET_COMMAND_PERFORM 2
#define PET_COMMAND_RETURN_TO_EGG 3
#define PET_COMMAND_UNEQUIP 4
#define REQ_PET_RENAME 0x91

#define PET_NAME_MAX 0x18
#define PET_EVOLVE_MIN_INTIMACY 900
#define PET_INFO_MSG_EXTRA_COLOUR 0xff

enum {
    MSG_PET_NAME = 0x197, MSG_PET_LEVEL = 0x198,
    MSG_PET_TITLE = 0x248, MSG_PET_FULLNESS = 0x249, MSG_PET_INTIMACY = 0x24a,
    MSG_PET_MENU_FEED = 0x250, MSG_PET_MENU_PERFORM = 0x251, MSG_PET_MENU_RETURN = 0x252,
    MSG_PET_MENU_UNEQUIP = 0x253,
    MSG_PET_ACCESSORY = 0x255, MSG_PET_ACCESSORY_ON = 0x256, MSG_PET_ACCESSORY_OFF = 0x258,
    MSG_PET_FEED_QUESTION = 0x259,
    MSG_PET_FULL_0 = 0x29b, MSG_PET_FULL_1 = 0x29c, MSG_PET_FULL_2 = 0x29d, MSG_PET_FULL_3 = 0x29e,
    MSG_PET_FULL_4 = 0x29f,
    MSG_PET_INTIMACY_1 = 0x2a0, MSG_PET_INTIMACY_2 = 0x2a1, MSG_PET_INTIMACY_4 = 0x2a2,
    MSG_PET_INTIMACY_5 = 0x2a3, MSG_PET_OUT_OF_RANGE = 0x2a4,
    MSG_PET_INTIMACY_LOW_A = 0x3fe, MSG_PET_INTIMACY_LOW_B = 0x3fd,
    MSG_PET_NAME_TOO_LONG = 0x2aa, MSG_PET_EVOLVE_LIST = 0xa08, MSG_PET_EVOLVE_REFUSED = 0xa10,
    MSG_PET_AUTO_FEED = 0xa11, MSG_PET_NAME_HAS_LINK = 0xafc, MSG_PET_RENAME_QUESTION = 0xb73,
    MSG_PET_NAME_FILTERED = 0xb84, MSG_PET_MAIL_OPEN = 0xba9, MSG_PET_FEED_REFUSED = 0xdde,
};
#define PET_RENAME_KEEP 0xb4

struct UIPetInfoBarGraph {
    struct UIWindow base;
    unsigned percent;
};
OFFSET_ASSERT(UIPetInfoBarGraph, percent, 0x7c);
SIZE_ASSERT(UIPetInfoBarGraph, 0x80);

void ui_pet_info_bar_graph_destroy(struct UIWindow *window);
void ui_pet_info_bar_graph_on_draw(struct UIWindow *self);
const struct UIWindowOps ui_pet_info_bar_graph_ops = {
    .destroy = ui_pet_info_bar_graph_destroy,
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
    .on_draw = ui_pet_info_bar_graph_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_window_on_l_btn_down_base,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_window_on_mouse_move_base,
    .on_mouse_hover = ui_window_on_mouse_hover_base,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_window_on_l_btn_up_base,
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
};

struct UIPetInfoBarGraph *ui_pet_info_bar_graph_construct(struct UIPetInfoBarGraph *self)
{
    ui_window_construct(&self->base, 0);
    self->base.ops = &ui_pet_info_bar_graph_ops;
    self->percent = 0;
    return self;
}

void ui_pet_info_bar_graph_destruct(struct UIPetInfoBarGraph *self)
{
    self->base.ops = &ui_pet_info_bar_graph_ops;
    ui_window_destruct(&self->base);
}

void ui_pet_info_bar_graph_destroy(struct UIWindow *window)
{
    struct UIPetInfoBarGraph *self = (struct UIPetInfoBarGraph *)window;
    ui_pet_info_bar_graph_destruct(self);
    mem_free(self);
}

static void ui_pet_info_bar_graph_set_percent(struct UIPetInfoBarGraph *self, int percent)
{
    self->percent = percent;
    ui_window_invalidate_base(&self->base);
}

static struct BitmapRes *skin_bitmap(const char *path)
{
    return res_mgr_get(g_resMgr(), ui_skin_path(path));
}

void ui_pet_info_bar_graph_on_draw(struct UIWindow *window)
{
    struct UIPetInfoBarGraph *self = (struct UIPetInfoBarGraph *)window;
    struct BitmapRes *left, *mid, *right;
    int inner, run, i;

    left = skin_bitmap(UI_SKIN "basic_interface\\gze_bg_left.bmp");
    mid = skin_bitmap(UI_SKIN "basic_interface\\gze_bg_mid.bmp");
    right = skin_bitmap(UI_SKIN "basic_interface\\gze_bg_right.bmp");
    inner = self->base.width - left->width - right->width;
    ui_window_draw_bitmap(&self->base, 0, 0, left, 0);
    for (i = 0; i < inner; i++)
        ui_window_draw_bitmap(&self->base, left->width + i, 0, mid, 0);
    ui_window_draw_bitmap(&self->base, self->base.width - right->width, 0, right, 0);

    if (self->percent == 0)
        return;
    left = skin_bitmap(UI_SKIN "basic_interface\\gzeblue_left.bmp");
    mid = skin_bitmap(UI_SKIN "basic_interface\\gzeblue_mid.bmp");
    right = skin_bitmap(UI_SKIN "basic_interface\\gzeblue_right.bmp");
    inner = self->base.width - right->width - left->width;
    run = (int)((float)inner * ((float)(double)self->percent / 100.0f));
    ui_window_draw_bitmap(&self->base, 0, 0, left, 1);
    for (i = 0; i < run; i++)
        ui_window_draw_bitmap(&self->base, left->width + i, 0, mid, 1);
    ui_window_draw_bitmap(&self->base, mid->width * i + left->width, 0, right, 1);
}

struct UIPetInfoWnd {
    struct UIFrameWnd frame;
    struct UIWindow *name_edit;
    struct WindowInfo *info;
    struct UIBitmapButton *rename;
    struct UIWindow *command_box;
    struct UIWindow *feed_box;
    struct UIPetInfoBarGraph *intimacy_bar;
    struct String intimacy_text;
};
OFFSET_ASSERT(UIPetInfoWnd, name_edit, 0x94);
OFFSET_ASSERT(UIPetInfoWnd, feed_box, 0xa4);
OFFSET_ASSERT(UIPetInfoWnd, intimacy_text, 0xac);

void ui_pet_info_wnd_destroy(struct UIWindow *window);
void ui_pet_info_wnd_store_info(struct UIWindow *self);
void ui_pet_info_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_pet_info_wnd_on_draw(struct UIWindow *self);
int ui_pet_info_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_pet_info_wnd_ops = {
    .destroy = ui_pet_info_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_pet_info_wnd_store_info,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_pet_info_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_pet_info_wnd_on_draw,
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
    .send_msg = ui_pet_info_wnd_send_msg,
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

struct UIPetInfoWnd *ui_pet_info_wnd_construct(struct UIPetInfoWnd *self)
{
    ui_window_construct(&self->frame.base, 0);
    self->frame.base.ops = &ui_frame_wnd_ops;
    self->frame.press_x = 0;
    self->frame.press_y = 0;
    self->frame.default_id = 0x239;
    self->frame.cancel_id = 0x239;
    self->frame.base.ops = &ui_pet_info_wnd_ops;
    self->intimacy_bar = NULL;
    string_init(&self->intimacy_text);
    self->name_edit = NULL;
    self->info = NULL;
    self->rename = NULL;
    self->command_box = NULL;
    self->feed_box = NULL;
    return self;
}

struct UIWindow *ui_pet_info_wnd_new(void)
{
    return (struct UIWindow *)ui_pet_info_wnd_construct(mem_alloc(sizeof(struct UIPetInfoWnd)));
}

void ui_pet_info_wnd_destruct(struct UIPetInfoWnd *self)
{
    self->frame.base.ops = &ui_pet_info_wnd_ops;
    if (self->feed_box)
        ui_window_mgr_post_quit2(&g_windowMgr, self->feed_box);
    string_free(&self->intimacy_text);
    self->frame.base.ops = &ui_frame_wnd_ops;
    ui_window_destruct(&self->frame.base);
}

void ui_pet_info_wnd_destroy(struct UIWindow *window)
{
    struct UIPetInfoWnd *self = (struct UIPetInfoWnd *)window;
    ui_pet_info_wnd_destruct(self);
    mem_free(self);
}

void ui_pet_info_wnd_update_intimacy(struct UIPetInfoWnd *self)
{
    struct String before;
    int value = g_session1.petIntimacy;
    int lo, hi, msg;

    string_copy(&before, &self->intimacy_text);
    if (value >= 0 && value <= 100) {
        if (session_get_drawn_job(&g_session1) >= 4050 && session_get_drawn_job(&g_session1) <= 4052) {
            if (value <= 20) {
                msg = MSG_PET_INTIMACY_LOW_A, lo = 0, hi = 100;
            } else if (value <= 40) {
                msg = MSG_PET_INTIMACY_LOW_B, lo = 20, hi = 40;
            } else {
                msg = MSG_PET_INTIMACY_1, lo = 40, hi = 100;
            }
        } else {
            msg = MSG_PET_INTIMACY_1, lo = 0, hi = 100;
        }
    } else if (value > 100 && value <= 250) {
        msg = MSG_PET_INTIMACY_2, lo = 100, hi = 250;
    } else if (value > 250 && value <= 750) {
        msg = MSG_PET_FULL_2, lo = 250, hi = 750;
    } else if (value > 750 && value <= 900) {
        msg = MSG_PET_INTIMACY_4, lo = 750, hi = 900;
    } else if (value > 900 && value <= 1000) {
        msg = MSG_PET_INTIMACY_5, lo = 900, hi = 1000;
    } else {
        msg = MSG_PET_OUT_OF_RANGE, lo = 0, hi = 0;
    }
    {
        const char *word = msg_str(msg);

        string_assign_cstr(&self->intimacy_text, word);
    }
    ui_pet_info_bar_graph_set_percent(self->intimacy_bar,
                                 (int)((float)(double)(unsigned)(value - lo) / (float)(double)(unsigned)(hi - lo) *
                                       100.0f));
    if (!string_equal(&before, &self->intimacy_text))
        ui_window_invalidate_base(&self->frame.base);
    string_free(&before);
}

void ui_pet_info_update_intimacy(struct UIWindow *wnd)
{
    ui_pet_info_wnd_update_intimacy((struct UIPetInfoWnd *)wnd);
}

void ui_pet_info_set_name(struct UIWindow *wnd, const char *name)
{
    ui_edit_ctrl_set_text((struct UIEditCtrl *)(((struct UIPetInfoWnd *)wnd)->name_edit), name);
}

void ui_pet_info_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UIPetInfoWnd *self = (struct UIPetInfoWnd *)window;
    static const char *const arrow[3] = { "txtbox_btn_a", "txtbox_btn_b", "txtbox_btn_c" };
    char tooltip[12] = "Pet Command";
    char path[MAX_PATH];
    struct UIWindow *edit;
    struct UIBitmapButton *arrow_btn;
    struct UIPetInfoBarGraph *bar;
    const char *text;
    int state;

    ui_window_add_close_button(&self->frame.base, "", 0);

    edit = mem_alloc(sizeof(struct UIEditCtrl));
    if (edit)
        edit = ui_edit_ctrl2_construct(edit);
    self->name_edit = edit;
    ui_window_create(edit, 0x50, 0x10);
    if (strcmp(g_session1.petName, "") == 0)
        _snprintf_s(g_session1.petName, sizeof g_session1.petName, _TRUNCATE, "%s",
                    session_get_job_name(&g_session1, g_session1.petJob, -1));
    ui_window_move(self->name_edit, 0x87, 0x18);
    ui_edit_ctrl_set_text((struct UIEditCtrl *)self->name_edit, g_session1.petName);
    ((struct UIEditCtrl *)self->name_edit)->max_chars = 0x32;
    ui_edit_ctrl_set_back_colour(self->name_edit, 0xe8, 0xe8, 0xe8);
    ui_window_add_child(&self->frame.base, self->name_edit);

    self->rename = ui_add_skin_button(&self->frame.base, "btn_rewrite", 0xe6, 0x15, PET_ID_RENAME);

    self->command_box = mem_alloc(sizeof(struct UINoneVisibleButton));
    if (self->command_box)
        self->command_box = (struct UIWindow *)ui_none_visible_button_construct(
            (struct UINoneVisibleButton *)self->command_box);
    ui_window_create(self->command_box, 0x78, 0x12);
    ui_window_move_base(self->command_box, 0x96, 0x2c);
    self->command_box->id = PET_ID_COMMAND;
    ui_window_add_child(&self->frame.base, self->command_box);

    arrow_btn = mem_alloc(sizeof(struct UIBitmapButton));
    if (arrow_btn)
        arrow_btn = ui_bitmap_button_construct(arrow_btn);
    for (state = UI_BTN_UP; state <= UI_BTN_DOWN; state++) {
        _snprintf(path, sizeof path, UI_SKIN "basic_interface\\%s.bmp", arrow[state]);
        ui_bitmap_button_set_bitmap(arrow_btn, path, state, 0);
    }
    ui_window_create(&arrow_btn->button.base, arrow_btn->bitmap_width, arrow_btn->bitmap_height);
    ui_window_move_base(&arrow_btn->button.base, 0x10e - arrow_btn->bitmap_width, 0x2c);
    ui_bitmap_button_set_tool_tip(arrow_btn, tooltip);
    arrow_btn->button.base.id = PET_ID_COMMAND;
    ui_window_add_child(&self->frame.base, &arrow_btn->button.base);

    text = msg_str(MSG_PET_OUT_OF_RANGE);
    string_assign_cstr(&self->intimacy_text, text);

    bar = mem_alloc(sizeof *bar);
    if (bar)
        bar = ui_pet_info_bar_graph_construct(bar);
    self->intimacy_bar = bar;
    ui_window_create(&bar->base, 0x78, 9);
    ui_window_move_base(&self->intimacy_bar->base, 0x91, 0x69);
    ui_window_add_child(&self->frame.base, &self->intimacy_bar->base);

    ui_pet_info_wnd_update_intimacy(self);
    UI_FRAME_DEFAULT_ID(self) = UI_ID_CLOSE;
}

static void draw_label(struct UIPetInfoWnd *self, int x, int y, const char *text)
{
    ui_window_text_out(&self->frame.base, x, y, text, 0, 0, 12, 0, 0, 0);
}

void ui_pet_info_wnd_on_draw(struct UIWindow *window)
{
    struct UIPetInfoWnd *self = (struct UIPetInfoWnd *)window;
    char illust[MAX_PATH + 4];
    char command[12];
    char title[0x20];
    char level[8];
    char fullness[0x18];
    struct BitmapRes *res;
    int top, msg, f, job;

    ui_window_clear_canvas(&self->frame.base, ui_skin_colour(2, 2));
    top = ui_window_draw_title_bar(&self->frame.base, 0, "", 0);
    ui_window_draw_body(&self->frame.base, top, 0, "");
    res = skin_bitmap(UI_SKIN "sysboxs_ld.bmp");
    ui_window_draw_bitmap(&self->frame.base, 0, self->frame.base.height - res->height, res, 0);
    res = skin_bitmap(UI_SKIN "sysboxs_rd.bmp");
    ui_window_draw_bitmap(&self->frame.base, self->frame.base.width - res->width,
                        self->frame.base.height - res->height, res, 0);

    _snprintf_s(title, sizeof title, (size_t)-1, "%s", msg_str(MSG_PET_TITLE));
    draw_label(self, 5, 2, title);
    draw_label(self, 0x64, 0x18, msg_str(MSG_PET_NAME));
    draw_label(self, 0x64, 0x2c, msg_str(MSG_PET_LEVEL));
    draw_label(self, 0x64, 0x45, msg_str(MSG_PET_FULLNESS));
    draw_label(self, 0x64, 0x59, msg_str(MSG_PET_INTIMACY));

    f = g_session1.petFullness;
    if (f >= 0 && f <= 10)
        msg = MSG_PET_FULL_0;
    else if (f > 10 && f <= 25)
        msg = MSG_PET_FULL_1;
    else if (f > 25 && f <= 75)
        msg = MSG_PET_FULL_2;
    else if (f > 75 && f <= 90)
        msg = MSG_PET_FULL_3;
    else if (f > 90 && f <= 100)
        msg = MSG_PET_FULL_4;
    else
        msg = MSG_PET_OUT_OF_RANGE;
    strcpy(fullness, msg_str(msg));
    _itoa(g_session1.petLevel, level, 10);
    draw_label(self, 0x87, 0x2c, level);
    draw_label(self, 0x91, 0x45, fullness);
    draw_label(self, 0x91, 0x59, string_cstr(&self->intimacy_text));

    job = g_session1.petJob;
    if (job == -1)
        return;
    if (!(job > 1000 && job < 0xf9f) && (unsigned)(job - 0x4e35) > 0x2eca)
        return;
    memset(illust, 0, 0x104);
    session_get_pet_illust_name(&g_session1, job, illust, sizeof illust);
    res = object_as_type(res_mgr_get(g_resMgr(), illust), OBJECT_BITMAP_RESOURCE);
    if (res)
        ui_window_draw_bitmap(&self->frame.base, 5, 0x15, res, 0);

    memcpy(command, "< Command >", sizeof command);
    ui_window_fill_rect(&self->frame.base, 0xa0, 0x2c, 0x6e, 0x12, 0xfae6e6);
    draw_label(self, 0xce - ui_window_text_width(&self->frame.base, command, 0, 0, 12, 0, 0) / 2, 0x30,
               command);
    draw_label(self, 0x64, 0x77, msg_str(MSG_PET_ACCESSORY));
    draw_label(self, 0x64, 0x8b, msg_str(g_session1.petAccessory ? MSG_PET_ACCESSORY_ON : MSG_PET_ACCESSORY_OFF));
}

void ui_pet_info_wnd_store_info(struct UIWindow *window)
{
    struct UIPetInfoWnd *self = (struct UIPetInfoWnd *)window;
    if (!self->info)
        return;
    self->info->x = self->frame.base.x;
    self->info->y = self->frame.base.y;
    self->info->width = self->frame.base.width;
    self->info->height = self->frame.base.height;
}

static void chat_refusal(int msg)
{
    ui_window_mgr_send_msg_(&g_windowMgr, 1, (int)msg_str(msg), PET_INFO_MSG_EXTRA_COLOUR, 0, 0);
}

static int message_box(const char *text, int type, int arg3)
{
    return ui_window_mgr_message_box(&g_windowMgr, text, type, arg3, 0, 0, "Message", 0x118, 0x78, 0);
}

static void pet_rename(struct UIPetInfoWnd *self)
{
    char question[0x100];

    if (strlen(ui_edit_ctrl_get_text(self->name_edit)) >= PET_NAME_MAX) {
        message_box(msg_str(MSG_PET_NAME_TOO_LONG), 0, 1);
        return;
    }
    if (_mbsstr((const unsigned char *)ui_edit_ctrl_get_text(self->name_edit),
                (const unsigned char *)string_cstr(&text_tag_ITEML)) ||
        _mbsstr((const unsigned char *)ui_edit_ctrl_get_text(self->name_edit),
                (const unsigned char *)string_cstr(&text_tag_ITEML_END))) {
        message_box(msg_str(MSG_PET_NAME_HAS_LINK), 0, 1);
        return;
    }
    sprintf(question, msg_str(MSG_PET_RENAME_QUESTION), ui_edit_ctrl_get_text(self->name_edit));
    if (message_box(question, 2, 0) != PET_RENAME_KEEP) {
        if (word_filter_contains(&g_nameFilter, ui_edit_ctrl_get_text(self->name_edit)) != 1) {
            mode_send_message(game_current_base_mode(), REQ_PET_RENAME,
                          (int)ui_edit_ctrl_get_text(self->name_edit), 0, 0, 0);
            _snprintf_s(g_session1.petName, sizeof g_session1.petName, _TRUNCATE, "%s",
                        ui_edit_ctrl_get_text(self->name_edit));
            ui_window_invalidate_base(&self->frame.base);
            return;
        }
        message_box(msg_str(MSG_PET_NAME_FILTERED), 0, 1);
    }
    ui_edit_ctrl_set_text((struct UIEditCtrl *)self->name_edit, g_session1.petName);
    ui_window_invalidate(self->name_edit);
}

static void menu_add(struct UIWindow *menu, const char *text, int id, int value)
{
    ui_window_send_msg(menu, NULL, UI_MSG_MENU_ADD, (int)text, id, value, 0);
}

static void pet_open_menu(struct UIPetInfoWnd *self)
{
    struct UIWindow *menu = ui_window_open(UI_WND_PET_MENU);
    struct String feed, perform, back, unequip;
    struct ItemInfo egg;
    struct PetFoundList evolutions;
    int egg_id, rows, x, y;

    string_init(&feed);
    string_init(&perform);
    string_init(&back);
    string_init(&unequip);
    string_assign_cstr(&feed, msg_str(MSG_PET_MENU_FEED));
    string_assign_cstr(&perform, msg_str(MSG_PET_MENU_PERFORM));
    string_assign_cstr(&back, msg_str(MSG_PET_MENU_RETURN));
    string_assign_cstr(&unequip, msg_str(MSG_PET_MENU_UNEQUIP));
    menu_add(menu, string_cstr(&feed), PET_MENU_FEED, 0);
    menu_add(menu, string_cstr(&perform), PET_MENU_PERFORM, 0);
    menu_add(menu, string_cstr(&unequip), PET_MENU_UNEQUIP, 0);
    menu_add(menu, string_cstr(&back), PET_MENU_RETURN_TO_EGG, 0);

    egg_id = item_info_get_item_id(session_get_inventory_item(&g_session1, &egg, g_session1.hatchedEggIndex));
    item_info_destruct(&egg);
    pet_evolution_targets(g_petTable, &evolutions, egg_id);
    if (evolutions.found == 1) {
        int *it;

        for (it = evolutions.list.first; it != evolutions.list.last; it++) {
            struct String line, tmp;
            int job = session_get_pet_job_by_egg(&g_session1, *it);
            const char *name = session_get_pet_string(&g_session1, job);

            string_init(&line);
            if (!name)
                name = session_get_job_name(&g_session1, job, -1);
            string_init(&tmp);
            string_format(&tmp, msg_str(MSG_PET_EVOLVE_LIST), name);
            string_assign(&line, string_cstr(&tmp), tmp.size);
            string_free(&tmp);
            menu_add(menu, string_cstr(&line), PET_MENU_EVOLVE, *it);
            string_free(&line);
        }
    }
    rows = (int)VEC_COUNT(&evolutions.list, int) + 4;
    if (pet_evolution_is_auto_feeding(g_petTable, egg_id)) {
        struct String line;

        string_init(&line);
        string_assign_cstr(&line, msg_str(MSG_PET_AUTO_FEED));
        menu_add(menu, string_cstr(&line), PET_MENU_AUTO_FEED, 0);
        rows++;
        string_free(&line);
    }
    ui_window_send_msg(menu, NULL, COMBO_MSG_SELECT, 0, 0, 0, 0);
    ui_window_resize(menu, PET_MENU_WIDTH, rows * PET_MENU_ROW_HEIGHT);
    ui_window_send_msg(menu, NULL, UI_MSG_MENU_OWNER, UI_WND_PET_INFO, 0, 0, 0);
    x = 0xa0;
    y = 0x2c;
    ui_window_to_screen(&self->frame.base, &x, &y);
    ui_window_move(menu, x, y);
    vec_free(&evolutions.list);
    string_free(&unequip);
    string_free(&back);
    string_free(&perform);
    string_free(&feed);
}

static void pet_menu_choice(struct UIPetInfoWnd *self, int choice, int value)
{
    struct Mode *mode = game_current_base_mode();

    switch (choice) {
    case PET_MENU_FEED:
        if (g_windowMgr.rodex_compose_wnd) {
            chat_refusal(MSG_PET_MAIL_OPEN);
            break;
        }
        self->feed_box = ui_normal_mb_new();
        ui_normal_mb_create(self->feed_box, &self->frame.base, msg_str(MSG_PET_FEED_QUESTION), 2, 1, 0, 0x118,
                          0x78, 0);
        ui_window_mgr_add_window(&g_windowMgr, self->feed_box);
        break;
    case PET_MENU_PERFORM:
        mode_send_message(mode, REQ_PET_COMMAND, PET_COMMAND_PERFORM, 0, 0, 0);
        break;
    case PET_MENU_UNEQUIP:
        mode_send_message(mode, REQ_PET_COMMAND, PET_COMMAND_UNEQUIP, 0, 0, 0);
        break;
    case PET_MENU_RETURN_TO_EGG:
        mode_send_message(mode, REQ_PET_COMMAND, PET_COMMAND_RETURN_TO_EGG, 0, 0, 0);
        break;
    case PET_MENU_EVOLVE:
        if (g_session1.petIntimacy > PET_EVOLVE_MIN_INTIMACY) {
            struct UIWindow *evolution;
            struct ItemInfo egg;
            int egg_id;

            ui_window_mgr_post_quit1(&g_windowMgr, UI_WND_PET_MENU);
            evolution = ui_window_open(UI_WND_PET_EVOLUTION);
            egg_id = item_info_get_item_id(session_get_inventory_item(&g_session1, &egg, g_session1.hatchedEggIndex));
            item_info_destruct(&egg);
            ui_window_send_msg(evolution, NULL, UI_MSG_PET_EVOLUTION_EGGS, egg_id, value, 0, 0);
            ui_window_send_msg(&self->frame.base, NULL, UI_MSG_COMMAND, UI_ID_CLOSE, 0, 0, 0);
        } else {
            chat_refusal(MSG_PET_EVOLVE_REFUSED);
        }
        break;
    case PET_MENU_AUTO_FEED:
        session_send_pet_auto_feed(&g_session1, g_session1.petAutoFeed == 1 ? 0 : 1);
        break;
    }
    ui_window_mgr_post_quit1(&g_windowMgr, UI_WND_PET_MENU);
}

int ui_pet_info_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1, int val2,
                                    int val3, int val4)
{
    struct UIPetInfoWnd *self = (struct UIPetInfoWnd *)window;
    if (self->feed_box && sender != self->feed_box)
        return 0;

    switch (msg) {
    case 0:
        return 0;

    case UI_MSG_PET_MENU:
        pet_menu_choice(self, val1, val2);
        return 0;

    case UI_MSG_SET_INFO:
        self->info = (struct WindowInfo *)val1;
        if (!window_info_is_on_screen(self->info))
            window_info_set_default(self->info);
        ui_frame_wnd_move(&self->frame.base, self->info->x, self->info->y);
        return 0;

    case UI_MSG_COMMAND:
        if (val1 == PET_ID_RENAME || val1 == UI_ID_CLOSE)
            ui_window_record_command(&self->frame.base, val1, val2, val3, val4);
        if (val1 == PET_ID_RENAME) {
            pet_rename(self);
        } else if (val1 == UI_ID_CLOSE) {
            ui_window_mgr_post_quit1(&g_windowMgr, UI_WND_PET_INFO);
        } else if (val1 == PET_ID_COMMAND) {
            pet_open_menu(self);
        } else if (val1 == PET_ID_FEED_ANSWER) {
            struct Mode *mode = game_current_base_mode();

            if (sender == self->feed_box && val2 == PET_FEED_YES) {
                if (g_windowMgr.exchange_wnd) {
                    chat_refusal(MSG_PET_FEED_REFUSED);
                    self->feed_box = NULL;
                    return 0;
                }
                mode_send_message(mode, REQ_PET_COMMAND, PET_COMMAND_FEED, 0, 0, 0);
            }
            self->feed_box = NULL;
        }
        return 0;
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
}
