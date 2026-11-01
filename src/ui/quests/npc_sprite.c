#include "render/rp_pool.h"
#include "render/sprite_atlas.h"
#include "resources/sprite.h"
#include "resources/animation.h"
#include "navi/navi_manager.h"
#include "core/object_type_tables.h"
#include "world/mode_manager.h"
#include <stdio.h>
#include "ui/quests/quests.h"
#include "diag/perf.h"

#define NPC_SPRITE_DIR "npc\\"

struct UINpcSprWnd;
struct UIWindow;
void ui_npc_spr_wnd_destroy(struct UIWindow *window);
void ui_npc_spr_wnd_on_draw(struct UIWindow *self);
void ui_npc_spr_wnd_on_mouse_shape(struct UIWindow *self, int x_unused, int y_unused);
void ui_npc_spr_wnd_on_l_btn_up(struct UIWindow *self, int x_unused, int y_unused);
int ui_npc_spr_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_npc_spr_wnd_ops = {
    .destroy = ui_npc_spr_wnd_destroy,
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
    .on_draw = ui_npc_spr_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_window_on_l_btn_down_base,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_window_on_mouse_move_base,
    .on_mouse_hover = ui_window_on_mouse_hover_base,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_npc_spr_wnd_on_mouse_shape,
    .on_l_btn_up = ui_npc_spr_wnd_on_l_btn_up,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_window_snap_base,
    .send_msg = ui_npc_spr_wnd_send_msg,
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

struct UINpcSprWnd {
    struct UIWindow base;
    struct String map;
    struct ActRes *act;
    struct SprRes *spr;
    int x, y;
};

static struct SprImg *spr_res_image(struct SprRes *spr, const struct SprClip *clip)
{
    return clip ? sprite_resource_image(spr, clip->clipType, clip->sprIndex) : NULL;
}

static struct UINpcSprWnd *ui_npc_spr_wnd_construct(struct UINpcSprWnd *self)
{
    ui_window_construct(&self->base, 0);
    self->base.ops = &ui_npc_spr_wnd_ops;
    string_init(&self->map);
    self->act = NULL;
    self->spr = NULL;
    self->x = 0;
    self->y = 0;
    return self;
}

struct UINpcSprWnd *ui_npc_spr_wnd_new(void)
{
    struct UINpcSprWnd *self = mem_alloc(sizeof *self);

    return self ? ui_npc_spr_wnd_construct(self) : NULL;
}

static void ui_npc_spr_wnd_destruct(struct UINpcSprWnd *self)
{
    self->base.ops = &ui_npc_spr_wnd_ops;
    string_free(&self->map);
    ui_window_destruct(&self->base);
}

void ui_npc_spr_wnd_destroy(struct UIWindow *window)
{
    struct UINpcSprWnd *self = (struct UINpcSprWnd *)window;
    ui_npc_spr_wnd_destruct(self);
    mem_free(self);
}

static struct SprClip *ui_npc_spr_wnd_first_clip(struct UINpcSprWnd *self)
{
    struct Motion *motion = act_res_get_motion(self->act, 0, 0);
    int i;

    for (i = 0; i < motion->clip_count; i++) {
        struct SprClip *clip = motion_get_clip(motion, i);
        if (clip->sprIndex != -1)
            return clip;
    }
    return NULL;
}

void ui_npc_spr_wnd_create(struct UINpcSprWnd *self, int width, int height, struct String name)
{
    char path[MAX_PATH];
    struct SprClip *clip;

    sprintf(path, NPC_SPRITE_DIR "%s.act", string_cstr(&name));
    if (res_mgr_is_available(g_resMgr(), path))
        self->act = (struct ActRes *)res_mgr_get(g_resMgr(), path);
    sprintf(path, NPC_SPRITE_DIR "%s.spr", string_cstr(&name));
    if (res_mgr_is_available(g_resMgr(), path))
        self->spr = (struct SprRes *)res_mgr_get(g_resMgr(), path);

    if (self->act && self->spr && (clip = ui_npc_spr_wnd_first_clip(self)) != NULL) {
        struct SprImg *img = spr_res_image(self->spr, clip);
        if (img) {
            width = img->width;
            height = img->height;
        }
    }
    ui_window_create(&self->base, width, height);
    string_free(&name);
}

void ui_npc_spr_wnd_set_target(struct UINpcSprWnd *self, struct String map, int x, int y)
{
    string_assign(&self->map, string_cstr(&map), map.size);
    self->x = x;
    self->y = y;
    string_free(&map);
}

void ui_npc_spr_wnd_on_draw(struct UIWindow *window)
{
    struct UINpcSprWnd *self = (struct UINpcSprWnd *)window;
    struct Motion *motion;
    struct SpriteRegion place = {0};
    int i;

    ui_window_clear_canvas(&self->base, 0xffff00ff);
    if (self->base.parent)
        ui_window_invalidate(self->base.parent);
    if (!self->act || !self->spr)
        return;

    motion = act_res_get_motion(self->act, 0, 0);
    for (i = 0; i < motion->clip_count; i++) {
        struct SprClip *clip = motion_get_clip(motion, i);
        struct SprImg *img;
        void *tex;

        if (clip->sprIndex == -1)
            continue;
        place.u = place.v = 0;
        img = spr_res_image(self->spr, clip);
        if (!img)
            continue;
        if (clip->clipType == 0) {
            tex = sprite_atlas_resolve(g_renderer, img, self->spr->palette, &place);
        } else {
            tex = img->texture;
            place.x = place.y = 0;
        }
        if (!tex)
            continue;
        if (img->width != self->base.width || img->height != self->base.height)
            ui_window_resize_base(&self->base, img->width, img->height);
        ui_canvas_draw_texture(self->base.canvas, 0, 0, tex, place.x, place.y,
                             img->width, img->height, clip->flags, 1, 1);
    }
}

void ui_npc_spr_wnd_on_mouse_shape(struct UIWindow *self, int x_unused, int y_unused)
{
    mode_set_cursor_action(client_current_mode(), CURSOR_HAND);
}

void ui_npc_spr_wnd_on_l_btn_up(struct UIWindow *window, int x_unused, int y_unused)
{
    struct UINpcSprWnd *self = (struct UINpcSprWnd *)window;
    if (!self->map.size)
        return;
    navi_navigate(&g_navi, string_cstr(&self->map), 0, 5, 0, self->x, self->y, 0x3ea);
}

int ui_npc_spr_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1, int val2,
                                   int val3, int val4)
{
    struct UINpcSprWnd *self = (struct UINpcSprWnd *)window;
    ui_window_send_msg_base(&self->base, sender, msg, val1, val2, val3, val4);
    return 0;
}
