#include "render/rp_pool.h"
#include "resources/emblem_types.h"
#include "contents/guild_mgr.h"
#include "session/state.h"
#include "core/object_type_tables.h"
#include "ui/social/social.h"
#include "diag/perf.h"
#include "session/session_jobs.h"
#include "session/session_names.h"

#define EMBLEM_SIZE_SMALL 0x18
#define EMBLEM_SIZE_LARGE 0x1c

#define EMBLEM_COLOUR_KEY 0xffff00ff

#define MSG_EMBLEM_SET UI_MSG_SET_INFO
#define MSG_EMBLEM_CHANGED 0x3c

struct UIEmblemWnd {
    struct UIWindow base;
    int last_x, last_y;
    int aid;
    int emblem_id;
};
void ui_emblem_wnd_destroy(struct UIWindow *window);
void ui_emblem_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_emblem_wnd_on_process(struct UIWindow *self);
void ui_emblem_wnd_on_draw(struct UIWindow *self);
int ui_emblem_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_emblem_wnd_ops = {
    .destroy = ui_emblem_wnd_destroy,
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
    .on_create = ui_emblem_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_emblem_wnd_on_process,
    .on_draw = ui_emblem_wnd_on_draw,
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
    .send_msg = ui_emblem_wnd_send_msg,
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

static struct UIEmblemWnd *ui_emblem_wnd_construct(struct UIEmblemWnd *self)
{
    ui_window_construct(&self->base, 0);
    self->base.ops = &ui_emblem_wnd_ops;
    self->aid = 0;
    self->emblem_id = 0;
    ui_window_create(&self->base, EMBLEM_SIZE_SMALL, EMBLEM_SIZE_SMALL);
    return self;
}

struct UIEmblemWnd *ui_emblem_wnd_new(void)
{
    struct UIEmblemWnd *self = mem_alloc(sizeof *self);

    return self ? ui_emblem_wnd_construct(self) : NULL;
}

static void ui_emblem_wnd_destruct(struct UIEmblemWnd *self)
{
    self->base.ops = &ui_emblem_wnd_ops;
    ui_window_destruct(&self->base);
}

void ui_emblem_wnd_destroy(struct UIWindow *window)
{
    struct UIEmblemWnd *self = (struct UIEmblemWnd *)window;
    ui_emblem_wnd_destruct(self);
    mem_free(self);
}

void ui_emblem_wnd_on_create(struct UIWindow *self, int width, int height)
{
}

static struct EmblemRes *emblem_res(struct UIEmblemWnd *self, struct String *path)
{
    struct String name;

    guild_mgr_emblem_file_name(g_guildMgr, path, self->emblem_id);
    string_copy(&name, path);
    return res_mgr_get_string(g_resMgr(), name);
}

static int in_siege(void)
{
    return session_is_siege_mode(&g_session1) && !session_get_battlefield_state(&g_session1);
}

void ui_emblem_wnd_on_draw(struct UIWindow *window)
{
    struct UIEmblemWnd *self = (struct UIEmblemWnd *)window;
    struct String path;
    struct EmblemRes *res;

    ui_window_clear_canvas(&self->base, EMBLEM_COLOUR_KEY);
    if (g_session1.emblemFrame && in_siege()) {
        void *frame = res_mgr_get(g_resMgr(), UI_SKIN "emblem_frame.bmp");

        if (frame)
            ui_window_draw_bitmap(&self->base, 0, 0, frame, 1);
    }
    res = emblem_res(self, &path);
    if (res && in_siege()) {
        int at = g_session1.emblemFrame == 1 ? 2 : 0;

        ui_window_draw_bitmap(&self->base, at, at, &res->picture.bmp, 1);
    }
    string_free(&path);
}

void ui_emblem_wnd_on_process(struct UIWindow *window)
{
    struct UIEmblemWnd *self = (struct UIEmblemWnd *)window;
    struct UIWindow *wnd = &self->base;
    struct GameActor *actor;
    struct String path;
    struct EmblemRes *res;

    if (g_session1.emblemFrame) {
        if (wnd->width == EMBLEM_SIZE_SMALL)
            ui_window_resize_base(wnd, EMBLEM_SIZE_LARGE, EMBLEM_SIZE_LARGE);
    } else if (wnd->width == EMBLEM_SIZE_LARGE) {
        ui_window_resize_base(wnd, EMBLEM_SIZE_SMALL, EMBLEM_SIZE_SMALL);
    }

    actor = session_get_actor_by_aid(&g_session1, self->aid);
    if (!actor) {
        ui_window_move_base(wnd, -100, -100);
    } else {
        int ax = actor->screen_x, ay = actor->screen_y;

        if (self->last_x != ax || self->last_x != ay) {
            float scale = actor->scale;
            int dx = g_renderer->width * 10 / 640;
            int dy = g_renderer->height * 100 / 480;

            self->last_y = ay;
            self->last_x = ax;
            ui_window_move_base(wnd, (int)((float)ax - (float)dx * scale), (int)((float)ay - (float)dy * scale));
        }
    }

    res = emblem_res(self, &path);
    if (res && res->picture.changed == 1)
        ui_window_invalidate_base(wnd);
    string_free(&path);
}

int ui_emblem_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1, int val2,
                                   int val3, int val4)
{
    struct UIEmblemWnd *self = (struct UIEmblemWnd *)window;
    if (msg == MSG_EMBLEM_SET) {
        self->aid = val1;
        self->emblem_id = val2;
    } else if (msg == MSG_EMBLEM_CHANGED && self->emblem_id == val1) {
        ui_window_invalidate_base(&self->base);
    }
    return 0;
}
