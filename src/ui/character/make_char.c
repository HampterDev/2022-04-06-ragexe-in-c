#include "ui/controls/edit.h"
#include "ui/window_access.h"
#include "ui/character/char_preview.h"
#include "render/rp_pool.h"
#include "render/sprite_atlas.h"
#include "resources/imf.h"
#include "resources/palette.h"
#include "resources/sprite.h"
#include "session/number_codec.h"
#include "session/state.h"
#include "core/object_type_tables.h"
#include "core/memory.h"
#include "world/mode_manager.h"
#include "core/markup_tags.h"
#include <stdlib.h>
#include <string.h>
#include "platform/code_page.h"
#include "ui/window_mgr.h"
#include "ui/game.h"
#include "ui/ui_items.h"
#include "ui/controls/controls.h"
#include "ui/ui_std.h"
#include "ui/character/character.h"
#include "login/char_info.h"
#include "core/msg_str.h"
#include "diag/perf.h"
#include "session/session_names.h"
#include "session/session_party.h"
#include "core/cstr.h"

#define MAKE_CHAR_ID_LEFT 0xdb
#define MAKE_CHAR_ID_RIGHT 0xda
#define MAKE_CHAR_NAME_MAX 0x18
#define MAKE_CHAR_HAIR_MAX 0x16
#define MAKE_CHAR_COLOUR_MAX 8
#define MAKE_CHAR_DIRECTIONS 8
#define MAKE_CHAR_MALE 1

#define MODE_MSG_GET_NEW_CHAR_NAME 0x271e
#define MODE_REQ_MAKE_CHAR_INFO 0x2720
#define MODE_REQ_MAKE_CHAR_GO 0x2715
#define MODE_REQ_MAKE_CHAR_NAME 0x271f
#define MODE_REQ_MAKE_CHAR_STATS 0x2721
#define MODE_MSG_MAKE_CHAR_CANCEL 0x2714

#define MSG_MAKE_CHAR_TITLE 0x940
#define MSG_MAKE_CHAR_LEFT_TIP 0x87e
#define MSG_MAKE_CHAR_RIGHT_TIP 0x87f
#define MSG_NAME_SHORT 0x15
#define MSG_NAME_FILTERED 0x14
#define MSG_NAME_SERVICE 0xbe
#define MSG_NAME_BLANK_EDGE 0xc7
#define MSG_NAME_LONG 0x213
#define MSG_NAME_HAS_TAG 0xafc

struct UIMakeCharWnd;
struct UIWindow;
void ui_make_char_wnd_destroy(struct UIWindow *window);
void ui_make_char_wnd_on_create(struct UIWindow *self, int cx, int cy);
void ui_make_char_wnd_on_destroy(struct UIWindow *self);
void ui_make_char_wnd_on_process(struct UIWindow *self);
void ui_make_char_wnd_on_draw(struct UIWindow *self);
intptr_t ui_make_char_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4);
const struct UIWindowOps ui_make_char_wnd_ops = {
    .destroy = ui_make_char_wnd_destroy,
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
    .on_create = ui_make_char_wnd_on_create,
    .on_destroy = ui_make_char_wnd_on_destroy,
    .on_process = ui_make_char_wnd_on_process,
    .on_draw = ui_make_char_wnd_on_draw,
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
    .send_msg = ui_make_char_wnd_send_msg,
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

struct UIMakeCharWnd {
    struct UIFrameWnd frame;
    struct UIWindow *name_edit;
    struct CharInfo info;
    struct CharPreview view;
    struct UIBitmapButton *left;
    struct UIBitmapButton *right;
    struct UIScrollBar *hair_bar;
    struct UIScrollBar *colour_bar;
    int hair_pos;
    int colour_pos;
};

struct UIMakeCharWnd *ui_make_char_wnd_construct(struct UIMakeCharWnd *self)
{
    struct CharPreview *view = &self->view;
    const struct NewCharLooks *start;
    struct String path;
    int head, i;

    ui_window_construct(&self->frame.base, 0);
    self->frame.base.ops = &ui_frame_wnd_ops;
    self->frame.press_x = 0;
    self->frame.press_y = 0;
    self->frame.default_id = UI_ID_NONE;
    self->frame.cancel_id = UI_ID_NONE;
    self->frame.base.ops = &ui_make_char_wnd_ops;
    char_preview_init(view);
    self->left = NULL;
    self->right = NULL;
    self->hair_bar = NULL;
    self->colour_bar = NULL;
    self->hair_pos = 0;
    self->colour_pos = 0;
    self->frame.base.state_count = 0;
    self->name_edit = NULL;
    self->info.sex = ((BYTE)g_session1.accountSex);

    start = (const struct NewCharLooks *)mode_send_message(client_current_mode(), MODE_REQ_MAKE_CHAR_INFO,
                                                           0, 0, 0, 0);
    for (i = 0; i < 6; i++)
        self->info.stats[i] = (BYTE)start->stats[i];
    self->info.head_palette = (short)start->head_palette;
    self->info.head = (short)start->head;
    self->info.name[0] = 0;
    self->info.job = 0;
    self->info.status_points = 0;
    self->info.exp = 0;
    self->info.level = 1;
    self->info.skill_points = 1;
    self->info.hp = 100;
    self->info.max_hp = 100;
    self->info.sp = 100;
    self->info.max_sp = 100;
    self->info.slot = g_loginCharInfo.slot;
    self->info.head_palette = 0;
    self->info.body_palette = 0;
    self->info.shield = 0;
    self->info.accessory2 = 0;
    self->info.accessory3 = 0;
    view->action_base = 0;
    view->action = 0;
    view->frame = 0;
    view->x = 0x46;
    view->y = 0x7b;

    string_free(&view->act[0]);
    string_move(&view->act[0], session_get_body_spr_name_variant0(&path, 0, self->info.sex, 0, 0));
    string_free(&view->spr[0]);
    string_move(&view->spr[0], session_get_body_spr_name_variant1(&path, self->info.job, self->info.sex, 0, 0));
    if (self->info.sex == MAKE_CHAR_MALE && self->info.head == 0xd)
        self->info.head = 0xe;
    head = self->info.head;
    string_free(&view->act[1]);
    string_move(&view->act[1], session_get_head_spr_name0(&path, self->info.job, self->info.sex, &head));
    string_free(&view->spr[1]);
    string_move(&view->spr[1], session_get_head_spr_name1(&path, self->info.job, self->info.sex, &head));
    self->info.head = (short)head;
    view->body_palette = self->info.body_palette;
    if (view->body_palette) {
        string_free(&view->body_palette_name);
        string_move(&view->body_palette_name, session_get_body_pal_name(&path, self->info.job, self->info.sex,
                                                       view->body_palette, self->info.body, 0));
    }
    view->head_palette = self->info.head_palette;
    if (view->head_palette) {
        string_free(&view->head_palette_name);
        string_move(&view->head_palette_name, session_get_head_pal_name(&path, head, self->info.job, self->info.sex, view->head_palette));
    }
    string_free(&view->imf);
    string_move(&view->imf, session_get_imf_name(&path, self->info.job, self->info.sex));
    self->frame.base.alpha = 0x80;
    self->frame.base.alpha_target = 0xff;
    return self;
}

struct UIWindow *ui_make_char_wnd_new(void)
{
    return (struct UIWindow *)ui_make_char_wnd_construct(mem_alloc(sizeof(struct UIMakeCharWnd)));
}

void ui_make_char_wnd_destruct(struct UIMakeCharWnd *self)
{
    self->frame.base.ops = &ui_make_char_wnd_ops;
    char_preview_destroy(&self->view);
    self->frame.base.ops = &ui_frame_wnd_ops;
    ui_window_destruct(&self->frame.base);
}

void ui_make_char_wnd_destroy(struct UIWindow *window)
{
    struct UIMakeCharWnd *self = (struct UIMakeCharWnd *)window;
    ui_make_char_wnd_destruct(self);
    mem_free(self);
}

static struct UIBitmapButton *new_arrow(const char *bitmap)
{
    void *mem = mem_try_alloc(sizeof(struct UIBitmapButton));
    struct UIBitmapButton *button = mem ? ui_bitmap_button_construct(mem) : NULL;

    ui_bitmap_button_set_bitmap(button, bitmap, 0, 0);
    ui_bitmap_button_set_bitmap(button, bitmap, 1, 0);
    ui_bitmap_button_set_bitmap(button, bitmap, 2, 0);
    return button;
}

void ui_make_char_wnd_make_controls(struct UIMakeCharWnd *self)
{
    static const char *const names[2][3] = {
        { "btn_ok", "btn_ok_a", "btn_ok_b" },
        { "btn_cancel", "btn_cancel_a", "btn_cancel_b" },
    };
    static const int pos[2][2] = { { 0x39, 0x106 }, { 0x68, 0x106 } };
    static const char *const tips[2] = { "", "" };
    static const int ids[2] = { UI_ID_OK, UI_ID_CANCEL };
    struct String dir;
    struct UIBitmapButton *button;
    void *mem;
    int i;

    ui_skin_dir(&dir);
    for (i = 0; i < 2; i++) {
        mem = mem_alloc(sizeof(struct UIBitmapButton));
        button = mem ? ui_bitmap_button_construct(mem) : NULL;
        ui_button_bitmap(button, &dir, names[i][0], 0);
        ui_button_bitmap(button, &dir, names[i][1], 1);
        ui_button_bitmap(button, &dir, names[i][2], 2);
        ui_window_create(&button->button.base, button->bitmap_width, button->bitmap_height);
        ui_window_move_base(&button->button.base, pos[i][0], self->frame.base.height - button->bitmap_height - 5);
        button->button.base.id = ids[i];
        ui_bitmap_button_set_tool_tip(button, tips[i]);
        ui_window_add_child(&self->frame.base, &button->button.base);
    }

    self->left = new_arrow(UI_SKIN "scroll1left.bmp");
    ui_window_create(&self->left->button.base, self->left->bitmap_width, self->left->bitmap_height);
    ui_window_move_base(&self->left->button.base, 0x14, 0x50);
    self->left->button.base.id = MAKE_CHAR_ID_LEFT;
    ui_bitmap_button_set_tool_tip(self->left, msg_str(MSG_MAKE_CHAR_LEFT_TIP));
    ui_window_add_child(&self->frame.base, &self->left->button.base);

    self->right = new_arrow(UI_SKIN "scroll1right.bmp");
    ui_window_create(&self->right->button.base, self->left->bitmap_width, self->left->bitmap_height);
    ui_window_move_base(&self->right->button.base, 0x6e, 0x50);
    self->right->button.base.id = MAKE_CHAR_ID_RIGHT;
    ui_bitmap_button_set_tool_tip(self->right, msg_str(MSG_MAKE_CHAR_RIGHT_TIP));
    ui_window_add_child(&self->frame.base, &self->right->button.base);

    self->hair_pos = self->info.head - 1;
    if (self->hair_pos < 0)
        self->hair_pos = 0;
    self->colour_pos = self->view.head_palette < 0 ? 0 : self->view.head_palette;

    mem = mem_alloc(sizeof(struct UIScrollBar));
    self->hair_bar = mem ? ui_scroll_bar_construct(mem) : NULL;
    ui_scroll_bar_create(self->hair_bar, 0xa, 0xbe, 0, 0x82);
    ui_window_add_child(&self->frame.base, (struct UIWindow *)self->hair_bar);
    mem = mem_alloc(sizeof(struct UIScrollBar));
    self->colour_bar = mem ? ui_scroll_bar_construct(mem) : NULL;
    ui_scroll_bar_create(self->colour_bar, 0xa, 0xe6, 0, 0x82);
    ui_window_add_child(&self->frame.base, (struct UIWindow *)self->colour_bar);
    if (self->hair_bar) {
        ui_scroll_bar_set_range(self->hair_bar, MAKE_CHAR_HAIR_MAX, 1, MAKE_CHAR_HAIR_MAX + 1);
        ui_window_invalidate_base(&self->hair_bar->base);
    }
    if (self->colour_bar) {
        ui_scroll_bar_set_range(self->colour_bar, MAKE_CHAR_COLOUR_MAX, 1, MAKE_CHAR_COLOUR_MAX + 1);
        ui_window_invalidate_base(&self->colour_bar->base);
    }
    ui_scroll_bar_set_pos(self->hair_bar, self->hair_pos);
    ui_scroll_bar_set_pos(self->colour_bar, self->colour_pos);
    string_free(&dir);
}

void ui_make_char_wnd_on_create(struct UIWindow *window, int cx, int cy)
{
    struct UIMakeCharWnd *self = (struct UIMakeCharWnd *)window;
    struct UIBitmapEditCtrl *edit;
    void *mem;

    mode_send_message(client_current_mode(), MODE_MSG_GET_NEW_CHAR_NAME, 0, 0, 0, 0);
    mem = mem_alloc(sizeof(struct UIBitmapEditCtrl));
    edit = NULL;
    if (mem) {
        ui_edit_ctrl_construct(mem);
        edit = mem;
        edit->edit.base.ops = &ui_bitmap_edit_ctrl_ops.base;
        string_init(&edit->bitmap);
    }
    self->name_edit = (struct UIWindow *)edit;
    ui_bitmap_edit_ctrl_set_bitmap(edit, UI_SKIN "login_interface\\name-edit.bmp");
    ui_window_create(self->name_edit, self->name_edit->width, self->name_edit->height);
    ui_window_move_base(self->name_edit, 0x28, 0x8e);
    UI_EDIT_MAX_CHARS(self->name_edit) = MAKE_CHAR_NAME_MAX;
    self->name_edit->id = UI_ID_OK;
    ui_window_add_child(&self->frame.base, self->name_edit);
    ui_make_char_wnd_make_controls(self);
    ui_window_mgr_set_focus_edit(&g_windowMgr, self->name_edit);
    self->frame.default_id = UI_ID_OK;
}

void ui_make_char_wnd_on_destroy(struct UIWindow *self)
{
}

void ui_make_char_wnd_on_process(struct UIWindow *self)
{
}

static void *res_of(const struct String *name)
{
    return res_mgr_get(g_resMgr(), string_cstr(name));
}

void ui_make_char_wnd_on_draw(struct UIWindow *window)
{
    struct UIMakeCharWnd *self = (struct UIMakeCharWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct CharPreview *view = &self->view;
    struct SpriteRegion place = {0};
    int layer;

    ui_window_draw_bitmap(wnd, 0, 0x11, ui_skin_bitmap(UI_SKIN "login_interface\\win_make2.bmp"), 0);
    ui_window_draw_title_bar(wnd, 1, msg_str(MSG_MAKE_CHAR_TITLE), 0);
    ui_window_draw_button_bar(wnd, 2, 0, 1);
    memset(&place, 0, sizeof place);
    for (layer = 0; layer < 2; layer++) {
        const struct ImfRes *imf = res_of(&view->imf);
        struct ActRes *act = res_of(&view->act[layer]);
        struct SprRes *spr = res_of(&view->spr[layer]);
        struct Motion *motion = act_res_get_motion(act, view->action, view->frame);
        DWORD *palette;
        struct SprClip *clip;
        POINT pt;

        pt = imf_resource_position(imf, layer, view->action, view->frame);
        if (layer == 1) {
            struct Motion *body = act_res_get_motion(res_of(&view->act[0]), view->action, view->frame);

            if (motion->attachment_count && body->attachment_count) {
                const struct AttachPointInfo *head_at = motion->attachments, *body_at = body->attachments;

                if (head_at->attr == body_at->attr) {
                    pt.x += body_at->x - head_at->x;
                    pt.y += body_at->y - head_at->y;
                }
            }
        }
        palette = spr->palette;
        if (layer == 0 ? view->body_palette != 0 : view->head_palette != 0)
            palette = palette_resource_colors(res_of(layer == 0 ? &view->body_palette_name : &view->head_palette_name), spr->palette);
        clip = motion_get_clip(motion, layer);
        if (clip->sprIndex != -1) {
            struct SprImg *img = spr_res_image(spr, clip);
            if (!img)
                continue;
            void *tex;

            if (clip->clipType == 0) {
                tex = sprite_atlas_resolve(g_renderer, img, palette, &place);
            } else {
                tex = img->texture;
                place.x = place.y = 0;
            }
            if (!tex) continue;
            ui_canvas_draw_texture(wnd->canvas, view->x + clip->x + pt.x, view->y + clip->y + pt.y, tex,
                                 place.x, place.y, img->width, img->height,
                                 clip->flags, img->widthHalved + 1, img->heightHalved + 1);
        }
    }
}

static void name_box(int msg)
{
    ui_message_box(msg_str(msg));
}

static int has_gm_tag(const char *name)
{
    int i;

    for (i = 0; i < 3; i++)
        if (name[i] == '[' && name[i + 1] == 'g' && name[i + 2] == 'm')
            return 1;
    return 0;
}

static int make_char_ok(struct UIMakeCharWnd *self)
{
    struct String copy;
    struct Mode *mode;
    char name[MAKE_CHAR_NAME_MAX];
    struct NewCharLooks looks;
    int control_chars_ok, service, blank, i;

    if (platform_code_page_find(ui_edit_ctrl_get_text(self->name_edit),
                                string_cstr(&text_tag_ITEML)) ||
        platform_code_page_find(ui_edit_ctrl_get_text(self->name_edit),
                                string_cstr(&text_tag_ITEML_END))) {
        name_box(MSG_NAME_HAS_TAG);
        ui_edit_ctrl_set_text_default((struct UIEditCtrl *)self->name_edit, "");
        return 0;
    }
    if (ui_edit_ctrl_get_text_size(self->name_edit) < 4) {
        name_box(MSG_NAME_SHORT);
        goto send;
    }
    service = 1;
    if (g_serviceType == 1)
        service = session_is_ascii_name(&g_session1, ui_edit_ctrl_get_text(self->name_edit)) ? 1 : 0;
    string_init(&copy);
    string_assign_cstr(&copy, ui_edit_ctrl_get_text(self->name_edit));
    control_chars_ok = !session_has_control_char(&g_session1, copy);
    blank = np_dect(ui_edit_ctrl_get_text(self->name_edit));
    if (!control_chars_ok) {
        name_box(MSG_NAME_FILTERED);
        if (!service)
            name_box(MSG_NAME_SERVICE);
        if (blank)
            name_box(MSG_NAME_BLANK_EDGE);
        goto send;
    }
    if (!service) {
        name_box(MSG_NAME_SERVICE);
        if (blank)
            name_box(MSG_NAME_BLANK_EDGE);
        goto send;
    }
    if (blank) {
        name_box(MSG_NAME_BLANK_EDGE);
        goto send;
    }
    if (ui_edit_ctrl_get_text_size(self->name_edit) >= MAKE_CHAR_NAME_MAX) {
        name_box(MSG_NAME_LONG);
        ui_edit_ctrl_set_text_default((struct UIEditCtrl *)self->name_edit, "");
        return 0;
    }
    memset(name, 0, sizeof name);
    strcpy(name, ui_edit_ctrl_get_text(self->name_edit));
    memcpy(self->info.name, name, sizeof name);
    g_loginCharInfo = self->info;
    if (strstr(g_loginCharInfo.name, "그라비티"))
        return 0;
    cstr_lower(name);
    if (strstr(name, "gravity"))
        return 0;
    if (has_gm_tag(name))
        return 0;
    mode_send_message(client_current_mode(), MODE_REQ_MAKE_CHAR_GO, 0, 0, 0, 0);
    ui_window_mgr_post_quit2(&g_windowMgr, &self->frame.base);
send:
    mode = client_current_mode();
    mode_send_message(mode, MODE_REQ_MAKE_CHAR_NAME, (intptr_t)ui_edit_ctrl_get_text(self->name_edit), 0, 0, 0);
    for (i = 0; i < 6; i++)
        looks.stats[i] = self->info.stats[i];
    looks.head_palette = self->info.head_palette;
    looks.head = self->info.head;
    mode_send_message(mode, MODE_REQ_MAKE_CHAR_STATS, (intptr_t)&looks, 0, 0, 0);
    return 0;
}

static int bar_step(int pos, int msg, int val1)
{
    if (msg == UI_MSG_PAGE_LEFT)
        return pos - 1;
    if (msg == UI_MSG_PAGE_RIGHT)
        return pos + 1;
    return val1 + pos;
}

intptr_t ui_make_char_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender,
                                     int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4)
{
    struct UIMakeCharWnd *self = (struct UIMakeCharWnd *)window;
    struct CharPreview *view = &self->view;
    struct String path;
    char name[MAKE_CHAR_NAME_MAX + 1];
    int sex, old, pos, head;

    switch (msg) {
    case UI_MSG_DEFAULT_BUTTON:
        memset(name, 0, sizeof name);
        strcpy(name, ui_edit_ctrl_get_text(self->name_edit));
        memcpy(self->info.name, name, MAKE_CHAR_NAME_MAX);
        ui_window_send_msg(&self->frame.base, NULL, UI_MSG_COMMAND, self->frame.default_id, 0, 0, 0);
        return 0;
    case UI_MSG_COMMAND:
        switch (val1) {
        case UI_ID_OK:
            return make_char_ok(self);
        case UI_ID_CANCEL:
            ui_window_mgr_post_quit2(&g_windowMgr, &self->frame.base);
            mode_send_message(client_current_mode(), 0, MODE_MSG_MAKE_CHAR_CANCEL, 0, 0, 0);
            return 0;
        case MAKE_CHAR_ID_RIGHT:
            view->action--;
            if (view->action < view->action_base)
                view->action = view->action_base + MAKE_CHAR_DIRECTIONS - 1;
            ui_window_invalidate_base(&self->frame.base);
            return 0;
        case MAKE_CHAR_ID_LEFT:
            view->action++;
            if (view->action >= view->action_base + MAKE_CHAR_DIRECTIONS)
                view->action = view->action_base;
            ui_window_invalidate_base(&self->frame.base);
            return 0;
        }
        return 0;
    case UI_MSG_SCROLL_H:
    case UI_MSG_PAGE_LEFT:
    case UI_MSG_PAGE_RIGHT:
        sex = self->info.sex;
        if (sender == (struct UIWindow *)self->hair_bar) {
            old = self->hair_pos;
            pos = bar_step(old, msg, val1);
            if (pos < 0)
                pos = 0;
            else if (pos > MAKE_CHAR_HAIR_MAX)
                pos = MAKE_CHAR_HAIR_MAX;
            self->hair_pos = pos;
            ui_scroll_bar_set_pos(self->hair_bar, pos);
            ui_window_invalidate_base(&self->hair_bar->base);
            ui_window_invalidate_base(&self->frame.base);
            self->info.head = (short)(self->hair_pos + 1);
            head = self->info.head;
            string_free(&view->act[1]);
            string_move(&view->act[1], session_get_head_spr_name0(&path, self->info.job, sex, &head));
            string_free(&view->spr[1]);
            string_move(&view->spr[1], session_get_head_spr_name1(&path, self->info.job, sex, &head));
            self->info.head = (short)head;
            string_free(&view->imf);
            string_move(&view->imf, session_get_imf_name(&path, self->info.job, sex));
            if (view->head_palette) {
                session_get_head_pal_name(&path, head, self->info.job, sex, view->head_palette);
                string_assign(&view->head_palette_name, string_cstr(&path), path.size);
                string_free(&path);
            }
            return self->hair_pos - old;
        }
        if (sender == (struct UIWindow *)self->colour_bar) {
            old = self->colour_pos;
            pos = bar_step(old, msg, val1);
            if (pos < 0)
                pos = 0;
            else if (pos > MAKE_CHAR_COLOUR_MAX)
                pos = MAKE_CHAR_COLOUR_MAX;
            self->colour_pos = pos;
            ui_scroll_bar_set_pos(self->colour_bar, pos);
            ui_window_invalidate_base(&self->colour_bar->base);
            ui_window_invalidate_base(&self->frame.base);
            view->head_palette = self->colour_pos;
            if (view->head_palette) {
                session_get_head_pal_name(&path, self->info.head, self->info.job, sex, view->head_palette);
                string_assign(&view->head_palette_name, string_cstr(&path), path.size);
                string_free(&path);
            }
            self->info.head_palette = (short)view->head_palette;
            return self->colour_pos - old;
        }
        return 0;
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
}
