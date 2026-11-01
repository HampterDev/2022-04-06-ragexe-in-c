#include "ui/window_access.h"
#include "ui/chat/balloon.h"
#include "render/rp_pool.h"
#include "render/sprite_atlas.h"
#include "resources/palette.h"
#include "resources/sprite.h"
#include "contents/client_lua_calls.h"
#include "session/state.h"
#include "core/object_type_tables.h"
#include "world/mode_manager.h"
#include "contents/connection_api.h"
#include <stdlib.h>
#include <string.h>
#include "ui/character/sel_char_userver.h"
#include "ui/ui_std.h"
#include "ui/character/character_std.h"
#include "core/msg_str.h"
#include "core/skin_path.h"
#include "diag/perf.h"
#include "session/session_jobs.h"
#include "session/session_names.h"
#include "session/session_talk.h"
#include "ui/draw_key.h"

static int g_selCharHeadOffsetX = 0;

static int g_selCharHeadOffsetY = 0;

#define OK_ID 0xb0
#define MAX_CHECKED 9
#define CELL_W 0x51
#define CELL_H 0x95
#define MSG_TOO_MANY 0x4bc
#define MSG_CONFIRM 0x4bb
#define MSG_NONE_CHECKED 0x4c1
#define HEADTOP_LOW_A 0xb5
#define HEADTOP_LOW_B 0xc5
#define LAYER_BODY 0
#define LAYER_HEAD 1

static void *get_res(const struct String *path)
{
    return res_mgr_get(g_resMgr(), string_cstr(path));
}

static struct CharInfo *char_info(int i)
{
    return (struct CharInfo *)mode_send_message(client_current_mode(), CMODE_GET_CHAR_INFO, i, 0, 0, 0);
}

void ui_sel_char_for_u_server_wnd_destruct(struct UISelCharForUServerWnd *self)
{
    int i;

    self->frame.base.ops = &ui_sel_char_for_u_server_wnd_ops;
    string_vec_free(&self->lines);
    for (i = SEL_CHAR_MAX - 1; i >= 0; i--)
        char_preview_destroy(&self->looks[i]);
    ui_frame_wnd_destruct(&self->frame);
}

void ui_sel_char_for_u_server_wnd_destroy(struct UIWindow *window)
{
    struct UISelCharForUServerWnd *self = (struct UISelCharForUServerWnd *)window;
    ui_sel_char_for_u_server_wnd_destruct(self);
    mem_free(self);
}

static void load_look(struct UISelCharForUServerWnd *self, struct CharInfo *info)
{
    int slot = info->slot;
    struct CharPreview *look = &self->looks[slot];
    struct Session *s = &g_session1;
    int job = info->job;
    int head;
    int palette;
    struct String tmp;

    look->job = info->job;
    look->robe = info->robe;
    self->hidden[slot] = 0;
    look->x = slot % self->columns * CELL_W + 0x44;
    look->action_base = 0;
    look->action = 0;
    look->frame = 0;
    look->y = slot / self->columns * CELL_H + 0xbf;

    string_free(&look->act[0]);
    string_move(&look->act[0], session_get_body_spr_name_variant0(&tmp, job, session_get_sex(s), info->body, 0));
    string_free(&look->spr[0]);
    string_move(&look->spr[0], session_get_body_spr_name_variant1(&tmp, job, session_get_sex(s), info->body, 0));
    head = info->head;
    string_free(&look->act[1]);
    string_move(&look->act[1], session_get_head_spr_name0(&tmp, job, session_get_sex(s), &head));
    string_free(&look->spr[1]);
    string_move(&look->spr[1], session_get_head_spr_name1(&tmp, job, session_get_sex(s), &head));
    string_free(&look->act[2]);
    string_move(&look->act[2], session_get_res_name0(&tmp, job, session_get_sex(s), head,
                                                    info->accessory3));
    string_free(&look->spr[2]);
    string_move(&look->spr[2], session_get_res_name1(&tmp, job, session_get_sex(s), head, info->accessory3));
    info->head = (short)head;
    string_free(&look->act[3]);
    string_move(&look->act[3], session_get_res_name0(&tmp, job, session_get_sex(s), head,
                                                    info->accessory2));
    string_free(&look->spr[3]);
    string_move(&look->spr[3], session_get_res_name1(&tmp, job, session_get_sex(s), head, info->accessory2));
    self->headtop[slot] = info->accessory2;
    string_free(&look->act[4]);
    string_move(&look->act[4], session_get_res_name0(&tmp, job, session_get_sex(s), head,
                                                    info->accessory));
    string_free(&look->spr[4]);
    string_move(&look->spr[4], session_get_res_name1(&tmp, job, session_get_sex(s), head, info->accessory));

    if (info->body <= 0 && (WORD)(info->job - 0xfa1) <= 6)
        palette = info->body_palette >= 1 ? info->body_palette : 1;
    else
        palette = info->body_palette;
    look->body_palette = palette;
    if (palette) {
        string_free(&look->body_palette_name);
        string_move(&look->body_palette_name, session_get_body_pal_name(&tmp, job, session_get_sex(s), palette, info->body, 0));
    }

    look->head_palette = info->head_palette;
    if (look->head_palette) {
        string_free(&look->head_palette_name);
        string_move(&look->head_palette_name, session_get_head_pal_name(&tmp, head, job, session_get_sex(s), look->head_palette));
    }

    string_free(&look->imf);
    string_move(&look->imf, session_get_imf_name(&tmp, job, session_get_sex(s)));
}

void ui_sel_char_for_u_server_wnd_on_create(struct UIWindow *window, int cx, int cy)
{
    struct UISelCharForUServerWnd *self = (struct UISelCharForUServerWnd *)window;
    static const char *const ok_names[3] = {"btn_ok", "btn_ok_a", "btn_ok_b"};
    struct UIWindow *wnd = &self->frame.base;
    struct String dir;
    int i;

    string_init(&dir);
    string_assign(&dir, UI_SKIN, 0xf);
    self->ok = (struct UIWindow *)ui_add_named_skin_button(wnd, &dir, ok_names, wnd->width - 0x2e, wnd->height - 0x18,
                                                     OK_ID);
    string_free(&dir);
    self->frame.default_id = OK_ID;

    for (i = 0; i < g_session1.charCount; i++) {
        void *mem = mem_alloc(sizeof(struct UICheckBox));
        struct UIWindow *check = mem ? (struct UIWindow *)ui_check_box_construct(mem) : NULL;

        self->checks[i] = check;
        ui_window_create(check, 10, 10);
        ui_check_box_set_bitmaps(self->checks[i], "유저인터페이스\\checkbox_1.bmp", "유저인터페이스\\checkbox_0.bmp", "");
        ui_window_move_base(self->checks[i], -100, -100);
        ui_window_add_child(wnd, self->checks[i]);
    }

    for (i = 0; i < g_session1.charCount; i++) {
        struct CharInfo *info = char_info(i);

        if (!info)
            continue;
        ui_window_move_base(self->checks[i], i % self->columns * CELL_W + 0x3f, i / self->columns * CELL_H + 0x4d);
        load_look(self, info);
    }
}

static void doram_offset(struct UISelCharForUServerWnd *self, int index, int layer, struct CharPreview *look,
                         int *dx, int *dy)
{
    struct CharInfo *info = char_info(index);
    int item = 0, dir = 0, ox = 0, oy = 0;

    if (layer == 4)
        item = info->accessory;
    else if (layer == 3)
        item = info->accessory2;
    else if (layer == 2)
        item = info->accessory3;
    if (look->frame == 2)
        dir = -1;
    else if (look->frame == 1)
        dir = 1;
    dir = dir - look->action_base + look->action;
    if (dir < 0)
        dir = 7;
    if (dir > 7)
        dir = 0;

    client_lua_call(g_session1.luaState, "OffsetItemPos_GetOffsetForDoram", "ddd>dd", item, dir, session_get_sex(&g_session1), &ox, &oy);
    *dx = ox;
    *dy = oy;
}

static int attach_delta(struct Motion *motion, struct Motion *base, int *dx, int *dy)
{
    if (!animation_attachment_count(motion) || !animation_attachment_count(base) ||
        animation_attachment(motion, 0)->attr != animation_attachment(base, 0)->attr)
        return 0;
    *dx += animation_attachment(base, 0)->x - animation_attachment(motion, 0)->x;
    *dy += animation_attachment(base, 0)->y - animation_attachment(motion, 0)->y;
    return 1;
}

static void draw_layer(struct UISelCharForUServerWnd *self, int index, struct CharPreview *look, int layer,
                       struct SpriteRegion *place)
{
    struct ActRes *act;
    struct SprRes *spr;
    struct Motion *motion;
    struct SprClip *clip;
    struct SprImg *img;
    DWORD *palette;
    int dx = 0, dy = 0;
    void *tex;

    get_res(&look->imf);
    act = get_res(&look->act[layer]);
    spr = get_res(&look->spr[layer]);
    motion = act_res_get_motion(act, look->action, look->frame);
    if (session_is_doram_job(&g_session1, look->job) == 1)
        doram_offset(self, index, layer, look, &dx, &dy);

    if (layer == LAYER_HEAD) {
        struct Motion *body = act_res_get_motion(get_res(&look->act[LAYER_BODY]), look->action, look->frame);

        if (attach_delta(motion, body, &dx, &dy)) {
            g_selCharHeadOffsetX = dx;
            g_selCharHeadOffsetY = dy;
        }
    } else if (layer == 2 || layer == 3 || layer == 4) {
        struct Motion *head = act_res_get_motion(get_res(&look->act[LAYER_HEAD]), look->action, look->frame);
        int step = 0;
        struct MotionStepping stepping;
        struct Motion *stepped = session_get_stepped_motion(get_res(&look->act[LAYER_HEAD]),
                                                           get_res(&look->act[layer]), look->action, look->frame,
                                                           self->start_time, &step);

        if (session_get_motion_stepping(get_res(&look->act[LAYER_HEAD]), get_res(&look->act[layer]), look->action,
                                        look->frame, self->start_time, &stepping))
            ui_animation_add(&self->animation, &stepping, step);
        if (stepped && step != look->frame)
            motion = stepped;
        attach_delta(motion, head, &dx, &dy);
        dx += g_selCharHeadOffsetX;
        dy += g_selCharHeadOffsetY;
        if (layer == 3 && (self->headtop[index] == HEADTOP_LOW_A || self->headtop[index] == HEADTOP_LOW_B))
            dy -= 7;
    }

    palette = spr->palette;
    if (layer == LAYER_BODY) {
        if (look->body_palette)
            palette = palette_resource_colors(get_res(&look->body_palette_name), spr->palette);
    } else if (layer == LAYER_HEAD) {
        if (look->head_palette)
            palette = palette_resource_colors(get_res(&look->head_palette_name), spr->palette);
    }

    clip = motion_get_clip(motion, layer <= 1 ? layer : 0);
    if (clip->sprIndex == -1)
        return;
    img = spr_res_image(spr, clip);
    if (!img)
        return;
    if (clip->clipType == 0) {
        tex = sprite_atlas_resolve(g_renderer, img, palette, place);
    } else {
        tex = img->texture;
        place->x = place->y = 0;
    }
    if (!tex) return;
    ui_canvas_draw_texture(self->frame.base.canvas, look->x + clip->x + dx, look->y + clip->y + dy, tex,
                         place->x, place->y, img->width, img->height, clip->flags,
                         img->widthHalved + 1, img->heightHalved + 1);
}

static unsigned long long sel_char_key(struct UISelCharForUServerWnd *self)
{
    unsigned long long key = DRAW_KEY_START;
    int i;

    key = draw_key_int(key, g_session1.charCount);
    key = draw_key_int(key, session_get_sex(&g_session1));
    for (i = 0; i < g_session1.charCount; i++)
        key = draw_key_int(key, self->hidden[i]);
    return draw_key_children(key, &self->frame.base);
}

BYTE ui_sel_char_for_u_server_wnd_render(struct UIWindow *window, int force)
{
    struct UISelCharForUServerWnd *self = (struct UISelCharForUServerWnd *)window;

    if (sel_char_key(self) != self->drawn_key || ui_animation_due(&self->animation))
        ui_window_invalidate_base(window);
    return ui_window_render_base(window, force);
}

void ui_sel_char_for_u_server_wnd_on_draw(struct UIWindow *window)
{
    struct UISelCharForUServerWnd *self = (struct UISelCharForUServerWnd *)window;
    int i, layer;

    ui_animation_reset(&self->animation);
    ui_window_draw_bitmap(&self->frame.base, 0, 0,
                        res_mgr_get(g_resMgr(),
                                     ui_skin_path(g_session1.charCount > 0xc
                                                     ? UI_SKIN "login_interface\\selCharForIntergration2.bmp"
                                                     : UI_SKIN "login_interface\\selCharForIntergration.bmp")),
                        0);
    for (i = 0; i < g_session1.charCount; i++) {
        struct CharPreview *look = &self->looks[i];
        struct SpriteRegion place = {0};

        if (self->hidden[i])
            continue;
        ui_window_draw_robe(&self->frame.base, look->job, session_get_sex(&g_session1), look->robe, look->x, look->y,
                   0, 0, 0);
        memset(&place, 0, sizeof place);
        for (layer = 0; layer < 5; layer++) {
            if (look->act[layer].size && look->spr[layer].size)
                draw_layer(self, i, look, layer, &place);
        }
    }
    self->drawn_key = sel_char_key(self);
}

static void push_line(struct Vec *lines, const char *text)
{
    struct String s;

    string_init(&s);
    string_assign_cstr(&s, text);
    string_vec_push_move(lines, &s);
    string_free(&s);
}

static void show_balloon(struct UISelCharForUServerWnd *self, int x, int y, DWORD colour)
{
    BYTE r = (BYTE)(colour >> 16), g = (BYTE)(colour >> 8), b = (BYTE)colour;
    struct CharInfo *info;
    char name[0x19];
    char num[0x40];
    int width, i;
    static const char *const stats[6] = {"STR", "AGI", "VIT", "INT", "DEX", "LUK"};

    if (!self->balloon) {
        struct UIWindow *balloon = mem_alloc(sizeof(struct UITransBalloonText));

        if (balloon) {
            ui_trans_balloon_text_construct(balloon);
            balloon->ops = &ui_char_info_balloon_text_ops;
        }
        self->balloon = balloon;
        ui_window_create(balloon, 0x8c, 0xc8);
        ui_window_mgr_add_window(&g_windowMgr, self->balloon);
    }
    ui_trans_balloon_text_move(self->balloon, -400, -400);
    string_vec_clear(&self->lines);

    if (x < 0x1b || x >= self->frame.base.width - 0x1a || (unsigned)(y - 0x5f) > 0x10d || (y < 0xfa && y > 0xd2))
        goto close;
    info = char_info((y - 0x5f) / CELL_H * self->columns + (x - 0x1b) / CELL_W);
    if (!info)
        goto close;

    memcpy(name, info->name, 0x18);
    name[0x18] = 0;
    width = ui_window_text_width(&self->frame.base, name, 0, 0, 0xc, 0, 0) + 0x3c;
    push_line(&self->lines, "Name");
    push_line(&self->lines, name);
    push_line(&self->lines, "Job");
    push_line(&self->lines, session_get_job_name(&g_session1, info->job, -1));
    push_line(&self->lines, "Lv. ");
    push_line(&self->lines, _itoa(info->level, num, 10));
    push_line(&self->lines, "EXP");
    push_line(&self->lines, _itoa((int)info->exp, num, 10));
    push_line(&self->lines, "HP");
    push_line(&self->lines, _i64toa(info->hp, num, 10));
    push_line(&self->lines, "SP");
    push_line(&self->lines, _i64toa(info->sp, num, 10));
    for (i = 0; i < 6; i++) {
        push_line(&self->lines, stats[i]);
        push_line(&self->lines, _itoa(info->stats[i], num, 10));
    }

    ui_trans_balloon_text_resize(self->balloon, width < 0x8c ? 0x8c : width, 0xc8);
    {
        struct UITransBalloonText *balloon = (struct UITransBalloonText *)self->balloon;
        const struct String *line;

        balloon->balloon.outline = 0;
        balloon->balloon.colour = (DWORD)b << 16 | (DWORD)g << 8 | r;
        balloon->backdrop.argb = 0x88000000;
        text_array_resize(&balloon->balloon.lines, 0);
        for (line = self->lines.first; line != self->lines.last; line++)
            text_array_append(&balloon->balloon.lines, text_view(string_cstr(line), line->size));
    }
    {
        int sx = x + 0x14, sy = y;

        ui_window_to_screen(&self->frame.base, &sx, &sy);
        if (sx < -3)
            sx = -3;
        if (sx > g_renderer->width - self->balloon->width + 3)
            sx = g_renderer->width - self->balloon->width + 3;
        if (sy < -3)
            sy = -3;
        if (sy > g_renderer->height - self->balloon->height + 3)
            sy = g_renderer->height - self->balloon->height + 3;
        ui_trans_balloon_text_move(self->balloon, sx, sy);
    }
    return;

close:
    ui_window_mgr_post_quit2(&g_windowMgr, self->balloon);
    self->balloon = NULL;
}

void ui_sel_char_for_u_server_wnd_on_mouse_move(struct UIWindow *window, int x, int y)
{
    struct UISelCharForUServerWnd *self = (struct UISelCharForUServerWnd *)window;
    show_balloon(self, x, y, 0xffffffff);
    ui_frame_wnd_on_mouse_move((struct UIWindow *)(&self->frame), x, y);
}

static void send_checked(struct UISelCharForUServerWnd *self)
{
    int gids[SEL_CHAR_MAX];
    int n = 0, i;

    if (ui_window_mgr_message_box(&g_windowMgr, msg_str(MSG_CONFIRM), UI_MSGBOX_QUESTION, 0, 0, 0, "메시지",
                               0x118, 0x78, 0) != UI_MSGBOX_ANSWER_YES)
        return;
    memset(gids, 0, sizeof gids);
    for (i = 0; i < g_session1.charCount; i++) {
        struct CharInfo *info;

        if (!self->checks[i]->state)
            continue;
        info = char_info(i);
        if (info)
            gids[n++] = info->gid;
    }
    if (n) {
        struct PACKET_CZ_SELECT_CHARS packet;

        packet.id = 0x28c;
        packet.aid = g_session1.ownAid;
        packet.count = n;
        memcpy(packet.gids, gids, sizeof packet.gids);
        rag_connection_send_packet(rag_connection_instance(),
                                  rag_connection_get_packet_length(rag_connection_instance(), 0x28c), &packet);
        ui_window_mgr_post_quit2(&g_windowMgr, &self->frame.base);
    } else {
        ui_window_mgr_message_box(&g_windowMgr, msg_str(MSG_NONE_CHECKED), UI_MSGBOX_OK, 1, 1, 0, "메시지",
                               0x118, 0x78, 0);
    }
}

int ui_sel_char_for_u_server_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                              int val1, int val2, int val3, int val4)
{
    struct UISelCharForUServerWnd *self = (struct UISelCharForUServerWnd *)window;
    if (msg != UI_MSG_COMMAND)
        return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
    if (val1 == OK_ID) {
        send_checked(self);
    } else if (val1 == UI_ID_CHECKBOX) {
        self->checked += sender->state != 0 ? 1 : -1;
        if (self->checked > MAX_CHECKED) {
            ui_window_mgr_message_box(&g_windowMgr, msg_str(MSG_TOO_MANY), UI_MSGBOX_OK, 1, 1, 0, "메시지",
                                   0x118, 0x78, 0);
            ui_check_box_set_check(sender, 0);
        }
    }
    return 0;
}

struct Vec;
struct UISelCharForUServerWnd;
struct UIWindow;
void ui_sel_char_for_u_server_wnd_destroy(struct UIWindow *window);
void ui_sel_char_for_u_server_wnd_on_create(struct UIWindow *self, int cx, int cy);
void ui_sel_char_for_u_server_wnd_on_draw(struct UIWindow *self);
BYTE ui_sel_char_for_u_server_wnd_render(struct UIWindow *self, int force);
void ui_sel_char_for_u_server_wnd_on_mouse_move(struct UIWindow *self, int x, int y);
int ui_sel_char_for_u_server_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_sel_char_for_u_server_wnd_ops = {
    .destroy = ui_sel_char_for_u_server_wnd_destroy,
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
    .on_create = ui_sel_char_for_u_server_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_sel_char_for_u_server_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_frame_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_sel_char_for_u_server_wnd_on_mouse_move,
    .on_mouse_hover = ui_frame_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_frame_wnd_on_l_btn_up,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_sel_char_for_u_server_wnd_send_msg,
    .invalidate = ui_window_invalidate_base,
    .render = ui_sel_char_for_u_server_wnd_render,
    .has_custom_backdrop = 0,
    .draw_backdrop = ui_window_draw_backdrop_base,
    .type = UI_TYPE_FRAME,
    .on_hover_enter = ui_window_on_hover_enter_base,
    .on_hover_leave = ui_window_on_hover_leave_base,
    .make_quad = ui_window_make_quad_base,
    .hit_test = ui_window_hit_test_base,
    .render_overlay = ui_window_render_overlay_base,
};

struct UISelCharForUServerWnd *ui_sel_char_for_u_server_wnd_construct(struct UISelCharForUServerWnd *self)
{
    int i;

    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_sel_char_for_u_server_wnd_ops;
    for (i = 0; i < SEL_CHAR_MAX; i++)
        char_preview_init(&self->looks[i]);
    self->lines.first = self->lines.last = self->lines.end = NULL;
    self->columns = 6;
    if (g_session1.charCount > 0xc)
        self->columns = 9;
    self->balloon = NULL;
    self->checked = 0;
    self->frame.base.state_count = 0;
    self->ok = NULL;
    for (i = 0; i < g_session1.charCount; i++)
        self->hidden[i] = 1;
    self->start_time = timeGetTime();
    self->frame.base.redraws_tree = 1;
    ui_animation_reset(&self->animation);
    self->drawn_key = 0;
    return self;
}

struct UIWindow *ui_sel_char_for_u_server_wnd_new(void)
{
    return (struct UIWindow *)ui_sel_char_for_u_server_wnd_construct(mem_alloc(sizeof(struct UISelCharForUServerWnd)));
}
