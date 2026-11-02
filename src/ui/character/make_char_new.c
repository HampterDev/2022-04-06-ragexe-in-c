#include "ui/controls/edit.h"
#include "ui/window_access.h"
#include "ui/character/selection_buttons.h"
#include "render/rp_pool.h"
#include "render/sprite_atlas.h"
#include "resources/imf.h"
#include "resources/palette.h"
#include "resources/sprite.h"
#include "session/number_codec.h"
#include "core/object_type_tables.h"
#include "world/mode_manager.h"
#include "core/markup_tags.h"
#include <string.h>
#include <errno.h>
#include <mbstring.h>
#include "ui/window_mgr.h"
#include "ui/game.h"
#include "ui/ui_items.h"
#include "ui/controls/controls.h"
#include "ui/character/char_info.h"
#include "ui/runtime.h"
#include "ui/ui_std.h"
#include <stdlib.h>
#include "ui/character/character.h"
#include "core/msg_str.h"
#include "core/skin_path.h"
#include "diag/perf.h"
#include "session/session_jobs.h"
#include "session/session_names.h"
#include "session/session_party.h"
#include "platform/clock.h"

#define MAKE_DIR UI_SKIN "make_character_ver2\\"

#define HAIR_STYLES 24
#define HAIR_COLOURS 9
#define RACE_HUMAN 0
#define RACE_DORAM 1
#define DORAM_JOB 0x107a
#define SEX_FEMALE 0
#define SEX_MALE 1
#define TURN_DIRECTIONS 8

#define ID_MAKE UI_ID_OK
#define ID_CANCEL UI_ID_CANCEL
#define ID_RACE 0xc4
#define ID_COLOUR UI_ID_RADIO
#define ID_TURN_RIGHT 0xda
#define ID_TURN_LEFT 0xdb
#define ID_HAIR_STYLE 0x204
#define ID_SEX 0x205
#define COLOUR_GROUP 0x1e7

#define MSG_CHOOSE 0xd
#define MSG_HAIR_REFRESH 0x28

#define LOGIN_REQ_MAKE_DEFAULTS 0x2720
#define LOGIN_CMD_CANCEL 0x2714
#define LOGIN_REQ_MAKE 0x2715
#define LOGIN_REQ_NAME 0x271f
#define LOGIN_REQ_LOOKS 0x2721

struct UINewMakeCharWnd;
void ui_new_make_char_wnd_destroy(struct UIWindow *window);
void ui_new_make_char_wnd_on_create(struct UIWindow *self, int width_unused, int height_unused);
void ui_new_make_char_wnd_on_process(struct UIWindow *self);
void ui_new_make_char_wnd_on_draw(struct UIWindow *self);
int ui_new_make_char_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_new_make_char_wnd_ops = {
    .destroy = ui_new_make_char_wnd_destroy,
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
    .on_create = ui_new_make_char_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_new_make_char_wnd_on_process,
    .on_draw = ui_new_make_char_wnd_on_draw,
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
    .send_msg = ui_new_make_char_wnd_send_msg,
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

struct UINewMakeCharWnd {
    struct UIFrameWnd frame;
    struct CharPreview preview;
    struct CharInfo info;
    struct UIWindow *name_edit;
    int race;
    struct UIWindow *race_buttons[2];
    struct UIHairStyleButton *hair[HAIR_STYLES];
    struct UIWindow *colours[HAIR_COLOURS];
    struct UIBitmapButton *sex_buttons[2];
    int anim_action;
    DWORD created_time;
    int anim_frame;
    int anim_elapsed;
};

#define WND(self) (&(self)->frame.base)

static void load_sprites(struct UINewMakeCharWnd *self, int fix_head)
{
    struct CharInfo *info = &self->info;
    struct CharPreview *preview = &self->preview;
    struct String tmp;
    int head;

    string_free(&preview->act[0]);
    string_move(&preview->act[0], session_get_body_spr_name_variant0(&tmp, info->job, info->sex, 0, 0));
    string_free(&preview->spr[0]);
    string_move(&preview->spr[0], session_get_body_spr_name_variant1(&tmp, info->job, info->sex, 0, 0));
    if (fix_head && !session_is_doram_job(&g_session1, info->job) && info->sex == SEX_MALE && info->head == 0xd)
        info->head = 0xe;
    head = info->head;
    string_free(&preview->act[1]);
    string_move(&preview->act[1], session_get_head_spr_name0(&tmp, info->job, info->sex, &head));
    string_free(&preview->spr[1]);
    string_move(&preview->spr[1], session_get_head_spr_name1(&tmp, info->job, info->sex, &head));
    info->head = (short)head;
    preview->body_palette = info->body_palette;
    if (preview->body_palette > 0) {
        string_free(&preview->body_palette_name);
        string_move(&preview->body_palette_name, session_get_body_pal_name(&tmp, info->job, info->sex, preview->body_palette, 0, 0));
    }
    preview->head_palette = info->head_palette;
    if (preview->head_palette > 0) {
        string_free(&preview->head_palette_name);
        string_move(&preview->head_palette_name, session_get_head_pal_name(&tmp, head, info->job, info->sex,
                                               preview->head_palette));
    }
    string_free(&preview->imf);
    string_move(&preview->imf, session_get_imf_name(&tmp, info->job, info->sex));
}

struct UINewMakeCharWnd *ui_new_make_char_wnd_construct(struct UINewMakeCharWnd *self)
{
    struct CharInfo *info = &self->info;
    const struct NewCharLooks *defaults;
    int i;

    ui_frame_wnd_construct(&self->frame, 0);
    WND(self)->ops = &ui_new_make_char_wnd_ops;
    char_preview_init(&self->preview);
    self->race = RACE_HUMAN;
    self->name_edit = NULL;
    self->race_buttons[0] = NULL;
    self->race_buttons[1] = NULL;
    WND(self)->state_count = 0;

    defaults = (const struct NewCharLooks *)mode_send_message(client_current_mode(), LOGIN_REQ_MAKE_DEFAULTS,
                                                              0, 0, 0, 0);
    for (i = 0; i < 6; i++)
        info->stats[i] = (BYTE)defaults->stats[i];
    info->head_palette = (short)defaults->head_palette;
    info->head = (short)defaults->head;
    info->name[0] = 0;
    info->job = 0;
    info->level = 1;
    info->skill_points = 1;
    info->status_points = 0;
    info->max_hp = 100;
    info->hp = 100;
    info->max_sp = 100;
    info->sp = 100;
    info->exp = 0;
    info->slot = g_loginCharInfo.slot;
    info->head_palette = 0;
    info->body_palette = 0;
    info->shield = 0;
    info->accessory2 = 0;
    info->accessory3 = 0;
    info->sex = (BYTE)g_session1.accountSex;
    self->preview.x = 0x1f4;
    self->preview.y = 0xe4;
    self->preview.action_base = 0;
    self->preview.action = 0;
    self->preview.frame = 0;
    load_sprites(self, 1);

    self->anim_action = 0;
    self->created_time = platform_ticks_ms();
    self->anim_frame = 0;
    self->anim_elapsed = 0;
    memset(self->hair, 0, 6 * sizeof self->hair[0]);
    memset(self->colours, 0, 2 * sizeof self->colours[0]);
    *(BYTE *)&self->colours[2] = 0;
    *(WORD *)&self->sex_buttons[0] = 0;
    return self;
}

struct UIWindow *ui_new_make_char_wnd_new(void)
{
    return (struct UIWindow *)ui_new_make_char_wnd_construct(mem_alloc(sizeof(struct UINewMakeCharWnd)));
}

static void make_char_destruct(struct UINewMakeCharWnd *self)
{
    WND(self)->ops = &ui_new_make_char_wnd_ops;
    char_preview_destroy(&self->preview);
    ui_frame_wnd_destruct(&self->frame);
}

void ui_new_make_char_wnd_destroy(struct UIWindow *window)
{
    struct UINewMakeCharWnd *self = (struct UINewMakeCharWnd *)window;
    make_char_destruct(self);
    mem_free(self);
}

static struct UIBitmapButton *new_bitmap_button(void)
{
    struct UIBitmapButton *button = mem_alloc(sizeof(struct UIBitmapButton));

    return button ? ui_bitmap_button_construct(button) : NULL;
}

static void set_bitmaps(struct UIBitmapButton *button, const char *up, const char *hover, const char *down)
{
    ui_bitmap_button_set_bitmap(button, up, 0, 0);
    ui_bitmap_button_set_bitmap(button, hover, 1, 0);
    ui_bitmap_button_set_bitmap(button, down, 2, 0);
}

static void create_at(struct UIBitmapButton *button, int x, int y)
{
    ui_window_create(&button->button.base, button->bitmap_width, button->bitmap_height);
    ui_window_move_base(&button->button.base, x, y);
}

static void add_text_button(struct UINewMakeCharWnd *self, int x, int y, int caption_id, int id)
{
    struct UIBmpTextButton *button = mem_alloc(sizeof(struct UIBmpTextButton));
    int state;

    if (button)
        button = ui_bmp_text_button_construct(button);
    set_bitmaps(&button->bmp, MAKE_DIR "bt_make_normal.bmp", MAKE_DIR "bt_make_over.bmp",
                MAKE_DIR "bt_make_press.bmp");
    create_at(&button->bmp, x, y);
    ui_bmp_text_button_set_caption(button, text_view_cstr(msg_str(caption_id)));
    for (state = 0; state < 3; state++)
        ui_bmp_text_button_set_text_style(button, state, 0xe, 0x313131, 1, 1, UI_TEXT_CENTRED, UI_TEXT_CENTRED);
    button->bmp.button.base.id = id;
    ui_window_add_child(WND(self), &button->bmp.button.base);
}

static void add_label(struct UINewMakeCharWnd *self, int x, int y, int text_id)
{
    struct UIWindow *label = mem_alloc(sizeof(struct UIStaticText));

    if (label)
        label = ui_static_text_construct(label);
    ui_window_create(label, 0x64, 0xf);
    ui_static_text_set_text(label, msg_str(text_id), 0);
    ui_static_text_set_text_colour(label, 0x40, 0x40, 0x40);
    ui_static_text_set_font(label, 0, 0xb, 1, 0);
    ui_window_move(label, x, y);
    ui_window_add_child(WND(self), label);
}

static void add_turn_button(struct UINewMakeCharWnd *self, const char *up, const char *hover, const char *down,
                            int x, int id, int tooltip_id)
{
    struct UIBitmapButton *button = new_bitmap_button();

    set_bitmaps(button, up, hover, down);
    create_at(button, x, 0xb3);
    button->button.base.id = id;
    button->button.cmd_value = 0;
    ui_bitmap_button_set_tool_tip(button, msg_str(tooltip_id));
    ui_window_add_child(WND(self), &button->button.base);
}

void ui_new_make_char_wnd_on_create(struct UIWindow *window, int width_unused,
                                          int height_unused)
{
    struct UINewMakeCharWnd *self = (struct UINewMakeCharWnd *)window;
    struct UIWindow *wnd = WND(self);
    struct UIBitmapButton *button;
    struct UIWindow *race;
    char path[0x80];
    int i;

    button = new_bitmap_button();
    set_bitmaps(button, MAKE_DIR "bt_close_normal.bmp", MAKE_DIR "bt_close_over.bmp",
                MAKE_DIR "bt_close_press.bmp");
    create_at(button, 0x300, 9);
    button->button.base.id = ID_CANCEL;
    ui_window_add_child(wnd, &button->button.base);

    add_text_button(self, 0x268, 0x17b, 0xd11, ID_MAKE);

    self->name_edit = mem_alloc(sizeof(struct UIEditCtrl));
    if (self->name_edit)
        self->name_edit = ui_edit_ctrl2_construct(self->name_edit);
    ui_window_create(self->name_edit, 0x82, 0x14);
    ui_window_move(self->name_edit, 0x1b3, 0x10c);
    ((struct UIEditCtrl *)self->name_edit)->max_chars = 0x17;
    ui_window_add_child(wnd, self->name_edit);

    add_text_button(self, 0x19, 0x17b, 0xd17, ID_CANCEL);

    race = ui_descript_race_create(RACE_HUMAN);
    set_bitmaps((struct UIBitmapButton *)race, MAKE_DIR "img_human_off.bmp", MAKE_DIR "img_human_on.bmp",
                MAKE_DIR "img_human_on.bmp");
    create_at((struct UIBitmapButton *)race, 0x15, 0x24);
    self->race_buttons[RACE_HUMAN] = race;
    ui_window_add_child(wnd, race);

    race = ui_descript_race_create(RACE_DORAM);
    if (!g_session1.doramMakeable)
        set_bitmaps((struct UIBitmapButton *)race, MAKE_DIR "img_doram_comingSoon.bmp",
                    MAKE_DIR "img_doram_comingSoon.bmp", MAKE_DIR "img_doram_comingSoon.bmp");
    else
        set_bitmaps((struct UIBitmapButton *)race, MAKE_DIR "img_doram_off.bmp", MAKE_DIR "img_doram_on.bmp",
                    MAKE_DIR "img_doram_on.bmp");
    create_at((struct UIBitmapButton *)race, 0x15, 0xca);
    self->race_buttons[RACE_DORAM] = race;
    ui_window_add_child(wnd, race);

    self->sex_buttons[SEX_MALE] = new_bitmap_button();
    set_bitmaps(self->sex_buttons[SEX_MALE], MAKE_DIR "bt_male_off.bmp", MAKE_DIR "bt_male_over.bmp",
                MAKE_DIR "bt_male_press.bmp");
    create_at(self->sex_buttons[SEX_MALE], 0x1b5, 0x3b);
    self->sex_buttons[SEX_MALE]->button.base.id = ID_SEX;
    ui_window_add_child(wnd, &self->sex_buttons[SEX_MALE]->button.base);

    self->sex_buttons[SEX_FEMALE] = new_bitmap_button();
    set_bitmaps(self->sex_buttons[SEX_FEMALE], MAKE_DIR "bt_female_off.bmp", MAKE_DIR "bt_female_over.bmp",
                MAKE_DIR "bt_female_press.bmp");
    create_at(self->sex_buttons[SEX_FEMALE], 0x1f4, 0x3b);
    self->sex_buttons[SEX_FEMALE]->button.base.id = ID_SEX;
    ui_window_add_child(wnd, &self->sex_buttons[SEX_FEMALE]->button.base);
    if (self->info.sex == SEX_MALE)
        ui_bitmap_button_set_bitmap(self->sex_buttons[SEX_MALE], MAKE_DIR "bt_male_on.bmp", 0, 0);
    else if (self->info.sex == SEX_FEMALE)
        ui_bitmap_button_set_bitmap(self->sex_buttons[SEX_FEMALE], MAKE_DIR "bt_female_on.bmp", 0, 0);

    add_turn_button(self, MAKE_DIR "bt_leftTurn_normal.bmp", MAKE_DIR "bt_leftTurn_over.bmp",
                    MAKE_DIR "bt_leftTurn_press.bmp", 0x1aa, ID_TURN_LEFT, 0x87e);
    add_turn_button(self, MAKE_DIR "bt_rightTurn_normal.bmp", MAKE_DIR "bt_rightTurn_over.bmp",
                    MAKE_DIR "bt_rightTurn_press.bmp", 0x227, ID_TURN_RIGHT, 0x87f);

    add_label(self, 0x261, 0x2e, 0xd12);
    for (i = 0; i < HAIR_STYLES; i++) {
        struct UIHairStyleButton *hair = mem_alloc(sizeof *hair);

        if (hair)
            hair = ui_hair_style_button_construct(hair, i,
                                                                  self->info.sex);
        self->hair[i] = hair;
        set_bitmaps(&hair->button, MAKE_DIR "bt_hairStyle_normal.bmp",
                    MAKE_DIR "bt_hairStyle_over.bmp", MAKE_DIR "bt_hairStyle_select.bmp");
        ui_window_create(&self->hair[i]->button.button.base, 0x24, 0x25);
        self->hair[i]->button.button.base.id = ID_HAIR_STYLE;
        ui_window_move_base(&self->hair[i]->button.button.base, (i & 3) * 0x26 + 0x25e, (i >> 2) * 0x25 + 0x3b);
        ui_window_add_child(wnd, &self->hair[i]->button.button.base);
        if (i == 0)
            ui_hair_style_button_send_msg(&self->hair[0]->button.button.base, wnd, MSG_CHOOSE, 1, 0, 0, 0);
    }

    add_label(self, 0x261, 0x11f, 0xd13);
    memset(path, 0, sizeof path);
    for (i = 0; i < HAIR_COLOURS; i++) {
        struct UIWindow *colour = ui_bmp_radio_btn_create(COLOUR_GROUP);
        memset(path, 0, 0x40);
        sprintf(path, MAKE_DIR "color%02d_off.bmp", i + 1);
        ui_bmp_radio_btn_set_bitmap(colour, path, 0);
        memset(path, 0, 0x40);
        sprintf(path, MAKE_DIR "color%02d_over.bmp", i + 1);
        ui_bmp_radio_btn_set_bitmap(colour, path, 1);
        memset(path, 0, 0x40);
        sprintf(path, MAKE_DIR "color%02d_on.bmp", i + 1);
        ui_bmp_radio_btn_set_bitmap(colour, path, 2);
        if (i == 0)
            ui_window_send_msg(colour, wnd, MSG_CHOOSE, 1, 0, 0, 0);
        ui_window_move(colour, (i % 5) * 0x1b + 0x26b, (i / 5) * 0x18 + 0x132);
        self->colours[i] = colour;
        ui_window_add_child(wnd, colour);
    }
    ui_window_mgr_set_focus_edit(&g_windowMgr, self->name_edit);
    self->frame.default_id = ID_MAKE;
}

static void *get_res(const struct String *path)
{
    return res_mgr_get(g_resMgr(), string_cstr(path));
}

void ui_new_make_char_wnd_on_draw(struct UIWindow *window)
{
    struct UINewMakeCharWnd *self = (struct UINewMakeCharWnd *)window;
    struct UIWindow *wnd = WND(self);
    struct CharPreview *preview = &self->preview;
    int layer;

    ui_window_draw_bitmap(wnd, 0, 0, res_mgr_get(g_resMgr(), ui_skin_path(MAKE_DIR "bg_makeBg.bmp")),
                        0);
    ui_window_text_out(wnd, 0x2c, 0x11, msg_str(0xd1b), 0, 0, 0xc, 0x5f5f5f, 1, 0);
    for (layer = 0; layer < 2; layer++) {
        const struct ImfRes *imf = get_res(&preview->imf);
        struct ActRes *act = get_res(&preview->act[layer]);
        struct SprRes *spr = get_res(&preview->spr[layer]);
        struct Motion *motion = act_res_get_motion(act, preview->action, preview->frame);
        struct SprClip *clip;
        DWORD *palette;
        POINT pt;

        pt = imf_resource_position(imf, layer, preview->action, preview->frame);
        if (layer == 1) {
            struct Motion *body = act_res_get_motion(get_res(&preview->act[0]), preview->action,
                                                     preview->frame);

            if (animation_attachment_count(motion) && animation_attachment_count(body) &&
                animation_attachment(motion, 0)->attr == animation_attachment(body, 0)->attr) {
                pt.x += animation_attachment(body, 0)->x - animation_attachment(motion, 0)->x;
                pt.y += animation_attachment(body, 0)->y - animation_attachment(motion, 0)->y;
            }
        }
        palette = spr->palette;
        if (layer == 0 && preview->body_palette > 0)
            palette = palette_resource_colors(get_res(&preview->body_palette_name), spr->palette);
        else if (layer == 1 && preview->head_palette > 0)
            palette = palette_resource_colors(get_res(&preview->head_palette_name), spr->palette);

        clip = motion_get_clip(motion, layer);
        if (clip->sprIndex != -1) {
            struct SprImg *img = spr_res_image(spr, clip);
            if (!img)
                continue;
            struct SpriteRegion place = {0};
            void *tex;

            memset(&place, 0, sizeof place);
            if (clip->clipType == 0) {
                tex = sprite_atlas_resolve(g_renderer, img, palette, &place);
            } else {
                tex = img->texture;
                place.x = place.y = 0;
            }
            if (!tex) continue;
            ui_canvas_draw_texture(wnd->canvas, preview->x + pt.x + clip->x, clip->y + preview->y + pt.y, tex,
                                 place.x, place.y, img->width, img->height,
                                 clip->flags, img->widthHalved + 1, img->heightHalved + 1);
        }
    }
}

void ui_new_make_char_wnd_on_process(struct UIWindow *window)
{
    struct UINewMakeCharWnd *self = (struct UINewMakeCharWnd *)window;
    ui_window_invalidate_base(self->race_buttons[self->race]);
}

static void box(int msg_id)
{
    ui_window_mgr_message_box(&g_windowMgr, msg_str(msg_id), UI_MSGBOX_OK, 1, 0, 0, "메시지", 0x118, 0x78, 0);
}

static int has_gm_tag(const char *s)
{
    int i;

    for (i = 0; i < 3; i++)
        if (s[i] == '[' && s[i + 1] == 'g' && s[i + 2] == 'm')
            return 1;
    return 0;
}

static void on_make(struct UINewMakeCharWnd *self, struct UIWindow *sender)
{
    struct String arg;
    struct NewCharLooks looks;
    BYTE service_ok, control_chars_ok, blank_edge;
    int i;

    for (i = 0; i < 2; i++)
        if ((g_session1.doramMakeable || i != RACE_DORAM) && sender == self->race_buttons[i])
            return;
    if (ui_edit_ctrl_get_text_size(self->name_edit) < 4) {
        box(0x15);
        return;
    }
    if (_mbsstr((const unsigned char *)ui_edit_ctrl_get_text(self->name_edit),
                (const unsigned char *)string_cstr(&text_tag_ITEML)) ||
        _mbsstr((const unsigned char *)ui_edit_ctrl_get_text(self->name_edit),
                (const unsigned char *)string_cstr(&text_tag_ITEML_END))) {
        box(0xafc);
        return;
    }
    service_ok = 1;
    if (g_serviceType == 1)
        service_ok = session_is_ascii_name(&g_session1, ui_edit_ctrl_get_text(self->name_edit)) ? 1 : 0;
    string_init(&arg);
    string_assign_cstr(&arg, ui_edit_ctrl_get_text(self->name_edit));
    control_chars_ok = !session_has_control_char(&g_session1, arg);
    blank_edge = np_dect(ui_edit_ctrl_get_text(self->name_edit));

    if (control_chars_ok == 1 && service_ok == 1 && !blank_edge) {
        char name[25];

        if (ui_edit_ctrl_get_text_size(self->name_edit) >= 0x18) {
            box(0x213);
            ui_edit_ctrl_set_text((struct UIEditCtrl *)self->name_edit, "");
            return;
        }
        memset(name, 0, sizeof name);
        strcpy(name, ui_edit_ctrl_get_text(self->name_edit));
        memcpy(self->info.name, name, sizeof self->info.name);
        memcpy_s(&g_loginCharInfo, sizeof g_loginCharInfo, &self->info, sizeof self->info);
        if (strstr(g_loginCharInfo.name, "그라비티"))
            return;
        _strlwr(name);
        if (strstr(name, "gravity") || has_gm_tag(name))
            return;
        mode_send_message(client_current_mode(), LOGIN_REQ_MAKE, 0, 0, 0, 0);
        ui_window_mgr_post_quit2(&g_windowMgr, WND(self));
    } else {
        if (control_chars_ok != 1) {
            if (!control_chars_ok)
                box(0x14);
        }
        if (control_chars_ok != 1 || service_ok != 1) {
            if (!service_ok)
                box(0xbe);
        }
        if (blank_edge == 1)
            box(0xc7);
    }
    mode_send_message(client_current_mode(), LOGIN_REQ_NAME, (int)ui_edit_ctrl_get_text(self->name_edit), 0, 0, 0);
    for (i = 0; i < 6; i++)
        looks.stats[i] = self->info.stats[i];
    looks.head_palette = self->info.head_palette;
    looks.head = self->info.head;
    mode_send_message(client_current_mode(), LOGIN_REQ_LOOKS, (int)&looks, 0, 0, 0);
}

static void on_race(struct UINewMakeCharWnd *self, struct UIWindow *sender)
{
    int i;

    if (!g_session1.doramMakeable && sender == self->race_buttons[RACE_DORAM])
        return;
    for (i = 0; i < 2; i++) {
        if (sender == self->race_buttons[i]) {
            self->race = i;
            ui_descript_race_send_msg(self->race_buttons[i], WND(self), MSG_CHOOSE, 1, 0, 0, 0);
        } else {
            ui_descript_race_send_msg(self->race_buttons[i], WND(self), MSG_CHOOSE, 0, 0, 0, 0);
        }
        ui_window_invalidate_base(self->race_buttons[i]);
    }
    if (sender != self->race_buttons[self->race])
        return;
    self->info.head = 1;
    for (i = 0; i < HAIR_STYLES; i++) {
        ui_hair_style_button_send_msg(&self->hair[i]->button.button.base, WND(self), MSG_CHOOSE, i == 0, 0, 0, 0);
        self->hair[i]->doram = self->race;
        ui_hair_style_button_send_msg(&self->hair[i]->button.button.base, NULL, MSG_HAIR_REFRESH, 0, 0, 0, 0);
    }
    if (self->race == RACE_DORAM)
        self->info.job = DORAM_JOB;
    else if (self->race == RACE_HUMAN)
        self->info.job = 0;
    load_sprites(self, 0);
}

static void on_colour(struct UINewMakeCharWnd *self, struct UIWindow *sender)
{
    int i;

    for (i = 0; i < HAIR_COLOURS; i++) {
        if (sender == self->colours[i]) {
            ui_window_send_msg(self->colours[i], WND(self), MSG_CHOOSE, 1, 0, 0, 0);
            self->info.head_palette = (short)i;
            if (self->info.head_palette < 0)
                self->info.head_palette = 8;
            load_sprites(self, 0);
        } else {
            ui_window_send_msg(self->colours[i], WND(self), MSG_CHOOSE, 0, 0, 0, 0);
        }
    }
}

static void on_sex(struct UINewMakeCharWnd *self, struct UIWindow *sender)
{
    int i;

    for (i = 0; i < 2; i++) {
        struct UIBitmapButton *button = self->sex_buttons[i];

        if (sender == &button->button.base) {
            if (i == self->info.sex)
                return;
            self->info.sex = (BYTE)i;
            if (i == SEX_MALE)
                ui_bitmap_button_set_bitmap(self->sex_buttons[SEX_MALE], MAKE_DIR "bt_male_on.bmp", 0, 0);
            else if (i == SEX_FEMALE)
                ui_bitmap_button_set_bitmap(self->sex_buttons[SEX_FEMALE], MAKE_DIR "bt_female_on.bmp", 0, 0);
        } else if (i == SEX_MALE) {
            ui_bitmap_button_set_bitmap(button, MAKE_DIR "bt_male_off.bmp", 0, 0);
        } else if (self->info.sex == SEX_FEMALE) {
            ui_bitmap_button_set_bitmap(button, MAKE_DIR "bt_female_off.bmp", 0, 0);
        }
        ui_window_invalidate_base(&self->sex_buttons[i]->button.base);
    }
    for (i = 0; i < HAIR_STYLES; i++) {
        self->hair[i]->male = self->info.sex;
        ui_hair_style_button_send_msg(&self->hair[i]->button.button.base, NULL, MSG_HAIR_REFRESH, 0, 0, 0, 0);
    }
    load_sprites(self, 0);
}

static void on_hair(struct UINewMakeCharWnd *self, struct UIWindow *sender)
{
    int i;

    for (i = 0; i < HAIR_STYLES; i++) {
        if (sender == &self->hair[i]->button.button.base) {
            if (i >= (session_is_doram_job(&g_session1, self->info.job) == 1 ? 6 : 0x17))
                return;
            ui_hair_style_button_send_msg(&self->hair[i]->button.button.base, WND(self), MSG_CHOOSE, 1, 0, 0, 0);
            self->info.head = (short)(i + 1);
            if (self->info.head < 0)
                self->info.head = 0;
            load_sprites(self, 0);
        } else {
            ui_hair_style_button_send_msg(&self->hair[i]->button.button.base, WND(self), MSG_CHOOSE, 0, 0, 0, 0);
        }
    }
}

int ui_new_make_char_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender,
                                        int msg, int val1, int val2, int val3, int val4)
{
    struct UINewMakeCharWnd *self = (struct UINewMakeCharWnd *)window;
    if (msg == UI_MSG_DEFAULT_BUTTON) {
        char name[25];

        memset(name, 0, sizeof name);
        strcpy(name, ui_edit_ctrl_get_text(self->name_edit));
        memcpy(self->info.name, name, sizeof self->info.name);
        ui_new_make_char_wnd_send_msg(WND(self), NULL, UI_MSG_COMMAND, self->frame.default_id, 0, 0, 0);
        return 0;
    }
    if (msg != UI_MSG_COMMAND) {
        if (msg == MSG_CHOOSE)
            self->race = val1;
        return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
    }
    switch (val1) {
    case ID_MAKE:
        on_make(self, sender);
        break;
    case ID_CANCEL:
        ui_window_mgr_post_quit2(&g_windowMgr, WND(self));
        mode_send_message(client_current_mode(), MODE_MSG_UI_COMMAND, LOGIN_CMD_CANCEL, 0, 0, 0);
        break;
    case ID_RACE:
        on_race(self, sender);
        break;
    case ID_COLOUR:
        if (val2 == COLOUR_GROUP)
            on_colour(self, sender);
        break;
    case ID_TURN_RIGHT:
        if (val2)
            break;
        if (--self->preview.action < self->preview.action_base)
            self->preview.action = self->preview.action_base + TURN_DIRECTIONS - 1;
        ui_window_invalidate_base(WND(self));
        break;
    case ID_TURN_LEFT:
        if (val2)
            break;
        if (++self->preview.action >= self->preview.action_base + TURN_DIRECTIONS)
            self->preview.action = self->preview.action_base;
        ui_window_invalidate_base(WND(self));
        break;
    case ID_HAIR_STYLE:
        on_hair(self, sender);
        break;
    case ID_SEX:
        on_sex(self, sender);
        break;
    }
    return 0;
}
