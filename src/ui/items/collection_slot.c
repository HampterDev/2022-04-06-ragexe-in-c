#include "render/rp_pool.h"
#include "render/sprite_atlas.h"
#include "resources/sprite.h"
#include "resources/animation.h"
#include "core/object_type_tables.h"
#include "ui/items/items.h"
#include "game/context.h"
#include "ui/character/character.h"
#include "diag/perf.h"
#include "platform/clock.h"

#define WND_COLLECTION_SLOT_MACHINE 0xc4

#define UI_MSG_SLOT_MACHINE 0x50
#define MODE_MSG_SLOT_MACHINE_SPIN 0x11a

#define SLOT_FRAME_MS_STOPPING 0xc8
#define SLOT_CLOSE_DELAY_MS 2000

struct UICollectionSystemWnd {
    struct UIFrameWnd frame;
    int frame_ms;
    int spin_id;
    int action;
    int motion;
    int stop_motion;
    BYTE stopped;
    DWORD frame_time;
    struct ActRes *act;
    struct SprRes *spr;
};

struct UIWindow;
void ui_collection_system_wnd_destroy(struct UIWindow *window);
void ui_collection_system_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_collection_system_wnd_on_process(struct UIWindow *self);
void ui_collection_system_wnd_on_draw(struct UIWindow *self);
void ui_collection_system_wnd_on_l_btn_down(struct UIWindow *self, int x, int y);
int ui_collection_system_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_collection_system_wnd_ops = {
    .destroy = ui_collection_system_wnd_destroy,
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
    .on_create = ui_collection_system_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_collection_system_wnd_on_process,
    .on_draw = ui_collection_system_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_collection_system_wnd_on_l_btn_down,
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
    .send_msg = ui_collection_system_wnd_send_msg,
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

void ui_collection_system_wnd_destruct(struct UICollectionSystemWnd *self)
{
    self->frame.base.ops = &ui_collection_system_wnd_ops;
    ui_frame_wnd_destruct(&self->frame);
}

void ui_collection_system_wnd_destroy(struct UIWindow *window)
{
    struct UICollectionSystemWnd *self = (struct UICollectionSystemWnd *)window;
    ui_collection_system_wnd_destruct(self);
    mem_free(self);
}

void ui_collection_system_wnd_on_create(struct UIWindow *self, int width, int height)
{
}

void ui_collection_system_wnd_on_process(struct UIWindow *window)
{
    struct UICollectionSystemWnd *self = (struct UICollectionSystemWnd *)window;
    DWORD now;

    ui_window_invalidate_base(&self->frame.base);
    now = platform_ticks_ms();
    if (self->motion != self->stop_motion)
        return;
    if (!self->stopped) {
        self->stopped = 1;
        self->frame_time = now;
    }
    if ((int)(now - self->frame_time) > SLOT_CLOSE_DELAY_MS)
        ui_close_window(WND_COLLECTION_SLOT_MACHINE);
}

void ui_collection_system_wnd_draw_clips(struct UICollectionSystemWnd *self)
{
    struct UIWindow *wnd = &self->frame.base;
    struct Motion *motion = act_res_get_motion(self->act, self->action, self->motion);
    DWORD *palette = self->spr->palette;
    struct SpriteRegion place = {0};
    int i;

    for (i = 0; i < motion->clip_count; i++) {
        struct SprClip *clip = motion_get_clip(motion, i);
        struct SprImg *img;
        void *tex;

        if (clip->sprIndex == -1)
            continue;
        img = sprite_resource_image(self->spr, 0, clip->sprIndex);
        if (!img) continue;
        tex = sprite_atlas_resolve(g_renderer, img, palette, &place);
        if (!tex) continue;
        ui_canvas_draw_texture(wnd->canvas, wnd->width / 2 + clip->x, wnd->height / 2 + clip->y, tex,
                             place.x, place.y, img->width, img->height,
                             clip->flags, img->widthHalved + 1, img->heightHalved + 1);
    }
}

void ui_collection_system_wnd_advance(struct UICollectionSystemWnd *self, DWORD now)
{
    if (!self->stopped && (int)(now - self->frame_time) > self->frame_ms) {
        self->motion++;
        if (self->motion >= (int)actres_frame_count(self->act, self->action))
            self->motion = 0;
        self->frame_time = now;
    }
}

void ui_collection_system_wnd_on_draw(struct UIWindow *window)
{
    struct UICollectionSystemWnd *self = (struct UICollectionSystemWnd *)window;
    ui_collection_system_wnd_draw_clips(self);
    ui_collection_system_wnd_advance(self, platform_ticks_ms());
}

void ui_collection_system_wnd_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UICollectionSystemWnd *self = (struct UICollectionSystemWnd *)window;
    if (self->action != 0 || !game_current_mode())
        return;
    self->action = 1;
    game_send_message(MODE_MSG_SLOT_MACHINE_SPIN, self->spin_id, 0, 0, 0);
}

int ui_collection_system_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                              int val1, int val2, int val3, int val4)
{
    struct UICollectionSystemWnd *self = (struct UICollectionSystemWnd *)window;
    if (msg == UI_MSG_SLOT_MACHINE) {
        if (val1 == 0) {
            self->spin_id = val2;
            return 0;
        }
        self->stop_motion = val2;
        self->motion = 0;
        self->frame_ms = SLOT_FRAME_MS_STOPPING;
    }
    return 0;
}

struct UICollectionSystemWnd *ui_collection_system_wnd_construct(struct UICollectionSystemWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_collection_system_wnd_ops;
    self->frame_ms = 0xf;
    self->spin_id = -1;
    self->action = 0;
    self->motion = 0;
    self->stop_motion = -1;
    self->stopped = 0;
    self->frame_time = 0;
    self->act = (struct ActRes *)res_mgr_get(g_resMgr(), "collection_slotmachine.act");
    self->spr = (struct SprRes *)res_mgr_get(g_resMgr(), "collection_slotmachine.spr");
    return self;
}

struct UIWindow *ui_collection_system_wnd_new(void)
{
    return (struct UIWindow *)ui_collection_system_wnd_construct(mem_alloc(sizeof(struct UICollectionSystemWnd)));
}
