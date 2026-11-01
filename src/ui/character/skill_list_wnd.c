#include "game/drag.h"
#include "world/tooltip.h"
#include "render/rp_pool.h"
#include "std/list.h"
#include "skills/skill_info.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "contents/mouse.h"
#include "core/object_type_tables.h"
#include "world/mode_manager.h"
#include "game/context.h"
#include <string.h>
#include "ui/social/social.h"
#include "ui/controls/controls.h"
#include "ui/window_mgr.h"
#include "ui/game.h"
#include "ui/ui_items.h"
#include "ui/runtime.h"
#include <mmsystem.h>
#include "ui/character/skill_list_wnd.h"
#include "core/msg_str.h"
#include "diag/perf.h"
#include "session/session_names.h"
#include "session/session_skills.h"
#include "session/skill_levels.h"
#include "ui/character/skill_tree.h"
#include "ui/ui_std.h"

int g_skillPointPending = 0;

static BYTE g_skillListRowCount = 0;

static int g_skillListRowsChangedTime = 0;

static int g_skillListUpSkillId = 0;

#define SKILL_LIST_KIND_HOMUN 0
#define SKILL_LIST_KIND_MERCENARY 1
#define WND_ID_HOMUN_SKILL_LIST 0x72
#define WND_ID_MER_SKILL_LIST 0x7e

#define ROWS_SHOWN 7
#define ROW_HEIGHT 0x20
#define LIST_TOP 0x11
#define LIST_LEFT 0x28
#define LIST_HIT_WIDTH 0x20
#define LIST_MARGIN 0x26
#define HIDDEN (-200)

#define MIN_WIDTH 0xe8
#define MAX_WIDTH 0x140
#define BASE_WIDTH 0x118
#define MIN_HEIGHT 0x78
#define MAX_HEIGHT 0x120

#define ID_USE 0xb0
#define ID_UP_FIRST 0xe7
#define ID_DOWN_FIRST 0xf5
#define ID_RAISE_FIRST 0xfc

#define SKILL_LIST_MSG_REBUILD 0x17
#define SKILL_LIST_MSG_MENU 0x31
#define SKILL_LIST_MSG_USE 0x3a

#define WND_ID_MENU 0x12
#define MSG_MENU_CHOICE 0x30
#define MSG_MENU_FIT 0x1e
#define MSG_MENU_ADD_ITEM 0x1f
#define MSG_MENU_SET_OWNER 0x53
#define MENU_ACTION_DESCRIBE 3
#define MENU_ACTION_RENAME 0xe
#define MSG_SKILL_DESCRIPTION 0x142

#define WND_ID_SKILL_DESCRIBE 0x2e
#define SKILL_DESCRIBE_MSG_SHOW 0x3d
#define WND_ID_SKILL_NAME_CHANGE 0x37
#define SKILL_NAME_MSG_OPEN 0x44

#define MODE_REQ_SKILL_UP 0x43
#define MODE_REQ_USE_SKILL 0x71
#define DRAG_KIND_SKILL 8

#define MSG_TITLE_HOMUN 0x3a9
#define MSG_TITLE_MERCENARY 0x4ee
#define MSG_SKILL_POINTS 0x11c
#define MSG_PASSIVE 0x11e

#define WAIT_MS 1000

struct UISkillListWnd {
    struct UIFrameWnd frame;
    struct UIResizer *resizer;
    struct UIScrollBar *bar;
    struct UIBitmapButton *use_button;
    struct UIBitmapButton *close_button;
    struct UIBitmapButton *up_buttons[ROWS_SHOWN];
    struct UIBitmapButton *lower_buttons[ROWS_SHOWN];
    struct UIBitmapButton *raise_buttons[ROWS_SHOWN];
    int selected;
    int top;
    int per_row;
    int rows;
    int scrollable;
    struct List skills;
    struct WindowInfo *info;
    struct Vec menu;
    int kind;
};
OFFSET_ASSERT(UISkillListWnd, up_buttons, 0xa4);
OFFSET_ASSERT(UISkillListWnd, selected, 0xf8);
OFFSET_ASSERT(UISkillListWnd, skills, 0x10c);
OFFSET_ASSERT(UISkillListWnd, menu, 0x118);
SIZE_ASSERT(UISkillListWnd, 0x128);
struct UIWindow;
void ui_skill_list_wnd_destroy(struct UIWindow *window);
void ui_skill_list_wnd_store_info(struct UIWindow *self);
void ui_skill_list_wnd_on_create(struct UIWindow *self, int width_unused, int height);
void ui_skill_list_wnd_on_draw(struct UIWindow *self);
void ui_skill_list_wnd_on_l_btn_down(struct UIWindow *self, int x, int y);
void ui_skill_list_wnd_on_l_btn_dbl_clk(struct UIWindow *self, int x, int y);
void ui_skill_list_wnd_on_mouse_hover(struct UIWindow *self, int x, int y);
void ui_skill_list_wnd_on_r_btn_down(struct UIWindow *self, int x, int y);
void ui_skill_list_wnd_on_r_btn_up(struct UIWindow *self, int x_unused, int y_unused);
void ui_skill_list_wnd_on_wheel(struct UIWindow *self, int delta);
int ui_skill_list_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_skill_list_wnd_ops = {
    .destroy = ui_skill_list_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 1,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_skill_list_wnd_store_info,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_skill_list_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_skill_list_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_skill_list_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_skill_list_wnd_on_l_btn_dbl_clk,
    .on_mouse_move = ui_frame_wnd_on_mouse_move,
    .on_mouse_hover = ui_skill_list_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_frame_wnd_on_l_btn_up,
    .on_r_btn_down = ui_skill_list_wnd_on_r_btn_down,
    .on_r_btn_up = ui_skill_list_wnd_on_r_btn_up,
    .on_wheel = ui_skill_list_wnd_on_wheel,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_skill_list_wnd_send_msg,
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

#define WND(self) (&(self)->frame.base)

static int chosen_level(const struct UISkillListWnd *self, int skill_id)
{
    if (self->kind == SKILL_LIST_KIND_HOMUN)
        return session_get_homun_skill_use_level(&g_session1, skill_id);
    if (self->kind == SKILL_LIST_KIND_MERCENARY)
        return session_get_merc_skill_use_level(&g_session1, skill_id);
    return 0;
}

static void set_chosen_level(const struct UISkillListWnd *self, int skill_id, int level)
{
    if (self->kind == SKILL_LIST_KIND_HOMUN)
        session_set_homun_skill_use_level(&g_session1, skill_id, level);
    else if (self->kind == SKILL_LIST_KIND_MERCENARY)
        session_set_merc_skill_use_level(&g_session1, skill_id, level);
}

static void check_wait(struct UISkillListWnd *self)
{
    int elapsed;

    if (g_skillPointPending != 1)
        return;
    elapsed = (int)(timeGetTime() - g_skillListRowsChangedTime);
    if (elapsed > WAIT_MS || elapsed < 0) {
        g_skillPointPending = 0;
        ui_window_invalidate_base(&self->frame.base);
    }
}

static int hit_row(struct UISkillListWnd *self, int x, int y)
{
    struct ListNode *node;
    int row, index, shown;

    if (x < LIST_LEFT || y < LIST_TOP || y >= self->rows * ROW_HEIGHT + LIST_TOP)
        return -1;
    row = (y - LIST_TOP) / ROW_HEIGHT;
    shown = 0;
    index = 0;
    for (node = self->skills.head->next; node != self->skills.head; node = node->next, index++) {
        if (index < self->top * self->per_row)
            continue;
        if (shown == self->per_row * row)
            return index;
        shown++;
    }
    return -1;
}

static struct SkillInfo *get_entry(struct UISkillListWnd *self, struct SkillInfo *out, int index)
{
    struct ListNode *node;
    int i = 0;

    skill_info_construct_unknown(out);
    for (node = self->skills.head->next; node != self->skills.head; node = node->next, i++) {
        if (i == index) {
            skill_info_assign(out, LIST_VALUE(node, struct SkillInfo));
            break;
        }
    }
    return out;
}

static struct SkillInfo *entry_at(struct UISkillListWnd *self, struct SkillInfo *out, int x, int y)
{
    struct ListNode *node;
    int row, index, shown;

    if ((unsigned)(x - LIST_LEFT) < LIST_HIT_WIDTH && y >= LIST_TOP && y < self->rows * ROW_HEIGHT + LIST_TOP) {
        row = (y - LIST_TOP) / ROW_HEIGHT;
        shown = 0;
        index = 0;
        for (node = self->skills.head->next; node != self->skills.head; node = node->next, index++) {
            if (index < self->top * self->per_row)
                continue;
            if (shown == self->per_row * row)
                return skill_info_copy(out, LIST_VALUE(node, struct SkillInfo));
            shown++;
        }
    }
    return skill_info_construct_unknown(out);
}

static struct SkillInfo *get_row_entry(struct UISkillListWnd *self, struct SkillInfo *out, int i)
{
    struct ListNode *node;
    int index = 0;

    for (node = self->skills.head->next; node != self->skills.head; node = node->next, index++)
        if (index == self->top + i)
            return skill_info_copy(out, LIST_VALUE(node, struct SkillInfo));
    return skill_info_construct_unknown(out);
}

static void refresh_range(struct UISkillListWnd *self)
{
    int lines, excess;

    if (!self->bar)
        return;
    lines = self->skills.size / self->per_row + (self->skills.size % self->per_row != 0);
    excess = lines - self->rows;
    if (excess < 0)
        excess = 0;
    self->scrollable = excess != 0;
    if (excess) {
        ui_scroll_bar_set_range(self->bar, excess, self->rows, lines);
        if (self->top < 0)
            self->top = 0;
        if (self->top > lines - self->rows)
            self->top = lines - self->rows;
        ui_scroll_bar_set_pos(self->bar, self->top);
    } else {
        self->top = 0;
        ui_scroll_bar_set_range(self->bar, 0, self->rows, lines);
    }
    ui_window_invalidate_base(&self->bar->base);
}

static void place_buttons(struct UISkillListWnd *self)
{
    int i, y;

    for (i = 0, y = 0x22; y < 0x102; i++, y += ROW_HEIGHT) {
        struct SkillInfo info;
        int points = 0;
        int in_view;

        get_row_entry(self, &info, i);
        if (self->kind == SKILL_LIST_KIND_HOMUN)
            points = g_session1.homunSkillPoints;
        else if (self->kind == SKILL_LIST_KIND_MERCENARY)
            points = g_session1.merSkillPoints;
        in_view = i < self->rows && (unsigned)(self->top + i) < self->skills.size;
        if (in_view && points && info.found && SKILL_INFO_UPGRADABLE(&info))
            ui_window_move_base(&self->up_buttons[i]->button.base, 0x4e, y - 0xd);
        else
            ui_window_move_base(&self->up_buttons[i]->button.base, HIDDEN, y - 0x11);
        if (in_view && info.found && info.level && skill_info_is_level_used(info.id)) {
            ui_window_move_base(&self->lower_buttons[i]->button.base, 0x70, y);
            ui_window_move_base(&self->raise_buttons[i]->button.base, 0xc0, y);
        } else {
            ui_window_move_base(&self->lower_buttons[i]->button.base, HIDDEN, y);
            ui_window_move_base(&self->raise_buttons[i]->button.base, HIDDEN, y);
        }
        skill_info_destruct(&info);
    }
}

struct UISkillListWnd *ui_skill_list_wnd_construct(struct UISkillListWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    WND(self)->ops = &ui_skill_list_wnd_ops;
    list_init(&self->skills);
    self->menu.first = self->menu.last = self->menu.end = NULL;
    self->use_button = NULL;
    self->close_button = NULL;
    self->selected = -1;
    self->per_row = 1;
    self->info = NULL;
    return self;
}

struct UIWindow *ui_skill_list_wnd_new(void)
{
    return (struct UIWindow *)ui_skill_list_wnd_construct(mem_alloc(sizeof(struct UISkillListWnd)));
}

void ui_skill_list_wnd_destruct(struct UISkillListWnd *self)
{
    WND(self)->ops = &ui_skill_list_wnd_ops;
    ui_menu_entry_vec_free(&self->menu);
    list_free(&self->skills, skill_info_destroy);
    ui_frame_wnd_destruct(&self->frame);
}

void ui_skill_list_wnd_destroy(struct UIWindow *window)
{
    struct UISkillListWnd *self = (struct UISkillListWnd *)window;
    ui_skill_list_wnd_destruct(self);
    mem_free(self);
}

void ui_skill_list_wnd_store_info(struct UIWindow *window)
{
    struct UISkillListWnd *self = (struct UISkillListWnd *)window;
    if (!self->info)
        return;
    self->info->x = WND(self)->x;
    self->info->y = WND(self)->y;
    self->info->width = WND(self)->width;
    self->info->height = WND(self)->height;
}

static struct UIBitmapButton *add_arrow(struct UIWindow *wnd, const char *path, int x, int y, int id)
{
    return ui_add_bitmap_button(wnd, path, path, path, x, y, id);
}

void ui_skill_list_wnd_on_create(struct UIWindow *window, int width_unused, int height)
{
    struct UISkillListWnd *self = (struct UISkillListWnd *)window;
    static const char *const names[2][3] = {
        { "btn_use", "btn_use_a", "btn_use_b" },
        { "btn_close", "btn_close_a", "btn_close_b" },
    };
    static const int ids[2] = { ID_USE, UI_ID_CLOSE };
    struct UIWindow *wnd = WND(self);
    struct UIResizer *resizer;
    struct UIScrollBar *bar;
    struct UIBitmapButton **slots[2];
    struct String base;
    int xs[2];
    int b, i, y;

    resizer = mem_alloc(sizeof *resizer);
    resizer = resizer ? ui_resizer_construct(resizer) : NULL;
    self->resizer = resizer;
    ui_window_create(&resizer->base, 0x10, 0x10);
    ui_resizer_set_edge(self->resizer, UI_EDGE_BOTTOM);
    ui_resizer_set_bitmap(self->resizer, UI_SKIN "btn_resize.bmp");
    ui_window_move_base(&self->resizer->base, wnd->width - 0xf, wnd->height - 0xf);
    ui_resizer_set_colours(self->resizer, UI_TRANSPARENT, UI_TRANSPARENT);
    ui_window_add_child(wnd, &self->resizer->base);

    bar = mem_alloc(sizeof *bar);
    bar = bar ? ui_scroll_bar_construct(bar) : NULL;
    self->bar = bar;
    ui_scroll_bar_create(bar, wnd->width - 0xf, 0xe, UI_SCROLL_VERT, wnd->height - LIST_MARGIN);
    ui_window_add_child(wnd, &self->bar->base);

    string_init(&base);
    string_assign(&base, UI_SKIN, sizeof UI_SKIN - 1);
    slots[0] = &self->use_button;
    slots[1] = &self->close_button;
    xs[0] = wnd->width - 0x6b;
    xs[1] = wnd->width - 0x3d;
    for (b = 0; b < 2; b++) {
        struct UIBitmapButton *button = mem_alloc(sizeof(struct UIBitmapButton));
        int state;

        if (button)
            button = ui_bitmap_button_construct(button);
        *slots[b] = button;
        for (state = 0; state < 3; state++) {
            struct String path;

            string_copy(&path, &base);
            string_append_cstr(&path, names[b][state]);
            string_append_cstr(&path, ".bmp");
            ui_bitmap_button_set_bitmap(button, string_cstr(&path), state, 0);
            string_free(&path);
        }
        ui_window_create(&button->button.base, button->bitmap_width, button->bitmap_height);
        ui_window_move_base(&button->button.base, xs[b], wnd->height - 0x18);
        button->button.base.id = ids[b];
        ui_window_add_child(wnd, &button->button.base);
    }

    self->per_row = 1;
    self->rows = (height - LIST_MARGIN) / ROW_HEIGHT;
    ui_window_add_sys_button(wnd);
    ui_window_add_close_button(wnd, "Alt+S", 0);

    for (i = 0, y = 0x15; y < 0xf5; i++, y += ROW_HEIGHT)
        self->up_buttons[i] = ui_add_bitmap_button(wnd, UI_SKIN "basic_interface\\skill_up_a.bmp",
                                                   UI_SKIN "basic_interface\\skill_up_b.bmp",
                                                   UI_SKIN "basic_interface\\skill_up_c.bmp", 0x4e, y,
                                                   ID_UP_FIRST + i);
    for (i = 0, y = 0x22; y < 0x102; i++, y += ROW_HEIGHT)
        self->lower_buttons[i] = add_arrow(wnd, UI_SKIN "basic_interface\\arw_left.bmp", 0x70, y,
                                           ID_DOWN_FIRST + i);
    for (i = 0, y = 0x22; y < 0x102; i++, y += ROW_HEIGHT)
        self->raise_buttons[i] = add_arrow(wnd, UI_SKIN "basic_interface\\arw_right.bmp", 0xc0, y,
                                           ID_RAISE_FIRST + i);
    ui_skill_list_wnd_send_msg(&self->frame.base, NULL, SKILL_LIST_MSG_REBUILD, 0, 0, 0, 0);
    string_free(&base);
}

static struct BitmapRes *title_bitmap(const char *normal, const char *waiting)
{
    return ui_skin_bitmap(g_skillPointPending ? waiting : normal);
}

void ui_skill_list_wnd_on_draw(struct UIWindow *window)
{
    struct UISkillListWnd *self = (struct UISkillListWnd *)window;
    struct UIWindow *wnd = WND(self);
    struct ListNode *node;
    struct BitmapRes *bitmap;
    char buf[0x50];
    int body, strips, columns, rows, i, x, y, index, drawn;
    int level = 0;

    ui_window_draw_bitmap(wnd, 0, 0,
                        title_bitmap(UI_SKIN "basic_interface\\titlebar_left.bmp",
                                     UI_SKIN "basic_interface\\titlebar_left2.bmp"),
                        0);
    bitmap = title_bitmap(UI_SKIN "basic_interface\\titlebar_right.bmp",
                          UI_SKIN "basic_interface\\titlebar_right2.bmp");
    ui_window_draw_bitmap(wnd, wnd->width - bitmap->width, 0, bitmap, 0);
    body = wnd->width - 0x18;
    strips = body / 0xc + (body % 0xc != 0);
    for (i = 0, x = 0xc; i < strips; i++, x += 0xc)
        ui_window_draw_bitmap(wnd, x, 0,
                            title_bitmap(UI_SKIN "basic_interface\\titlebar_mid.bmp",
                                         UI_SKIN "basic_interface\\titlebar_mid2.bmp"),
                            0);
    if (wnd->height == LIST_TOP)
        return;

    body = wnd->height - LIST_MARGIN;
    strips = body / 8 + (body % 8 != 0);
    for (i = 0, y = LIST_TOP; i < strips; i++, y += 8)
        ui_window_draw_bitmap(wnd, 0, y, ui_skin_bitmap(UI_SKIN "basic_interface\\itemwin_left.bmp"), 0);
    body = wnd->height - LIST_MARGIN;
    strips = body / 8 + (body % 8 != 0);
    for (i = 0, y = LIST_TOP; i < strips; i++, y += 8) {
        bitmap = ui_skin_bitmap(UI_SKIN "basic_interface\\itemwin_right.bmp");
        ui_window_draw_bitmap(wnd, wnd->width - bitmap->width, y, bitmap, 0);
    }
    ui_window_fill_rect(wnd, 0x14, LIST_TOP, (wnd->width - 0x3c) % 32, wnd->height - LIST_MARGIN,
                      ui_skin_colour(2, 2));
    body = wnd->height - LIST_MARGIN;
    ui_window_fill_rect(wnd, 0x14, body / 32 * 32 + LIST_TOP, wnd->width - 0x18, body % 32,
                      ui_skin_colour(2, 2));
    ui_window_draw_button_bar(wnd, 2, 0, 1);

    columns = (wnd->width - 0x28) / 32;
    rows = (wnd->height - LIST_MARGIN) / 32;
    for (i = 0, y = LIST_TOP; i < rows; i++, y += 32)
        ui_window_draw_item_win_mid(wnd, 0x28, y);
    ui_window_fill_rect(wnd, 0x48, LIST_TOP, columns * 32 - 0x20, rows * 32, ui_skin_colour(6, 2));

    if (self->kind == SKILL_LIST_KIND_HOMUN) {
        ui_window_text_out(wnd, 0x11, 4, msg_str(MSG_TITLE_HOMUN), 0, 0, 0xc, 0xffffff, 0, 0);
        ui_window_text_out(wnd, 0x10, 3, msg_str(MSG_TITLE_HOMUN), 0, 0, 0xc, 0, 0, 0);
        sprintf(buf, msg_str(MSG_SKILL_POINTS), g_session1.homunSkillPoints);
    } else if (self->kind == SKILL_LIST_KIND_MERCENARY) {
        ui_window_text_out(wnd, 0x11, 4, msg_str(MSG_TITLE_MERCENARY), 0, 0, 0xc, 0xffffff, 0, 0);
        ui_window_text_out(wnd, 0x10, 3, msg_str(MSG_TITLE_MERCENARY), 0, 0, 0xc, 0, 0, 0);
    }
    if (self->kind != SKILL_LIST_KIND_MERCENARY)
        ui_window_text_out(wnd, 0x10, wnd->height - 0x14, buf, 0, 0, 0xc, 0, 0, 0);

    drawn = 0;
    index = 0;
    for (node = self->skills.head->next; node != self->skills.head; node = node->next, index++) {
        struct SkillInfo *info = LIST_VALUE(node, struct SkillInfo);
        struct String name = {0};
        int grey;

        if (index < self->top * self->per_row)
            continue;
        x = drawn % self->per_row * 32;
        y = drawn / self->per_row * 32;
        if (self->selected == index) {
            COLORREF colour = ui_skin_colour(6, 2);

            if (info->level == 0)
                colour = ui_skin_colour(0xa, 2);
            else if (info->sp_cost == 0)
                colour = ui_skin_colour(0xe, 2);
            ui_window_fill_rect(wnd, x + 0x68, y + 0x13, 0xa4, 0x1c, colour);
        }
        session_format_item_bitmap_path(SKILL_INFO_NAME(info), buf);
        ui_window_draw_bitmap(wnd, x + 0x2c, y + 0x15, res_mgr_get(g_resMgr(), buf), 1);
        if (info->level) {
            int sp;

            if (skill_info_is_level_used(info->id)) {
                sprintf(buf, g_serviceType != 0xc ? "Lv :" : "Nv :");
                ui_window_text_out(wnd, x + 0x7c, y + 0x22, buf, 0, 0, 0xc, 0, 0, 0);
                level = chosen_level(self, info->id);
                if (level <= 0 || level > info->level)
                    level = info->level;
                sprintf(buf, "%2d", level);
                ui_window_text_out(wnd, x + 0x95, y + 0x22, buf, 0, 0, 0xc, level == info->level ? 0 : 0xff,
                                 0, 0);
                sprintf(buf, "/ %2d", info->level);
                ui_window_text_out(wnd, x + 0xa8, y + 0x22, buf, 0, 0, 0xc, 0, 0, 0);
            } else {
                sprintf(buf, g_serviceType != 0xc ? "Lv : %d" : "Nv : %d", info->level);
                ui_window_text_out(wnd, x + 0x71, y + 0x22, buf, 0, 0, 0xc, 0, 0, 0);
            }
            if (info->type == 0) {
                sprintf(buf, msg_str(MSG_PASSIVE), MSG_PASSIVE);
            } else {
                int ap;

                sp = info->sp_cost;
                if (skill_info_is_level_used(info->id)) {
                    int amount;

                    level = chosen_level(self, info->id);
                    amount = skill_info_get_sp_amount(info, level);
                    if (amount && level <= info->level)
                        sp = amount;
                }
                ap = skill_info_get_ap_amount(info, info->level);
                if (ap)
                    sprintf(buf, "Sp : %d , Ap : %d", sp, ap);
                else
                    sprintf(buf, "Sp : %d", sp);
            }
            ui_window_text_out(wnd, x + 0xd4, y + 0x22, buf, 0, 0, 0xc, 0, 0, 0);
            grey = 0;
        } else {
            grey = 0x78;
        }
        ui_window_fit_text(wnd, &name, wnd->width - (x + 0x70) - 0x14, skill_info_get_name(info), 0, 0xc,
                         0);
        ui_window_text_out(wnd, x + 0x70, y + 0x16, string_cstr(&name), 0, 0, 0xc, RGB(grey, grey, grey), 0,
                         0);
        string_free(&name);
        if (++drawn == self->rows * self->per_row)
            break;
    }
}

void ui_skill_list_wnd_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UISkillListWnd *self = (struct UISkillListWnd *)window;
    struct SkillInfo info;
    struct GameMode *game_mode;
    int old;

    if (y < LIST_TOP) {
        ui_frame_wnd_on_l_btn_down((struct UIWindow *)(&self->frame), x, y);
        return;
    }
    old = self->selected;
    self->selected = hit_row(self, x, y);
    if (old != self->selected)
        ui_window_invalidate_base(&self->frame.base);
    entry_at(self, &info, x, y);
    if (info.found && info.level && info.type) {
        game_mode = game_current_mode();
        if (game_mode)
            game_mode_drag_by_name(game_mode, DRAG_KIND_SKILL, info.id, SKILL_INFO_NAME(&info),
                                session_get_skill_level(info.id), -1);
    }
    skill_info_destruct(&info);
}

void ui_skill_list_wnd_on_l_btn_dbl_clk(struct UIWindow *window, int x, int y)
{
    struct UISkillListWnd *self = (struct UISkillListWnd *)window;
    struct SkillInfo info;

    if (y < LIST_TOP) {
        ui_frame_wnd_on_l_btn_down((struct UIWindow *)(&self->frame), x, y);
        return;
    }
    if (self->selected == -1)
        return;
    get_entry(self, &info, self->selected);
    if (info.found && info.level && info.sp_cost)
        ui_skill_list_wnd_send_msg(&self->frame.base, NULL, SKILL_LIST_MSG_USE, (int)&info, 0, 0, 0);
    skill_info_destruct(&info);
}

void ui_skill_list_wnd_on_mouse_hover(struct UIWindow *window, int x, int y)
{
    struct UISkillListWnd *self = (struct UISkillListWnd *)window;
    struct SkillInfo info;
    int index;

    check_wait(self);
    index = hit_row(self, x, y);
    get_entry(self, &info, index);
    if (strlen(skill_info_get_name(&info)) != 0) {
        int tip_x = 0x70;
        int tip_y = (index - self->top) * ROW_HEIGHT + LIST_TOP;
        int room = WND(self)->width - 0x84;

        if (ui_window_text_width(WND(self), skill_info_get_name(&info), 0, 0, 0xc, 0, 0) > room
            && x > tip_x) {
            ui_window_to_screen(WND(self), &tip_x, &tip_y);
            mode_show_tooltip(client_current_mode(), skill_info_get_name(&info), tip_x - 8, tip_y, 0xffffffff, 0);
        }
    }
    skill_info_destruct(&info);
}

void ui_skill_list_wnd_on_r_btn_down(struct UIWindow *window, int x, int y)
{
    struct UISkillListWnd *self = (struct UISkillListWnd *)window;
    int old = self->selected;

    self->selected = hit_row(self, x, y);
    if (old != self->selected)
        ui_window_invalidate_base(&self->frame.base);
}

void ui_skill_list_wnd_on_r_btn_up(struct UIWindow *window, int x_unused, int y_unused)
{
    struct UISkillListWnd *self = (struct UISkillListWnd *)window;
    ui_skill_list_wnd_send_msg(&self->frame.base, NULL, SKILL_LIST_MSG_MENU, 0, 0, 0, 0);
}

void ui_skill_list_wnd_on_wheel(struct UIWindow *window, int delta)
{
    struct UISkillListWnd *self = (struct UISkillListWnd *)window;
    int step;

    if (!self->scrollable)
        return;
    step = self->rows <= 3 ? self->rows - 1 : 3;
    if (delta >= 0)
        step = -step;
    ui_skill_list_wnd_send_msg(&self->frame.base, &self->bar->base, UI_MSG_SCROLL_V, step, 0, 0, 0);
}

static void resize(struct UISkillListWnd *self, int width, int height)
{
    struct UIWindow *wnd = WND(self);
    int i, y;

    if (width > MAX_WIDTH)
        width = MAX_WIDTH;
    if (height > MAX_HEIGHT)
        height = MAX_HEIGHT;
    if (width < MIN_WIDTH)
        width = MIN_WIDTH;
    if (height < MIN_HEIGHT)
        height = MIN_HEIGHT;
    width = (width - BASE_WIDTH) / 32 * 32 + BASE_WIDTH;
    height = (height - MIN_HEIGHT) / 32 * 32 + MIN_HEIGHT;
    if (width != wnd->width || height != wnd->height) {
        ui_window_resize_base(&self->frame.base, width, height);
        self->per_row = 1;
        self->rows = (height - LIST_MARGIN) / ROW_HEIGHT;
        ui_window_move_base(&self->resizer->base, wnd->width - 0xf, wnd->height - 0xf);
        ui_window_move_base(&self->use_button->button.base, wnd->width - 0x6b, wnd->height - 0x18);
        ui_window_move_base(&self->close_button->button.base, wnd->width - 0x3d, wnd->height - 0x18);
        ui_window_move_base(&self->bar->base, wnd->width - 0xf, 0xe);
        ui_scroll_bar_resize(&self->bar->base, self->bar->base.width, wnd->height - LIST_MARGIN);
        for (i = 0, y = LIST_TOP; y < 0xf1; i++, y += ROW_HEIGHT) {
            if (i < self->rows && (unsigned)(self->top + i) < self->skills.size)
                ui_window_move_base(&self->up_buttons[i]->button.base, 0x35, y);
            else
                ui_window_move_base(&self->up_buttons[i]->button.base, HIDDEN, y);
        }
        refresh_range(self);
        place_buttons(self);
    }
    ui_window_mgr_update_snap_rect(&g_windowMgr, wnd);
}

static void rebuild(struct UISkillListWnd *self)
{
    int count = 0;
    BYTE added = 0;
    int i;

    list_clear(&self->skills, skill_info_destroy);
    if (self->kind == SKILL_LIST_KIND_HOMUN)
        count = ((int)g_session1.homunSkills.size);
    else if (self->kind == SKILL_LIST_KIND_MERCENARY)
        count = ((int)g_session1.merSkills.size);
    for (i = 0; i < count; i++) {
        struct SkillInfo info, tmp;

        skill_info_construct(&info);
        if (self->kind == SKILL_LIST_KIND_HOMUN) {
            skill_info_assign(&info, session_get_homun_skill_at(&g_session1, &tmp, i));
            skill_info_destruct(&tmp);
        } else if (self->kind == SKILL_LIST_KIND_MERCENARY) {
            skill_info_assign(&info, session_get_mer_skill_at(&g_session1, &tmp, i));
            skill_info_destruct(&tmp);
        }
        if (info.found) {
            if (SKILL_INFO_UPGRADABLE(&info) || info.level) {
                struct SkillInfo copy;

                skill_info_copy(&copy, &info);
                list_push_back(&self->skills, &copy, sizeof copy);
                added++;
            }
            if (skill_info_is_level_used(info.id) && info.level) {
                int level = chosen_level(self, info.id);

                if (level <= 0 || level > info.level)
                    set_chosen_level(self, info.id, info.level);
            }
        }
        skill_info_destruct(&info);
    }
    if (added != g_skillListRowCount) {
        g_skillListRowCount = added;
        if (g_skillListRowsChangedTime)
            g_skillPointPending = 1;
        g_skillListRowsChangedTime = timeGetTime();
    }
    ui_window_invalidate_base(&self->frame.base);
    refresh_range(self);
    place_buttons(self);
}

static void open_menu(struct UISkillListWnd *self)
{
    struct UIWindow *menu;
    unsigned i;
    int x, y, max;

    ui_menu_entry_vec_clear(&self->menu);
    if (g_windowMgr.menu_wnd)
        ui_close_window(WND_ID_MENU);
    if (self->selected != -1) {
        struct SkillInfo info;

        get_entry(self, &info, self->selected);
        if (info.found) {
            struct String text;
            struct UIMenuEntry item;

            string_init(&text);
            string_assign_cstr(&text, msg_str(MSG_SKILL_DESCRIPTION));
            ui_menu_entry_construct(&item, string_cstr(&text), MENU_ACTION_DESCRIBE);
            ui_menu_entry_vec_push_move(&self->menu, &item);
            string_free(&item.text);
            string_free(&text);
        }
        skill_info_destruct(&info);
    }
    if (VEC_COUNT(&self->menu, struct UIMenuEntry) == 0)
        return;
    menu = ui_make_window(WND_ID_MENU);
    for (i = 0; i < VEC_COUNT(&self->menu, struct UIMenuEntry); i++)
        ui_window_send_msg(menu, NULL, MSG_MENU_ADD_ITEM,
                 (int)string_cstr(&VEC_AT(&self->menu, struct UIMenuEntry, i).text), 0, 0, 0);
    if (self->kind == SKILL_LIST_KIND_HOMUN)
        ui_window_send_msg(menu, NULL, MSG_MENU_SET_OWNER, WND_ID_HOMUN_SKILL_LIST, 0, 0, 0);
    else if (self->kind == SKILL_LIST_KIND_MERCENARY)
        ui_window_send_msg(menu, NULL, MSG_MENU_SET_OWNER, WND_ID_MER_SKILL_LIST, 0, 0, 0);
    ui_window_send_msg(menu, NULL, MSG_MENU_FIT, 0, 0, 0, 0);
    x = g_mouse.x + 1;
    if (x < 1)
        x = 1;
    max = g_renderer->width - menu->width;
    if (x > max)
        x = max;
    y = g_mouse.y + 1;
    if (y < 1)
        y = 1;
    max = g_renderer->height - menu->height;
    if (y > max)
        y = max;
    ui_window_move(menu, x, y);
}

static void menu_choice(struct UISkillListWnd *self, int choice)
{
    struct SkillInfo info;

    if (self->selected == -1)
        return;
    get_entry(self, &info, self->selected);
    if (info.found) {
        int action = VEC_AT(&self->menu, struct UIMenuEntry, choice).id;

        if (action == MENU_ACTION_DESCRIBE)
            ui_window_send_msg(ui_make_window(WND_ID_SKILL_DESCRIBE), NULL, SKILL_DESCRIBE_MSG_SHOW, info.id,
                     0, 0, 0);
        else if (action == MENU_ACTION_RENAME)
            ui_window_send_msg(ui_make_window(WND_ID_SKILL_NAME_CHANGE), NULL, SKILL_NAME_MSG_OPEN, info.id, 0,
                     0, 0);
    }
    skill_info_destruct(&info);
}

static void skill_up(struct UISkillListWnd *self, int row)
{
    struct SkillInfo info;

    if (self->kind == SKILL_LIST_KIND_HOMUN) {
        if (!g_session1.homunSkillPoints)
            return;
    } else if (self->kind == SKILL_LIST_KIND_MERCENARY) {
        if (!g_session1.merSkillPoints)
            return;
    }
    if (!g_skillPointPending) {
        get_entry(self, &info, self->top + row);
        g_skillListUpSkillId = info.id;
        if (info.found)
            mode_send_message(client_current_mode(), MODE_REQ_SKILL_UP, info.id, 0, 0, 0);
    } else {
        get_entry(self, &info, self->top + row);
        if (g_skillListUpSkillId == info.id && info.found)
            mode_send_message(client_current_mode(), MODE_REQ_SKILL_UP, info.id, 0, 0, 0);
    }
    skill_info_destruct(&info);
}

static void step_level(struct UISkillListWnd *self, int row, int step)
{
    struct SkillInfo info;

    get_entry(self, &info, self->top + row);
    if (info.found) {
        int level = chosen_level(self, info.id);

        if (step < 0)
            level = level - 1 > 0 ? level - 1 : 1;
        else
            level = level >= 0 ? level + 1 : 1;
        if (level > info.level)
            level = info.level;
        if (self->kind == SKILL_LIST_KIND_HOMUN) {
            session_set_homun_skill_use_level(&g_session1, info.id, level);
        } else if (self->kind == SKILL_LIST_KIND_MERCENARY) {
            session_set_merc_skill_use_level(&g_session1, info.id, level);
        }
        ui_window_invalidate_base(&self->frame.base);
    }
    skill_info_destruct(&info);
}

static void command(struct UISkillListWnd *self, int id)
{
    struct SkillInfo info;

    if (id == ID_USE) {
        if (self->selected == -1)
            return;
        get_entry(self, &info, self->selected);
        if (info.found && info.level && info.sp_cost)
            ui_skill_list_wnd_send_msg(&self->frame.base, NULL, SKILL_LIST_MSG_USE, (int)&info, 0, 0, 0);
        skill_info_destruct(&info);
    } else if (id == UI_ID_CLOSE) {
        if (self->kind == SKILL_LIST_KIND_HOMUN)
            ui_close_window(WND_ID_HOMUN_SKILL_LIST);
        else if (self->kind == SKILL_LIST_KIND_MERCENARY)
            ui_close_window(WND_ID_MER_SKILL_LIST);
    } else if (id >= ID_UP_FIRST && id < ID_UP_FIRST + ROWS_SHOWN) {
        skill_up(self, id - ID_UP_FIRST);
    } else if (id >= ID_DOWN_FIRST && id < ID_DOWN_FIRST + ROWS_SHOWN) {
        step_level(self, id - ID_DOWN_FIRST, -1);
    } else if (id >= ID_RAISE_FIRST && id < ID_RAISE_FIRST + ROWS_SHOWN) {
        step_level(self, id - ID_RAISE_FIRST, 1);
    }
}

int ui_skill_list_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1,
                                      int val2, int val3, int val4)
{
    struct UISkillListWnd *self = (struct UISkillListWnd *)window;
    check_wait(self);
    switch (msg) {
    case UI_MSG_COMMAND:
        command(self, val1);
        return 0;
    case UI_MSG_SCROLL_V:
    case UI_MSG_PAGE_UP:
    case UI_MSG_PAGE_DOWN: {
        int old = self->top;

        if (msg == UI_MSG_PAGE_UP)
            self->top = old - self->rows + 1;
        else if (msg == UI_MSG_PAGE_DOWN)
            self->top = self->rows - 1 + old;
        else
            self->top = old + val1;
        if (old != self->top) {
            refresh_range(self);
            place_buttons(self);
            ui_window_invalidate_base(&self->frame.base);
        }
        return self->top - old;
    }
    case UI_MSG_RESIZE:
        if (val1 == UI_EDGE_BOTTOM)
            resize(self, val2, val3);
        return 0;
    case SKILL_LIST_MSG_REBUILD:
        rebuild(self);
        return 0;
    case UI_MSG_SET_INFO:
        self->info = (struct WindowInfo *)val1;
        if (!skill_list_info_is_valid(self->info))
            skill_list_info_set_default(self->info);
        ui_frame_wnd_move(&self->frame.base, self->info->x, self->info->y);
        ui_skill_list_wnd_send_msg(&self->frame.base, NULL, UI_MSG_RESIZE, UI_EDGE_BOTTOM, self->info->width, self->info->height, 0);
        return 0;
    case MSG_MENU_CHOICE:
        menu_choice(self, val1);
        return 0;
    case SKILL_LIST_MSG_MENU:
        open_menu(self);
        return 0;
    case SKILL_LIST_MSG_USE: {
        struct SkillInfo *info = (struct SkillInfo *)val1;

        if (info->level)
            mode_send_message(client_current_mode(), MODE_REQ_USE_SKILL, (int)info, chosen_level(self, info->id), 0, 0);
        return 0;
    }
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
}

int skill_list_info_is_valid(struct WindowInfo *self)
{
    return self->width >= 0x118 && self->height >= MIN_HEIGHT && self->x >= 0
           && self->x <= g_renderer->width - self->width && self->y >= 0
           && self->y <= g_renderer->height - self->height;
}

void skill_list_info_set_default(struct WindowInfo *self)
{
    self->x = 0x168;
    self->y = 0xc8;
    self->width = 0x118;
    self->height = 0xf0;
}

void ui_skill_list_set_kind(struct UIWindow *wnd, int kind)
{
    ((struct UISkillListWnd *)wnd)->kind = kind;
}
