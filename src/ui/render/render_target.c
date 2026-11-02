#include "ui/window_types.h"
#include "effects/effect_mgr.h"
#include "render/rp_pool.h"
#include "core/object_type_tables.h"
#include "ui/window_access.h"
#include "ui/render/character_render.h"
#include "diag/perf.h"

#define RENDER_TARGET_WND_ID 0x125
#define RENDER_TARGET_EFFECT 0x442

struct UIFrameWnd;
struct TexDrawPrim;
struct UIRenderTargetWnd;
struct UIWindow;
void ui_render_target_wnd_destroy(struct UIWindow *window);
void ui_render_target_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_render_target_wnd_on_process(struct UIWindow *self);
void ui_render_target_wnd_on_draw(struct UIWindow *self);
const struct UIWindowOps ui_render_target_wnd_ops = {
    .destroy = ui_render_target_wnd_destroy,
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
    .on_create = ui_render_target_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_render_target_wnd_on_process,
    .on_draw = ui_render_target_wnd_on_draw,
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
    .send_msg = ui_frame_wnd_send_msg,
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

struct UIRenderTargetWnd {
    struct UIFrameWnd frame;
    struct CharacterRender *character;
    struct List effects;
    struct List effect_quads;
};

static void quad_list_append(struct List *dst, const struct List *src)
{
    struct ListNode *node;

    for (node = src->head->next; node != src->head; node = node->next)
        list_push_back(dst, LIST_VALUE(node, struct TexDrawPrim *), sizeof(struct TexDrawPrim *));
}

struct UIRenderTargetWnd *ui_render_target_wnd_construct(struct UIRenderTargetWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 1);
    self->frame.base.ops = &ui_render_target_wnd_ops;
    self->character = NULL;
    list_init(&self->effects);
    list_init(&self->effect_quads);
    return self;
}

struct UIWindow *ui_render_target_wnd_new(void)
{
    return (struct UIWindow *)ui_render_target_wnd_construct(mem_alloc(sizeof(struct UIRenderTargetWnd)));
}

static void ui_render_target_wnd_destruct(struct UIRenderTargetWnd *self)
{
    struct ListNode *node;

    self->frame.base.ops = &ui_render_target_wnd_ops;
    for (node = self->effects.head->next; node != self->effects.head; node = node->next)
        effect_mgr_remove(g_effectMgr, *LIST_VALUE(node, struct Effect *), 1);
    if (self->character)
        character_render_destroy(self->character);
    list_free(&self->effect_quads, NULL);
    list_free(&self->effects, NULL);
    ui_frame_wnd_destruct(&self->frame);
}

void ui_render_target_wnd_destroy(struct UIWindow *window)
{
    struct UIRenderTargetWnd *self = (struct UIRenderTargetWnd *)window;
    ui_render_target_wnd_destruct(self);
    mem_free(self);
}

static void take_effect_quad(struct TexDrawPrim *quad, int list_flags)
{
    struct UIRenderTargetWnd *wnd =
        (struct UIRenderTargetWnd *)ui_window_find(RENDER_TARGET_WND_ID);

    (void)list_flags;
    if (wnd)
        list_push_back(&wnd->effect_quads, &quad, sizeof quad);
}

void ui_render_target_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UIRenderTargetWnd *self = (struct UIRenderTargetWnd *)window;
    struct CharacterRender *ch = character_render_create(width, height);
    Vec3 pos;
    struct Effect *effect;

    self->character = ch;
    ch->pos[0] = 0.0f;
    ch->pos[1] = 0.0f;
    ch->pos[2] = 0.0f;
    ch->job = 0xfe9;
    ch->sex = 1;
    ch->head = 1;
    ch->body_palette = 0;
    ch->head_palette = 0;
    ch->action = 4;
    ch->motion = 0;
    ch->accessory_top = 0x4f3;
    ch->accessory_mid = 0x1ef;
    ch->accessory_bottom = 0x369;
    character_render_set_robe(ch, 0x11);
    pos.x = ch->pos[0];
    pos.y = ch->pos[1];
    pos.z = ch->pos[2];
    effect = effect_mgr_spawn(g_effectMgr, 0, RENDER_TARGET_EFFECT, pos, 0, -1, 0, 0);
    if (effect) {
        effect->ops->set_submit(effect, take_effect_quad);
        list_push_back(&self->effects, &effect, sizeof effect);
    }
}

void ui_render_target_wnd_on_process(struct UIWindow *window)
{
    struct UIRenderTargetWnd *self = (struct UIRenderTargetWnd *)window;
    ui_window_invalidate_base(&self->frame.base);
}

void ui_render_target_wnd_on_draw(struct UIWindow *window)
{
    struct UIRenderTargetWnd *self = (struct UIRenderTargetWnd *)window;
    struct List quads;

    ui_window_reset_draws(&self->frame.base, self->frame.base.width, self->frame.base.height, 0xff000000);
    character_render_collect_quads(self->character, &quads);
    quad_list_append(&self->frame.base.draws, &quads);
    quad_list_append(&self->frame.base.draws, &self->effect_quads);
    list_clear(&self->effect_quads, NULL);
    list_free(&quads, NULL);
}
