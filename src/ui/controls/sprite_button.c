#include "ui/controls/buttons.h"
#include "render/rp_pool.h"
#include "render/sprite_atlas.h"
#include "resources/palette.h"
#include "resources/sprite.h"
#include "resources/animation.h"
#include "core/object_type_tables.h"
#include "core/object_type.h"
#include "ui/controls/controls.h"
#include "ui/ui_std.h"
#include "ui/character/character.h"
#include "base.h"
#include "diag/perf.h"

struct UISpriteButton;
struct UIWindow;
const struct UIButtonOps ui_headspr_button_ops = {
    .base = {
        .destroy = ui_headspr_button_destroy,
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
        .on_draw = ui_headspr_button_on_draw,
        .on_focus = ui_window_on_focus_base,
        .on_focus_lost = ui_window_on_focus_lost_base,
        .on_l_btn_down = ui_sprite_button_on_l_btn_down,
        .on_l_btn_dbl_clk = ui_sprite_button_on_l_btn_dbl_clk,
        .on_mouse_move = ui_sprite_button_on_mouse_move,
        .on_mouse_hover = ui_sprite_button_on_mouse_hover,
        .track_mouse = ui_button_track_mouse,
        .on_mouse_shape = ui_button_on_mouse_shape,
        .on_l_btn_up = ui_button_on_l_btn_up,
        .on_r_btn_down = ui_window_on_r_btn_down_base,
        .on_r_btn_up = ui_window_on_r_btn_up_base,
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

#define SPRITE_BUTTON(wnd) ((struct UISpriteButton *)(wnd))

struct UIWindow *ui_sprite_button_construct(struct UIWindow *wnd)
{
    struct UISpriteButton *self = SPRITE_BUTTON(wnd);

    ui_button_construct(&self->button);
    wnd->ops = &ui_sprite_button_ops.base;
    string_init(&self->spr);
    string_init(&self->act);
    string_init(&self->palette_path);
    self->action = 0;
    self->motion = 0;
    self->offset_x = 0;
    self->offset_y = 0;
    self->animating = 0;
    self->loop = 1;
    return wnd;
}

void ui_sprite_button_destruct(struct UISpriteButton *self)
{
    self->button.base.ops = &ui_sprite_button_ops.base;
    string_free(&self->palette_path);
    string_free(&self->act);
    string_free(&self->spr);
    ui_button_destruct(&self->button);
}

void ui_sprite_button_destroy(struct UIWindow *window)
{
    struct UISpriteButton *self = (struct UISpriteButton *)window;
    ui_sprite_button_destruct(self);
    mem_free(self);
}

static DWORD *sprite_button_palette(struct UISpriteButton *self, struct SprRes *spr)
{
    struct PaletteRes *pal;

    if (self->palette_path.size == 0)
        return spr->palette;
    pal = object_as_type(res_mgr_get(g_resMgr(), string_cstr(&self->palette_path)), OBJECT_PALETTE_RESOURCE);
    return palette_resource_colors(pal, spr->palette);
}

static void *sprite_texture(struct SprImg *img, const struct SprClip *clip, DWORD *palette,
                            struct SpriteRegion *place)
{
    void *tex;

    memset(place, 0, sizeof *place);
    if (clip->clipType != 0)
        return img->texture;
    tex = sprite_atlas_resolve(g_renderer, img, palette, place);
    return tex;
}

void ui_sprite_button_on_draw(struct UIWindow *window)
{
    struct UISpriteButton *self = (struct UISpriteButton *)window;
    struct UIWindow *wnd = &self->button.base;
    struct ActRes *act = res_mgr_get(g_resMgr(), string_cstr(&self->act));
    struct SprRes *spr = res_mgr_get(g_resMgr(), string_cstr(&self->spr));
    struct Motion *motion;
    DWORD *palette;
    int half_w = 0, half_h = 0, first_x = 0, first_y = 0;
    unsigned i;

    if (!act || !spr)
        return;
    palette = sprite_button_palette(self, spr);
    motion = (struct Motion *)act_res_get_motion((struct ActRes *)act, self->action, self->motion);
    for (i = 0; i < motion->clip_count; i++) {
        struct SpriteRegion place = {0};
        struct SprClip *clip = motion_get_clip((struct Motion *)motion, i);
        struct SprImg *img;
        void *tex;
        int x, y, w, h, left, top;

        if (clip->sprIndex == -1)
            continue;
        img = spr_res_image(spr, clip);
        if (!img)
            continue;
        tex = sprite_texture(img, clip, palette, &place);
        if (!tex)
            continue;
        if (i == 0) {
            half_w = img->width / 2;
            half_h = img->height / 2;
            first_x = clip->x;
            first_y = clip->y;
            x = wnd->width / 2 - half_w;
            y = wnd->height / 2 - half_h;
        } else {
            x = wnd->width / 2 - first_x - half_w + clip->x;
            y = wnd->height / 2 - first_y - half_h + clip->y;
        }
        w = img->width;
        h = img->height;
        if (x + w > wnd->width)
            w = wnd->width - x;
        if (y + h > wnd->height)
            h = wnd->height - y;
        left = x < 0 ? -x : 0;
        top = y < 0 ? -y : 0;
        if (wnd->canvas) {
            int sx = (int)((float)place.x + (float)left);
            int sy = (int)((float)place.y + (float)top);

            ui_canvas_draw_texture(wnd->canvas, x < 0 ? 0 : x, y < 0 ? 0 : y, tex, sx < 0 ? 0 : sx, sy < 0 ? 0 : sy,
                                 w - left < 0 ? 0 : w - left, h - top < 0 ? 0 : h - top, clip->flags,
                                 img->widthHalved + 1, img->heightHalved + 1);
        }
    }
    if (self->animating) {
        self->motion++;
        if (self->action >= (int)animation_action_count(act)) {
            self->action = 0;
            self->motion = 0;
        }
        if (self->motion >= (int)(int)actres_frame_count((struct ActRes *)act, self->action))
            self->motion = 0;
    }
}

void ui_sprite_button_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UISpriteButton *self = (struct UISpriteButton *)window;
    struct UIWindow *wnd = &self->button.base;

    ui_button_on_l_btn_down((struct UIWindow *)(&self->button), x, y);
    ui_window_on_l_btn_down(ui_window_get_ancestor(wnd), wnd->x + x, wnd->y + y);
}

void ui_sprite_button_on_l_btn_dbl_clk(struct UIWindow *window, int x, int y)
{
    struct UISpriteButton *self = (struct UISpriteButton *)window;
    struct UIWindow *wnd = &self->button.base;

    ui_sprite_button_on_l_btn_down(wnd, x, y);
    ui_window_on_l_btn_dbl_clk(ui_window_get_ancestor(wnd), wnd->x + x, wnd->y + y);
}

void ui_sprite_button_on_mouse_move(struct UIWindow *window, int x, int y)
{
    struct UISpriteButton *self = (struct UISpriteButton *)window;
    struct UIWindow *wnd = &self->button.base;

    ui_window_on_mouse_move(ui_window_get_ancestor(wnd), wnd->x + x, wnd->y + y);
}

void ui_sprite_button_on_mouse_hover(struct UIWindow *window, int x, int y)
{
    struct UISpriteButton *self = (struct UISpriteButton *)window;
    struct UIWindow *wnd = &self->button.base;

    ui_button_on_mouse_hover((struct UIWindow *)(&self->button), x, y);
    ui_window_on_mouse_hover(ui_window_get_ancestor(wnd), wnd->x + x, wnd->y + y);
}

void ui_sprite_button_set_act(struct UIWindow *wnd, const char *path)
{
    string_assign_cstr(&SPRITE_BUTTON(wnd)->act, path);
}

void ui_sprite_button_set_palette(struct UIWindow *wnd, const char *path)
{
    string_assign_cstr(&SPRITE_BUTTON(wnd)->palette_path, path);
}

struct UIWindow *ui_headspr_button_construct(struct UIWindow *wnd)
{
    ui_sprite_button_construct(wnd);
    wnd->ops = &ui_headspr_button_ops.base;
    return wnd;
}

void ui_headspr_button_destruct(struct UISpriteButton *self)
{
    self->button.base.ops = &ui_headspr_button_ops.base;
    ui_sprite_button_destruct(self);
}

void ui_headspr_button_destroy(struct UIWindow *window)
{
    struct UISpriteButton *self = (struct UISpriteButton *)window;
    ui_headspr_button_destruct(self);
    mem_free(self);
}

void ui_headspr_button_on_draw(struct UIWindow *window)
{
    struct UISpriteButton *self = (struct UISpriteButton *)window;
    struct UIWindow *wnd = &self->button.base;
    struct ActRes *act;
    struct SprRes *spr;
    struct SprClip *clip;
    struct SprImg *img;
    struct SpriteRegion place = {0};
    DWORD *palette;
    void *tex;
    int sx, sy;

    if (!wnd->canvas)
        return;
    act = res_mgr_get(g_resMgr(), string_cstr(&self->act));
    if (!act)
        return;
    spr = res_mgr_get(g_resMgr(), string_cstr(&self->spr));
    if (!spr)
        return;
    palette = sprite_button_palette(self, spr);
    clip = motion_get_clip(act_res_get_motion(act, self->action, 0), 1);
    img = spr_res_image(spr, clip);
    if (!img)
        return;
    tex = sprite_texture(img, clip, palette, &place);
    if (!tex)
        return;
    sx = place.x;
    sy = place.y;
    ui_canvas_draw_texture(wnd->canvas, 0, 0, tex, sx < 0 ? 0 : sx, sy < 0 ? 0 : sy, img->width, img->height,
                         clip->flags, img->widthHalved + 1, img->heightHalved + 1);
}

static struct SprImg *sprite_button_frame_image(struct UISpriteButton *self, int action, int motion)
{
    struct SprRes *spr = res_mgr_get(g_resMgr(), string_cstr(&self->spr));
    struct ActRes *act = res_mgr_get(g_resMgr(), string_cstr(&self->act));
    struct Motion *m;
    const struct SprClip *clip;

    if (!spr || !act)
        return NULL;
    m = act_res_get_motion(act, action, motion);
    clip = motion_get_clip(m, 0);
    return sprite_resource_image(spr, 0, clip->sprIndex);
}

int ui_sprite_button_frame_height(struct UIWindow *self, int action, int motion)
{
    const struct SprImg *image = sprite_button_frame_image(SPRITE_BUTTON(self), action, motion);
    return image ? image->height : 0;
}

int ui_sprite_button_frame_width(struct UIWindow *self, int action, int motion)
{
    const struct SprImg *image = sprite_button_frame_image(SPRITE_BUTTON(self), action, motion);
    return image ? image->width : 0;
}

const struct UIButtonOps ui_sprite_button_ops = {
    .base = {
        .destroy = ui_sprite_button_destroy,
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
        .on_draw = ui_sprite_button_on_draw,
        .on_focus = ui_window_on_focus_base,
        .on_focus_lost = ui_window_on_focus_lost_base,
        .on_l_btn_down = ui_sprite_button_on_l_btn_down,
        .on_l_btn_dbl_clk = ui_sprite_button_on_l_btn_dbl_clk,
        .on_mouse_move = ui_sprite_button_on_mouse_move,
        .on_mouse_hover = ui_sprite_button_on_mouse_hover,
        .track_mouse = ui_button_track_mouse,
        .on_mouse_shape = ui_button_on_mouse_shape,
        .on_l_btn_up = ui_button_on_l_btn_up,
        .on_r_btn_down = ui_window_on_r_btn_down_base,
        .on_r_btn_up = ui_window_on_r_btn_up_base,
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
