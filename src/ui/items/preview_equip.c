#include "ui/character/character_draw.h"
#include "render/rp_pool.h"
#include "session/state.h"
#include "core/object_type_tables.h"
#include "world/actor_lookup.h"
#include "game/context.h"
#include "actor/appearance.h"
#include "ui/window_mgr.h"
#include "ui/game.h"
#include "ui/controls/controls.h"
#include "ui/character/char_info.h"
#include "ui/runtime.h"
#include "ui/ui_std.h"
#include <stdlib.h>
#include <string.h>
#include "ui/ui_items.h"
#include "ui/render/character_render.h"
#include "core/msg_str.h"
#include "diag/perf.h"
#include "session/session_items.h"
#include "ui/wndmgr/make_window_internal.h"

#define WID_PREVIEW_EQUIP 0x126
#define MSG_PREVIEW_ITEM 0x17
#define ID_RESET 0x163
#define ID_TURN_LEFT 0x178
#define ID_TURN_RIGHT 0x179
#define MSG_TITLE 0xb90
#define MSG_RESET 0xb9f
#define PREVIEW_DIR UI_SKIN "equip_preview\\"

#define LOC_HEAD_LOW 0x1
#define LOC_GARMENT 0x4
#define LOC_HEAD_TOP 0x100
#define LOC_HEAD_MID 0x200
#define LOC_COSTUME_TOP 0x400
#define LOC_COSTUME_MID 0x800
#define LOC_COSTUME_LOW 0x1000
#define LOC_COSTUME_GARMENT 0x2000
#define SLOT_HEAD_LOW 0
#define SLOT_HEAD_TOP 8
#define SLOT_HEAD_MID 9

#define HG_LOW(chr) ((chr)->accessory_top)
#define HG_TOP(chr) ((chr)->accessory_mid)
#define HG_MID(chr) ((chr)->accessory_bottom)
#define HG_LOW_LOC(chr) ((chr)->head_low_location)
#define HG_TOP_LOC(chr) ((chr)->head_top_location)
#define HG_MID_LOC(chr) ((chr)->head_mid_location)

struct UIPreviewEquipWnd {
    struct UIFrameWnd frame;
    POINT *info;
    struct CharacterRender *chr;
    struct UIBitmapButton *turn_left;
    struct UIBitmapButton *turn_right;
    struct UIBasicButton *reset;
};

struct UIWindow;
void ui_preview_equip_wnd_destroy(struct UIWindow *window);
void ui_preview_equip_wnd_store_info(struct UIWindow *self);
void ui_preview_equip_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_preview_equip_wnd_on_process(struct UIWindow *self);
void ui_preview_equip_wnd_on_draw(struct UIWindow *self);
intptr_t ui_preview_equip_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4);
const struct UIWindowOps ui_preview_equip_wnd_ops = {
    .destroy = ui_preview_equip_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_preview_equip_wnd_store_info,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_preview_equip_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_preview_equip_wnd_on_process,
    .on_draw = ui_preview_equip_wnd_on_draw,
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
    .send_msg = ui_preview_equip_wnd_send_msg,
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

void ui_preview_equip_wnd_reset(struct UIPreviewEquipWnd *self);
void ui_preview_equip_wnd_apply_worn_locations(struct UIPreviewEquipWnd *self);

struct UIPreviewEquipWnd *ui_preview_equip_wnd_construct(struct UIPreviewEquipWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 1);
    self->frame.base.ops = &ui_preview_equip_wnd_ops;
    self->info = NULL;
    self->chr = NULL;
    self->turn_left = NULL;
    self->turn_right = NULL;
    self->reset = NULL;
    return self;
}

struct UIWindow *ui_preview_equip_wnd_new(void)
{
    return (struct UIWindow *)ui_preview_equip_wnd_construct(mem_alloc(sizeof(struct UIPreviewEquipWnd)));
}

void ui_preview_equip_wnd_destroy(struct UIWindow *window)
{
    struct UIPreviewEquipWnd *self = (struct UIPreviewEquipWnd *)window;
    self->frame.base.ops = &ui_preview_equip_wnd_ops;
    if (self->chr)
        character_render_destroy(self->chr);
    ui_frame_wnd_destruct(&self->frame);
    mem_free(self);
}

static void add_title(struct UIPreviewEquipWnd *self, int x, int y, int white)
{
    struct UIWindow *wnd = &self->frame.base;
    struct UIWindow *title = mem_alloc(sizeof(struct UIStaticText));
    int w, h;

    if (title)
        title = ui_static_text_construct(title);
    h = ui_window_text_height(wnd, msg_str(MSG_TITLE), 0, 0, 0xc, 0, 0);
    w = ui_window_text_width(wnd, msg_str(MSG_TITLE), 0, 0, 0xc, 0, 0);
    ui_window_create(title, w, h);
    ui_static_text_set_text(title, msg_str(MSG_TITLE), 0);
    if (white)
        ui_static_text_set_text_colour(title, 0xff, 0xff, 0xff);
    ui_window_move(title, x, y);
    ui_window_add_child(wnd, title);
}

static struct UIBitmapButton *add_arrow(struct UIPreviewEquipWnd *self, const char *out, const char *over,
                                        const char *press)
{
    struct UIBitmapButton *button = mem_alloc(sizeof(struct UIBitmapButton));

    if (button)
        button = ui_bitmap_button_construct(button);
    ui_bitmap_button_set_bitmap(button, out, 0, 0);
    ui_bitmap_button_set_bitmap(button, over, 1, 0);
    ui_bitmap_button_set_bitmap(button, press, 2, 0);
    ui_window_create(&button->button.base, button->bitmap_width, button->bitmap_height);
    return button;
}

static void add_buttons(struct UIPreviewEquipWnd *self, int dead_twin)
{
    struct UIWindow *wnd = &self->frame.base;
    struct UIBasicButton *reset;
    int x;

    self->turn_left = add_arrow(self, PREVIEW_DIR "chr_arrow_rotate_l_out.bmp", PREVIEW_DIR "chr_arrow_rotate_l_over.bmp",
                                PREVIEW_DIR "chr_arrow_rotate_l_press.bmp");
    ui_window_move_base(&self->turn_left->button.base, 0xd, 0x97);
    self->turn_left->button.base.id = ID_TURN_LEFT;
    ui_window_add_child(wnd, &self->turn_left->button.base);

    self->turn_right = add_arrow(self, PREVIEW_DIR "chr_arrow_rotate_r_out.bmp", PREVIEW_DIR "chr_arrow_rotate_r_over.bmp",
                                 PREVIEW_DIR "chr_arrow_rotate_r_press.bmp");
    ui_window_move_base(&self->turn_right->button.base, wnd->width - self->turn_right->bitmap_width - 0xd, 0x97);
    self->turn_right->button.base.id = ID_TURN_RIGHT;
    ui_window_add_child(wnd, &self->turn_right->button.base);

    reset = mem_alloc(sizeof(struct UIBasicButton));
    if (reset)
        reset = ui_basic_button_construct(reset);
    self->reset = reset;
    ui_basic_button_set_label(&self->reset->button.base, 0x50, msg_str(MSG_RESET));
    x = (int)((double)(wnd->width - (dead_twin ? 0x50 : self->reset->button.base.width)) * 0.5);
    ui_window_move_base(&self->reset->button.base, x, wnd->height - 0x1e);
    self->reset->button.base.id = ID_RESET;
    self->reset->redraw_parent = 1;
    ui_window_add_child(wnd, &self->reset->button.base);
}

static void draw_character(struct UIPreviewEquipWnd *self)
{
    struct CharacterRender *chr = self->chr;
    float m[12], eye[3], up[3], sx = 0, sy = 0, rhw = 0, nd;

    if (!chr)
        return;
    mat_set_rotate_x(m, chr->pitch / 180.0f * 3.141592f);
    mat_rotate_y(m, self->chr->yaw / 180.0f * 3.141592f);
    chr = self->chr;
    nd = -chr->distance;
    eye[0] = m[3] * 0.0f + m[0] * 0.0f + nd * m[6] + m[9];
    eye[1] = m[4] * 0.0f + m[1] * 0.0f + nd * m[7] + m[10];
    eye[2] = m[5] * 0.0f + m[2] * 0.0f + nd * m[8] + m[11];
    up[0] = chr->up[0];
    up[1] = chr->up[1];
    up[2] = chr->up[2];
    {
        Vec3 eye_point = {eye[0], eye[1], eye[2]};
        Vec3 target_point = {chr->pos[0], chr->pos[1], chr->pos[2]};
        Vec3 up_direction = {up[0], up[1], up[2]};
        mat_look_at(m, &eye_point, &target_point, &up_direction);
    }
    chr = self->chr;
    {
        Vec3 world_point = {chr->pos[0], chr->pos[1], chr->pos[2]};
        renderer_project_centred(g_renderer, &world_point, m, &sx, &sy, &rhw,
                            (float)self->frame.base.width + chr->centre_dx,
                            (float)self->frame.base.height + chr->centre_dy);
    }
    chr = self->chr;
    struct CharacterDrawOptions options = {
        .x = (int)sx,
        .y = (int)sy,
        .sex = (BYTE)chr->sex,
        .job = (short)chr->job,
        .body = (short)chr->body_style,
        .head = (short)chr->head,
        .accessory = (short)chr->accessory_top,
        .accessory2 = (short)chr->accessory_mid,
        .accessory3 = (short)chr->accessory_bottom,
        .robe = chr->robe,
        .riding = chr->riding,
        .body_palette = chr->body_palette,
        .head_palette = chr->head_palette,
        .action = chr->action,
        .motion = chr->motion,
        .start_time = 0,
    };
    ui_draw_character(&self->frame.base, &options, 1);
}

void ui_preview_equip_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UIPreviewEquipWnd *self = (struct UIPreviewEquipWnd *)window;
    struct UIWindow *wnd = &self->frame.base;

    ui_window_add_sys_button(wnd);
    ui_window_add_close_button(wnd, "", 0);
    add_title(self, 0x13, 4, 1);
    add_title(self, 0x12, 3, 0);
    if (!game_current_player())
        return;
    self->chr = character_render_create(width, height);
    if (!self->chr)
        return;
    ui_preview_equip_wnd_reset(self);
    add_buttons(self, 0);
}

void ui_preview_equip_wnd_on_draw(struct UIWindow *window)
{
    struct UIPreviewEquipWnd *self = (struct UIPreviewEquipWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct String name;

    ui_window_reset_draws(wnd, wnd->width, wnd->height, 0);
    string_init(&name);
    string_assign(&name, PREVIEW_DIR "bg_mounting2.bmp", 0x2d);
    ui_window_queue_bitmap(wnd, 0, 0, name, 0xffffffff, 0, 0);
    draw_character(self);
}

void ui_preview_equip_wnd_on_process(struct UIWindow *window)
{
    struct UIPreviewEquipWnd *self = (struct UIPreviewEquipWnd *)window;
    ui_window_invalidate_base(&self->frame.base);
}

void ui_preview_equip_wnd_reset(struct UIPreviewEquipWnd *self)
{
    struct PlayerAppearance appearance;

    if (!self->chr)
        return;
    self->chr->shadow = 0;
    if (!player_get_appearance(game_current_player(), &appearance))
        return;
    self->chr->job = appearance.job;
    self->chr->job2 = appearance.look_job;
    self->chr->sex = appearance.sex;
    self->chr->head = appearance.head;
    self->chr->body_palette = appearance.body_palette;
    self->chr->head_palette = appearance.head_palette;
    HG_LOW(self->chr) = appearance.accessory[0];
    HG_TOP(self->chr) = appearance.accessory[1];
    HG_MID(self->chr) = appearance.accessory[2];
    character_render_set_robe(self->chr, appearance.robe);
    self->chr->riding = appearance.mounted;
    self->chr->body_style = appearance.body_style;
    self->chr->centre_dx = -5.0f;
    self->chr->centre_dy = 136.0f;
    ui_preview_equip_wnd_apply_worn_locations(self);
    ui_window_invalidate_base(&self->frame.base);
}

static void try_on(struct UIPreviewEquipWnd *self, const struct ItemInfo *item)
{
    struct CharacterRender *chr = self->chr;
    int id = item->view;

    if (!chr)
        return;
    switch (item->equip_location) {
    case LOC_HEAD_TOP | LOC_HEAD_LOW:
    case LOC_COSTUME_TOP | LOC_COSTUME_LOW:
        HG_TOP(chr) = id;
        HG_LOW(self->chr) = 0;
        HG_TOP_LOC(self->chr) = 0x101;
        HG_LOW_LOC(self->chr) = 0x101;
        if (HG_MID_LOC(self->chr) != LOC_HEAD_MID) {
            HG_MID_LOC(self->chr) = 0;
            HG_MID(self->chr) = 0;
        }
        break;
    case LOC_HEAD_TOP | LOC_HEAD_MID | LOC_HEAD_LOW:
    case LOC_COSTUME_TOP | LOC_COSTUME_MID | LOC_COSTUME_LOW:
        HG_LOW(chr) = id;
        HG_TOP(self->chr) = 0;
        HG_MID(self->chr) = 0;
        HG_LOW_LOC(self->chr) = 0x301;
        HG_TOP_LOC(self->chr) = 0x301;
        HG_MID_LOC(self->chr) = 0x301;
        break;
    case LOC_HEAD_TOP:
    case LOC_COSTUME_TOP:
        HG_TOP(chr) = id;
        HG_TOP_LOC(self->chr) = LOC_HEAD_TOP;
        if (HG_LOW_LOC(self->chr) & LOC_HEAD_TOP) {
            HG_LOW_LOC(self->chr) = 0;
            HG_LOW(self->chr) = 0;
        }
        if (HG_MID_LOC(self->chr) & LOC_HEAD_TOP) {
            HG_MID_LOC(self->chr) = 0;
            HG_MID(self->chr) = 0;
        }
        break;
    case LOC_HEAD_LOW:
    case LOC_COSTUME_LOW:
        HG_LOW(chr) = id;
        HG_LOW_LOC(self->chr) = LOC_HEAD_LOW;
        if (HG_TOP_LOC(self->chr) & LOC_HEAD_LOW) {
            HG_TOP_LOC(self->chr) = 0;
            HG_TOP(self->chr) = 0;
        }
        if (HG_MID_LOC(self->chr) & LOC_HEAD_LOW) {
            HG_MID_LOC(self->chr) = 0;
            HG_MID(self->chr) = 0;
        }
        break;
    case LOC_HEAD_TOP | LOC_HEAD_MID:
    case LOC_COSTUME_TOP | LOC_COSTUME_MID:
        HG_TOP(chr) = id;
        HG_MID(self->chr) = 0;
        HG_TOP_LOC(self->chr) = 0x300;
        HG_MID_LOC(self->chr) = 0x300;
        if (HG_LOW_LOC(self->chr) != LOC_HEAD_LOW) {
            HG_LOW_LOC(self->chr) = 0;
            HG_LOW(self->chr) = 0;
        }
        break;
    case LOC_HEAD_MID:
    case LOC_COSTUME_MID:
        HG_MID(chr) = id;
        HG_MID_LOC(self->chr) = LOC_HEAD_MID;
        if (HG_LOW_LOC(self->chr) & LOC_HEAD_MID) {
            HG_LOW_LOC(self->chr) = 0;
            HG_LOW(self->chr) = 0;
        }
        if (HG_TOP_LOC(self->chr) & LOC_HEAD_MID) {
            HG_TOP_LOC(self->chr) = 0;
            HG_TOP(self->chr) = 0;
        }
        break;
    case LOC_HEAD_MID | LOC_HEAD_LOW:
    case LOC_COSTUME_MID | LOC_COSTUME_LOW:
        HG_MID(chr) = id;
        HG_LOW(self->chr) = 0;
        HG_MID_LOC(self->chr) = 0x201;
        HG_LOW_LOC(self->chr) = 0x201;
        if (HG_TOP_LOC(self->chr) != LOC_HEAD_TOP) {
            HG_TOP_LOC(self->chr) = 0;
            HG_TOP(self->chr) = 0;
        }
        break;
    case LOC_GARMENT:
    case LOC_COSTUME_GARMENT:
        character_render_set_robe(self->chr, id);
        break;
    }
}

intptr_t ui_preview_equip_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, intptr_t val1,
                                         intptr_t val2, intptr_t val3, intptr_t val4)
{
    struct UIPreviewEquipWnd *self = (struct UIPreviewEquipWnd *)window;
    if (msg == UI_MSG_COMMAND) {
        switch (val1) {
        case UI_ID_CLOSE:
            ui_window_mgr_post_quit1(&g_windowMgr, WID_PREVIEW_EQUIP);
            break;
        case ID_RESET:
            ui_preview_equip_wnd_reset(self);
            break;
        case ID_TURN_LEFT:
            if (!self->chr)
                break;
            self->chr->action++;
            if (self->chr->action > 7)
                self->chr->action = 0;
            ui_window_invalidate_base(&self->frame.base);
            break;
        case ID_TURN_RIGHT:
            if (!self->chr)
                break;
            self->chr->action--;
            if (self->chr->action < 0)
                self->chr->action = 7;
            ui_window_invalidate_base(&self->frame.base);
            break;
        }
    } else if (msg == MSG_PREVIEW_ITEM) {
        if (val1)
            try_on(self, (const struct ItemInfo *)val1);
    } else if (msg == UI_MSG_SET_INFO) {
        self->info = (POINT *)val1;
        if (!preview_equip_info_is_on_screen(self->info))
            preview_equip_info_set_default(self->info);
        ui_frame_wnd_move(&self->frame.base, self->info->x, self->info->y);
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
}

void ui_preview_equip_wnd_apply_worn_locations(struct UIPreviewEquipWnd *self)
{
    struct ItemInfo worn, costume;

    if (!self->chr)
        return;
    if (HG_LOW(self->chr)) {
        session_get_equipped_item(&g_session1, &worn, SLOT_HEAD_LOW);
        if (HG_LOW(self->chr) == worn.view) {
            HG_LOW_LOC(self->chr) = LOC_HEAD_LOW;
            if (worn.equip_location == 0x301) {
                HG_LOW_LOC(self->chr) = 0x301;
                HG_TOP_LOC(self->chr) = 0x301;
                HG_MID_LOC(self->chr) = 0x301;
            }
        }
        session_get_worn_costume(&g_session1, &costume, SLOT_HEAD_LOW);
        if (HG_LOW(self->chr) == costume.view) {
            HG_LOW_LOC(self->chr) = LOC_HEAD_LOW;
            if (costume.equip_location == 0x1c00) {
                HG_LOW_LOC(self->chr) = 0x301;
                HG_TOP_LOC(self->chr) = 0x301;
                HG_MID_LOC(self->chr) = 0x301;
            }
        }
        item_info_destruct(&costume);
        item_info_destruct(&worn);
    }

    if (HG_TOP(self->chr)) {
        session_get_equipped_item(&g_session1, &worn, SLOT_HEAD_TOP);
        if (HG_TOP(self->chr) == worn.view) {
            int loc = worn.equip_location;

            if (loc == 0x100)
                HG_TOP_LOC(self->chr) = loc;
            if (loc == 0x101) {
                HG_TOP_LOC(self->chr) = 0x101;
                HG_LOW_LOC(self->chr) = 0x101;
            }
            if (loc == 0x300) {
                HG_TOP_LOC(self->chr) = 0x300;
                HG_MID_LOC(self->chr) = 0x300;
            }
            if (loc == 0x301) {
                HG_LOW_LOC(self->chr) = 0x301;
                HG_TOP_LOC(self->chr) = 0x301;
                HG_MID_LOC(self->chr) = 0x301;
            }
        }
        session_get_worn_costume(&g_session1, &costume, SLOT_HEAD_TOP);
        if (HG_TOP(self->chr) == costume.view) {
            int loc = costume.equip_location;

            if (loc == 0x400)
                HG_TOP_LOC(self->chr) = 0x100;
            if (loc == 0x1400) {
                HG_TOP_LOC(self->chr) = 0x101;
                HG_LOW_LOC(self->chr) = 0x101;
            }
            if (loc == 0xc00) {
                HG_TOP_LOC(self->chr) = 0x300;
                HG_MID_LOC(self->chr) = 0x300;
            }
            if (loc == 0x1c00) {
                HG_LOW_LOC(self->chr) = 0x301;
                HG_TOP_LOC(self->chr) = 0x301;
                HG_MID_LOC(self->chr) = 0x301;
            }
        }
        item_info_destruct(&costume);
        item_info_destruct(&worn);
    }

    if (HG_MID(self->chr)) {
        session_get_equipped_item(&g_session1, &worn, SLOT_HEAD_MID);
        if (HG_MID(self->chr) == worn.view) {
            int loc = worn.equip_location;

            if (loc == 0x200)
                HG_MID_LOC(self->chr) = loc;
            if (loc == 0x201) {
                HG_MID_LOC(self->chr) = 0x201;
                HG_LOW_LOC(self->chr) = 0x201;
            }
        }
        session_get_worn_costume(&g_session1, &costume, SLOT_HEAD_MID);
        if (HG_MID(self->chr) == costume.view) {
            int loc = costume.equip_location;

            if (loc == 0x800)
                HG_MID_LOC(self->chr) = 0x200;
            if (loc == 0x1800) {
                HG_MID_LOC(self->chr) = 0x201;
                HG_LOW_LOC(self->chr) = 0x201;
            }
        }
        item_info_destruct(&costume);
        item_info_destruct(&worn);
    }
}

void ui_preview_equip_wnd_store_info(struct UIWindow *window)
{
    struct UIPreviewEquipWnd *self = (struct UIPreviewEquipWnd *)window;
    if (!self->info)
        return;
    self->info->x = self->frame.base.x;
    self->info->y = self->frame.base.y;
}
