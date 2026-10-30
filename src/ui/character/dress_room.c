#include "ui/character/character_draw.h"
#include "ui/controls/listbox.h"
#include "render/rp_pool.h"
#include "contents/client_lua_calls.h"
#include "session/state.h"
#include "core/object_type_tables.h"
#include "ui/character/dress_room.h"
#include "game/context.h"
#include "actor/appearance.h"
#include <mmsystem.h>
#include "ui/character/character_std.h"
#include "core/msg_str.h"
#include "diag/perf.h"
#include "session/session_jobs.h"
#include "session/session_talk.h"
#include "ui/draw_key.h"

struct DressOption {
    struct String text;
    int value;
};

struct Vec;
struct UISecondCostumeWnd;
struct UIWindow;
void ui_second_costume_wnd_destroy(struct UIWindow *window);
void ui_second_costume_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_second_costume_wnd_on_draw(struct UIWindow *self);
BYTE ui_second_costume_wnd_render(struct UIWindow *self, int force);
int ui_second_costume_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_second_costume_wnd_ops = {
    .destroy = ui_second_costume_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 1,
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
    .on_create = ui_second_costume_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_second_costume_wnd_on_draw,
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
    .send_msg = ui_second_costume_wnd_send_msg,
    .invalidate = ui_window_invalidate_base,
    .render = ui_second_costume_wnd_render,
    .has_custom_backdrop = 0,
    .draw_backdrop = ui_window_draw_backdrop_base,
    .get_records = ui_frame_wnd_get_records,
    .type = UI_TYPE_FRAME,
    .on_hover_enter = ui_window_on_hover_enter_base,
    .on_hover_leave = ui_window_on_hover_leave_base,
    .make_quad = ui_window_make_quad_base,
    .hit_test = ui_window_hit_test_base,
    .render_overlay = ui_window_render_overlay_base,
};

enum {
    DRESS_HEAD_PALETTE,
    DRESS_HAIR_STYLE,
    DRESS_BODY_PALETTE,
    DRESS_BODY,
    DRESS_LISTS
};
#define DRESS_STRINGS 7
#define DRESS_DIRECTIONS 8

#define DRESS_ID_LIST 0x148
#define DRESS_ID_TURN_LEFT 0xda
#define DRESS_ID_TURN_RIGHT 0xdb
#define DRESS_LIST_BACK 0xd2
#define DRESS_COMBO_BORDER 0x64
#define DRESS_COMBO_BACK 0xe8
#define DRESS_COMBO_LIST_HEIGHT 0x43
#define DRESS_PANEL_WIDTH 0xcc

#define MSG_DRESS_LIST_FIRST 0xacb

#define UI_WND_SECOND_COSTUME 0x103

#define COMBO_MSG_CHOSEN 0x27

struct UISecondCostumeWnd {
    struct UIFrameWnd frame;
    struct UIWindow *combo;
    struct UIListBox *list;
    int category;
    int selected[DRESS_LISTS];
    struct Vec lists[DRESS_LISTS];
    DWORD start_time;
    int direction;
    struct String spr_paths[DRESS_STRINGS];
    struct String act_paths[DRESS_STRINGS];
    struct UIAnimation animation;
    unsigned long long drawn_key;
};
OFFSET_ASSERT(UISecondCostumeWnd, lists, 0xb0);
OFFSET_ASSERT(UISecondCostumeWnd, start_time, 0xe0);

static size_t list_size(const struct Vec *v)
{
    return (const struct DressOption *)v->last - (const struct DressOption *)v->first;
}

struct UISecondCostumeWnd *ui_second_costume_wnd_construct(struct UISecondCostumeWnd *self)
{
    int i;

    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_second_costume_wnd_ops;
    for (i = 0; i < DRESS_LISTS; i++)
        vec_init(&self->lists[i]);
    for (i = 0; i < DRESS_STRINGS; i++)
        string_init(&self->spr_paths[i]);
    for (i = 0; i < DRESS_STRINGS; i++)
        string_init(&self->act_paths[i]);
    self->combo = NULL;
    self->list = NULL;
    self->category = 0;
    for (i = 0; i < DRESS_LISTS; i++)
        self->selected[i] = 0;
    self->start_time = timeGetTime();
    self->direction = 0;
    self->frame.base.redraws_tree = 1;
    ui_animation_reset(&self->animation);
    self->drawn_key = 0;
    return self;
}

struct UIWindow *ui_second_costume_wnd_new(void)
{
    return (struct UIWindow *)ui_second_costume_wnd_construct(mem_alloc(sizeof(struct UISecondCostumeWnd)));
}

static void dress_options_clear(struct Vec *v)
{
    struct DressOption *p;

    for (p = v->first; p != v->last; p++)
        string_free(&p->text);
    vec_clear(v);
}

static void destruct(struct UISecondCostumeWnd *self)
{
    int i;

    self->frame.base.ops = &ui_second_costume_wnd_ops;
    for (i = 0; i < DRESS_LISTS; i++)
        dress_options_clear(&self->lists[i]);
    for (i = DRESS_STRINGS - 1; i >= 0; i--)
        string_free(&self->act_paths[i]);
    for (i = DRESS_STRINGS - 1; i >= 0; i--)
        string_free(&self->spr_paths[i]);
    for (i = DRESS_LISTS - 1; i >= 0; i--)
        vec_free(&self->lists[i]);
    ui_frame_wnd_destruct(&self->frame);
}

void ui_second_costume_wnd_destroy(struct UIWindow *window)
{
    struct UISecondCostumeWnd *self = (struct UISecondCostumeWnd *)window;
    destruct(self);
    mem_free(self);
}

static int picked_value(struct UISecondCostumeWnd *self, int category)
{
    const struct Vec *v = &self->lists[category];

    if (v->first == v->last || (unsigned)self->selected[category] >= list_size(v))
        return 0;
    return ((const struct DressOption *)v->first)[self->selected[category]].value;
}

static int index_of(const struct Vec *v, int value)
{
    const struct DressOption *items = v->first;
    size_t i, n = list_size(v);

    for (i = 0; i < n; i++)
        if (items[i].value == value)
            return (int)i;
    return 0;
}

void ui_second_costume_wnd_draw_character(struct UISecondCostumeWnd *self, int x, int y)
{
    struct PlayerAppearance appearance;
    int head_palette = picked_value(self, DRESS_HEAD_PALETTE);
    int body_palette = picked_value(self, DRESS_BODY_PALETTE);
    int hair = picked_value(self, DRESS_HAIR_STYLE);
    int body = picked_value(self, DRESS_BODY);

    if (!player_get_appearance(game_current_player(), &appearance))
        return;
    struct CharacterDrawOptions options = {
        .x = x,
        .y = y,
        .sex = session_get_sex(&g_session1),
        .job = session_get_drawn_job(&g_session1),
        .body = body,
        .head = hair,
        .accessory = (WORD)g_session1.headBottom,
        .accessory2 = (WORD)g_session1.headTop,
        .accessory3 = (WORD)g_session1.headMid,
        .robe = g_session1.robe,
        .riding = appearance.mounted,
        .scaled = 0,
        .body_palette = body_palette,
        .head_palette = head_palette,
        .action = self->direction,
        .motion = 0,
        .start_time = self->start_time,
        .animation = &self->animation,
    };
    ui_draw_character(&self->frame.base, &options, 0);
}

static void load_list(struct Vec *list, const char *func, int sex, int job, int with_job)
{
    int i;

    for (i = 1;; i++) {
        const char *text = NULL;
        int value = 0;

        struct DressOption item;

        if (with_job)
            client_lua_call(g_session1.luaState, func, "ddd>sd", sex, job, i, &text, &value);
        else
            client_lua_call(g_session1.luaState, func, "dd>sd", sex, i, &text, &value);
        if (!text)
            break;
        string_init(&item.text);
        string_assign_cstr(&item.text, text);
        item.value = value;
        vec_push(list, &item, sizeof item);
    }
}

void ui_second_costume_wnd_pick_worn(struct UISecondCostumeWnd *self)
{
    self->selected[DRESS_HEAD_PALETTE] = index_of(&self->lists[DRESS_HEAD_PALETTE], g_session1.hairColor);
    self->selected[DRESS_HAIR_STYLE] = index_of(&self->lists[DRESS_HAIR_STYLE], g_session1.hairStyle);
    self->selected[DRESS_BODY_PALETTE] = index_of(&self->lists[DRESS_BODY_PALETTE], g_session1.clothesColor);
    self->selected[DRESS_BODY] = index_of(&self->lists[DRESS_BODY], g_session1.body);
}

void ui_second_costume_wnd_load_lists(struct UISecondCostumeWnd *self)
{
    int job = session_get_drawn_job(&g_session1);
    int sex = session_get_sex(&g_session1);

    load_list(&self->lists[DRESS_HEAD_PALETTE], "_get_DressHeadPaletteList", sex, 0, 0);
    load_list(&self->lists[DRESS_HAIR_STYLE], "_get_Headstylelist", sex, 0, 0);
    load_list(&self->lists[DRESS_BODY_PALETTE], "_get_DressBodyPaletteList", sex, 0, 0);
    load_list(&self->lists[DRESS_BODY], "_get_dresslist", sex, job, 1);
    ui_second_costume_wnd_pick_worn(self);
}

void ui_second_costume_wnd_show_list(struct UISecondCostumeWnd *self, int category)
{
    struct Vec *v = &self->lists[category];
    size_t i;

    ui_list_box_navi_clear_items(self->list);
    for (i = 0; i < list_size(v); i++)
        ui_list_box_navi_add_item(self->list, string_cstr(&((struct DressOption *)v->first)[i].text));
    self->list->selected = self->selected[category];
    ui_window_invalidate_base(&self->list->base);
    ui_window_invalidate_base(&self->frame.base);
}

static void add_arrow(struct UISecondCostumeWnd *self, const char *bitmap, int x, int id)
{
    struct UIBitmapButton *button = mem_alloc(sizeof *button);
    int state;

    if (button)
        button = ui_bitmap_button_construct(button);
    for (state = 0; state < 3; state++)
        ui_bitmap_button_set_bitmap(button, bitmap, state, 0);
    ui_window_create(&button->button.base, button->bitmap_width, button->bitmap_height);
    ui_window_move_base(&button->button.base, x, 0x6e);
    button->button.base.id = id;
    ui_window_add_child(&self->frame.base, &button->button.base);
}

void ui_second_costume_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UISecondCostumeWnd *self = (struct UISecondCostumeWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct UIListBox *list;
    struct UIComboBoxDefWnd *combo;
    int i;

    ui_second_costume_wnd_load_lists(self);
    ui_window_add_close_button(wnd, "Close", 0);

    list = mem_alloc(sizeof(struct UIListBoxNavi));
    if (list)
        list = &ui_list_box_navi_construct((struct UIListBoxNavi *)list)->list;
    self->list = list;
    ui_window_create(&list->base, width - 0xd4, 0xa0);
    self->list->r = DRESS_LIST_BACK;
    self->list->g = DRESS_LIST_BACK;
    self->list->b = DRESS_LIST_BACK;
    self->list->base.id = DRESS_ID_LIST;
    ui_window_move_base(&self->list->base, 4, 0x28);
    ui_window_add_child(wnd, &self->list->base);
    ui_second_costume_wnd_show_list(self, self->category);

    combo = mem_alloc(sizeof *combo);
    if (combo)
        combo = (struct UIComboBoxDefWnd *)ui_combo_box_def_wnd_construct(&combo->base);
    self->combo = &combo->base;
    combo->list_height = DRESS_COMBO_LIST_HEIGHT;
    ui_window_create(self->combo, width - 0xd4, 0x12);
    ui_window_move_base(self->combo, 4, 0x14);
    combo = (struct UIComboBoxDefWnd *)self->combo;
    combo->border_r = combo->border_g = combo->border_b = DRESS_COMBO_BORDER;
    combo->back_r = combo->back_g = combo->back_b = DRESS_COMBO_BACK;
    ui_window_add_child(wnd, self->combo);
    for (i = 0; i < DRESS_LISTS; i++)
        ui_combo_add(self->combo, msg_str(MSG_DRESS_LIST_FIRST + i));
    ui_combo_box_def_wnd_select(self->combo, self->category);

    add_arrow(self, UI_SKIN "scroll1left.bmp", width - 0xca, DRESS_ID_TURN_RIGHT);
    add_arrow(self, UI_SKIN "scroll1right.bmp", width - 0x13, DRESS_ID_TURN_LEFT);
}

static unsigned long long dress_key(struct UISecondCostumeWnd *self)
{
    struct PlayerAppearance appearance;
    unsigned long long key = DRAW_KEY_START;
    int i;

    for (i = 0; i < DRESS_LISTS; i++)
        key = draw_key_int(key, picked_value(self, i));
    key = draw_key_int(key, self->direction);
    key = draw_key_int(key, session_get_sex(&g_session1));
    key = draw_key_int(key, session_get_drawn_job(&g_session1));
    key = draw_key_int(key, g_session1.headBottom);
    key = draw_key_int(key, g_session1.headTop);
    key = draw_key_int(key, g_session1.headMid);
    key = draw_key_int(key, g_session1.robe);
    if (player_get_appearance(game_current_player(), &appearance))
        key = draw_key_int(key, appearance.mounted);
    else
        key = draw_key_int(key, -1);
    return draw_key_children(key, &self->frame.base);
}

BYTE ui_second_costume_wnd_render(struct UIWindow *window, int force)
{
    struct UISecondCostumeWnd *self = (struct UISecondCostumeWnd *)window;

    if (dress_key(self) != self->drawn_key || ui_animation_due(&self->animation))
        ui_window_invalidate_base(window);
    return ui_window_render_base(window, force);
}

void ui_second_costume_wnd_on_draw(struct UIWindow *window)
{
    struct UISecondCostumeWnd *self = (struct UISecondCostumeWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    int top = ui_window_draw_title_bar(wnd, 1, "Dress Room", 0);
    int x = wnd->width - DRESS_PANEL_WIDTH;
    struct BitmapRes *back;

    ui_animation_reset(&self->animation);
    ui_window_draw_body(wnd, 0x11, 0, "");
    back = ui_skin_bitmap(UI_SKIN "basic_interface\\dressroomback.bmp");
    if (back)
        ui_window_draw_bitmap(wnd, x, top + 3, back, 0);
    ui_second_costume_wnd_draw_character(self, x + 0x64, wnd->height - 0x23);
    self->drawn_key = dress_key(self);
}

int ui_second_costume_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                          int val1, int val2, int val3, int val4)
{
    struct UISecondCostumeWnd *self = (struct UISecondCostumeWnd *)window;
    if (msg == UI_MSG_COMMAND) {
        switch (val1) {
        case UI_ID_CLOSE:
            ui_window_mgr_post_quit1(&g_windowMgr, UI_WND_SECOND_COSTUME);
            break;
        case DRESS_ID_TURN_LEFT:
            self->direction = self->direction > 0 ? self->direction - 1 : DRESS_DIRECTIONS - 1;
            ui_window_invalidate_base(&self->frame.base);
            break;
        case DRESS_ID_TURN_RIGHT:
            self->direction = self->direction >= DRESS_DIRECTIONS - 1 ? 0 : self->direction + 1;
            ui_window_invalidate_base(&self->frame.base);
            break;
        case DRESS_ID_LIST:
            if (self->list->selected != -1) {
                self->selected[self->category] = self->list->selected;
                ui_window_invalidate_base(&self->frame.base);
            }
            break;
        }
        return 0;
    }
    if (msg == COMBO_MSG_CHOSEN) {
        self->category = val1;
        ui_second_costume_wnd_show_list(self, val1);
        return 0;
    }
    return ui_window_send_msg_base(&self->frame.base, sender, msg, val1, val2, val3, val4);
}
