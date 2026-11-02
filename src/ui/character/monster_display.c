#include "ui/window_access.h"
#include "render/rp_pool.h"
#include "render/sprite_atlas.h"
#include "resources/sprite.h"
#include "core/object_type_tables.h"
#include <stdio.h>
#include "ui/character/character.h"
#include "core/msg_str.h"
#include "diag/perf.h"

struct UIMonsterDisplayWnd;
struct UIWindow;
void ui_monster_display_wnd_destroy(struct UIWindow *window);
void ui_monster_display_wnd_on_create(struct UIWindow *self, int width_unused, int height_unused);
void ui_monster_display_wnd_on_draw(struct UIWindow *self);
intptr_t ui_monster_display_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4);
const struct UIWindowOps ui_monster_display_wnd_ops = {
    .destroy = ui_monster_display_wnd_destroy,
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
    .on_create = ui_monster_display_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_monster_display_wnd_on_draw,
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
    .send_msg = ui_monster_display_wnd_send_msg,
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

#define WND_ID_MONSTER_DISPLAY 0xc9
#define MSG_MONSTER_DISPLAY_NAME 0x3e
#define MSG_MONSTER_DISPLAY_TITLE 0x20f
#define MONSTER_SPRITE_DIR "npc\\"
#define MONSTER_BOX_WIDTH 0x4b
#define MONSTER_BOX_HEIGHT 0x7d
#define MONSTER_BODY_TOP 0x11
#define MONSTER_BODY_LESS 0x26

struct UIMonsterDisplayWnd {
    struct UIFrameWnd frame;
    struct String name;
    struct String title_text;
    BYTE sprite_loaded;
};

struct UIMonsterDisplayWnd *ui_monster_display_wnd_construct(struct UIMonsterDisplayWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_monster_display_wnd_ops;
    string_init(&self->name);
    string_init(&self->title_text);
    return self;
}

struct UIWindow *ui_monster_display_wnd_new(void)
{
    return (struct UIWindow *)ui_monster_display_wnd_construct(mem_alloc(sizeof(struct UIMonsterDisplayWnd)));
}

void ui_monster_display_wnd_destruct(struct UIMonsterDisplayWnd *self)
{
    self->frame.base.ops = &ui_monster_display_wnd_ops;
    string_free(&self->title_text);
    string_free(&self->name);
    ui_frame_wnd_destruct(&self->frame);
}

void ui_monster_display_wnd_destroy(struct UIWindow *window)
{
    struct UIMonsterDisplayWnd *self = (struct UIMonsterDisplayWnd *)window;
    ui_monster_display_wnd_destruct(self);
    mem_free(self);
}

void ui_monster_display_wnd_on_create(struct UIWindow *window, int width_unused, int height_unused)
{
    struct UIMonsterDisplayWnd *self = (struct UIMonsterDisplayWnd *)window;
    const char *text;

    self->sprite_loaded = 0;
    text = msg_str(MSG_MONSTER_DISPLAY_TITLE);
    string_assign_cstr(&self->title_text, text);
    ui_window_add_close_button(&self->frame.base, "", 0);
}

void ui_monster_display_wnd_on_draw(struct UIWindow *window)
{
    struct UIMonsterDisplayWnd *self = (struct UIMonsterDisplayWnd *)window;
    char act_path[0x80], spr_path[0x80];
    struct ActRes *act;
    struct SprRes *spr;
    struct Motion *motion;
    struct SpriteRegion place = {0};
    int reach_x = 0, reach_y = 0;
    int centre_x = 0, centre_y = 0;
    int i;

    ui_window_draw_title_bar(&self->frame.base, 1, "", 0);
    ui_window_fill_rect(&self->frame.base, 0, MONSTER_BODY_TOP, self->frame.base.width,
                      self->frame.base.height - MONSTER_BODY_LESS, ui_skin_colour(2, 2));

    sprintf(spr_path, MONSTER_SPRITE_DIR "%s.spr", string_cstr(&self->name));
    sprintf(act_path, MONSTER_SPRITE_DIR "%s.act", string_cstr(&self->name));
    if (!res_mgr_is_available(g_resMgr(), act_path) || !res_mgr_is_available(g_resMgr(), spr_path))
        return;
    act = (struct ActRes *)res_mgr_get(g_resMgr(), act_path);
    spr = (struct SprRes *)res_mgr_get(g_resMgr(), spr_path);
    if (!act || !spr)
        return;

    motion = act_res_get_motion(act, 0, 0);
    for (i = 0; i < motion->clip_count; i++) {
        struct SprClip *clip = motion_get_clip(motion, i);
        struct SprImg *img;

        if (clip->sprIndex == -1)
            continue;
        img = spr_res_image(spr, clip);
        if (!img)
            continue;
        if (reach_x < -clip->x) {
            reach_x = -clip->x;
            centre_x = (MONSTER_BOX_WIDTH - img->width) / 2;
        }
        if (reach_y < -clip->y) {
            reach_y = -clip->y;
            centre_y = (MONSTER_BOX_HEIGHT - img->height) / 2;
        }
    }

    for (i = 0; i < motion->clip_count; i++) {
        struct SprClip *clip = motion_get_clip(motion, i);
        struct SprImg *img;
        void *tex;
        int x, y, width, height;

        if (clip->sprIndex == -1)
            continue;
        place.u = 0;
        place.v = 0;
        img = spr_res_image(spr, clip);
        if (!img)
            continue;
        if (clip->clipType == 0) {
            tex = sprite_atlas_resolve(g_renderer, img, spr->palette, &place);
        } else {
            tex = img->texture;
            place.x = place.y = 0;
        }
        if (!tex) continue;
        x = clip->x + centre_x + reach_x;
        y = clip->y + centre_y + reach_y;
        if (x < 0)
            x = 0;
        if (y < 0)
            y = 0;
        width = x + img->width <= MONSTER_BOX_WIDTH ? img->width : MONSTER_BOX_WIDTH - x;
        height = y + img->height <= MONSTER_BOX_HEIGHT ? img->height : MONSTER_BOX_HEIGHT - y;
        ui_canvas_draw_texture(self->frame.base.canvas, x, y, tex, place.x, place.y,
                             width, height, clip->flags, img->widthHalved + 1, img->heightHalved + 1);
    }
}

intptr_t ui_monster_display_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                           intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4)
{
    struct UIMonsterDisplayWnd *self = (struct UIMonsterDisplayWnd *)window;
    if (msg == UI_MSG_COMMAND) {
        if (val1 == UI_ID_CLOSE)
            ui_close_window(WND_ID_MONSTER_DISPLAY);
        return 0;
    }
    if (msg == MSG_MONSTER_DISPLAY_NAME) {
        const char *name = (const char *)val1;

        string_assign_cstr(&self->name, name);
        ui_window_invalidate_base(&self->frame.base);
        ui_window_mgr_raise_window(&g_windowMgr, &self->frame.base);
        return 0;
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
}
