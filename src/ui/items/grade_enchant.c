#include "game/drag.h"
#include "ui/controls/buttons.h"
#include "ui/items/refine_controls.h"
#include "game/drag_info.h"
#include "render/rp_pool.h"
#include "iteminfo/item_api.h"
#include "session/state.h"
#include "core/object_type_tables.h"
#include "ui/window_access.h"
#include "game/context.h"
#include "contents/connection_api.h"
#include "ui/window_mgr.h"
#include "ui/game.h"
#include "ui/controls/controls.h"
#include "ui/character/char_info.h"
#include "ui/runtime.h"
#include "ui/ui_std.h"
#include <stdlib.h>
#include <string.h>
#include "ui/ui_items.h"
#include "ui/draw.h"
#include "core/msg_str.h"
#include "core/text_util.h"
#include "diag/perf.h"
#include "session/session_items.h"
#include "session/session_state.h"
#include "platform/clock.h"

#define GRADE_DIR UI_SKIN "grade_enchant\\"

#define WID_GRADE_ENCHANT 0x157
#define WID_ITEM_INFO 0xc
#define MSG_ITEM_INFO 0x18
#define MSG_ITEM_DROPPED 0x17
#define ID_START 0xb0
#define ID_CLOSE 0xb1
#define ID_CLOSE2 0xc1
#define ID_CHANCE_UP 0x10f
#define ID_CHANCE_DOWN 0x110
#define UI_MSGBOX_ANSWER_NO 0xb4

#define MATERIAL_ROWS 3
#define MATERIAL_LEFT 0x9e
#define MATERIAL_TOP 0x66
#define MATERIAL_PITCH 0x28

#define COLOUR_CAUTION 0x0a0adc
#define COLOUR_OK 0x5aeb00

#define MSG_NO_CHANCE 0xeed
#define MSG_NOT_ALLOWED 0xeee
#define MSG_PICK_MATERIAL 0xe7b
#define MSG_LACK_MATERIAL 0xe7c
#define MSG_LACK_ZENY 0xe7d
#define MSG_OVERWEIGHT 0xe7e
#define MSG_OK 0xe7f
#define MSG_MAY_BREAK 0xe80
#define MSG_DOWNGRADE 0xe81
#define MSG_RESULT_FIRST 0xe76
#define MSG_IDLE_FIRST 0xf1e
#define MSG_NAME_COUNT 0xde1
#define MSG_CONFIRM_START 0xeef
#define MSG_BLESSING_LOST 0xef7

#define CZ_GRADE_ENCHANT_SELECT 0xb59
#define CZ_GRADE_ENCHANT_START 0xb5b
#define CZ_GRADE_ENCHANT_CLOSE 0xb5c

#pragma pack(push, 1)
struct GradeMaterial {
    int item_id;
    int count;
    int zeny;
    int downgrade;
    BYTE allowed;
};
struct PacketGradeEnchantSelect {
    unsigned short id;
    short index;
};
struct PacketGradeEnchantStart {
    unsigned short id;
    short index;
    int material;
    BYTE blessing;
    int blessing_count;
    BYTE protect;
};
#pragma pack(pop)
SIZE_ASSERT(GradeMaterial, 17);
SIZE_ASSERT(PacketGradeEnchantStart, 0xe);

struct UIGradeEnchantWnd {
    struct UIFrameWnd frame;
    struct UIBitmapButton *btn_start;
    struct UIBitmapButton *btn_up;
    struct UIBitmapButton *btn_down;
    int state;
    int result;
    DWORD frame_delay;
    DWORD wheel_delay;
    DWORD frame_time;
    DWORD wheel_time;
    int anim_frame;
    int wheel;
    DWORD result_time;
    int selected;
    int hover;
    struct Vec materials;
    struct GradeBlessing blessing;
    int protect_item_id;
    int protect_amount;
    BYTE protect;
    int blessing_used;
    int success_chance;
    struct Vec cautions;
    struct ItemInfo item;
    BYTE waiting;
};

#define MATERIAL(self, i) (((struct GradeMaterial *)(self)->materials.first) + (i))
#define MATERIAL_COUNT(self) \
    ((unsigned)(((BYTE *)(self)->materials.last - (BYTE *)(self)->materials.first) / 17))

struct UIWindow;
void ui_grade_enchant_wnd_destroy(struct UIWindow *window);
void ui_grade_enchant_wnd_drag_and_drop(struct UIWindow *window, int x, int y, const struct DragInfo *drag);
void ui_grade_enchant_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_grade_enchant_wnd_on_process(struct UIWindow *self);
void ui_grade_enchant_wnd_on_draw(struct UIWindow *self);
void ui_grade_enchant_wnd_on_l_btn_down(struct UIWindow *self, int x, int y);
void ui_grade_enchant_wnd_on_l_btn_dbl_clk(struct UIWindow *self, int x, int y);
void ui_grade_enchant_wnd_on_mouse_hover(struct UIWindow *self, int x, int y);
void ui_grade_enchant_wnd_on_l_btn_up(struct UIWindow *self, int x, int y);
void ui_grade_enchant_wnd_on_r_btn_down(struct UIWindow *self, int x, int y);
void ui_grade_enchant_wnd_on_r_btn_up(struct UIWindow *self, int x, int y);
int ui_grade_enchant_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_grade_enchant_wnd_ops = {
    .destroy = ui_grade_enchant_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_grade_enchant_wnd_drag_and_drop,
    .store_info = ui_window_store_info_base,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_grade_enchant_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_grade_enchant_wnd_on_process,
    .on_draw = ui_grade_enchant_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_grade_enchant_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_grade_enchant_wnd_on_l_btn_dbl_clk,
    .on_mouse_move = ui_frame_wnd_on_mouse_move,
    .on_mouse_hover = ui_grade_enchant_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_grade_enchant_wnd_on_l_btn_up,
    .on_r_btn_down = ui_grade_enchant_wnd_on_r_btn_down,
    .on_r_btn_up = ui_grade_enchant_wnd_on_r_btn_up,
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_grade_enchant_wnd_send_msg,
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
struct UIWindow;
const struct UIButtonOps ui_grade_enchant_btn_start_ops = {
    .base = {
        .destroy = ui_grade_enchant_btn_start_destroy,
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
        .on_create = ui_window_on_create_base,
        .on_destroy = ui_window_on_destroy_base,
        .on_process = ui_window_on_process_base,
        .on_draw = ui_grade_enchant_btn_start_on_draw,
        .on_focus = ui_window_on_focus_base,
        .on_focus_lost = ui_window_on_focus_lost_base,
        .on_l_btn_down = ui_button_on_l_btn_down,
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
        .send_msg = ui_window_send_msg_base,
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
    .set_show = ui_refine_button_set_show,
};
const struct UIButtonOps ui_grade_enchant_btn_enhanced_up_ops = {
    .base = {
        .destroy = ui_grade_enchant_btn_enhanced_up_destroy,
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
        .on_create = ui_window_on_create_base,
        .on_destroy = ui_window_on_destroy_base,
        .on_process = ui_window_on_process_base,
        .on_draw = ui_grade_enchant_btn_enhanced_up_on_draw,
        .on_focus = ui_window_on_focus_base,
        .on_focus_lost = ui_window_on_focus_lost_base,
        .on_l_btn_down = ui_button_on_l_btn_down,
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
        .send_msg = ui_window_send_msg_base,
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
    .set_show = ui_refine_button_set_show,
};
const struct UIButtonOps ui_grade_enchant_btn_enhanced_down_ops = {
    .base = {
        .destroy = ui_grade_enchant_btn_enhanced_down_destroy,
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
        .on_create = ui_window_on_create_base,
        .on_destroy = ui_window_on_destroy_base,
        .on_process = ui_window_on_process_base,
        .on_draw = ui_grade_enchant_btn_enhanced_down_on_draw,
        .on_focus = ui_window_on_focus_base,
        .on_focus_lost = ui_window_on_focus_lost_base,
        .on_l_btn_down = ui_button_on_l_btn_down,
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
        .send_msg = ui_window_send_msg_base,
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
    .set_show = ui_refine_button_set_show,
};

int ui_grade_enchant_wnd_selected_zeny(struct UIGradeEnchantWnd *self);
int ui_grade_enchant_wnd_material_at(int x, int y);
int ui_grade_enchant_wnd_success_chance(struct UIGradeEnchantWnd *self);
void ui_grade_enchant_wnd_build_cautions(struct UIGradeEnchantWnd *self);
void ui_grade_enchant_wnd_show_result(struct UIGradeEnchantWnd *self);
void ui_grade_enchant_btn_start_init(struct UIBitmapButton *self);
void ui_grade_enchant_btn_enhanced_up_init(struct UIBitmapButton *self);
void ui_grade_enchant_btn_enhanced_down_init(struct UIBitmapButton *self);

static struct BitmapRes *bitmap(const char *path)
{
    return (struct BitmapRes *)res_mgr_get(g_resMgr(), path);
}

static void text_out(struct UIGradeEnchantWnd *self, int x, int y, const char *text, int width, int align,
                     COLORREF colour, int font_face, int font_height, int font_arg, BYTE flag)
{
    struct String s;

    string_init(&s);
    string_assign_cstr(&s, text);
    ui_window_draw_text_aligned(&self->frame.base, x, y, s, width, align, colour, 0, font_face, font_height, font_arg, flag);
}

static int inventory_count(int item_id)
{
    struct ItemInfo tmp;
    int count = session_get_inventory_item_by_id(&g_session1, &tmp, item_id, 0)->count;

    item_info_destruct(&tmp);
    return count;
}

static void show_item_info(const struct ItemInfo *info)
{
    struct UIWindow *wnd = ui_window_open(WID_ITEM_INFO);

    if (wnd)
        ui_window_send_msg(wnd, NULL, MSG_ITEM_INFO, (int)info, 0, 0, 0);
}

struct UIBitmapButton *ui_grade_enchant_btn_start_construct(struct UIBitmapButton *self)
{
    ui_bitmap_button_construct(self);
    self->button.base.ops = &ui_grade_enchant_btn_start_ops.base;
    return self;
}

struct UIBitmapButton *ui_grade_enchant_btn_enhanced_up_construct(struct UIBitmapButton *self)
{
    ui_bitmap_button_construct(self);
    self->button.base.ops = &ui_grade_enchant_btn_enhanced_up_ops.base;
    return self;
}

struct UIBitmapButton *ui_grade_enchant_btn_enhanced_down_construct(struct UIBitmapButton *self)
{
    ui_bitmap_button_construct(self);
    self->button.base.ops = &ui_grade_enchant_btn_enhanced_down_ops.base;
    return self;
}

void ui_grade_enchant_btn_start_init(struct UIBitmapButton *self)
{
    ui_bitmap_button_set_bitmap(self, GRADE_DIR "btn_start.bmp", 0, 0);
    ui_bitmap_button_set_bitmap(self, GRADE_DIR "btn_start_over.bmp", 1, 0);
    ui_bitmap_button_set_bitmap(self, GRADE_DIR "btn_start_press.bmp", 2, 0);
    ui_bitmap_button_set_bitmap(self, GRADE_DIR "btn_start_disable.bmp", 3, 0);
    ui_window_resize_base(&self->button.base, self->bitmap_width, self->bitmap_height);
}

void ui_grade_enchant_btn_enhanced_up_init(struct UIBitmapButton *self)
{
    ui_bitmap_button_set_bitmap(self, GRADE_DIR "btn_chance_up.bmp", 0, 0);
    ui_bitmap_button_set_bitmap(self, GRADE_DIR "btn_chance_up_over.bmp", 1, 0);
    ui_bitmap_button_set_bitmap(self, GRADE_DIR "btn_chance_up_pick.bmp", 2, 0);
    ui_window_resize_base(&self->button.base, self->bitmap_width, self->bitmap_height);
}

void ui_grade_enchant_btn_enhanced_down_init(struct UIBitmapButton *self)
{
    ui_bitmap_button_set_bitmap(self, GRADE_DIR "btn_chance_down.bmp", 0, 0);
    ui_bitmap_button_set_bitmap(self, GRADE_DIR "btn_chance_down_over.bmp", 1, 0);
    ui_bitmap_button_set_bitmap(self, GRADE_DIR "btn_chance_down_pick.bmp", 2, 0);
    ui_window_resize_base(&self->button.base, self->bitmap_width, self->bitmap_height);
}

struct UIGradeEnchantWnd *ui_grade_enchant_wnd_construct(struct UIGradeEnchantWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_grade_enchant_wnd_ops;
    self->btn_start = NULL;
    self->btn_up = NULL;
    self->btn_down = NULL;
    self->state = 0;
    self->result = -1;
    self->frame_delay = 200;
    self->wheel_delay = 100;
    self->frame_time = 0;
    self->wheel_time = 0;
    self->anim_frame = 1;
    self->wheel = 1;
    self->result_time = 0;
    self->selected = -1;
    self->hover = -1;
    self->materials.first = self->materials.last = self->materials.end = NULL;
    self->protect = 0;
    self->blessing_used = 0;
    self->success_chance = 0;
    self->cautions.first = self->cautions.last = self->cautions.end = NULL;
    item_info_construct_with_arg(&self->item);
    self->waiting = 0;
    memset(&self->blessing, 0, sizeof self->blessing);
    self->protect_item_id = 0;
    self->protect_amount = 0;
    return self;
}

struct UIWindow *ui_grade_enchant_wnd_new(void)
{
    return (struct UIWindow *)ui_grade_enchant_wnd_construct(mem_alloc(sizeof(struct UIGradeEnchantWnd)));
}

void ui_grade_enchant_wnd_destruct(struct UIGradeEnchantWnd *self)
{
    self->frame.base.ops = &ui_grade_enchant_wnd_ops;
    item_info_destruct(&self->item);
    vec_free(&self->cautions);
    vec_free(&self->materials);
    ui_frame_wnd_destruct(&self->frame);
}

void ui_grade_enchant_wnd_destroy(struct UIWindow *window)
{
    struct UIGradeEnchantWnd *self = (struct UIGradeEnchantWnd *)window;
    ui_grade_enchant_wnd_destruct(self);
    mem_free(self);
}

void ui_grade_enchant_wnd_reset(struct UIGradeEnchantWnd *self)
{
    struct ItemInfo empty;

    self->waiting = 0;
    self->anim_frame = 1;
    self->wheel = 1;
    item_info_assign(&self->item, item_info_construct_with_arg(&empty));
    item_info_destruct(&empty);
    self->selected = -1;
    self->materials.last = self->materials.first;
    memset(&self->blessing, 0, sizeof self->blessing);
    self->protect_item_id = 0;
    self->protect_amount = 0;
    self->blessing_used = 0;
    self->protect = 0;
    self->success_chance = 0;
    self->result_time = 0;
    self->state = 0;
    self->result = -1;
    self->frame_time = platform_ticks_ms();
    self->anim_frame = 1;
    ui_window_invalidate_base(&self->frame.base);
    ui_window_invalidate_base(&self->frame.base);
}

int ui_grade_enchant_wnd_selected_zeny(struct UIGradeEnchantWnd *self)
{
    unsigned count;

    if (!self->item.index)
        return 0;
    count = MATERIAL_COUNT(self);
    if (!count || self->selected < 0 || count <= (unsigned)self->selected)
        return 0;
    return MATERIAL(self, self->selected)->zeny;
}

int ui_grade_enchant_wnd_material_at(int x, int y)
{
    int row;

    if ((unsigned)(x - MATERIAL_LEFT) > 0xeb || (unsigned)(y - MATERIAL_TOP) > 0x75)
        return -1;
    row = (y - MATERIAL_TOP) / MATERIAL_PITCH;
    if (y > row * MATERIAL_PITCH + 0x8b || row >= MATERIAL_ROWS)
        return -1;
    return row;
}

int ui_grade_enchant_wnd_success_chance(struct UIGradeEnchantWnd *self)
{
    int chance;

    if (!self->item.index)
        return 0;
    chance = self->success_chance;
    if (self->blessing_used > 0)
        chance += self->blessing_used / self->blessing.step * self->blessing.bonus;
    return chance > 100 ? 100 : chance;
}

void ui_grade_enchant_wnd_build_cautions(struct UIGradeEnchantWnd *self)
{
    struct Vec *lines = &self->cautions;
    const struct GradeMaterial *mat;
    int id;

    lines->last = lines->first;
    if (self->state != 1)
        return;
    if (!self->success_chance) {
        id = MSG_NO_CHANCE;
        vec_push(lines, &id, sizeof id);
        return;
    }
    if (self->selected != -1 && !MATERIAL(self, self->selected)->allowed) {
        id = MSG_NOT_ALLOWED;
        vec_push(lines, &id, sizeof id);
    }
    if (self->selected == -1) {
        id = MSG_PICK_MATERIAL;
        vec_push(lines, &id, sizeof id);
    }
    if (self->selected != -1) {
        mat = MATERIAL(self, self->selected);
        if (inventory_count(mat->item_id) < MATERIAL(self, self->selected)->count) {
            id = MSG_LACK_MATERIAL;
            vec_push(lines, &id, sizeof id);
        }
    }
    if (g_session1.zeny < ui_grade_enchant_wnd_selected_zeny(self)) {
        id = MSG_LACK_ZENY;
        vec_push(lines, &id, sizeof id);
    }
    if ((float)g_session1.weight > (float)g_session1.maxWeight * 0.7f ||
        session_get_inventory_count(&g_session1) + session_count_worn_equips(&g_session1) +
                session_count_worn_costumes(&g_session1) >= g_session1.inventoryExtension + 100) {
        id = MSG_OVERWEIGHT;
        vec_push(lines, &id, sizeof id);
    }
    if (self->selected != -1 && MATERIAL(self, self->selected)->allowed == 1) {
        id = ui_grade_enchant_wnd_success_chance(self) < 100 ? MSG_MAY_BREAK : MSG_NOT_ALLOWED;
        vec_push(lines, &id, sizeof id);
    }
    if (self->selected != -1 && MATERIAL(self, self->selected)->downgrade > 0) {
        id = MSG_DOWNGRADE;
        vec_push(lines, &id, sizeof id);
    }
}

void ui_grade_enchant_wnd_show_result(struct UIGradeEnchantWnd *self)
{
    if ((unsigned)self->result > 4)
        return;
    ui_window_mgr_send_msg_(&g_windowMgr, 1, (int)msg_str(MSG_RESULT_FIRST + self->result), 0xffff00, 0, 0);
}

void ui_grade_enchant_wnd_set_materials(struct UIGradeEnchantWnd *self,
                                               unsigned short index, int success_chance, int count,
                                               struct GradeBlessing blessing, int protect_item_id,
                                               int protect_amount, const BYTE *materials)
{
    struct ItemInfo tmp;

    if ((unsigned)self->state > 1 || index == 0xffff)
        return;
    session_get_inventory_item(&g_session1, &tmp, (short)index);
    if (tmp.index) {
        item_info_assign(&self->item, &tmp);
        self->protect_item_id = protect_item_id;
        self->protect_amount = protect_amount;
        self->blessing = blessing;
        vec_assign(&self->materials, materials, materials + count * sizeof(struct GradeMaterial),
                   sizeof(struct GradeMaterial));
        self->selected = -1;
        self->blessing_used = 0;
        self->success_chance = success_chance;
        self->state = 1;
        self->result = -1;
        self->frame_time = platform_ticks_ms();
        self->anim_frame = 1;
        ui_window_invalidate_base(&self->frame.base);
        self->waiting = 0;
    }
    item_info_destruct(&tmp);
}

void grade_enchant_wnd_set_result(struct UIGradeEnchantWnd *self, int state,
                                            int result)
{
    DWORD now;

    self->state = state;
    self->result = result;
    now = platform_ticks_ms();
    self->frame_time = now;
    self->anim_frame = 1;
    if (state == 2)
        self->result_time = now;
    ui_window_invalidate_base(&self->frame.base);
}

void ui_grade_enchant_wnd_drag_and_drop(struct UIWindow *window, int x, int y, const struct DragInfo *drag)
{
    struct UIGradeEnchantWnd *self = (struct UIGradeEnchantWnd *)window;
    if ((unsigned)(x - 0x35) <= 0x2e && (unsigned)(y - 0x72) <= 0x2e && drag->source == 1)
        ui_grade_enchant_wnd_send_msg(&self->frame.base, &self->frame.base, MSG_ITEM_DROPPED, drag->index, 0, 0, 0);
    game_send_message(MODE_MSG_DRAG_END, 0, 0, 0, 0);
}

void ui_grade_enchant_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UIGradeEnchantWnd *self = (struct UIGradeEnchantWnd *)window;
    struct UIWindow *close = ui_window_add_close_button(&self->frame.base, "", 0);
    void *mem;
    struct UIBasicButton *button;
    struct UIBitmapButton *bmp;

    if (close)
        ui_window_move_base(close, 0x180, 0x14);

    mem = mem_alloc(sizeof(struct UIBasicButton));
    button = mem ? ui_basic_button_construct(mem) : NULL;
    ui_basic_button_set_label(&button->button.base, 0x2a, msg_str(0x513));
    ui_window_move_base(&button->button.base, 0x159, 0x12b);
    button->button.base.id = ID_CLOSE;
    ui_window_add_child(&self->frame.base, &button->button.base);

    mem = mem_alloc(sizeof(struct UIBitmapButton));
    bmp = mem ? ui_grade_enchant_btn_start_construct(mem) : NULL;
    self->btn_start = bmp;
    ui_grade_enchant_btn_start_init(bmp);
    ui_window_move_base(&self->btn_start->button.base, 0x23, 0x123);
    self->btn_start->button.base.id = ID_START;
    self->btn_start->button.disabled = 0;
    ui_window_invalidate_base(&self->btn_start->button.base);
    ui_window_add_child(&self->frame.base, &self->btn_start->button.base);

    mem = mem_alloc(sizeof(struct UIBitmapButton));
    bmp = mem ? ui_grade_enchant_btn_enhanced_up_construct(mem) : NULL;
    self->btn_up = bmp;
    ui_grade_enchant_btn_enhanced_up_init(bmp);
    ui_window_move_base(&self->btn_up->button.base, 0x9e, 0xe1);
    self->btn_up->button.base.id = ID_CHANCE_UP;
    self->btn_up->button.disabled = 0;
    ui_window_invalidate_base(&self->btn_up->button.base);
    ui_window_add_child(&self->frame.base, &self->btn_up->button.base);

    mem = mem_alloc(sizeof(struct UIBitmapButton));
    bmp = mem ? ui_grade_enchant_btn_enhanced_down_construct(mem) : NULL;
    self->btn_down = bmp;
    ui_grade_enchant_btn_enhanced_down_init(bmp);
    ui_window_move_base(&self->btn_down->button.base, 0x9e, 0xf0);
    self->btn_down->button.base.id = ID_CHANCE_DOWN;
    self->btn_down->button.disabled = 0;
    ui_window_invalidate_base(&self->btn_down->button.base);
    ui_window_add_child(&self->frame.base, &self->btn_down->button.base);
}

void ui_grade_enchant_wnd_on_process(struct UIWindow *window)
{
    struct UIGradeEnchantWnd *self = (struct UIGradeEnchantWnd *)window;
    int frames[3] = { 6, 6, 5 };
    DWORD now;
    BYTE disabled = 1;

    if (frames[self->state] <= self->anim_frame)
        return;
    now = platform_ticks_ms();
    if (now > self->frame_delay + self->frame_time) {
        self->anim_frame++;
        if (frames[self->state] <= self->anim_frame)
            self->anim_frame = 1;
        self->frame_time = platform_ticks_ms();
        ui_window_invalidate_base(&self->frame.base);
    }
    if (now > self->wheel_delay + self->wheel_time) {
        if (++self->wheel >= 4)
            self->wheel = 1;
        self->wheel_time = platform_ticks_ms();
        ui_window_invalidate_base(&self->frame.base);
    }
    if (self->state == 2 && now >= self->result_time + 3000) {
        ui_grade_enchant_wnd_show_result(self);
        ui_grade_enchant_wnd_reset(self);
    }
    if (self->waiting != 1 && self->item.index && self->selected != -1) {
        const struct GradeMaterial *mat = MATERIAL(self, self->selected);

        if (inventory_count(mat->item_id) >= MATERIAL(self, self->selected)->count &&
            g_session1.zeny >= ui_grade_enchant_wnd_selected_zeny(self) && self->success_chance != 0)
            disabled = 0;
    }
    if (disabled) {
        self->btn_start->button.disabled = 1;
        ui_window_invalidate_base(&self->btn_start->button.base);
        ui_window_set_state(&self->btn_start->button.base, UI_BMP_DISABLED);
    } else {
        self->btn_start->button.disabled = 0;
        ui_window_invalidate_base(&self->btn_start->button.base);
        ui_window_set_state(&self->btn_start->button.base, 0);
    }
}

static void draw_material(struct UIGradeEnchantWnd *self, int item_id, int count, int icon_x, int icon_y,
                          int text_x, int text_y, int width, COLORREF colour, int font_arg, BYTE flag)
{
    struct ItemInfo info;
    const char *name;
    struct String line;

    item_info_construct(&info);

    item_info_set_item_id(&info, item_id);
    name = item_info_get_name(&info);
    ui_window_draw_item_icon(&self->frame.base, icon_x, icon_y, &info, 0);
    string_init(&line);
    string_format(&line, msg_str(MSG_NAME_COUNT), name, count);
    text_out(self, text_x, text_y, string_cstr(&line), width, 0, colour, 0, 0xb, font_arg, flag);
    string_free(&line);
    item_info_destruct(&info);
}

void ui_grade_enchant_wnd_on_draw(struct UIWindow *window)
{
    struct UIGradeEnchantWnd *self = (struct UIGradeEnchantWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct String path;
    int i;

    ui_window_clear_canvas(wnd, 0xffff00ff);
    ui_window_draw_bitmap(wnd, 0, 0, bitmap(GRADE_DIR "bg_grade_enchant.bmp"), 0);
    for (i = 0; i < MATERIAL_ROWS; i++)
        ui_window_draw_bitmap(wnd, MATERIAL_LEFT, MATERIAL_TOP + i * MATERIAL_PITCH,
                            bitmap(GRADE_DIR "btn_material.bmp"), 0);

    if (self->state == 0) {
        for (i = 0; i < 3; i++) {
            ui_window_draw_bitmap(wnd, 0x98, 0x2e + i * 0x10, bitmap(GRADE_DIR "caution.bmp"), 1);
            text_out(self, 0xa6, 0x2e + i * 0x10, msg_str(MSG_IDLE_FIRST + i), 0xe4, 0,
                     i ? COLOUR_OK : COLOUR_CAUTION, 0, 0xb, 1, 0);
        }
    }
    if (self->state == 1 || self->state == 2) {
        const struct GradeMaterial *mat;
        int y, n;

        ui_window_draw_bitmap(wnd, 0xc0, 0xe1, bitmap(GRADE_DIR "btn_unbroken.bmp"), 0);
        string_init(&path);
        string_format(&path, GRADE_DIR "bg_gradeitem%d.bmp", self->anim_frame);
        ui_window_draw_bitmap(wnd, 0x18, 0x36, bitmap(string_cstr(&path)), 0);
        string_free(&path);
        if (self->hover >= 0 && self->hover != self->selected)
            ui_window_draw_bitmap(wnd, MATERIAL_LEFT, MATERIAL_TOP + self->hover * MATERIAL_PITCH,
                                bitmap(GRADE_DIR "btn_material_over.bmp"), 0);
        if (self->selected >= 0)
            ui_window_draw_bitmap(wnd, MATERIAL_LEFT, MATERIAL_TOP + self->selected * MATERIAL_PITCH,
                                bitmap(GRADE_DIR "btn_material_pick.bmp"), 0);
        y = 0x76;
        for (mat = self->materials.first; mat != self->materials.last; mat++, y += MATERIAL_PITCH)
            draw_material(self, mat->item_id, mat->count, 0xaa, y - 9, 0xcb, y, 0xb4, 0, 0, 0);
        if (self->blessing.item_id > 0)
            draw_material(self, self->blessing.item_id, self->blessing_used, 0xc3, 0xe4, 0xe4, 0xed, 0xa0,
                          0xffffff, 1, 1);

        ui_grade_enchant_wnd_build_cautions(self);
        n = (int)(((BYTE *)self->cautions.last - (BYTE *)self->cautions.first) >> 2);
        if (n > 0) {
            for (i = 0, y = 0x2e; y < 0x5e; i++, y += 0x10) {
                int id;

                if (i >= n)
                    continue;
                ui_window_draw_bitmap(wnd, 0x98, y, bitmap(GRADE_DIR "caution.bmp"), 1);
                id = ((int *)self->cautions.first)[i];
                text_out(self, 0xa6, y, msg_str(id), 0xe4, 0,
                         id == MSG_OK || id == MSG_NOT_ALLOWED ? COLOUR_OK : COLOUR_CAUTION, 0, 0xb, 1, 0);
            }
        }
        string_format(&path, "%d", ui_grade_enchant_wnd_selected_zeny(self));
        text_out(self, 0x133, 0x12f, string_cstr(&path), 0x78, 2, 0, 0, 0x10, 1, 0);
        string_free(&path);
    }
    string_init(&path);
    string_format(&path, GRADE_DIR "gradewheel%d.bmp", self->state == 2 ? self->wheel : 1);
    ui_window_draw_bitmap(wnd, 0x38, 0x10c, bitmap(string_cstr(&path)), 1);
    string_free(&path);

    if (self->item.index) {
        ui_window_draw_item_icon(wnd, 0x40, 0x7d, &self->item, 0);
        if (self->item.grade > 0) {
            string_format(&path, GRADE_DIR "grade_icon%d.bmp", self->item.grade);
            ui_window_draw_bitmap(wnd, 0x40, 0x89, bitmap(string_cstr(&path)), 1);
            string_free(&path);
        }
    }
    if ((unsigned)(self->state - 1) <= 1) {
        string_init(&path);
        string_format(&path, "%d%%", ui_grade_enchant_wnd_success_chance(self));
        text_out(self, 0x27, 0x58, string_cstr(&path), 0x4a, 6, 0xffffff, 0x1d, 0x1c, 1, 1);
        string_free(&path);
    }
}

void ui_grade_enchant_wnd_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIGradeEnchantWnd *self = (struct UIGradeEnchantWnd *)window;
    if (y < 0x26 && x > 0x8d) {
        ui_frame_wnd_on_l_btn_down((struct UIWindow *)(&self->frame), x, y);
        return;
    }
    if (self->state == 1 && self->item.index && (unsigned)(x - 0x40) <= 0x2e && (unsigned)(y - 0x7d) <= 0x2e)
        game_mode_drag_item(game_current_mode(), 0x21, &self->item, 1, -1);
}

void ui_grade_enchant_wnd_on_l_btn_dbl_clk(struct UIWindow *window, int x, int y)
{
    struct UIGradeEnchantWnd *self = (struct UIGradeEnchantWnd *)window;
    if (self->item.index && (unsigned)(x - 0x35) <= 0x2e && (unsigned)(y - 0x72) <= 0x2e)
        ui_grade_enchant_wnd_reset(self);
}

void ui_grade_enchant_wnd_on_mouse_hover(struct UIWindow *window, int x, int y)
{
    struct UIGradeEnchantWnd *self = (struct UIGradeEnchantWnd *)window;
    self->hover = ui_grade_enchant_wnd_material_at(x, y);
}

void ui_grade_enchant_wnd_on_l_btn_up(struct UIWindow *window, int x, int y)
{
    struct UIGradeEnchantWnd *self = (struct UIGradeEnchantWnd *)window;
    int row;

    if (y < 0x50 && x > 0x8d) {
        ui_frame_wnd_on_l_btn_up((struct UIWindow *)(&self->frame), x, y);
        return;
    }
    row = ui_grade_enchant_wnd_material_at(x, y);
    if (row < 0 || (unsigned)row >= MATERIAL_COUNT(self))
        return;
    if (self->selected != row) {
        if (self->blessing_used > 0)
            ui_window_mgr_message_box(&g_windowMgr, msg_str(MSG_BLESSING_LOST), UI_MSGBOX_OK, 1, 0, 0,
                                   "메시지", 0x118, 0x78, 0);
        self->selected = row;
        self->blessing_used = 0;
        self->protect = 0;
    }
    ui_window_invalidate_base(&self->frame.base);
}

void ui_grade_enchant_wnd_on_r_btn_down(struct UIWindow *self, int x, int y)
{
}

void ui_grade_enchant_wnd_on_r_btn_up(struct UIWindow *window, int x, int y)
{
    struct UIGradeEnchantWnd *self = (struct UIGradeEnchantWnd *)window;
    struct ItemInfo tmp;
    struct UIWindow *wnd;

    if (y < 0x50 && x > 0x8d)
        return;
    if (self->state == 1) {
        int row = ui_grade_enchant_wnd_material_at(x, y);

        if (row >= 0 && (unsigned)row < MATERIAL_COUNT(self)) {
            wnd = ui_window_open(WID_ITEM_INFO);
            if (!wnd)
                return;
            ui_window_send_msg(wnd, NULL, MSG_ITEM_INFO,
                     (int)session_get_inventory_item_by_id(&g_session1, &tmp, MATERIAL(self, row)->item_id, 0),
                     0, 0, 0);
            item_info_destruct(&tmp);
            return;
        }
        if ((unsigned)(x - 0xc3) <= 0x18 && (unsigned)(y - 0xe4) <= 0x18) {
            wnd = ui_window_open(WID_ITEM_INFO);
            if (wnd) {
                ui_window_send_msg(wnd, NULL, MSG_ITEM_INFO,
                         (int)session_get_inventory_item_by_id(&g_session1, &tmp, self->blessing.item_id, 0), 0,
                         0, 0);
                item_info_destruct(&tmp);
            }
        }
    }
    if (self->item.index && (unsigned)(x - 0x35) <= 0x2e && (unsigned)(y - 0x72) <= 0x2e)
        show_item_info(&self->item);
}

int ui_grade_enchant_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender,
                                         int msg, int val1, int val2, int val3, int val4)
{
    struct UIGradeEnchantWnd *self = (struct UIGradeEnchantWnd *)window;
    struct ItemInfo tmp;

    if (msg == MSG_ITEM_DROPPED) {
        if (self->state == 0 || self->state == 1) {
            session_get_inventory_item(&g_session1, &tmp, val1);
            if (session_is_in_switch_sets(&g_session1, tmp.index) == 1) {
                game_send_message(MODE_MSG_DRAG_END, 0, 0, 0, 0);
                item_info_destruct(&tmp);
                return -1;
            }
            if (tmp.index && tmp.identified == 1) {
                struct PacketGradeEnchantSelect packet;

                packet.id = CZ_GRADE_ENCHANT_SELECT;
                packet.index = (short)val1;
                rag_connection_send_packet(rag_connection_instance(),
                                          rag_connection_get_packet_length(rag_connection_instance(),
                                                                         CZ_GRADE_ENCHANT_SELECT),
                                          &packet);
            }
            item_info_destruct(&tmp);
        }
    } else if (msg == UI_MSG_COMMAND) {
        switch (val1) {
        case ID_START: {
            const struct GradeMaterial *mat;
            struct PacketGradeEnchantStart packet;

            if (self->waiting == 1 || !self->item.index || self->selected == -1)
                break;
            mat = MATERIAL(self, self->selected);
            if (inventory_count(mat->item_id) < MATERIAL(self, self->selected)->count)
                break;
            if (g_session1.zeny < ui_grade_enchant_wnd_selected_zeny(self))
                break;
            if (!self->protect && !self->blessing_used &&
                ui_window_mgr_message_box(&g_windowMgr, msg_str(MSG_CONFIRM_START), UI_MSGBOX_QUESTION, 1, 0, 0,
                                       "메시지", 0x118, 0x78, 0) == UI_MSGBOX_ANSWER_NO)
                break;
            self->waiting = 1;
            packet.id = CZ_GRADE_ENCHANT_START;
            packet.index = (short)self->item.index;
            packet.material = self->selected;
            packet.blessing = self->blessing_used > 0;
            packet.blessing_count = self->blessing_used / self->blessing.step;
            packet.protect = self->protect;
            rag_connection_send_packet(rag_connection_instance(),
                                      rag_connection_get_packet_length(rag_connection_instance(),
                                                                     CZ_GRADE_ENCHANT_START),
                                      &packet);
            break;
        }
        case ID_CLOSE:
        case ID_CLOSE2: {
            unsigned short packet = CZ_GRADE_ENCHANT_CLOSE;

            rag_connection_send_packet(rag_connection_instance(),
                                      rag_connection_get_packet_length(rag_connection_instance(),
                                                                     CZ_GRADE_ENCHANT_CLOSE),
                                      &packet);
            ui_window_mgr_post_quit1(&g_windowMgr, WID_GRADE_ENCHANT);
            break;
        }
        case ID_CHANCE_UP:
            if (self->blessing.item_id > 0 &&
                self->blessing_used < self->blessing.max_steps * self->blessing.step &&
                inventory_count(self->blessing.item_id) >= self->blessing_used + self->blessing.step)
                self->blessing_used += self->blessing.step;
            break;
        case ID_CHANCE_DOWN:
            if (self->blessing.item_id > 0 && self->blessing_used > 0)
                self->blessing_used -= self->blessing.step;
            break;
        }
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
}
