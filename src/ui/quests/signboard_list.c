#include "actor/object_ops.h"
#include "ui/window_access.h"
#include "render/projection.h"
#include "render/rp_pool.h"
#include "resources/sprite.h"
#include "resources/animation.h"
#include <windows.h>
#include <mmsystem.h>
#include "actor/render_object.h"
#include "resources/attr_queries.h"
#include "session/state.h"
#include "core/object_type_tables.h"
#include "core/memory.h"
#include "core/fatal.h"
#include <mbstring.h>
#include <string.h>
#include "ui/quests/quests.h"
#include "ui/quests/signboard_list.h"
#include "game/context.h"
#include "actor/appearance.h"

#define UI_SKIN_DIR "유저인터페이스"

#include "actor/render_ops.h"
#include "actor/game_actor.h"
#include "actor/sprite_draw.h"
#include "core/skin_path.h"
#include "core/text_util.h"
#include "effects/rag/rag_effect.h"
#include "diag/perf.h"

struct BmpObjWnd;
void bmp_obj_wnd_destroy(struct UIWindow *window);
void bmp_obj_wnd_on_create(struct UIWindow *self, int width, int height);
void bmp_obj_wnd_on_draw(struct UIWindow *self);
const struct UIWindowOps bmp_obj_wnd_ops = {
    .destroy = bmp_obj_wnd_destroy,
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
    .on_create = bmp_obj_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = bmp_obj_wnd_on_draw,
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
    .send_msg = ui_window_send_msg_base,
    .invalidate = ui_window_invalidate_base,
    .render = ui_window_render_base,
    .has_custom_backdrop = 0,
    .draw_backdrop = ui_window_draw_backdrop_base,
    .get_records = ui_window_get_records_base,
    .type = UI_TYPE_WINDOW,
    .on_hover_enter = ui_window_on_hover_enter_base,
    .on_hover_leave = ui_window_on_hover_leave_base,
    .make_quad = ui_window_make_quad_base,
    .hit_test = ui_window_hit_test_base,
    .render_overlay = ui_window_render_overlay_base,
};
struct BmpObjWnd {
    struct UIWindow base;
    struct String bitmap;
};

struct BmpObjWnd *bmp_obj_wnd_construct(struct BmpObjWnd *self)
{
    ui_window_construct(&self->base, 0);
    self->base.ops = &bmp_obj_wnd_ops;
    string_init(&self->bitmap);
    return self;
}

void bmp_obj_wnd_destruct(struct BmpObjWnd *self)
{
    self->base.ops = &bmp_obj_wnd_ops;
    string_free(&self->bitmap);
    ui_window_destruct(&self->base);
}

void bmp_obj_wnd_destroy(struct UIWindow *window)
{
    struct BmpObjWnd *self = (struct BmpObjWnd *)window;
    bmp_obj_wnd_destruct(self);
    mem_free(self);
}

void bmp_obj_wnd_on_create(struct UIWindow *self, int width, int height)
{
}

void bmp_obj_wnd_on_draw(struct UIWindow *window)
{
    struct BmpObjWnd *self = (struct BmpObjWnd *)window;
    ui_window_draw_bitmap(&self->base, 0, 0,
                        res_mgr_get(g_resMgr(), ui_skin_path(string_cstr(&self->bitmap))), 0);
}

static void bmp_fit(struct BmpObjWnd *self)
{
    struct BitmapRes *bmp = res_mgr_get(g_resMgr(), ui_skin_path(string_cstr(&self->bitmap)));

    ui_window_resize_base(&self->base, bmp->width, bmp->height);
}

struct SbSignBoardWnd {
    struct UIWindow base;
    struct UIWindow *label;
    struct String text;
    struct String icon;
    struct String colour;
};

static void board_show_text(struct SbSignBoardWnd *board)
{
    sign_board_wnd_apply_text_colour((struct SignBoardWnd *)&board->base);
    ui_static_text_set_text_ellipsis(board->label, string_cstr(&board->text));
}

struct SignboardObject;
struct GameObject;
struct RenderObject;
const struct RenderObjectOps signboard_object_ops = {
    .base = {
        .destroy = signboard_object_destroy,
        .on_process = signboard_object_on_process,
        .send_msg = render_object_send_msg,
        .render = signboard_object_render,
        .get_direction = game_object_dir_of,
    },
    .render_body = game_object_set_render_info,
    .set_action = render_object_set_action_base,
    .update_motion = render_object_update_motion_base,
    .get_effect_state = render_object_return_zero,
};
struct SignboardObject {
    struct RenderObject base;
    int type;
    struct UIWindow *wnd;
};

static struct SignboardObject *signboard_object_construct(struct SignboardObject *self)
{
    render_object_construct(&self->base);
    self->base.ops = &signboard_object_ops.base;
    self->wnd = NULL;
    return self;
}

void signboard_object_destruct(struct SignboardObject *self)
{
    self->base.ops = &signboard_object_ops.base;
    render_object_dc_render_object(&self->base);
}

void signboard_object_destroy(struct GameObject *object)
{
    struct SignboardObject *self = (struct SignboardObject *)object;

    signboard_object_destruct(self);
    mem_free(self);
}

static int signboard_object_make_window(struct SignboardObject *self)
{
    struct UIWindow *wnd;
    int width, height;

    if (self->type == SIGNBOARD_PICTURE) {
        struct BmpObjWnd *bmp = mem_try_alloc(sizeof *bmp);

        wnd = bmp ? &bmp_obj_wnd_construct(bmp)->base : NULL;
        width = 0xaa;
        height = 0xaa;
    } else if (self->type == SIGNBOARD_BOARD) {
        struct SbSignBoardWnd *board = mem_try_alloc(sizeof *board);

        if (board) {
            ui_window_construct(&board->base, 0);
            board->base.ops = &sign_board_wnd_ops;
            string_init(&board->text);
            string_init(&board->icon);
            string_init(&board->colour);
        }
        wnd = board ? &board->base : NULL;
        width = 0xa2;
        height = 0x24;
    } else {
        return 0;
    }
    if (!wnd)
        return 0;
    self->wnd = wnd;
    ui_window_create(wnd, width, height);
    ui_window_mgr_add_to_window_list(&g_windowMgr, self->wnd);
    return 1;
}

static void signboard_object_set_image(struct SignboardObject *self, const struct String *image)
{
    if (!self->wnd)
        return;
    if (self->type == SIGNBOARD_PICTURE) {
        struct BmpObjWnd *bmp = (struct BmpObjWnd *)self->wnd;

        string_assign(&bmp->bitmap, string_cstr(image), image->size);
        bmp_fit((struct BmpObjWnd *)self->wnd);
    } else if (self->type == SIGNBOARD_BOARD) {
        string_assign(&((struct SbSignBoardWnd *)self->wnd)->icon, string_cstr(image), image->size);
    }
}

static void signboard_object_set_text(struct SignboardObject *self, const struct String *text,
                                    const struct String *colour)
{
    struct SbSignBoardWnd *board = (struct SbSignBoardWnd *)self->wnd;

    string_assign(&board->text, string_cstr(text), text->size);
    board = (struct SbSignBoardWnd *)self->wnd;
    string_assign(&board->colour, string_cstr(colour), colour->size);
    board = (struct SbSignBoardWnd *)self->wnd;
    board_show_text(board);
}

unsigned char signboard_object_on_process(struct GameObject *object)
{
    struct SignboardObject *self = (struct SignboardObject *)object;

    struct ActorScreenPosition player;

    if (self->wnd && actor_get_screen_position((struct GameActor *)game_current_player(), &player)) {
        struct UIWindow *wnd = self->wnd;
        int y = self->base.screen_y - wnd->height / 2;
        int lift = g_renderer->height * 0x51 / 480;
        float top = (float)y - (float)lift * player.scale;

        ui_window_move(wnd, self->base.screen_x - wnd->width / 2, (int)top);
    }
    return 1;
}

void signboard_object_render(struct GameObject *object, const float *view)
{
    struct SignboardObject *self = (struct SignboardObject *)object;

    float x, y, rhw;

    renderer_project_point(g_renderer, &self->base.pos, view, &x, &y, &rhw);
    self->base.scale = renderer_screen_scale(g_renderer, rhw) / 1.34f;
    render_object_set_screen_position(&self->base, x, y);
}

struct GameObject;
struct Icon;
struct RenderObject;
const struct RenderObjectOps icon_ops = {
    .base = {
        .destroy = icon_destroy,
        .on_process = icon_on_process,
        .send_msg = icon_send_msg,
        .render = icon_render,
        .get_direction = game_object_dir_of,
    },
    .render_body = icon_render_body,
    .set_action = render_object_set_action_base,
    .update_motion = render_object_update_motion_base,
    .get_effect_state = render_object_return_zero,
};
struct Icon {
    struct RenderObject base;
};

static struct Icon *icon_construct(struct Icon *self)
{
    render_object_construct(&self->base);
    self->base.ops = &icon_ops.base;
    return self;
}

void icon_destruct(struct Icon *self)
{
    self->base.ops = &icon_ops.base;
    render_object_dc_render_object(&self->base);
}

void icon_destroy(struct GameObject *object)
{
    struct Icon *self = (struct Icon *)object;

    icon_destruct(self);
    mem_free(self);
}

unsigned char icon_on_process(struct GameObject *object)
{
    struct Icon *self = (struct Icon *)object;

    float frames = (float)(double)(unsigned)(timeGetTime() - self->base.start_tick) / 24.0f;
    int count;

    self->base.action = self->base.base_action;
    count = (int)actres_frame_count(render_object_get_act_res(&self->base), self->base.action);
    self->base.motion = (int)(frames / self->base.motion_delay) % count;
    return 1;
}

void icon_send_msg(struct GameObject *object, struct GameObject *source,
    RenderMsgArg msg, RenderMsgArg par1, RenderMsgArg par2, RenderMsgArg par3, RenderMsgArg par4)
{
    struct Icon *self = (struct Icon *)object;

    game_current_base_mode();
    render_object_send_msg((struct GameObject *)&self->base, source, msg, par1, par2, par3, par4);
}

void icon_render(struct GameObject *object, const float *view)
{
    struct Icon *self = (struct Icon *)object;

    render_object_render_sprite(&self->base, view, 0, 0.0f);
}

void icon_render_body(struct RenderObject *object, const struct ScreenBounds *rect, float x, float y)
{
    struct Icon *self = (struct Icon *)object;
    render_object_set_screen_position(&self->base, x, y);
}

static void icon_set_sprite(struct Icon *self, const struct String *name)
{
    struct String stem, spr, act;
    unsigned dot = string_find(name, ".", 0);

    string_init(&stem);
    if (dot != (unsigned)-1) {
        struct String part;

        string_init(&part);
        string_substring(&part, name, 0, string_find(name, ".", 0));
        string_free(&stem);
        stem = part;
    }
    string_copy(&spr, &stem);
    string_append_cstr(&spr, ".spr");
    string_copy(&act, &stem);
    string_append_cstr(&act, ".act");
    render_object_load_spr_res(&self->base, string_cstr(&spr));
    render_object_load_act_res(&self->base, string_cstr(&act));
    render_object_set_action_base(&self->base, 0, 0);
    string_free(&act);
    string_free(&spr);
    string_free(&stem);
}

static int place_object(struct RenderObject *obj, const struct SignBoardEntry *row)
{
    float wx = 0.0f, wz = 0.0f, ground;
    struct C3dAttr *attr = game_current_map_attr();

    if (!attr || !game_cell_to_world(row->x, row->y, &wx, &wz))
        return 0;
    ground = c3d_attr_get_height(attr, wx, wz);
    obj->pos.x = wx;
    obj->pos.y = ground - (float)row->height;
    obj->pos.z = wz;
    return 1;
}

static int add_to_world(struct RenderObject *obj)
{
    struct List *objects = game_current_object_list();
    if (!objects || !objects->head)
        return 0;
    list_push_back(objects, &obj, sizeof obj);
    return 1;
}

static void image_path(struct String *path, const char *image)
{
    string_init(path);
    if (!_mbsstr((const unsigned char *)image, (const unsigned char *)UI_SKIN_DIR))
        string_assign(path, UI_SKIN_DIR "\\", 0xf);
    string_append_cstr(path, image);
}

static struct SignboardObject *make_coo(const struct SignBoardEntry *row, struct String *path)
{
    struct SignboardObject *obj = mem_alloc(sizeof *obj);

    obj = obj ? signboard_object_construct(obj) : NULL;
    if (!obj)
        return NULL;
    obj->type = row->type;
    if (!signboard_object_make_window(obj)) {
        signboard_object_destroy((struct GameObject *)obj);
        return NULL;
    }
    image_path(path, row->image);
    signboard_object_set_image(obj, path);
    return obj;
}

void signboards_place(void)
{
    struct String map, stem;
    const char *name;
    unsigned at = (unsigned)-1, i;
    size_t index;
    const struct SignBoardEntry *row;
    struct World *world = game_current_world();

    if (!world || !game_current_map_attr())
        return;
    string_init(&map);
    name = game_current_map_name();
    string_assign_cstr(&map, name);
    name = string_cstr(&map);
    if (map.size >= 4) {
        for (i = 0; i + 4 <= map.size; i++) {
            if (memcmp(name + i, ".rsw", 4) == 0) {
                at = i;
                break;
            }
        }
    }
    string_init(&stem);
    string_assign(&stem, name, map.size < at ? map.size : at);
    string_free(&map);
    map = stem;

    for (index = 0; index < signboards_count(); ++index) {
        row = signboards_entry(index);
        struct String row_map = string_from_cstr(row->map);

        text_to_lower(&row_map);
        text_to_lower(&map);
        if (row_map.size == map.size && memcmp(string_cstr(&row_map), string_cstr(&map), map.size) == 0) {
            if (row->type == SIGNBOARD_PICTURE || row->type == SIGNBOARD_BOARD) {
                struct String path;
                struct SignboardObject *obj = make_coo(row, &path);

                if (obj && row->type == SIGNBOARD_BOARD) {
                    struct String colour = string_from_cstr(row->colour);
                    struct String text = string_from_cstr(row->text);

                    signboard_object_set_text(obj, &text, &colour);
                    string_free(&text);
                    string_free(&colour);
                }
                if (obj) {
                    if (game_current_world() != world || !place_object(&obj->base, row) || !add_to_world(&obj->base)) {
                        ui_window_mgr_post_quit2(&g_windowMgr, obj->wnd);
                        signboard_object_destroy((struct GameObject *)obj);
                    }
                    string_free(&path);
                }
            } else if (row->type == SIGNBOARD_ICON) {
                struct Icon *icon = mem_alloc(sizeof *icon);
                struct String sprite;

                icon = icon ? icon_construct(icon) : NULL;
                if (icon) {
                    if (game_current_world() == world && place_object(&icon->base, row)) {
                        sprite = string_from_cstr(row->image);
                        icon_set_sprite(icon, &sprite);
                        string_free(&sprite);
                        if (!add_to_world(&icon->base))
                            icon_destroy((struct GameObject *)icon);
                    } else {
                        icon_destroy((struct GameObject *)icon);
                    }
                }
            }
        }
        string_free(&row_map);
        if (game_current_world() != world)
            break;
    }
    string_free(&map);
}
