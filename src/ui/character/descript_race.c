#include "ui/controls/buttons.h"
#include "ui/character/selection_buttons.h"
#include "core/text_array.h"
#include "render/rp_pool.h"
#include "render/sprite_atlas.h"
#include "resources/imf.h"
#include "resources/sprite.h"
#include "core/object_type_tables.h"
#include "ui/window_access.h"
#include "ui/window_mgr.h"
#include "ui/game.h"
#include "ui/ui_items.h"
#include "ui/controls/controls.h"
#include "ui/ui_std.h"
#include <stdlib.h>
#include <string.h>
#include "ui/character/character.h"
#include "core/msg_str.h"
#include "diag/perf.h"
#include "session/session_names.h"
#include "platform/clock.h"

#define RACE_HUMAN 0
#define RACE_DORAM 1
#define DORAM_JOB 0x107a

#define MSG_HUMAN_TITLE 0xbc8
#define MSG_DORAM_TITLE 0xbca
#define MSG_HUMAN_TEXT 0xbcc
#define MSG_DORAM_TEXT 0xbcd

#define MAKE_CHAR_WND_ID 0x116
#define RACE_BUTTON_ID 0xc4

#define DESCRIPT_RACE_MSG_CHOOSE 0xd

#define ACTION_STAND 1
#define ACTION_WALK 9
#define FRAME_MS 0x3c

#define MAKE_DIR UI_SKIN "make_character_ver2\\"

struct UIDescriptRace {
    struct UIBitmapButton base;
    int race;
    BYTE animating;
    int frame;
    DWORD frame_time;
};
struct UIWindow;
void ui_descript_race_destroy(struct UIWindow *window);
void ui_descript_race_on_create(struct UIWindow *self, int cx, int cy);
void ui_descript_race_on_draw(struct UIWindow *self);
void ui_descript_race_on_l_btn_down(struct UIWindow *self, int x, int y);
intptr_t ui_descript_race_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4);
const struct UIButtonOps ui_descript_race_ops = {
    .base = {
        .destroy = ui_descript_race_destroy,
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
        .on_create = ui_descript_race_on_create,
        .on_destroy = ui_window_on_destroy_base,
        .on_process = ui_window_on_process_base,
        .on_draw = ui_descript_race_on_draw,
        .on_focus = ui_window_on_focus_base,
        .on_focus_lost = ui_window_on_focus_lost_base,
        .on_l_btn_down = ui_descript_race_on_l_btn_down,
        .on_l_btn_dbl_clk = ui_button_on_l_btn_dbl_clk,
        .on_mouse_move = ui_button_on_mouse_move,
        .on_mouse_hover = ui_button_on_mouse_hover,
        .track_mouse = ui_button_track_mouse,
        .on_mouse_shape = ui_button_on_mouse_shape,
        .on_l_btn_up = ui_button_on_l_btn_up,
        .on_r_btn_down = ui_window_on_r_btn_down_base,
        .on_r_btn_up = ui_window_on_r_btn_up_base,
        .on_wheel = ui_window_on_wheel_base,
        .snap = ui_window_snap_base,
        .send_msg = ui_descript_race_send_msg,
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

static void *get_res(struct String *path)
{
    void *res = res_mgr_get(g_resMgr(), string_cstr(path));

    string_free(path);
    return res;
}

void ui_descript_race_destroy(struct UIWindow *window)
{
    struct UIDescriptRace *self = (struct UIDescriptRace *)window;
    self->base.button.base.ops = &ui_descript_race_ops.base;
    ui_bitmap_button_destruct(&self->base);
    mem_free(self);
}

void ui_descript_race_on_create(struct UIWindow *window, int cx, int cy)
{
    struct UIDescriptRace *self = (struct UIDescriptRace *)window;
    ui_descript_race_send_msg(&self->base.button.base, NULL, DESCRIPT_RACE_MSG_CHOOSE, self->animating, 0, 0, 0);
}

static void draw_text(struct UIDescriptRace *self, int title, int text)
{
    struct UIWindow *wnd = (struct UIWindow *)self;
    struct TextArray lines = {0};
    size_t i;
    int y;

    ui_window_text_out(wnd, 0x32, 0x17, msg_str(title), 0, 0, 0xc, 0x99634c, 1, 0);
    ui_window_text_out(wnd, 0x31, 0x16, msg_str(title), 0, 0, 0xc, 0xffffff, 1, 0);
    text_split_lines(msg_str(text), &lines, 0x28);
    for (i = 0, y = 0x16; i < lines.size; i++, y += 0x14)
        ui_window_text_out(wnd, 0x70, y, text_array_at(&lines, i).data, 0, 0, 0xc, 0, 0,
                         0);
    text_array_clear(&lines);
}

static void draw_race(struct UIDescriptRace *self)
{
    struct String path;
    int job, action, frame, layer;

    if (self->race == RACE_DORAM) {
        if (!g_session1.doramMakeable)
            return;
        draw_text(self, MSG_DORAM_TITLE, MSG_DORAM_TEXT);
    } else if (self->race == RACE_HUMAN) {
        draw_text(self, MSG_HUMAN_TITLE, MSG_HUMAN_TEXT);
    }

    job = self->race == RACE_DORAM ? DORAM_JOB : 0;
    action = self->animating ? ACTION_WALK : ACTION_STAND;
    frame = 0;
    for (layer = 0; layer < 2; layer++) {
        void *act = NULL;
        const struct ImfRes *imf = NULL;
        struct SprRes *spr = NULL;
        struct Motion *motion;
        struct SprClip *clip;
        POINT pt;

        if (layer == 0) {
            act = get_res(session_get_body_spr_name_variant0(&path, job, g_session1.accountSex, 0, 0));
            spr = get_res(session_get_body_spr_name_variant1(&path, job, g_session1.accountSex, 0, 0));
            imf = get_res(session_get_imf_name(&path, job, g_session1.accountSex));
        } else {
            int head = 1;

            act = get_res(session_get_head_spr_name0(&path, job, g_session1.accountSex, &head));
            spr = get_res(session_get_head_spr_name1(&path, job, g_session1.accountSex, &head));
            imf = get_res(session_get_imf_name(&path, job, g_session1.accountSex));
        }

        if (self->animating) {
            if (self->frame_time + FRAME_MS <= platform_ticks_ms()) {
                int next = self->frame + 1;

                if (next >= (int)actres_frame_count(act, action))
                    next = 0;
                self->frame = next;
                self->frame_time = platform_ticks_ms();
            }
            frame = self->frame;
        }

        motion = act_res_get_motion(act, action, frame);
        pt = imf_resource_position(imf, layer, action, frame);
        if (layer == 1) {
            struct Motion *body = act_res_get_motion(
                get_res(session_get_body_spr_name_variant0(&path, job, g_session1.accountSex, 0, 0)), action, frame);

            if (animation_attachment_count(motion) && animation_attachment_count(body) &&
                animation_attachment(motion, 0)->attr == animation_attachment(body, 0)->attr) {
                pt.x += animation_attachment(body, 0)->x - animation_attachment(motion, 0)->x;
                pt.y += animation_attachment(body, 0)->y - animation_attachment(motion, 0)->y;
            }
        }

        clip = motion_get_clip(motion, layer);
        if (clip->sprIndex != -1) {
            struct SprImg *img = spr_res_image(spr, clip);
            if (!img)
                continue;
            struct SpriteRegion place = {0};
            void *tex;

            memset(&place, 0, sizeof place);
            if (clip->clipType == 0) {
                tex = sprite_atlas_resolve(g_renderer, img, spr->palette, &place);
            } else {
                tex = img->texture;
                place.x = place.y = 0;
            }
            if (!tex) continue;
            ui_canvas_draw_texture(((struct UIWindow *)self)->canvas, pt.x + 0x3c + clip->x, pt.y + 0x8c + clip->y,
                                 tex, place.x, place.y, img->width, img->height,
                                 clip->flags, img->widthHalved + 1, img->heightHalved + 1);
        }
    }
}

void ui_descript_race_on_draw(struct UIWindow *window)
{
    struct UIDescriptRace *self = (struct UIDescriptRace *)window;
    ui_bitmap_button_on_draw((struct UIWindow *)(&self->base));
    draw_race(self);
}

void ui_descript_race_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIDescriptRace *self = (struct UIDescriptRace *)window;
    struct UIWindow *wnd = ui_window_find(MAKE_CHAR_WND_ID);

    if (wnd)
        ui_window_send_msg(wnd, (struct UIWindow *)self, UI_MSG_COMMAND, RACE_BUTTON_ID, 0, 0, 0);
}

intptr_t ui_descript_race_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, intptr_t val1,
                                      intptr_t val2, intptr_t val3, intptr_t val4)
{
    struct UIDescriptRace *self = (struct UIDescriptRace *)window;
    if (msg != DESCRIPT_RACE_MSG_CHOOSE)
        return 0;
    if (self->race == RACE_DORAM && !g_session1.doramMakeable)
        return 0;
    if (val1) {
        self->animating = 1;
        ui_bitmap_button_set_bitmap(&self->base,
                                 self->race == RACE_DORAM ? MAKE_DIR "img_doram_on.bmp" : MAKE_DIR "img_human_on.bmp",
                                 0, 0);
    } else {
        self->animating = 0;
        ui_bitmap_button_set_bitmap(&self->base,
                                 self->race == RACE_DORAM ? MAKE_DIR "img_doram_off.bmp"
                                                          : MAKE_DIR "img_human_off.bmp",
                                 0, 0);
    }
    return 0;
}

static struct UIDescriptRace *ui_descript_race_construct(struct UIDescriptRace *self, int race)
{
    ui_bitmap_button_construct(&self->base);
    self->base.button.base.ops = &ui_descript_race_ops.base;
    self->race = race;
    self->frame = 0;
    self->frame_time = platform_ticks_ms();
    self->animating = race == RACE_HUMAN;
    return self;
}

struct UIWindow *ui_descript_race_create(int race)
{
    struct UIDescriptRace *button = mem_alloc(sizeof *button);
    if (!button) return NULL;
    ui_descript_race_construct(button, race);
    return &button->base.button.base;
}
