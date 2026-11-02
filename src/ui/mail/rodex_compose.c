#include "ui/controls/edit.h"
#include "ui/controls/scroll.h"
#include "ui/controls/buttons.h"
#include "ui/window_access.h"
#include "world/tooltip.h"
#include "render/rp_pool.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "core/object_type_tables.h"
#include "world/mode_manager.h"
#include "game/context.h"
#include "contents/connection_api.h"
#include "ui/mail/rodex_ui.h"
#include "ui/mail/rodex_compose.h"
#include "core/msg_str.h"
#include "core/text_util.h"
#include "diag/perf.h"
#include "session/session_items.h"
#include "session/session_names.h"
#include "session/session_party.h"
#include "ui/wndmgr/wndmgr.h"
#include "ui/items/item_tags_internal.h"
#include <stdint.h>
#include "core/cstr.h"

int g_rodexBodyMaxChars = 500;

struct UIScrollBar;
struct UIWindow;
void ui_rodex_scroll_bar_destroy(struct UIWindow *window);
void ui_rodex_scroll_bar_on_draw(struct UIWindow *self);
const struct UIWindowOps ui_rodex_scroll_bar_ops = {
    .destroy = ui_rodex_scroll_bar_destroy,
    .resize = ui_scroll_bar_resize,
    .is_frame = 0,
    .refreshes_children = 0,
    .move = ui_window_move_base,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_scroll_bar_is_click_through,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_window_store_info_base,
    .save_pos = ui_window_save_pos_base,
    .move_delta = ui_window_move_delta_base,
    .set_show = ui_window_set_show_base,
    .on_create = ui_window_on_create_base,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_rodex_scroll_bar_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_scroll_bar_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_scroll_bar_on_l_btn_dbl_clk,
    .on_mouse_move = ui_scroll_bar_on_mouse_move,
    .on_mouse_hover = ui_window_on_mouse_hover_base,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_scroll_bar_on_mouse_shape,
    .on_l_btn_up = ui_scroll_bar_on_l_btn_up,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_scroll_bar_on_r_btn_up,
    .on_wheel = ui_scroll_bar_on_wheel,
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
};

struct UIEditCtrl3;
void ui_rodex_edit_ctrl3_destroy(struct UIWindow *window);
void ui_rodex_edit_ctrl3_on_create(struct UIWindow *self, int width, int height);
const struct UIEditCtrl3Ops ui_rodex_edit_ctrl3_ops = {
    .edit = {
        .base = {
            .destroy = ui_rodex_edit_ctrl3_destroy,
            .resize = ui_edit_ctrl3_resize,
            .is_frame = 0,
            .refreshes_children = 0,
            .move = ui_window_move_base,
            .is_focusable = ui_edit_ctrl_is_focusable,
            .get_backdrop_box = ui_window_get_backdrop_box_base,
            .is_click_through = ui_window_is_click_through_base,
            .is_hit_testable = ui_window_is_hit_testable_base,
            .drag_and_drop = ui_window_drag_and_drop_base,
            .store_info = ui_window_store_info_base,
            .save_pos = ui_window_save_pos_base,
            .move_delta = ui_window_move_delta_base,
            .set_show = ui_window_set_show_base,
            .on_create = ui_rodex_edit_ctrl3_on_create,
            .on_destroy = ui_window_on_destroy_base,
            .on_process = ui_window_on_process_base,
            .on_draw = ui_edit_ctrl3_on_draw,
            .on_focus = ui_edit_ctrl_on_focus,
            .on_focus_lost = ui_edit_ctrl_on_focus_lost,
            .on_l_btn_down = ui_edit_ctrl_on_l_btn_down,
            .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
            .on_mouse_move = ui_window_on_mouse_move_base,
            .on_mouse_hover = ui_edit_ctrl_on_mouse_hover,
            .track_mouse = ui_window_track_mouse_base,
            .on_mouse_shape = ui_window_on_mouse_shape_base,
            .on_l_btn_up = ui_window_on_l_btn_up_base,
            .on_r_btn_down = ui_window_on_r_btn_down_base,
            .on_r_btn_up = ui_window_on_r_btn_up_base,
            .on_wheel = ui_edit_ctrl3_on_wheel,
            .snap = ui_window_snap_base,
            .send_msg = ui_edit_ctrl3_send_msg,
            .invalidate = ui_window_invalidate_base,
            .render = ui_window_render_base,
            .has_custom_backdrop = 0,
            .draw_backdrop = ui_window_draw_backdrop_base,
            .type = UI_TYPE_EDIT,
            .on_hover_enter = ui_window_on_hover_enter_base,
            .on_hover_leave = ui_window_on_hover_leave_base,
            .make_quad = ui_window_make_quad_base,
            .hit_test = ui_window_hit_test_base,
            .render_overlay = ui_window_render_overlay_base,
        },
        .set_text = ui_edit_ctrl3_set_text,
        .draw_text = ui_edit_ctrl3_draw_text,
        .truncate = ui_edit_ctrl_truncate_to_ime,
    },
    .update_bar = ui_edit_ctrl3_update_bar_default,
};

#define ID_CHECK_RECEIVER 0xcd
#define ID_SEND 0x151
#define ID_BODY ITEM_LINK_EDIT_RODEX

#define MSG_ADD_DROPPED 0x26
#define MSG_ITEM_TAG 0x62
#define MSG_ADD_COUNTED 0x68

#define MODE_RODEX_SEND 0xc0
#define MODE_RODEX_ADD_DROPPED 0xc3
#define MODE_RODEX_ADD_COUNTED 0x126

#define PACKET_CZ_CHECK_NAME 0xb97

#define MSG_NO_RECEIVER 0xa23
#define MSG_BAD_TITLE 0xa24
#define MSG_NAME_TOO_LONG 0xa27
#define MSG_NOT_CONFIRMED 0xa33
#define MSG_NO_ZENY 0xa35
#define MSG_BODY_TOO_LONG 0xa44
#define MSG_NO_ZENY_FEE 0xa53
#define MSG_NAVI_LINK 0xafc
#define MSG_BODY_NAVI_LINK 0xbe2
#define MSG_WEIGHT 0xbdb
#define MSG_DORAM 0xd3c
#define MSG_SEND 0xdd8
#define MSG_CHECK 0xdd9
#define MSG_NAME_OPTIONS 0xa28

#define JOB_SUMMONER 0xfce
#define JOB_SPIRIT_HANDLER 0x1081

#define TITLE_MAX 0x33
#define TITLE_MIN 4
#define BODY_MAX 0x3e9
#define RECEIVER_MAX 0x18

#define ITEM_ROW_LEFT 0x1e
#define ITEM_ROW_TOP 0x135
#define ITEM_STEP 0x22
#define ITEM_SIZE 0x18

#define COLOUR_TEXT 0x313131
#define COLOUR_GREY 0x737373
#define COLOUR_COUNT 0xd66e44
#define COLOUR_SHORT 0x4e71cf

struct UIRodexComposeWnd {
    struct UIFrameWnd frame;
    struct UIEditCtrlCustom *receiver;
    struct UIEditCtrlCustom *receiver_name;
    struct UIBmpTextButton *btn_check;
    struct UIWindow *level_text;
    struct UIWindow *job_text;
    struct UIWindow *id_text;
    int char_id;
    struct UIEditCtrlCustom *title;
    struct UIEditCtrl3 *body;
    struct UIBasicButton *btn_send;
    struct UIEditCtrlCurrency *zeny;
    int zeny_amount_low, zeny_amount_high;
    struct List items;
    int hover;
    int64_t fee;
    struct UIItemTagOnMail *tags;
};
struct DragInfo;
void ui_rodex_compose_wnd_destroy(struct UIWindow *window);
void ui_rodex_compose_wnd_drag_and_drop(struct UIWindow *window, int x, int y, const struct DragInfo *drag);
void ui_rodex_compose_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_rodex_compose_wnd_on_process(struct UIWindow *self);
void ui_rodex_compose_wnd_on_draw(struct UIWindow *self);
void ui_rodex_compose_wnd_on_l_btn_down(struct UIWindow *self, int x, int y);
void ui_rodex_compose_wnd_on_mouse_hover(struct UIWindow *self, int x, int y);
void ui_rodex_compose_wnd_on_r_btn_down(struct UIWindow *self, int x, int y);
intptr_t ui_rodex_compose_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4);
const struct UIWindowOps ui_rodex_compose_wnd_ops = {
    .destroy = ui_rodex_compose_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_rodex_compose_wnd_drag_and_drop,
    .store_info = ui_window_store_info_base,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_rodex_compose_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_rodex_compose_wnd_on_process,
    .on_draw = ui_rodex_compose_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_rodex_compose_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_frame_wnd_on_mouse_move,
    .on_mouse_hover = ui_rodex_compose_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_frame_wnd_on_l_btn_up,
    .on_r_btn_down = ui_rodex_compose_wnd_on_r_btn_down,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_rodex_compose_wnd_send_msg,
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

struct RodexSendRequest {
    int char_id;
    struct String receiver;
    struct String title;
    struct String body;
    int zeny;
};

#pragma pack(push, 1)
struct PacketCheckName {
    WORD id;
    char name[RECEIVER_MAX];
    BYTE known;
};
#pragma pack(pop)
SIZE_ASSERT(PacketCheckName, 0x1b);

#define WND(self) (&(self)->frame.base)
#define EDIT(e) (&(e)->edit.base)

struct UIEditCtrl3 *ui_rodex_edit_ctrl3_construct(struct UIEditCtrl3 *self)
{
    ui_edit_ctrl3_construct(&self->edit.base);
    self->edit.base.ops = &ui_rodex_edit_ctrl3_ops.edit.base;
    return self;
}

void ui_rodex_edit_ctrl3_destroy(struct UIWindow *window)
{
    struct UIEditCtrl3 *self = (struct UIEditCtrl3 *)window;
    ui_edit_ctrl3_destruct(&self->edit.base);
    mem_free(self);
}

struct UIScrollBar *ui_rodex_scroll_bar_construct(struct UIScrollBar *self)
{
    ui_scroll_bar_construct(self);
    self->base.ops = &ui_rodex_scroll_bar_ops;
    self->button_size = RODEX_SCROLL_BUTTON;
    return self;
}

void ui_rodex_scroll_bar_destroy(struct UIWindow *window)
{
    struct UIScrollBar *self = (struct UIScrollBar *)window;
    ui_scroll_bar_destruct(self);
    mem_free(self);
}

void ui_rodex_edit_ctrl3_on_create(struct UIWindow *window, int width, int height)
{
    struct UIEditCtrl3 *self = (struct UIEditCtrl3 *)window;
    struct UIScrollBar *sb = mem_alloc(sizeof *sb);

    sb = sb ? ui_rodex_scroll_bar_construct(sb) : NULL;
    self->bar = sb;
    ui_scroll_bar_create(sb, width - RODEX_SCROLL_BUTTON, 0, 1, height);
    ui_window_add_child(&self->edit.base, &self->bar->base);
}

void ui_rodex_scroll_bar_on_draw(struct UIWindow *window)
{
    struct UIScrollBar *self = (struct UIScrollBar *)window;
    struct UIWindow *wnd = &self->base;
    int thumb, size;

    ui_window_clear_canvas(wnd, 0xffff00ff);
    if (!self->max_pos)
        return;
    if (self->orientation == 1) {
        ui_window_fill_rect(wnd, 0, 0, RODEX_SCROLL_BUTTON, wnd->height, 0xffffff);
        ui_window_draw_bitmap(wnd, 0, 0, ui_skin_bitmap(RODEX_DIR "scrollbar_arrow_upper.bmp"), 0);
        thumb = ui_scroll_bar_thumb_pos(self) + self->button_size;
        if (thumb < 5)
            thumb = 5;
        size = ui_scroll_bar_get_thumb_size(self);
        if (wnd->height - 10 < size)
            size = 1;
        ui_window_fill_rect(wnd, 0, thumb, RODEX_SCROLL_BUTTON, size, 0xe9e9e9);
        ui_window_draw_bitmap(wnd, 0, wnd->height - self->button_size,
                            ui_skin_bitmap(RODEX_DIR "scrollbar_arrow_down.bmp"), 0);
    }
    self->min_thumb = 0;
}

struct UIRodexComposeWnd *ui_rodex_compose_wnd_construct(struct UIRodexComposeWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    WND(self)->ops = &ui_rodex_compose_wnd_ops;
    self->receiver = NULL;
    self->receiver_name = NULL;
    self->btn_check = NULL;
    self->level_text = NULL;
    self->job_text = NULL;
    self->id_text = NULL;
    self->char_id = 0;
    self->title = NULL;
    self->body = NULL;
    self->btn_send = NULL;
    self->zeny = NULL;
    self->zeny_amount_low = 0;
    self->zeny_amount_high = 0;
    list_init(&self->items);
    self->tags = NULL;
    self->fee = 0;
    self->hover = 0;
    list_clear(&self->items, item_info_destroy);
    return self;
}

struct UIWindow *ui_rodex_compose_wnd_new(void)
{
    return (struct UIWindow *)ui_rodex_compose_wnd_construct(mem_alloc(sizeof(struct UIRodexComposeWnd)));
}

void ui_rodex_compose_wnd_destruct(struct UIRodexComposeWnd *self)
{
    WND(self)->ops = &ui_rodex_compose_wnd_ops;
    if (self->tags) {
        ui_item_tag_ctrl_clear_tags(&self->tags->ctrl);
        if (self->tags)
            ui_item_tag_on_mail_destroy(&self->tags->ctrl.edit.base);
    }
    list_free(&self->items, item_info_destroy);
    ui_frame_wnd_destruct(&self->frame);
}

void ui_rodex_compose_wnd_destroy(struct UIWindow *window)
{
    struct UIRodexComposeWnd *self = (struct UIRodexComposeWnd *)window;
    ui_rodex_compose_wnd_destruct(self);
    mem_free(self);
}

static struct UIEditCtrlCustom *name_box(int grey)
{
    struct UIWindow *mem = mem_alloc(sizeof(struct UIEditCtrlCustom));
    struct UIEditCtrlCustom *box = (struct UIEditCtrlCustom *)(mem ? ui_edit_ctrl_custom_construct(mem) : NULL);
    struct UIWindow *w = EDIT(box);

    ui_window_create(w, 0x96, 0x12);
    ui_window_move_base(w, 8, 0x1a);
    box->edit.max_chars = 0x32;
    if (grey)
        ui_edit_ctrl_set_back_colour(w, 0x110, 0x110, 0x110);
    else
        ui_edit_ctrl_set_back_colour(w, 0xff, 0xff, 0xff);
    box->edit.no_border = 0;
    ui_edit_ctrl_set_border_colour(w, 0xa6, 0xa6, 0xa6);
    return box;
}

static struct UIWindow *detail_text(struct UIRodexComposeWnd *self, int width, int y)
{
    struct UIWindow *mem = mem_alloc(sizeof(struct UIStaticText));
    struct UIWindow *text = mem ? ui_static_text_construct(mem) : NULL;

    ui_window_create(text, width, 0x10);
    ui_window_move(text, 0xac, y);
    ui_static_text_set_text_colour(text, 0xc6, 0x28, 0x28);
    ui_static_text_set_font(text, 0, 0xb, 0, 0);
    ui_window_set_show(text, 0);
    ui_window_add_child(WND(self), text);
    return text;
}

void ui_rodex_compose_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UIRodexComposeWnd *self = (struct UIRodexComposeWnd *)window;
    struct UIWindow *mem;
    struct UIBmpTextButton *btn;
    const char *caption;
    int x, y, state;

    ui_window_add_sys_button(WND(self));
    ui_window_add_close_button(WND(self), "", 0);

    self->receiver = name_box(0);
    ui_edit_ctrl_set_text_colour(EDIT(self->receiver), 0x31, 0x31, 0x31, -1);
    self->receiver->offset_x = 1;
    self->receiver->offset_y = 1;
    ui_window_add_child(WND(self), EDIT(self->receiver));

    self->receiver_name = name_box(1);
    ui_edit_ctrl_set_text_colour(EDIT(self->receiver_name), 0xf, 0x33, 0x95, -1);
    self->receiver_name->font_flag = 1;
    self->receiver_name->offset_x = 1;
    self->receiver_name->offset_y = 1;
    self->receiver_name->edit.disabled = 1;
    ui_window_invalidate_base(EDIT(self->receiver_name));
    ui_window_set_show_base(EDIT(self->receiver_name), 0);
    ui_window_add_child(WND(self), EDIT(self->receiver_name));

    mem = mem_alloc(sizeof(struct UIBmpTextButton));
    btn = mem ? ui_bmp_text_button_construct((struct UIBmpTextButton *)mem) : NULL;
    self->btn_check = btn;
    ui_bitmap_button_set_bitmap(&btn->bmp, RODEX_DIR "btn_confirm_id_empty_out.bmp", UI_BTN_UP, 0);
    ui_bitmap_button_set_bitmap(&self->btn_check->bmp, RODEX_DIR "btn_confirm_id_empty_over.bmp", UI_BTN_HOVER,
                             0);
    ui_bitmap_button_set_bitmap(&self->btn_check->bmp, RODEX_DIR "btn_confirm_id_empty_press.bmp", UI_BTN_DOWN,
                             0);
    ui_bitmap_button_set_bitmap(&self->btn_check->bmp, RODEX_DIR "btn_confirm_id_empty_press.bmp",
                             UI_BMP_DISABLED, 0);
    btn = self->btn_check;
    ui_window_create(&btn->bmp.button.base, btn->bmp.bitmap_width, btn->bmp.bitmap_height);
    x = (int)((float)(self->btn_check->bmp.button.base.width
                      - ui_window_text_width(WND(self), msg_str(MSG_CHECK), 0, 0, 0xc, 0, 0)) * 0.5f);
    y = (int)((float)(self->btn_check->bmp.button.base.height
                      - ui_window_text_height(WND(self), msg_str(MSG_CHECK), 0, 0, 0xc, 0, 0)) * 0.5f);
    for (state = UI_BTN_UP; state <= UI_BTN_DOWN; state++)
        ui_bmp_text_button_set_text_style(self->btn_check, state, 0xc, COLOUR_TEXT, 0, state == UI_BTN_DOWN, x, y);
    ui_bmp_text_button_set_text_style(self->btn_check, UI_BMP_DISABLED, 0xc, 0xc2c2c2, 0, 0, x, y);
    caption = msg_str(MSG_CHECK);
    btn = self->btn_check;
    string_assign_cstr(&btn->caption, caption);
    ui_window_invalidate_base(&btn->bmp.button.base);
    ui_window_move_base(&self->btn_check->bmp.button.base, 0xa2, 0x1a);
    self->btn_check->bmp.button.base.id = ID_CHECK_RECEIVER;
    ui_window_add_child(WND(self), &self->btn_check->bmp.button.base);

    self->level_text = detail_text(self, 0x100, 0x19);
    self->job_text = detail_text(self, 0x100, 0x25);
    self->id_text = detail_text(self, 0xfd, 0x31);

    mem = mem_alloc(sizeof(struct UIEditCtrlCustom));
    self->title = (struct UIEditCtrlCustom *)(mem ? ui_edit_ctrl_custom_construct(mem) : NULL);
    ui_window_create(EDIT(self->title), 0x9b, 0xe);
    ui_window_move_base(EDIT(self->title), 6, 0x32);
    self->title->edit.max_chars = 0x32;
    ui_edit_ctrl_set_text_default(&self->title->edit, "TITLE");
    ui_window_add_child(WND(self), EDIT(self->title));

    mem = mem_alloc(sizeof(struct UIEditCtrl3));
    self->body = mem ? ui_rodex_edit_ctrl3_construct((struct UIEditCtrl3 *)mem) : NULL;
    ui_window_create(EDIT(self->body), 0x10b, 0xc0);
    self->body->edit.multiline = 1;
    ui_window_move_base(EDIT(self->body), 0xf, 0x51);
    self->body->edit.max_chars = g_rodexBodyMaxChars;
    ui_edit_ctrl_set_back_colour(EDIT(self->body), 0xbd, 0xce, 0xf7);
    ui_edit_ctrl_set_text_colour(EDIT(self->body), 0x31, 0x31, 0x31, -1);
    ui_window_add_child(WND(self), EDIT(self->body));
    ui_window_mgr_set_focus_edit(&g_windowMgr, EDIT(self->body));
    EDIT(self->body)->id = ID_BODY;

    mem = mem_alloc(sizeof(struct UIEditCtrlCurrency));
    self->zeny = mem ? ui_edit_ctrl_currency_construct((struct UIEditCtrlCurrency *)mem) : NULL;
    ui_window_create(&self->zeny->custom.edit.base, 0x54, 0xe);
    self->zeny->custom.edit.multiline = 0;
    ui_window_move_base(&self->zeny->custom.edit.base, 0x2d, 0x15c);
    self->zeny->custom.edit.max_chars = 0xd;
    ui_edit_ctrl_set_text_default(&self->zeny->custom.edit, "0");
    ui_window_add_child(WND(self), &self->zeny->custom.edit.base);

    mem = mem_alloc(sizeof(struct UIBasicButton));
    self->btn_send = mem ? ui_basic_button_construct((struct UIBasicButton *)mem) : NULL;
    ui_basic_button_set_label(&self->btn_send->button.base, 0x46, msg_str(MSG_SEND));
    ui_window_move_base(&self->btn_send->button.base, 0xe2, 0x178);
    self->btn_send->button.base.id = ID_SEND;
    ui_window_add_child(WND(self), &self->btn_send->button.base);
}

int64_t ui_rodex_compose_wnd_get_zeny(struct UIRodexComposeWnd *self)
{
    char buf[0x104];

    memset(buf, 0, sizeof buf);
    cstr_copy(buf, sizeof buf, ui_edit_ctrl_get_text(&self->zeny->custom.edit.base));
    session_is_number_text(&g_session1, buf);
    return strtoll(buf, NULL, 10);
}

struct ItemInfo *ui_rodex_compose_wnd_item_at(struct UIRodexComposeWnd *self, struct ItemInfo *out, int x, int y)
{
    if ((unsigned)(x - ITEM_ROW_LEFT) <= 0xa0 && (unsigned)(y - ITEM_ROW_TOP) <= ITEM_SIZE) {
        struct ListNode *node;
        int left = ITEM_ROW_LEFT;

        self->hover = 0;
        for (node = self->items.head->next; node != self->items.head; node = node->next) {
            if (x > left && x < left + ITEM_SIZE)
                return item_info_copy(out, LIST_VALUE(node, struct ItemInfo));
            self->hover++;
            left += ITEM_STEP;
        }
    }
    return item_info_construct_with_arg(out);
}

void ui_rodex_compose_wnd_set_receiver(struct UIRodexComposeWnd *self, int char_id, WORD job, WORD level, const char *name)
{
    struct String text;

    string_init(&text);
    string_format(&text, "Lv%d", level);
    ui_static_text_set_text_or_shrink2(self->level_text, string_cstr(&text), 0);
    string_free(&text);
    ui_window_set_show(self->level_text, 1);
    ui_static_text_set_text_or_shrink2(self->job_text, session_get_job_name(&g_session1, job, -1), 0);
    if (job == JOB_SUMMONER || job == JOB_SPIRIT_HANDLER)
        ui_static_text_set_text_or_shrink2(self->job_text, msg_str(MSG_DORAM), 0);
    ui_window_set_show(self->job_text, 1);
    self->char_id = char_id;
    session_scramble_number(&g_session1, &text, char_id);
    ui_static_text_set_text_or_shrink2(self->id_text, string_cstr(&text), 0);
    string_free(&text);
    ui_window_set_show(self->id_text, 1);
    ui_window_set_show_base(EDIT(self->receiver), 0);
    ui_button_set_show(&self->btn_check->bmp.button, 0);
    ui_window_set_show_base(EDIT(self->receiver_name), 1);
    ui_edit_ctrl_set_text_default(&self->receiver_name->edit, name);
}

void rodex_compose_wnd_set_receiver(struct UIRodexComposeWnd *self, const char *name)
{
    ui_edit_ctrl_set_text_default(&self->receiver->edit, name);
}

void ui_rodex_compose_wnd_add_item_link(struct UIWindow *wnd, const struct ItemInfo *info)
{
    struct UIRodexComposeWnd *self = (struct UIRodexComposeWnd *)wnd;
    struct String name = {0};
    struct String out = {0};
    const char *body_text;
    BYTE fits;

    if (!self->tags) {
        struct UIItemTagOnMail *mem = mem_alloc(sizeof *mem);

        self->tags = mem ? ui_item_tag_on_mail_construct(mem) : NULL;
        if (!self->tags)
            return;
    }
    item_info_get_display_name((struct ItemInfo *)info, &name);
    fits = ui_item_tag_ctrl_can_add(&self->tags->ctrl, string_view(&name), EDIT(self->body)->width);
    string_free(&name);
    if (!fits)
        return;
    body_text = ui_edit_ctrl_get_text(EDIT(self->body));
    string_assign_cstr(&self->tags->ctrl.text, body_text);
    item_info_get_display_name((struct ItemInfo *)info, &name);
    ui_item_tag_ctrl_append_tag(&self->tags->ctrl, &out, string_view(&name));
    ui_edit_ctrl_set_text(&self->body->edit, string_cstr(&out));
    string_free(&out);
    string_free(&name);
    ui_edit_ctrl_make_select(EDIT(self->body));
    ui_item_tag_ctrl_add_item(&self->tags->ctrl, info, RODEX_TAG_COLOUR);
}

void ui_rodex_compose_wnd_drag_and_drop(struct UIWindow *window, int x, int y, const struct DragInfo *drag)
{
    struct UIRodexComposeWnd *self = (struct UIRodexComposeWnd *)window;
    struct Mode *mode;

    if (!g_windowMgr.rodex_compose_wnd) {
        ui_window_drag_and_drop_base(WND(self), x, y, drag);
        return;
    }
    mode = game_current_base_mode();
    if (drag->source != DRAG_INVENTORY) {
        mode_send_message(mode, MODE_REQ_DRAG_END, 0, 0, 0, 0);
        return;
    }
    if (drag->drop_count == 1) {
        ui_rodex_compose_wnd_send_msg(WND(self), NULL, MSG_ADD_DROPPED, 0, 0, 0, 0);
        return;
    }
    ui_window_mgr_show_option_window(&g_windowMgr, WND_ID_COUNT_INPUT, 1);
    ui_window_send_msg(g_windowMgr.item_drop_cnt_wnd, NULL, COUNT_MSG_OPEN, 0xd, 0, 0, 0);
}

void ui_rodex_compose_wnd_on_draw(struct UIWindow *window)
{
    struct UIRodexComposeWnd *self = (struct UIRodexComposeWnd *)window;
    struct UIWindow *wnd = WND(self);
    struct String text;
    char money[0x104], fee_text[0x104], zeny_text[0x104];
    struct ListNode *node;
    int64_t item_tax, zeny;
    int x;

    ui_window_clear_canvas(wnd, 0xff00ff);
    ui_window_draw_bitmap(wnd, 0, 0, ui_skin_bitmap(RODEX_DIR "bg_rodex_read.bmp"), 0);
    if (EDIT(self->receiver_name)->visible == 1)
        ui_window_draw_bitmap(wnd, 0xa1, 0x15, ui_skin_bitmap(RODEX_DIR "bg_id_balloon.bmp"), 1);

    string_init(&text);
    string_format(&text, "/ %d", g_rodexBodyMaxChars);
    ui_window_text_out(wnd, 0x105, 0x114, string_cstr(&text), 0, 0, 0xa, COLOUR_COUNT, 0, 0);
    string_free(&text);
    string_format(&text, "%d", ui_edit_ctrl_get_text_size(EDIT(self->body)));
    ui_window_text_out_right(wnd, 0x102, 0x114, string_cstr(&text), 0, 0, 0xa, COLOUR_COUNT, 0);
    string_free(&text);

    ui_window_draw_bitmap(wnd, 0xc7, 0x131, res_mgr_get(g_resMgr(), RODEX_DIR "bg_weight.bmp"), 1);
    ui_window_text_out(wnd, 0xf5, 0x13c, msg_str(MSG_WEIGHT), 0, 0, 0xb, COLOUR_GREY, 0, 0);
    string_format(&text, "%d", (short)g_session1.mailWeight);
    ui_window_text_out_right(wnd, 0xf4, 0x13c, string_cstr(&text), 0, 0, 0xb, COLOUR_GREY, 0);
    string_free(&text);

    ui_window_fill_rect(wnd, 0x23, 0x15b, 0x5f, 0x10, 0xffffff);
    ui_window_draw_outline(wnd, 0x23, 0x15b, 0x5f, 0x10, 0xa6a6a6);
    ui_window_draw_bitmap(wnd, 0x13, 0x156, res_mgr_get(g_resMgr(), RODEX_DIR "icon_zeny.bmp"), 1);
    ui_window_text_out(wnd, 0x87, 0x15e, "/", 0, 0, 0xb, COLOUR_GREY, 0, 0);
    memset(money, 0, sizeof money);
    format_money64(g_session1.zeny, money, sizeof money);
    string_format(&text, "%s Zeny", money);
    ui_window_text_out(wnd, 0x90, 0x15e, string_cstr(&text), 0, 0, 0xb, COLOUR_GREY, 1, 0);
    string_free(&text);

    ui_window_draw_bitmap(wnd, 9, 0x179, res_mgr_get(g_resMgr(), RODEX_DIR "bg_tax.bmp"), 1);
    memset(fee_text, 0, sizeof fee_text);
    item_tax = session_count_mail_items(&g_session1) * 2500;
    memset(zeny_text, 0, sizeof zeny_text);
    cstr_copy(zeny_text, sizeof zeny_text, ui_edit_ctrl_get_text(&self->zeny->custom.edit.base));
    session_is_number_text(&g_session1, zeny_text);
    zeny = strtoll(zeny_text, NULL, 10);
    if (ui_edit_ctrl_get_text(&self->zeny->custom.edit.base)) {
        self->fee = zeny * 2 / 100 + item_tax;
        format_money64(self->fee, fee_text, sizeof fee_text);
    } else {
        format_money64(item_tax, fee_text, sizeof fee_text);
    }
    ui_window_text_out_right(wnd, 0x68, 0x17e, fee_text, 0, 0, 0xb,
                          self->fee + zeny > (int64_t)g_session1.zeny ? COLOUR_SHORT : 0, 0);
    if (ui_edit_ctrl_get_text_size(&self->zeny->custom.edit.base)) {
        if (zeny > (int64_t)g_session1.zeny || self->fee + zeny > (int64_t)g_session1.zeny)
            ui_edit_ctrl_set_text_colour(&self->zeny->custom.edit.base, 0xff, 0, 0, -1);
        else
            ui_edit_ctrl_set_text_colour(&self->zeny->custom.edit.base, 0, 0, 0, -1);
    }

    for (x = 0x1a; x < 0xc4; x += ITEM_STEP)
        ui_window_draw_bitmap(wnd, x, 0x13f, res_mgr_get(g_resMgr(), RODEX_DIR "shadow_item_off.bmp"), 1);
    for (node = self->items.head->next, x = ITEM_ROW_LEFT; node != self->items.head; node = node->next, x += ITEM_STEP) {
        struct ItemInfo *item = LIST_VALUE(node, struct ItemInfo);

        ui_window_draw_bitmap(wnd, x - 4, 0x13f, res_mgr_get(g_resMgr(), RODEX_DIR "shadow_item_on.bmp"),
                            1);
        if (self->fee > (int64_t)g_session1.zeny)
            ui_window_draw_item_icon_count_grey(wnd, 1, x, ITEM_ROW_TOP, item, (BYTE)item->bind_on_equip_type, 0);
        else
            ui_window_draw_item_icon_count(wnd, x, ITEM_ROW_TOP, item, 0, 0);
        if (item->grade > 0) {
            string_format(&text, UI_SKIN "grade_enchant\\grade_icon%d.bmp", item->grade);
            ui_window_draw_bitmap(wnd, x, 0x141, res_mgr_get(g_resMgr(), string_cstr(&text)), 1);
            string_free(&text);
        }
    }

    if (self->tags) {
        const char *body_text;

        ui_item_tag_on_mail_set_lines(self->tags, &self->body->shown_lines);
        body_text = ui_edit_ctrl_get_text(EDIT(self->body));
        string_init(&text);
        string_assign_cstr(&text, body_text);
        ui_item_tag_ctrl_set_text(&self->tags->ctrl, string_view(&text), EDIT(self->body)->width);
        string_free(&text);
        ui_item_tag_ctrl_update(&self->tags->ctrl);
        if (self->tags->ctrl.ordered_count == 0) {
            if (self->tags)
                ui_item_tag_on_mail_destroy(&self->tags->ctrl.edit.base);
            self->tags = NULL;
        }
    }
}

void ui_rodex_compose_wnd_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIRodexComposeWnd *self = (struct UIRodexComposeWnd *)window;
    struct ItemInfo item;

    if (y < 0x11)
        ui_frame_wnd_on_l_btn_down((struct UIWindow *)(&self->frame), x, y);
    ui_rodex_compose_wnd_item_at(self, &item, x, y);
    if ((unsigned)item.index > 0)
        game_mode_drag_item(game_current_mode(), 0xf, &item, item.count, -1);
    item_info_destruct(&item);
}

void ui_rodex_compose_wnd_on_mouse_hover(struct UIWindow *window, int x, int y)
{
    struct UIRodexComposeWnd *self = (struct UIRodexComposeWnd *)window;
    struct ItemInfo item;
    struct String text;
    struct String name = {0};
    int tx, ty;

    ui_rodex_compose_wnd_item_at(self, &item, x, y);
    if (atoi(string_cstr(&item.id_text))) {
        tx = self->hover * ITEM_STEP + ITEM_ROW_LEFT;
        ty = 0x122;
        ui_window_to_screen(WND(self), &tx, &ty);
        item_info_get_display_name(&item, &name);
        if (item.identified == 1 && item.option_count) {
            string_init(&text);
            string_format(&text, msg_str(MSG_NAME_OPTIONS), string_cstr(&name), item.option_count);
            mode_show_tooltip(client_current_mode(), string_cstr(&text), tx, ty,
                              session_get_item_dis_color(&item), 0);
            string_free(&text);
        } else {
            mode_show_tooltip(client_current_mode(), string_cstr(&name), tx, ty,
                              session_get_item_dis_color(&item), 0);
        }
        string_free(&name);
    }
    item_info_destruct(&item);
}

void ui_rodex_compose_wnd_on_process(struct UIWindow *window)
{
    struct UIRodexComposeWnd *self = (struct UIRodexComposeWnd *)window;
    struct UIWindow *zeny = &self->zeny->custom.edit.base;

    if (ui_window_mgr_get_focus_edit(&g_windowMgr) == zeny)
        ui_edit_ctrl_currency_on_process(zeny);
}

void ui_rodex_compose_wnd_on_r_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIRodexComposeWnd *self = (struct UIRodexComposeWnd *)window;
    struct ItemInfo item;

    ui_rodex_compose_wnd_item_at(self, &item, x, y);
    if (atoi(string_cstr(&item.id_text)))
        ui_window_send(ui_window_open(ITEM_WND_ID), NULL, UI_MSG_SHOW_ITEM, (intptr_t)&item, 0, 0, 0);
    item_info_destruct(&item);
}

static void check_receiver(struct UIRodexComposeWnd *self)
{
    struct UIWindow *receiver = EDIT(self->receiver);
    struct PacketCheckName packet;
    struct String name;
    int len = ui_edit_ctrl_get_text_size(receiver);
    void *conn;

    if (len != 0 && len < RECEIVER_MAX) {
        if (navi_links_has_link(text_view_cstr(ui_edit_ctrl_get_text(receiver)))) {
            rodex_chat(MSG_NAVI_LINK);
            return;
        }
        memset(&packet, 0, sizeof packet);
        packet.id = PACKET_CZ_CHECK_NAME;
        cstr_copy(packet.name, sizeof packet.name, ui_edit_ctrl_get_text(receiver));
        string_init(&name);
        string_assign_cstr(&name, packet.name);
        if (session_is_account_char_name(&g_session1, name)) {
            packet.known = 1;
        } else {
            string_init(&name);
            string_assign_cstr(&name, packet.name);
            packet.known = session_is_known_name(&g_session1, name);
        }
        conn = rag_connection_instance();
        len = rag_connection_get_packet_length(conn, (short)packet.id);
        rag_connection_send_packet(rag_connection_instance(), len, &packet);
        return;
    }
    if (ui_edit_ctrl_get_text_size(receiver) >= RECEIVER_MAX)
        rodex_chat(MSG_NAME_TOO_LONG);
    rodex_chat(navi_links_has_link(text_view_cstr(ui_edit_ctrl_get_text(receiver))) ? MSG_NAVI_LINK : MSG_NO_RECEIVER);
}

static void send_mail(struct UIRodexComposeWnd *self)
{
    struct RodexSendRequest request;
    struct String body;
    struct String joined = {0};
    int64_t zeny;
    int len;

    if (!ui_edit_ctrl_get_text_size(EDIT(self->receiver)) && !ui_edit_ctrl_get_text_size(EDIT(self->receiver_name))) {
        rodex_chat(MSG_NO_RECEIVER);
        return;
    }
    len = ui_edit_ctrl_get_text_size(EDIT(self->title));
    if (len < TITLE_MIN || ui_edit_ctrl_get_text_size(EDIT(self->title)) > TITLE_MAX) {
        rodex_chat(MSG_BAD_TITLE);
        return;
    }
    if (ui_edit_ctrl_get_text_size(EDIT(self->body)) > BODY_MAX) {
        rodex_chat(MSG_BODY_TOO_LONG);
        return;
    }
    zeny = ui_rodex_compose_wnd_get_zeny(self);
    if (zeny > 0) {
        if (zeny > (int64_t)g_session1.zeny) {
            rodex_chat(MSG_NO_ZENY);
            return;
        }
        if (self->fee + zeny > (int64_t)g_session1.zeny) {
            rodex_chat(MSG_NO_ZENY_FEE);
            return;
        }
    }
    if (self->fee > 0 && self->fee > (int64_t)g_session1.zeny) {
        rodex_chat(MSG_NO_ZENY_FEE);
        return;
    }
    if (((struct UIStaticText *)self->id_text)->text.size == 0) {
        rodex_chat(MSG_NOT_CONFIRMED);
        return;
    }

    string_init(&body);
    string_assign_cstr(&body, ui_edit_ctrl_get_text(EDIT(self->body)));
    {
        if (navi_links_has_link(text_view_cstr(ui_edit_ctrl_get_text(EDIT(self->body))))) {
            ui_edit_ctrl_set_text(&self->body->edit, "");
            string_clear(&body);
            ui_chat_line(msg_str(MSG_BODY_NAVI_LINK), 0xff);
            string_free(&body);
            return;
        }
    }
    if (self->body->edit.navigation_link) {
        string_clear(&body);
        ui_edit_ctrl3_join_lines((struct UIEditCtrl3 *)self->body, &joined);
        string_assign(&body, string_cstr(&joined), joined.size);
        string_free(&joined);
    }
    if (self->tags && self->tags->ctrl.linked_text.size)
        string_assign_cstr(&body, string_cstr(&self->tags->ctrl.linked_text));

    string_init(&request.receiver);
    string_init(&request.title);
    string_init(&request.body);
    request.char_id = self->char_id;
    string_assign_cstr(&request.receiver, ui_edit_ctrl_get_text(EDIT(self->receiver_name)));
    string_assign_cstr(&request.title, ui_edit_ctrl_get_text(EDIT(self->title)));
    string_assign(&request.body, string_cstr(&body), body.size);
    request.zeny = (int)zeny;
    game_send_message(MODE_RODEX_SEND, (intptr_t)&request, 0, 0, 0);
    string_free(&request.body);
    string_free(&request.title);
    string_free(&request.receiver);
    string_free(&body);
}

static void reload_items(struct UIRodexComposeWnd *self)
{
    struct ItemInfo item;
    int slot;

    list_clear(&self->items, item_info_destroy);
    for (slot = 0; slot < RODEX_ITEM_SLOTS; slot++) {
        session_get_mail_item(&g_session1, &item, slot);
        if (item.count > 0) {
            item_info_copy(list_push_back(&self->items, NULL, sizeof(struct ItemInfo)), &item);
        }
        item_info_destruct(&item);
    }
    ui_window_invalidate_base(WND(self));
}

static void attach_dragged(int request)
{
    const struct DragInfo *drag = game_current_drag();

    mode_send_message(game_current_base_mode(), request, drag->index, drag->drop_count, 0, 0);
    game_send_message(MODE_REQ_DRAG_END, 0, 0, 0, 0);
}

intptr_t ui_rodex_compose_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, intptr_t val1,
                                         intptr_t val2, intptr_t val3, intptr_t val4)
{
    struct UIRodexComposeWnd *self = (struct UIRodexComposeWnd *)window;
    switch (msg) {
    case UI_MSG_COMMAND:
        if (val1 == UI_ID_CLOSE)
            ui_close_window(WID_RODEX_COMPOSE);
        else if (val1 == ID_CHECK_RECEIVER)
            check_receiver(self);
        else if (val1 == ID_SEND)
            send_mail(self);
        return 0;
    case UIM_RODEX_REFRESH:
        reload_items(self);
        return 0;
    case MSG_ADD_DROPPED:
        attach_dragged(MODE_RODEX_ADD_DROPPED);
        return 0;
    case MSG_ADD_COUNTED:
        attach_dragged(MODE_RODEX_ADD_COUNTED);
        return 0;
    case MSG_ITEM_TAG: {
        const struct ItemInfo *info = (const struct ItemInfo *)val3;
        struct ItemInfo *worn;
        struct UIWindow *compare;

        if (!info)
            return 0;
        ui_window_send(ui_window_open(ITEM_WND_ID), NULL, UI_MSG_SHOW_ITEM, val3, 0, 0, 0);
        if (!g_windowMgr.item_collection_wnd)
            return 0;
        worn = session_get_worn_counterpart(&g_session1, info);
        if (!g_session1.itemCompare || !worn || val4)
            return 0;
        compare = ui_make_window(WID_ITEM_COMPARE);
        if (compare)
            ui_window_send_msg(compare, NULL, UI_MSG_SHOW_ITEM, (intptr_t)worn, 1, 0, 0);
        else
            ui_close_window(WID_ITEM_COMPARE);
        return 0;
    }
    default:
        return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
    }
}

struct UIEditCtrl3 *ui_rodex_compose_body(struct UIWindow *compose)
{
    return ((struct UIRodexComposeWnd *)compose)->body;
}
