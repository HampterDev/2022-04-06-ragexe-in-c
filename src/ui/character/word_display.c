#include "render/rp_pool.h"
#include "session/state.h"
#include "core/object_type_tables.h"
#include "ui/character/character.h"
#include "game/context.h"
#include "actor/appearance.h"
#include "diag/perf.h"

struct UIWindow;
struct UIWordDisplayWnd;
void ui_word_display_wnd_destroy(struct UIWindow *window);
void ui_word_display_wnd_on_create(struct UIWindow *self, int width_unused, int height_unused);
void ui_word_display_wnd_on_process(struct UIWindow *self);
void ui_word_display_wnd_on_draw(struct UIWindow *self);
int ui_word_display_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender_unused, int msg, int val1, int val2_unused, int val3_unused, int val4_unused);
const struct UIWindowOps ui_word_display_wnd_ops = {
    .destroy = ui_word_display_wnd_destroy,
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
    .on_create = ui_word_display_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_word_display_wnd_on_process,
    .on_draw = ui_word_display_wnd_on_draw,
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
    .send_msg = ui_word_display_wnd_send_msg,
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

#define WORD_DISPLAY_MSG_ACTOR 0x22

struct UIWordDisplayWnd {
    struct UIFrameWnd frame;
    int last_x, last_y;
    struct String text;
    int actor_id;
};

static struct UIWordDisplayWnd *ui_word_display_wnd_construct(struct UIWordDisplayWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_word_display_wnd_ops;
    string_init(&self->text);
    self->actor_id = 0;
    string_clear(&self->text);
    ui_window_create(&self->frame.base, 0x18, 0x18);
    return self;
}

struct UIWordDisplayWnd *ui_word_display_wnd_new(void)
{
    struct UIWordDisplayWnd *self = mem_alloc(sizeof *self);

    return self ? ui_word_display_wnd_construct(self) : NULL;
}

void ui_word_display_wnd_destroy(struct UIWindow *window)
{
    struct UIWordDisplayWnd *self = (struct UIWordDisplayWnd *)window;
    self->frame.base.ops = &ui_word_display_wnd_ops;
    string_free(&self->text);
    ui_frame_wnd_destruct(&self->frame);
    mem_free(self);
}

void ui_word_display_wnd_on_create(struct UIWindow *self, int width_unused, int height_unused)
{
}

void ui_word_display_wnd_on_draw(struct UIWindow *window)
{
    struct UIWordDisplayWnd *self = (struct UIWordDisplayWnd *)window;
    ui_window_draw_bitmap(&self->frame.base, 0, 0,
                        ui_skin_bitmap(UI_SKIN "전장적표시.bmp"  ), 1);
}

int ui_word_display_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender_unused, int msg,
                                        int val1, int val2_unused, int val3_unused, int val4_unused)
{
    struct UIWordDisplayWnd *self = (struct UIWordDisplayWnd *)window;
    if (msg == WORD_DISPLAY_MSG_ACTOR)
        self->actor_id = val1;
    return 0;
}

void ui_word_display_wnd_on_process(struct UIWindow *window)
{
    struct UIWordDisplayWnd *self = (struct UIWordDisplayWnd *)window;
    struct GameActor *actor = self->actor_id == g_session1.ownAid
        ? (struct GameActor *)game_current_player()
        : game_find_actor(self->actor_id);
    struct ActorScreenPosition position;
    int ax, ay;
    float scale;

    if (!actor_get_screen_position(actor, &position))
        return;
    ax = position.x;
    ay = position.y;
    if (self->last_x == ax && self->last_x == ay)
        return;
    self->last_y = ay;
    self->last_x = ax;
    scale = position.scale;
    ui_frame_wnd_move(&self->frame.base,
             (int)((float)ax - (float)(g_renderer->width * 5 / 640) * scale),
             (int)((float)ay - (float)(g_renderer->height * 100 / 480) * scale));
}
