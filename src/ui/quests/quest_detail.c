#include "render/rp_pool.h"
#include "render/texture.h"
#include "render/texture_cache.h"
#include "core/object_type_tables.h"
#include <stdio.h>
#include "ui/quests/quests.h"
#include "core/app.h"
#include "core/msg_str.h"
#include "core/skin_path.h"
#include "diag/perf.h"

#define QUEST_MSG_REFRESH 0x17

#define QUEST_UI_DIR UI_SKIN "renew_questui\\"
#define CLOSE_BTN_DIR UI_SKIN "basic_interface\\"

#define MSG_QUEST_DETAIL_TITLE 0xd48
#define MSG_QUEST_END_TIME 0x893

#define QUEST_TITLE_COLOUR 0x55321d
#define QUEST_END_TIME_COLOUR 0x161bd2

#define QUEST_PANE_WIDTH 0x156
#define QUEST_PANE_HEIGHT 0x142
#define QUEST_PANE_Y 0x3e

#define QUEST_DETAIL_DRAG_HEIGHT 0x11

struct UIRenewDetailQuestInfoWnd;
struct UIWindow;
void ui_renew_detail_quest_info_wnd_destroy(struct UIWindow *window);
void ui_renew_detail_quest_info_wnd_on_create(struct UIWindow *self, int width_unused, int height_unused);
void ui_renew_detail_quest_info_wnd_on_draw(struct UIWindow *self);
void ui_renew_detail_quest_info_wnd_on_l_btn_down(struct UIWindow *self, int x, int y);
int ui_renew_detail_quest_info_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_renew_detail_quest_info_wnd_ops = {
    .destroy = ui_renew_detail_quest_info_wnd_destroy,
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
    .on_create = ui_renew_detail_quest_info_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_renew_detail_quest_info_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_renew_detail_quest_info_wnd_on_l_btn_down,
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
    .send_msg = ui_renew_detail_quest_info_wnd_send_msg,
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

struct UIRenewDetailQuestInfoWnd {
    struct UIFrameWnd frame;
    struct UIWindow *ongoing;
    struct UIWindow *recommended;
    struct UIWindow *close;
    int quest_id;
    struct String end_time;
    struct String image;
    BYTE is_recommended;
};

static void canvas_blend_texture(struct UIWindow *wnd, int x, int y, int width, int height, struct Texture *tex)
{
    dc_bitmap_draw_texels_scaled(wnd->canvas, x, y, width, height, tex, tex->reqWidth, tex->reqHeight,
                              1.0f, 1.0f, 0x80ffffff, 0, 0, 0, 0, 0);
}

struct UIRenewDetailQuestInfoWnd *ui_renew_detail_quest_info_wnd_construct(struct UIRenewDetailQuestInfoWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_renew_detail_quest_info_wnd_ops;
    self->ongoing = NULL;
    self->recommended = NULL;
    self->quest_id = 0;
    string_init(&self->end_time);
    string_init(&self->image);
    self->is_recommended = 0;
    return self;
}

struct UIWindow *ui_renew_detail_quest_info_wnd_new(void)
{
    return (struct UIWindow *)ui_renew_detail_quest_info_wnd_construct(mem_alloc(sizeof(struct UIRenewDetailQuestInfoWnd)));
}

static void ui_renew_detail_quest_info_wnd_destruct(struct UIRenewDetailQuestInfoWnd *self)
{
    self->frame.base.ops = &ui_renew_detail_quest_info_wnd_ops;
    string_free(&self->image);
    string_free(&self->end_time);
    ui_frame_wnd_destruct(&self->frame);
}

void ui_renew_detail_quest_info_wnd_destroy(struct UIWindow *window)
{
    struct UIRenewDetailQuestInfoWnd *self = (struct UIRenewDetailQuestInfoWnd *)window;
    ui_renew_detail_quest_info_wnd_destruct(self);
    mem_free(self);
}

void ui_renew_detail_quest_info_wnd_on_create(struct UIWindow *window, int width_unused,
                                                   int height_unused)
{
    struct UIRenewDetailQuestInfoWnd *self = (struct UIRenewDetailQuestInfoWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct UIWindow *pane, *close;

    pane = (struct UIWindow *)ui_ongoing_quest_scroll_wnd_new(QUEST_PANE_WIDTH, QUEST_PANE_HEIGHT);
    self->ongoing = pane;
    ui_window_move(pane, 0, QUEST_PANE_Y);
    ui_window_add_child(wnd, self->ongoing);

    pane = (struct UIWindow *)ui_recommeded_quest_scroll_wnd_new(QUEST_PANE_WIDTH, QUEST_PANE_HEIGHT);
    self->recommended = pane;
    ui_window_move(pane, 0, QUEST_PANE_Y);
    ui_window_add_child(wnd, self->recommended);

    close = mem_alloc(sizeof(struct UIBitmapButton));
    if (close)
        close = (struct UIWindow *)ui_bitmap_button_construct((struct UIBitmapButton *)close);
    self->close = close;
    ui_bitmap_button_set_bitmap((struct UIBitmapButton *)self->close, CLOSE_BTN_DIR "btn_close.bmp", 0, 0);
    ui_bitmap_button_set_bitmap((struct UIBitmapButton *)self->close, CLOSE_BTN_DIR "btn_close_a.bmp", 1, 0);
    ui_bitmap_button_set_bitmap((struct UIBitmapButton *)self->close, CLOSE_BTN_DIR "btn_close_b.bmp", 2, 0);
    ui_window_create(self->close, ((struct UIBitmapButton *)self->close)->bitmap_width,
                     ((struct UIBitmapButton *)self->close)->bitmap_height);
    ui_window_move_base(self->close, 0x120, 0x184);
    self->close->id = UI_ID_CLOSE;
    ui_window_add_child(wnd, self->close);
}

static void quest_detail_draw_title(struct UIWindow *wnd, const struct String *title)
{
    ui_window_text_out_centred(wnd, wnd->width >> 1, 0x27, string_cstr(title), title->size, 0, 12,
                             QUEST_TITLE_COLOUR, 1);
}

void ui_renew_detail_quest_info_wnd_on_draw(struct UIWindow *window)
{
    struct UIRenewDetailQuestInfoWnd *self = (struct UIRenewDetailQuestInfoWnd *)window;
    struct UIWindow *wnd = &self->frame.base;

    ui_window_clear_canvas(wnd, 0xffffffff);
    if (self->is_recommended) {
        struct Texture *tex;

        ui_window_draw_body(wnd, 0, 0, QUEST_UI_DIR "bg_questSub2.bmp");
        quest_detail_draw_title(wnd, &renew_quest_recommended_info(g_questTable, self->quest_id)->base.text[RENEW_QUEST_TEXT_TITLE]);
        if (self->image.size)
            ui_window_draw_bitmap(wnd, 3, 0x40, res_mgr_get(g_resMgr(), ui_skin_path(string_cstr(&self->image))),
                                0);
        tex = texture_cache_get(&g_textureCache, QUEST_UI_DIR "bg_quest01.bmp", 0, 0);
        if (tex)
            canvas_blend_texture(wnd, 9, 0x46, 0x144, 0x8a, tex);
        tex = texture_cache_get(&g_textureCache, QUEST_UI_DIR "bg_quest02.bmp", 0, 0);
        if (tex)
            canvas_blend_texture(wnd, 9, 0x11b, 0xd0, 0x5c, tex);
    } else {
        ui_window_draw_body(wnd, 0, 0, QUEST_UI_DIR "bg_questSub.bmp");
        quest_detail_draw_title(wnd, &renew_quest_ongoing_info(g_questTable, self->quest_id)->base.text[RENEW_QUEST_TEXT_TITLE]);
        ui_window_text_out(wnd, 0xc, 0x189, string_cstr(&self->end_time), 0, 0, 12, QUEST_END_TIME_COLOUR,
                         0, 0);
    }
    ui_window_text_out(wnd, 0x10, 2, msg_str(MSG_QUEST_DETAIL_TITLE), 0, 0, 13, 0, 0, 0);
}

void ui_renew_detail_quest_info_wnd_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIRenewDetailQuestInfoWnd *self = (struct UIRenewDetailQuestInfoWnd *)window;
    if (y < QUEST_DETAIL_DRAG_HEIGHT)
        ui_frame_wnd_on_l_btn_down((struct UIWindow *)(&self->frame), x, y);
}

int ui_renew_detail_quest_info_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender,
                                                 int msg, int val1, int val2, int val3, int val4)
{
    struct UIRenewDetailQuestInfoWnd *self = (struct UIRenewDetailQuestInfoWnd *)window;
    if (msg == UI_MSG_COMMAND) {
        if (val1 == UI_ID_CLOSE)
            ui_close_window(QUEST_DETAIL_WND_ID);
        return 0;
    }
    if (msg == QUEST_MSG_REFRESH) {
        struct UIWindow *pane = self->is_recommended ? self->recommended : self->ongoing;
        ui_window_send_msg(pane, &self->frame.base, QUEST_MSG_REFRESH, 0, 0, 0, 0);
        return 0;
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
}

void ui_renew_detail_quest_info_wnd_show_quest(struct UIRenewDetailQuestInfoWnd *self, int quest_id,
                                                    BYTE recommended)
{
    if (!quest_id)
        return;
    self->is_recommended = recommended;
    self->quest_id = quest_id;
    string_clear(&self->end_time);

    if (recommended) {
        struct RenewQuestA *info;

        ui_window_set_show(self->ongoing, 0);
        ui_window_set_show(self->recommended, 1);
        ui_recommeded_quest_scroll_wnd_set_quest((struct UIRecommededQuestScrollWnd *)self->recommended,
                                            self->quest_id);
        info = renew_quest_recommended_info(g_questTable, quest_id);
        string_assign(&self->image, string_cstr(&info->image), info->image.size);
        if (self->image.size) {
            char path[MAX_PATH];
            _snprintf(path, sizeof path, QUEST_UI_DIR "%s", string_cstr(&self->image));
            path[sizeof path - 1] = 0;
            string_assign_cstr(&self->image, path);
        }
    } else {
        struct RenewQuestB *info = renew_quest_ongoing_info(g_questTable, quest_id);

        if (!info->base.text[RENEW_QUEST_TEXT_TITLE].size) {
            char text[64];
            sprintf(text, "Not found Quest Info = %lu", (unsigned long)quest_id);
            MessageBoxA(g_hMainWnd, text, "Error", MB_OK);
            return;
        }
        ui_window_set_show(self->recommended, 0);
        ui_window_set_show(self->ongoing, 1);
        ui_ongoing_quest_scroll_wnd_set_quest((struct UIOngoingQuestScrollWnd *)self->ongoing, self->quest_id);
        if (info->plain.deadline.year > 1970) {
            char text[MAX_PATH];
            _snprintf(text, sizeof text, "%s [%02d/%02d/%02d %02d:%02d]", msg_str(MSG_QUEST_END_TIME),
                      info->plain.deadline.year, info->plain.deadline.month, info->plain.deadline.day, info->plain.deadline.hour, info->plain.deadline.minute);
            text[sizeof text - 1] = 0;
            string_assign_cstr(&self->end_time, text);
        }
    }
    ui_window_invalidate_base(&self->frame.base);
}
