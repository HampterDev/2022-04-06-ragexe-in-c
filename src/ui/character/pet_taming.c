#include "render/rp_pool.h"
#include "render/sprite_atlas.h"
#include "resources/sprite.h"
#include "resources/animation.h"
#include "core/object_type_tables.h"
#include "game/context.h"
#include "ui/window.h"
#include <mmsystem.h>
#include "ui/draw.h"
#include "ui/skin.h"
#include "ui/window_mgr.h"
#include "ui/game.h"
#include "ui/ui_items.h"
#include "ui/controls/controls.h"
#include "diag/perf.h"
#include "ui/wndmgr/make_window_internal.h"

#define PET_TAMING_WND_ID 0x5b
#define MSG_PET_TAMING_RESULT 0x50
#define REQ_PET_TAMING 0x95
#define PET_TAMING_CLOSE_MS 2000
#define PET_TAMING_STOP_STEP 7

struct UIPetTamingDeceiveWnd {
    struct UIFrameWnd frame;
    int step;
    int action;
    int ticks;
    int item;
    int stop_frame;
    int max_step;
    DWORD stopped_at;
    int clicked;
};
OFFSET_ASSERT(UIPetTamingDeceiveWnd, stopped_at, 0xac);
SIZE_ASSERT(UIPetTamingDeceiveWnd, 0xb4);

struct UIWindow;
void ui_pet_taming_deceive_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_pet_taming_deceive_wnd_on_process(struct UIWindow *self);
void ui_pet_taming_deceive_wnd_on_draw(struct UIWindow *self);
void ui_pet_taming_deceive_wnd_on_l_btn_down(struct UIWindow *self, int x, int y);
int ui_pet_taming_deceive_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
void ui_pet_taming_deceive_wnd_destroy(struct UIWindow *window);
const struct UIWindowOps ui_pet_taming_deceive_wnd_ops = {
    .destroy = ui_pet_taming_deceive_wnd_destroy,
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
    .on_create = ui_pet_taming_deceive_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_pet_taming_deceive_wnd_on_process,
    .on_draw = ui_pet_taming_deceive_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_pet_taming_deceive_wnd_on_l_btn_down,
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
    .send_msg = ui_pet_taming_deceive_wnd_send_msg,
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

struct UIPetTamingDeceiveWnd *ui_pet_taming_deceive_wnd_construct(struct UIPetTamingDeceiveWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_pet_taming_deceive_wnd_ops;
    self->step = 1;
    self->action = 0;
    self->ticks = 0;
    self->stop_frame = -1;
    self->max_step = 10000;
    self->clicked = 0;
    self->stopped_at = (DWORD)-1;
    return self;
}

struct UIWindow *ui_pet_taming_deceive_wnd_new(void)
{
    return (struct UIWindow *)ui_pet_taming_deceive_wnd_construct(mem_alloc(sizeof(struct UIPetTamingDeceiveWnd)));
}

void ui_pet_taming_deceive_wnd_on_create(struct UIWindow *self, int width, int height)
{
}

void ui_pet_taming_deceive_wnd_on_process(struct UIWindow *window)
{
    struct UIPetTamingDeceiveWnd *self = (struct UIPetTamingDeceiveWnd *)window;
    ui_window_invalidate_base(&self->frame.base);
    if (self->stop_frame != self->ticks / self->step)
        return;
    if (self->stopped_at == (DWORD)-1) {
        if (self->step <= self->max_step)
            self->stopped_at = timeGetTime();
    } else if (self->step <= self->max_step && timeGetTime() - self->stopped_at >= PET_TAMING_CLOSE_MS) {
        ui_window_mgr_post_quit1(&g_windowMgr, PET_TAMING_WND_ID);
    }
}

void ui_pet_taming_deceive_wnd_on_draw(struct UIWindow *window)
{
    struct UIPetTamingDeceiveWnd *self = (struct UIPetTamingDeceiveWnd *)window;
    void *act = res_mgr_get(g_resMgr(), "SlotMachine.act");
    struct SprRes *spr = res_mgr_get(g_resMgr(), "SlotMachine.spr");
    int frame = self->ticks / self->step;
    struct Motion *motion = act_res_get_motion(act, self->action, frame);
    struct SpriteRegion place = {0};
    int i;

    for (i = 0; i < motion->clip_count; i++) {
        struct SprClip *clip = motion_get_clip(motion, i);
        struct SprImg *img;
        void *tex;

        if (clip->sprIndex == -1)
            continue;
        img = sprite_resource_image(spr, 0, clip->sprIndex);
        if (!img) continue;
        tex = sprite_atlas_resolve(g_renderer, img, spr->palette, &place);
        if (!tex) continue;
        ui_canvas_draw_texture(self->frame.base.canvas, self->frame.base.width / 2 + clip->x,
                             self->frame.base.height / 2 + clip->y, tex, place.x,
                             place.y, img->width, img->height, clip->flags, img->widthHalved + 1,
                             img->heightHalved + 1);
    }
    if (self->stop_frame != frame || self->step > self->max_step)
        self->ticks++;
    if (frame >= (int)actres_frame_count(act, self->action)) {
        self->ticks = 0;
        if (self->stop_frame != -1)
            self->step++;
    }
}

void ui_pet_taming_deceive_wnd_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIPetTamingDeceiveWnd *self = (struct UIPetTamingDeceiveWnd *)window;
    if (self->clicked)
        return;
    self->action = 1;
    self->stop_frame = PET_TAMING_STOP_STEP;
    self->clicked = 1;
    game_send_message(REQ_PET_TAMING, self->item, 0, 0, 0);
}

int ui_pet_taming_deceive_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                             int val1, int val2, int val3, int val4)
{
    struct UIPetTamingDeceiveWnd *self = (struct UIPetTamingDeceiveWnd *)window;
    if (msg == MSG_PET_TAMING_RESULT) {
        if (val1 == 0) {
            self->item = val2;
            return 0;
        }
        self->stop_frame = val2;
        self->step = PET_TAMING_STOP_STEP;
        self->max_step = PET_TAMING_STOP_STEP;
    }
    return 0;
}

void ui_pet_taming_deceive_wnd_destroy(struct UIWindow *window)
{
    struct UIFrameWnd *self = (struct UIFrameWnd *)window;
    self->base.ops = &ui_pet_taming_deceive_wnd_ops;
    ui_frame_wnd_destruct(self);
    mem_free(self);
}
