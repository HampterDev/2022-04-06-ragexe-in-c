#include "game/actor_names.h"
#include "world/tooltip.h"
#include "ui/window_access.h"
#include "render/rp_pool.h"
#include "session/state.h"
#include "core/object_type_tables.h"
#include "game/context.h"
#include <string.h>
#include "ui/character/character.h"
#include "core/msg_str.h"
#include "diag/perf.h"
#include "session/session_names.h"

struct UIMonsterInfoWnd;
struct UIWindow;
void ui_monster_info_wnd_destroy(struct UIWindow *window);
void ui_monster_info_wnd_on_create(struct UIWindow *self, int width_unused, int height_unused);
void ui_monster_info_wnd_on_draw(struct UIWindow *self);
void ui_monster_info_wnd_on_mouse_hover(struct UIWindow *self, int x, int y);
intptr_t ui_monster_info_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4);
const struct UIWindowOps ui_monster_info_wnd_ops = {
    .destroy = ui_monster_info_wnd_destroy,
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
    .on_create = ui_monster_info_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_monster_info_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_frame_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_frame_wnd_on_mouse_move,
    .on_mouse_hover = ui_monster_info_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_frame_wnd_on_l_btn_up,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_monster_info_wnd_send_msg,
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

#define CRT_TRUNCATE ((size_t)-1)

#define MONSTER_INFO_ID_OK 0xb0
#define MSG_MONSTER_INFO 0x4f
#define MONSTER_NAME_WIDTH 100
#define MONSTER_SPRITE_DIR "몬스터\\"

#define MSG_MONSTER_INFO_TITLE 0x196
#define MSG_MONSTER_LABEL_NAME 0x197
#define MSG_MONSTER_LABEL_LEVEL 0x198
#define MSG_MONSTER_LABEL_HP 0x199
#define MSG_MONSTER_LABEL_DEF 0xf46
#define MSG_MONSTER_LABEL_SIZE 0x19a
#define MSG_MONSTER_LABEL_RACE 0x19b
#define MSG_MONSTER_LABEL_MDEF 0x19c
#define MSG_MONSTER_LABEL_PROPERTY 0x19d
#define MSG_MONSTER_SIZE_FIRST 0xf43
#define MSG_MONSTER_RACE_FIRST 0xf2f
#define MSG_MONSTER_PROPERTY_FIRST 0xf39

#pragma pack(push, 1)
struct MonsterInfo {
    unsigned short packet_type;
    short job;
    short level;
    short size;
    int hp;
    short def;
    short race;
    short mdef;
    short property;
    BYTE element_values[9];
};
#pragma pack(pop)
SIZE_ASSERT(MonsterInfo, 0x1d);

struct UIMonsterInfoWnd {
    struct UIFrameWnd frame;
    struct MonsterInfo info;
    int actor_id;
};

struct UIMonsterInfoWnd *ui_monster_info_wnd_construct(struct UIMonsterInfoWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_monster_info_wnd_ops;
    memset(&self->info, 0, sizeof self->info);
    self->actor_id = 0;
    return self;
}

struct UIWindow *ui_monster_info_wnd_new(void)
{
    return (struct UIWindow *)ui_monster_info_wnd_construct(mem_alloc(sizeof(struct UIMonsterInfoWnd)));
}

void ui_monster_info_wnd_destroy(struct UIWindow *window)
{
    struct UIMonsterInfoWnd *self = (struct UIMonsterInfoWnd *)window;
    self->frame.base.ops = &ui_monster_info_wnd_ops;
    ui_frame_wnd_destruct(&self->frame);
    mem_free(self);
}

void ui_monster_info_wnd_on_create(struct UIWindow *window, int width_unused, int height_unused)
{
    struct UIMonsterInfoWnd *self = (struct UIMonsterInfoWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct UIBitmapButton *ok =
        ui_bitmap_button(UI_SKIN "btn_ok.bmp", UI_SKIN "btn_ok_a.bmp", UI_SKIN "btn_ok_b.bmp");

    ui_window_move_base(&ok->button.base, wnd->width - 0x2e, wnd->height - 0x18);
    ok->button.base.id = MONSTER_INFO_ID_OK;
    ui_window_add_child(wnd, &ok->button.base);
    UI_FRAME_DEFAULT_ID(self) = MONSTER_INFO_ID_OK;
    self->actor_id = g_session1.senseTargetAid;
}

static void cut_at_hash(struct String *name)
{
    const char *text = string_cstr(name);
    const char *hash = strchr(text, '#');

    if (hash)
        string_truncate(name, (size_t)(hash - text));
}

static void text_at(struct UIWindow *wnd, int x, int y, const char *text, COLORREF colour)
{
    ui_window_text_out(wnd, x, y, text, 0, 0, 12, colour, 0, 0);
}

static void property_box(struct UIWindow *wnd, int x, int y, const char *name, int value, COLORREF frame,
                         COLORREF fill)
{
    char text[0x40];
    int red, green, blue;

    wsprintfA(text, "%s:%d", name, value);
    ui_skin_color_chip(&g_skinMgr, 0x16, 2, &red, &green, &blue);
    ui_window_fill_rect(wnd, x, y, 0x46, 0x10, frame);
    ui_window_fill_rect(wnd, x + 1, y + 1, 0x44, 0xe, fill);
    ui_window_text_out(wnd, x + (0x46 - ui_window_text_width(wnd, text, 0, 0, 12, 0, 0)) / 2, y + 2,
                     text, 0, 0, 12, RGB((BYTE)red, (BYTE)blue, (BYTE)green), 0, 0);
}

void ui_monster_info_wnd_on_draw(struct UIWindow *window)
{
    struct UIMonsterInfoWnd *self = (struct UIMonsterInfoWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct MonsterInfo *info = &self->info;
    COLORREF back, frame;
    struct String name;
    struct String fitted = {0};
    char number[0x100], spr_path[0x80], act_path[0x80];
    int red, green, blue, kind, i;

    ui_skin_color_chip(&g_skinMgr, 2, 2, &red, &green, &blue);
    back = RGB((BYTE)red, (BYTE)green, (BYTE)blue);
    ui_window_clear_canvas(wnd, back);
    ui_window_draw_body(wnd, ui_window_draw_title_bar(wnd, 0, "", 0), 0, "");
    ui_window_draw_button_bar(wnd, 2, 0, 1);
    ui_skin_color_chip(&g_skinMgr, 0x16, 2, &red, &green, &blue);
    frame = RGB((BYTE)red, (BYTE)green, (BYTE)blue);

    text_at(wnd, 0x11, 3, msg_str(MSG_MONSTER_INFO_TITLE), 0xffffff);
    text_at(wnd, 0x10, 2, msg_str(MSG_MONSTER_INFO_TITLE), 0);

    text_at(wnd, 0x78, 0x1e, msg_str(MSG_MONSTER_LABEL_NAME), 0);
    game_mode_get_actor_name(game_current_mode(), &name, self->actor_id);
    cut_at_hash(&name);
    ui_window_fit_text(wnd, &fitted, MONSTER_NAME_WIDTH, string_cstr(&name), 0, 12, 0);
    string_free(&name);
    text_at(wnd, 0xa0, 0x1e, string_cstr(&fitted), 0);

    _snprintf_s(number, sizeof number, CRT_TRUNCATE, "%d", info->level);
    text_at(wnd, 0x78, 0x32, msg_str(MSG_MONSTER_LABEL_LEVEL), 0);
    text_at(wnd, 0xa0, 0x32, number, 0);
    _snprintf_s(number, sizeof number, CRT_TRUNCATE, "%d", info->hp);
    text_at(wnd, 0x78, 0x46, msg_str(MSG_MONSTER_LABEL_HP), 0);
    text_at(wnd, 0xa0, 0x46, number, 0);
    _snprintf_s(number, sizeof number, CRT_TRUNCATE, "%d", info->def);
    text_at(wnd, 0x78, 0x5a, msg_str(MSG_MONSTER_LABEL_DEF), 0);
    text_at(wnd, 0xa0, 0x5a, number, 0);

    text_at(wnd, 0x104, 0x1e, msg_str(MSG_MONSTER_LABEL_SIZE), 0);
    if (info->size >= 0 && info->size <= 2)
        text_at(wnd, 0x12c, 0x1e, msg_str(MSG_MONSTER_SIZE_FIRST + info->size), 0);
    _snprintf_s(number, sizeof number, CRT_TRUNCATE, "%d", info->race);
    text_at(wnd, 0x104, 0x32, msg_str(MSG_MONSTER_LABEL_RACE), 0);
    if ((unsigned)info->race <= 9)
        text_at(wnd, 0x12c, 0x32, msg_str(MSG_MONSTER_RACE_FIRST + info->race), 0);
    _snprintf_s(number, sizeof number, CRT_TRUNCATE, "%d", info->mdef);
    text_at(wnd, 0x104, 0x5a, msg_str(MSG_MONSTER_LABEL_MDEF), 0);
    text_at(wnd, 0x12c, 0x5a, number, 0);
    text_at(wnd, 0x104, 0x46, msg_str(MSG_MONSTER_LABEL_PROPERTY), 0);
    kind = info->property % 20;
    if ((unsigned)kind <= 9)
        text_at(wnd, 0x12c, 0x46, msg_str(MSG_MONSTER_PROPERTY_FIRST + kind), 0);

    for (i = 0; i < 9; i++)
        property_box(wnd, 0x78 + i / 3 * 0x50, 0x6e + i % 3 * 0x12, msg_str(MSG_MONSTER_PROPERTY_FIRST + 1 + i),
                     info->element_values[i], frame, back);

    _snprintf_s(spr_path, sizeof spr_path, CRT_TRUNCATE, MONSTER_SPRITE_DIR "%s.spr",
                   session_get_job_name(&g_session1, info->job, -1));
    _snprintf_s(act_path, sizeof act_path, CRT_TRUNCATE, MONSTER_SPRITE_DIR "%s.act",
                   session_get_job_name(&g_session1, info->job, -1));
    if (res_mgr_is_available(g_resMgr(), act_path) && res_mgr_is_available(g_resMgr(), spr_path)) {
        void *act = res_mgr_get(g_resMgr(), act_path);
        void *spr = res_mgr_get(g_resMgr(), spr_path);

        ui_window_draw_actor_sprite(wnd, spr, act, 2, 0x13, 0x74, 0x98, 0, 1, NULL);
    }
    string_free(&fitted);
}

void ui_monster_info_wnd_on_mouse_hover(struct UIWindow *window, int x, int y)
{
    struct UIMonsterInfoWnd *self = (struct UIMonsterInfoWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct String name;
    int screen_x, screen_y;

    if (x < 0xa1 || x > 0x103 || y < 0x1f || y > 0x29)
        return;
    screen_x = 0;
    screen_y = 0;
    ui_window_to_screen(wnd, &screen_x, &screen_y);
    game_mode_get_actor_name(game_current_mode(), &name, g_session1.senseTargetAid);
    cut_at_hash(&name);
    if (ui_window_text_width(wnd, string_cstr(&name), 0, 0, 12, 0, 0) > MONSTER_NAME_WIDTH)
        mode_show_tooltip(g_modeMgr.curMode, string_cstr(&name), screen_x + x, screen_y + y,
                              0xffffffff, 0);
    string_free(&name);
}

intptr_t ui_monster_info_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, intptr_t val1,
                                        intptr_t val2, intptr_t val3, intptr_t val4)
{
    struct UIMonsterInfoWnd *self = (struct UIMonsterInfoWnd *)window;
    if (msg == UI_MSG_COMMAND) {
        if (val1 == MONSTER_INFO_ID_OK)
            ui_window_mgr_post_quit2(&g_windowMgr, &self->frame.base);
        return 0;
    }
    if (msg == MSG_MONSTER_INFO) {
        if (val1)
            memcpy(&self->info, (const void *)val1, sizeof self->info);
        ui_window_invalidate_base(&self->frame.base);
        return 0;
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
}
