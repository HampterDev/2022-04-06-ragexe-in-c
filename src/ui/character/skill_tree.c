#include "ui/character/skill_tree.h"
#include "render/rp_pool.h"
#include "skills/skill_info.h"
#include "contents/client_lua_calls.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "core/object_type_tables.h"
#include "world/mode_manager.h"
#include "game/context.h"
#include "ui/window_access.h"
#include <windows.h>
#include <mmsystem.h>
#include "ui/character/skill_tree_internal.h"
#include "ui/chat/chat_window_parts.h"
#include "core/msg_str.h"
#include "diag/perf.h"
#include "session/session_jobs.h"
#include "session/session_options.h"
#include "session/skill_levels.h"
#include "ui/ui_std.h"
#include "ui/wndmgr/wndmgr.h"

int g_skillWndHeadDx;
int g_skillWndHeadDy;

struct UINewSkillListWnd;
struct UIWindow;
void ui_new_skill_list_wnd_destroy(struct UIWindow *window);
void ui_new_skill_list_wnd_store_info(struct UIWindow *self);
void ui_new_skill_list_wnd_on_create(struct UIWindow *self, int width_unused, int height_unused);
void ui_new_skill_list_wnd_on_l_btn_down(struct UIWindow *self, int x, int y);
void ui_new_skill_list_wnd_on_l_btn_dbl_clk(struct UIWindow *self, int x, int y);
void ui_new_skill_list_wnd_on_mouse_hover(struct UIWindow *self, int x, int y);
void ui_new_skill_list_wnd_on_l_btn_up(struct UIWindow *self, int x, int y);
void ui_new_skill_list_wnd_on_r_btn_up(struct UIWindow *self, int x, int y);
void ui_new_skill_list_wnd_on_wheel(struct UIWindow *self, int delta);
int ui_new_skill_list_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_new_skill_list_wnd_ops = {
    .destroy = ui_new_skill_list_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_new_skill_list_wnd_store_info,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_new_skill_list_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_new_skill_list_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_new_skill_list_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_new_skill_list_wnd_on_l_btn_dbl_clk,
    .on_mouse_move = ui_frame_wnd_on_mouse_move,
    .on_mouse_hover = ui_new_skill_list_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_new_skill_list_wnd_on_l_btn_up,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_new_skill_list_wnd_on_r_btn_up,
    .on_wheel = ui_new_skill_list_wnd_on_wheel,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_new_skill_list_wnd_send_msg,
    .invalidate = ui_window_invalidate_base,
    .render = ui_new_skill_list_wnd_render,
    .has_custom_backdrop = 0,
    .draw_backdrop = ui_window_draw_backdrop_base,
    .type = UI_TYPE_FRAME,
    .on_hover_enter = ui_window_on_hover_enter_base,
    .on_hover_leave = ui_window_on_hover_leave_base,
    .make_quad = ui_window_make_quad_base,
    .hit_test = ui_window_hit_test_base,
    .render_overlay = ui_window_render_overlay_base,
};

#define WND_ID_SKILL_LIST 0x25
#define WND_ID_SKILL_DESCRIBE 0x2e
#define SKILL_DESCRIBE_MSG_SHOW 0x3d
#define ID_INFO_CHECK 0xcd
#define ID_NEXT_TAB 0xd0
#define ID_UP_FIRST 0xe7
#define ID_APPLY 0x107
#define ID_RESET 0x163
#define ID_RAISE_FIRST SKILL_TREE_CELLS
#define MSG_TAB_SELECTED 0x16
#define MSG_REFRESH 0x17
#define MSG_USE_SKILL 0x3a
#define MSG_SETUP_TABS 0x3c
#define MSG_NEW_MARK 0xa1
#define MODE_REQ_USE_SKILL 0x71
#define DRAG_KIND_SKILL 8

#define MSG_NO_TABS 0x445
#define MSG_SHOW_INFO 0x560
#define MSG_APPLY_QUESTION 0x561

int skill_tree_tab_count(const struct UINewSkillListWnd *self)
{
    return (int)VEC_COUNT(&self->tabs->names, struct String);
}

struct List *skill_tree_shown_list(struct UINewSkillListWnd *self)
{
    if (skill_tree_tab_count(self) - 1 == self->tab)
        return &g_session1.skills.extra_skills;
    return &self->tab_skills[self->tab];
}

struct UINewSkillListWnd *ui_new_skill_list_wnd_construct(struct UINewSkillListWnd *self)
{
    int i;

    ui_frame_wnd_construct(&self->frame, 0);
    WND(self)->ops = &ui_new_skill_list_wnd_ops;
    for (i = 0; i < SKILL_TREE_TABS; i++)
        list_init(&self->tab_skills[i]);
    list_init(&self->required);
    list_init(&self->planned);
    self->tab = 0;
    self->dirty = 1;
    self->hover = -1;
    self->anim_time = timeGetTime();
    self->anim_frame_counts[0] = self->anim_frame_counts[1] = 0;
    self->anim_frames[0] = self->anim_frames[1] = 0;
    self->drawn_key = 0;
    self->top_margin = 0x14;
    self->bottom_margin = 0x20;
    self->info = NULL;
    self->columns = 1;
    self->rows = 1;
    self->show_required = 0;
    self->required_cell = -1;
    return self;
}

struct UIWindow *ui_new_skill_list_wnd_new(void)
{
    return (struct UIWindow *)ui_new_skill_list_wnd_construct(mem_alloc(sizeof(struct UINewSkillListWnd)));
}

static void destruct(struct UINewSkillListWnd *self)
{
    int i;

    WND(self)->ops = &ui_new_skill_list_wnd_ops;
    list_free(&self->planned, NULL);
    list_free(&self->required, NULL);
    for (i = SKILL_TREE_TABS - 1; i >= 0; i--)
        list_free(&self->tab_skills[i], skill_info_destroy);
    ui_frame_wnd_destruct(&self->frame);
}

void ui_new_skill_list_wnd_destroy(struct UIWindow *window)
{
    struct UINewSkillListWnd *self = (struct UINewSkillListWnd *)window;
    destruct(self);
    mem_free(self);
}

struct SkillRequirement *skill_tree_level_list_find(struct List *list, int skill_id)
{
    struct ListNode *node;

    for (node = list->head->next; node != list->head; node = node->next)
        if (LIST_VALUE(node, struct SkillRequirement)->skill_id == skill_id)
            return LIST_VALUE(node, struct SkillRequirement);
    return NULL;
}

static struct List *tab_list(struct UINewSkillListWnd *self, int tab, BYTE session)
{
    if (skill_tree_tab_count(self) - 1 == tab)
        return &g_session1.skills.extra_skills;
    if (session)
        return player_skill_bundle_tree_tab(&g_session1.skills, tab);
    return &self->tab_skills[tab];
}

BYTE skill_tree_is_last_tab(struct UINewSkillListWnd *self, int tab)
{
    return skill_tree_tab_count(self) - 1 == tab;
}

struct SkillInfo *skill_tree_skill_at(struct UINewSkillListWnd *self, struct SkillInfo *out, int cell, BYTE session)
{
    struct List *list;
    struct ListNode *node;
    int row = 0;

    skill_info_construct_unknown(out);
    list = tab_list(self, self->tab, session);
    for (node = list->head->next; node != list->head;) {
        struct SkillInfo *skill = LIST_VALUE(node, struct SkillInfo);
        int key = g_session1.simpleSkillWnd ? row : SKILL_INFO_SLOT(skill);

        if (key == cell) {
            skill_info_assign(out, skill);
            break;
        }
        node = node->next;
        if (g_session1.simpleSkillWnd)
            row++;
    }
    return out;
}

static struct SkillInfo *list_row(struct UINewSkillListWnd *self, struct SkillInfo *out, int row)
{
    struct List *list = skill_tree_shown_list(self);
    struct ListNode *node;
    int i = 0;

    for (node = list->head->next; node != list->head; node = node->next, i++) {
        if (row + self->top == i) {
            skill_info_copy(out, LIST_VALUE(node, struct SkillInfo));
            return out;
        }
    }
    skill_info_construct_unknown(out);
    return out;
}

static BYTE in_cells(struct UINewSkillListWnd *self, int x, int y)
{
    int cell_h = CELL_H, left = 0x4c, extra = 0x28;
    BYTE inside = 1, hit = 0;

    if (g_session1.simpleSkillWnd) {
        cell_h = ROW_H;
        left = 0x28;
        extra = 0x1e;
    }
    if (y < self->top_margin || y >= self->rows * cell_h + 4 + extra)
        inside = 0;
    if (x >= left)
        hit = inside;
    if (!g_session1.simpleSkillWnd && x > 0x1f8)
        hit = 0;
    return hit;
}

static int cell_at(struct UINewSkillListWnd *self, int x, int y)
{
    int column = 0, row;

    if (!in_cells(self, x, y))
        return -1;
    if (g_session1.simpleSkillWnd) {
        row = (y - 0x22) / ROW_H;
    } else {
        column = (x - 0x4c) / CELL_W;
        row = (y - 0x2c) / CELL_H;
        if (x > column * CELL_W + 0x64)
            return -1;
        if (y > row * CELL_H + 0x3a)
            return -1;
    }
    return (self->top + row) * self->columns + column;
}

static void layout_grid(struct UINewSkillListWnd *self)
{
    struct UIWindow *wnd = WND(self);
    struct List *list;
    int rows, total, rem, excess;

    if (g_session1.simpleSkillWnd) {
        self->columns = 1;
        self->rows = (wnd->height - self->bottom_margin - 0x1e) / ROW_H;
    } else {
        self->columns = 7;
        self->rows = 6;
    }
    if (!self->scroll)
        return;
    ui_window_move_base(&self->scroll->base, wnd->width - 0xf, 0x14);
    rows = self->rows;
    list = skill_tree_shown_list(self);
    if (g_session1.simpleSkillWnd) {
        total = list->size / (unsigned)self->columns;
        rem = list->size % (unsigned)self->columns;
    } else {
        struct ListNode *node;
        int last = 0;

        for (node = list->head->next; node != list->head; node = node->next) {
            int slot = SKILL_INFO_SLOT(LIST_VALUE(node, struct SkillInfo));

            last = slot > last ? slot : last;
        }
        total = (last + 1) / self->columns;
        rem = (last + 1) % self->columns;
    }
    total += rem != 0;
    excess = total - rows > 0 ? total - rows : 0;
    self->scrollable = excess != 0;
    if (excess) {
        ui_scroll_bar_set_range(self->scroll, excess, rows, total);
        if (self->top < 0)
            self->top = 0;
        if (self->top > total - self->rows)
            self->top = total - self->rows;
        ui_scroll_bar_set_pos(self->scroll, self->top);
    } else {
        self->top = 0;
        ui_scroll_bar_set_range(self->scroll, 0, rows, total);
    }
    ui_window_invalidate_base(&self->scroll->base);
    self->dirty = 1;
}

#define HIDE(w) ui_window_move_base(UI_WINDOW((w)), -200, -200)

void skill_tree_layout_buttons(struct UINewSkillListWnd *self)
{
    struct UIWindow *wnd = WND(self);
    struct List *list;
    int inset = 0, i;

    if (g_session1.simpleSkillWnd) {
        inset = 0xf;
        self->hover = -1;
        ui_window_move_base(&self->resizer->base, wnd->width - inset, wnd->height - inset);
    } else {
        ui_window_move_base(&self->resizer->base, UI_POS_HIDDEN, UI_POS_HIDDEN);
    }
    ui_window_move_base(self->mini_button, wnd->width - 0x1e, 5);
    ui_window_move_base(self->close_button, wnd->width - 0x10, 5);
    ui_window_move_base(&self->apply_button->button.base, wnd->width - inset - 0x64, wnd->height - 0x1b);
    ui_window_move_base(&self->reset_button->button.base, wnd->width - inset - 0x32, wnd->height - 0x1b);
    self->apply_button->button.disabled = self->pending == 0;
    ui_window_invalidate_base(&self->apply_button->button.base);
    self->reset_button->button.disabled = self->pending == 0;
    ui_window_invalidate_base(&self->reset_button->button.base);
    if (!self->pending) {
        ui_window_set_state(&self->apply_button->button.base, 0);
        ui_window_set_state(&self->reset_button->button.base, 0);
    }
    ui_window_move_base(self->info_check, wnd->width - 0x82, 5);
    for (i = 0; i < SKILL_TREE_UP_BUTTONS; i++)
        HIDE(self->up_buttons[i]);
    for (i = 0; i < SKILL_TREE_CELLS; i++) {
        HIDE(self->lower_buttons[i]);
        HIDE(self->raise_buttons[i]);
    }
    list = skill_tree_shown_list(self);
    if (g_session1.simpleSkillWnd) {
        int y;

        for (i = 0, y = 0x30; y < 0x170; i++, y += ROW_H) {
            struct SkillInfo info;

            list_row(self, &info, i);
            if (info.level < SKILL_INFO_MAX_LEVEL(&info) && i < self->rows
                && (unsigned)(self->top + i) < list->size && self->points_left && info.found
                && SKILL_INFO_UPGRADABLE(&info))
                ui_window_move_base(&self->up_buttons[i]->button.base, 0x4e, y - 0xe);
            if (!self->pending && i < self->rows && (unsigned)(self->top + i) < list->size && info.found
                && info.level && skill_info_is_level_used(info.id)) {
                ui_window_move_base(&self->lower_buttons[i]->button.base, 0x70, y);
                ui_window_move_base(&self->raise_buttons[i]->button.base, 0xc0, y);
            }
            skill_info_destruct(&info);
        }
        return;
    }
    if (!self->pending) {
        struct ListNode *node;

        for (node = list->head->next; node != list->head; node = node->next) {
            const struct SkillInfo *skill = LIST_VALUE(node, struct SkillInfo);
            int slot = SKILL_INFO_SLOT(skill);
            unsigned cell = slot - self->columns * self->top;
            int x, y;

            if (cell > SKILL_TREE_CELLS - 1 || !skill->found || slot == -1 || !skill->level
                || !skill_info_is_level_used(skill->id))
                continue;
            y = (int)cell / self->columns * CELL_H + 0x46;
            x = (int)cell % self->columns * CELL_W + 0x38;
            ui_window_move_base(&self->raise_buttons[cell]->button.base, x + 0x37, y);
            ui_window_move_base(&self->lower_buttons[cell]->button.base, x, y);
        }
    }
}

static const BYTE g_jobTabIndex[0xfa] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 27, 12, 13, 14,
    15, 16, 17, 18, 27, 19, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
    10, 11, 27, 12, 13, 14, 15, 16, 17, 18, 27, 20, 21, 22, 27, 23,
    27, 27, 27, 27, 6, 8, 10, 7, 9, 11, 6, 8, 10, 7, 9, 11,
    12, 14, 17, 18, 13, 16, 15, 12, 14, 17, 18, 13, 16, 15, 27, 27,
    27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 6, 8,
    10, 7, 9, 11, 12, 14, 17, 18, 13, 16, 15, 27, 27, 27, 27, 27,
    27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
    27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
    27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
    27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
    27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 20, 20, 27, 27,
    27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
    27, 24, 24, 27, 27, 25, 27, 27, 26, 27, 26, 27, 24, 24, 24, 21,
    22, 23, 25, 25, 27, 27, 27, 27, 27, 27, 27, 27, 27, 22, 23, 22,
    23, 27, 27, 27, 27, 27, 27, 27, 27, 27,
};
#define JOB_TAB_KEEP 27
static const int g_jobTabValue[JOB_TAB_KEEP] = {
    1, 2, 3, 4, 5, 6, 7, 8, 9, 0xa, 0xb, 0xc, 0xe, 0xf, 0x10, 0x11, 0x12, 0x13, 0x14, 0,
    0x17, 0xfce, 0xfcf, 0xfd1, 0x19, 0x18, 0x107a,
};

static int job_tab_table(int job)
{
    if (job > 0xfa1) {
        unsigned i = job - 0xfa2;

        if (i <= 0xf9 && g_jobTabIndex[i] != JOB_TAB_KEEP)
            return g_jobTabValue[g_jobTabIndex[i]];
        return job;
    }
    if (job == 0xfa1)
        return 0;
    return job;
}

static void tab_names(int table, const char *names[4])
{
    client_lua_call(g_session1.luaState, "JobSkillTab_GetTabName", "d>ssss", table, &names[0], &names[1], &names[2], &names[3]);
}

static void setup_tabs(struct UINewSkillListWnd *self)
{
    const char *base[4] = { NULL, NULL, NULL, NULL };
    const char *own[4] = { NULL, NULL, NULL, NULL };
    int count = skill_tree_tab_count(self), i;

    tab_names(0, base);
    tab_names(job_tab_table(session_get_job_without_mount(&g_session1, session_get_drawn_job(&g_session1))), own);
    for (i = skill_tree_tab_count(self); i < SKILL_TREE_TABS; i++) {
        if (player_skill_bundle_tree_tab_size(&g_session1.skills, i) > 0) {
            ui_tab_control_add_tab(&self->tabs->base, own[i] ? own[i] : base[i]);
            self->tab = i;
        }
    }
    if (count == 0)
        ui_tab_control_add_tab(&self->tabs->base, msg_str(MSG_NO_TABS));
    self->tabs->selected = self->tab;
}

void ui_new_skill_list_wnd_on_create(struct UIWindow *window, int width_unused, int height_unused)
{
    struct UINewSkillListWnd *self = (struct UINewSkillListWnd *)window;
    static const char *const names[2][3] = {
        { "btn_apply", "btn_apply_a", "btn_apply_b" },
        { "btn_reset", "btn_reset_a", "btn_reset_b" },
    };
    struct UIWindow *wnd = WND(self);
    struct UIWindow *tabs, *check;
    struct UIScrollBar *scroll;
    struct UIResizer *resizer;
    struct UIWindow **title_buttons;
    struct String tooltip, up_a, up_b, up_c, arrow, prefix;
    int xs[3], pos[2][2], ids[2], i, k, y;

    tabs = mem_alloc(sizeof(struct UITabControl));
    if (tabs)
        tabs = ui_tab_control_construct(tabs);
    self->tabs = (struct UITabControl *)tabs;
    ui_tab_control_setup(tabs, UI_TABS_LEFT, 0x11, 0xf, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    ui_window_move_base(&self->tabs->base, 0, 6);
    ui_window_add_child(wnd, &self->tabs->base);

    string_init(&tooltip);
    self->sys_button = ui_window_add_sys_button(wnd);
    self->mini_button = ui_window_add_mini_button(wnd, string_cstr(&tooltip), 0);
    self->close_button = ui_window_add_close_button(wnd, "Alt+S", 0);
    xs[0] = 0x16;
    xs[1] = wnd->width - 0x1e;
    xs[2] = wnd->width - 0x10;
    title_buttons = &self->sys_button;
    for (i = 0; i < 3; i++)
        ui_window_move(title_buttons[i], xs[i], 5);
    string_free(&tooltip);

    scroll = mem_alloc(sizeof(struct UIScrollBar));
    if (scroll)
        scroll = ui_scroll_bar_construct(scroll);
    self->scroll = scroll;
    ui_scroll_bar_create(scroll, wnd->width - 0xf, 0x14, UI_SCROLL_VERT,
                       wnd->height - self->top_margin - self->bottom_margin);
    ui_window_move_base(&self->scroll->base, wnd->width - 0xf, 0x14);
    ui_scroll_bar_resize(&self->scroll->base, self->scroll->base.width, wnd->height - self->top_margin - self->bottom_margin);
    ui_window_add_child(wnd, &self->scroll->base);

    string_init(&up_a);
    string_assign(&up_a, BASIC_INTERFACE "skill_up_a.bmp", 0x2d);
    string_init(&up_b);
    string_assign(&up_b, BASIC_INTERFACE "skill_up_b.bmp", 0x2d);
    string_init(&up_c);
    string_assign(&up_c, BASIC_INTERFACE "skill_up_c.bmp", 0x2d);
    for (i = 0, y = 0x15; y < 0x155; i++, y += ROW_H) {
        struct UIBitmapButton *btn = mem_alloc(sizeof(struct UIBitmapButton));

        if (btn)
            btn = ui_bitmap_button_construct(btn);
        self->up_buttons[i] = btn;
        ui_bitmap_button_set_bitmap(btn, string_cstr(&up_a), UI_BTN_UP, 0);
        ui_bitmap_button_set_bitmap(self->up_buttons[i], string_cstr(&up_b), UI_BTN_HOVER, 0);
        ui_bitmap_button_set_bitmap(self->up_buttons[i], string_cstr(&up_c), UI_BTN_DOWN, 0);
        btn = self->up_buttons[i];
        ui_window_create(&btn->button.base, btn->bitmap_width, btn->bitmap_height);
        ui_window_move_base(&self->up_buttons[i]->button.base, 0x4e, y);
        self->up_buttons[i]->button.base.id = ID_UP_FIRST + i;
        ui_window_add_child(wnd, &self->up_buttons[i]->button.base);
    }
    string_free(&up_c);
    string_free(&up_b);
    string_free(&up_a);

    string_init(&arrow);
    string_assign(&arrow, BASIC_INTERFACE "arw_left.bmp", 0x2b);
    for (i = 0, y = 0x22; y < 0x562; i++, y += ROW_H) {
        struct UIBitmapButton *btn = mem_alloc(sizeof(struct UIBitmapButton));

        if (btn)
            btn = ui_bitmap_button_construct(btn);
        self->lower_buttons[i] = btn;
        ui_bitmap_button_set_bitmap(btn, string_cstr(&arrow), UI_BTN_UP, 0);
        ui_bitmap_button_set_bitmap(self->lower_buttons[i], string_cstr(&arrow), UI_BTN_HOVER, 0);
        ui_bitmap_button_set_bitmap(self->lower_buttons[i], string_cstr(&arrow), UI_BTN_DOWN, 0);
        btn = self->lower_buttons[i];
        ui_window_create(&btn->button.base, btn->bitmap_width, btn->bitmap_height);
        ui_window_move_base(&self->lower_buttons[i]->button.base, 0x6a, y);
        self->lower_buttons[i]->button.base.id = i;
        ui_window_add_child(wnd, &self->lower_buttons[i]->button.base);
    }
    string_free(&arrow);

    string_init(&arrow);
    string_assign(&arrow, BASIC_INTERFACE "arw_right.bmp", 0x2c);
    for (i = 0, y = 0x22; y < 0x562; i++, y += ROW_H) {
        struct UIBitmapButton *btn = mem_alloc(sizeof(struct UIBitmapButton));

        if (btn)
            btn = ui_bitmap_button_construct(btn);
        self->raise_buttons[i] = btn;
        ui_bitmap_button_set_bitmap(btn, string_cstr(&arrow), UI_BTN_UP, 0);
        ui_bitmap_button_set_bitmap(self->raise_buttons[i], string_cstr(&arrow), UI_BTN_HOVER, 0);
        ui_bitmap_button_set_bitmap(self->raise_buttons[i], string_cstr(&arrow), UI_BTN_DOWN, 0);
        btn = self->raise_buttons[i];
        ui_window_create(&btn->button.base, btn->bitmap_width, btn->bitmap_height);
        ui_window_move_base(&self->raise_buttons[i]->button.base, 0xc6, y);
        self->raise_buttons[i]->button.base.id = ID_RAISE_FIRST + i;
        ui_window_add_child(wnd, &self->raise_buttons[i]->button.base);
    }
    string_free(&arrow);

    string_init(&prefix);
    string_assign(&prefix, UI_SKIN, 0xf);
    pos[0][0] = wnd->width - 0x64;
    pos[0][1] = wnd->height - 0x1b;
    pos[1][0] = wnd->width - 0x32;
    pos[1][1] = wnd->height - 0x1b;
    ids[0] = ID_APPLY;
    ids[1] = ID_RESET;
    for (k = 0; k < 2; k++) {
        struct UIBitmapButton **slot = k ? &self->reset_button : &self->apply_button;
        struct UIBitmapButton *btn = mem_alloc(sizeof(struct UIBitmapButton));
        int state;

        if (btn)
            btn = ui_bitmap_button_construct(btn);
        *slot = btn;
        for (state = UI_BTN_UP; state <= UI_BTN_DOWN; state++) {
            struct String path;

            string_copy(&path, &prefix);
            string_append_cstr(&path, names[k][state]);
            string_append_cstr(&path, ".bmp");
            ui_bitmap_button_set_bitmap(*slot, string_cstr(&path), state, 0);
            string_free(&path);
        }
        btn = *slot;
        ui_window_create(&btn->button.base, btn->bitmap_width, btn->bitmap_height);
        ui_window_move_base(&(*slot)->button.base, pos[k][0], pos[k][1]);
        (*slot)->button.base.id = ids[k];
        (*slot)->button.disabled = 1;
        ui_window_invalidate_base(&(*slot)->button.base);
        ui_window_add_child(wnd, &(*slot)->button.base);
    }
    string_free(&prefix);

    check = mem_alloc(sizeof(struct UICheckBox));
    if (check)
        check = &ui_check_box_construct(check)->base;
    self->info_check = check;
    ui_window_create(check, 0x5f, 0xc);
    ui_check_box_set_bitmaps(self->info_check, UI_SKIN "checkbox_1.bmp", UI_SKIN "checkbox_0.bmp",
                          msg_str(MSG_SHOW_INFO));
    ui_window_move_base(self->info_check, wnd->width - 0x82, 5);
    ui_check_box_set_check(self->info_check, g_session1.showSkillDescript);
    ui_window_add_child(wnd, self->info_check);

    resizer = mem_alloc(sizeof(struct UIResizer));
    if (resizer)
        resizer = ui_resizer_construct(resizer);
    self->resizer = resizer;
    ui_window_create(&resizer->base, 0x10, 0x10);
    ui_resizer_set_edge(self->resizer, UI_EDGE_BOTTOM);
    ui_resizer_set_bitmap(self->resizer, UI_SKIN "btn_resize.bmp");
    ui_window_move_base(&self->resizer->base, wnd->width - 0xf, wnd->height - 0xf);
    ui_resizer_set_colours(self->resizer, UI_TRANSPARENT, UI_TRANSPARENT);
    ui_window_add_child(wnd, &self->resizer->base);

    setup_tabs(self);
    if (g_session1.skills.extra_skill_ids.first != g_session1.skills.extra_skill_ids.last)
        ui_tab_control_set_new_mark(self->tabs, skill_tree_tab_count(self) - 1, 1);
    ui_new_skill_list_wnd_send_msg(wnd, NULL, MSG_REFRESH, 0, 0, 0, 0);
}

void ui_new_skill_list_wnd_on_l_btn_dbl_clk(struct UIWindow *window, int x, int y)
{
    struct UINewSkillListWnd *self = (struct UINewSkillListWnd *)window;
    struct SkillInfo info;

    if (session_get_instant_var(&g_session1, 5) > 0)
        return;
    if (y < self->top_margin) {
        ui_frame_wnd_on_l_btn_down((struct UIWindow *)(&self->frame), x, y);
        return;
    }
    if (self->hover == -1)
        return;
    skill_tree_skill_at(self, &info, self->hover, 0);
    if (info.found && info.level) {
        if (info.id == 0x7f4) {
            int count = session_get_instant_var(&g_session1, 4) + 1;

            session_set_pair(&g_session1, 4, count);
        } else {
            session_reset_option2(&g_session1);
        }
        ui_new_skill_list_wnd_send_msg(WND(self), NULL, MSG_USE_SKILL, (int)&info, 0, 0, 0);
    }
    skill_info_destruct(&info);
}

void ui_new_skill_list_wnd_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UINewSkillListWnd *self = (struct UINewSkillListWnd *)window;
    int old;

    if (y < self->top_margin) {
        ui_frame_wnd_on_l_btn_down((struct UIWindow *)(&self->frame), x, y);
        return;
    }
    old = self->hover;
    self->hover = cell_at(self, x, y);
    if (old != self->hover)
        ui_window_invalidate_base(WND(self));
    if (in_cells(self, x, y) && !self->pending && self->hover != -1) {
        struct SkillInfo info;

        skill_tree_skill_at(self, &info, self->hover, 0);
        if (info.found && info.level && info.type) {
            struct GameMode *mode = game_current_mode();

            if (mode) {
                int level = session_get_skill_table_value(&g_session1, info.id);

                game_mode_drag_by_name(mode, DRAG_KIND_SKILL, info.id, SKILL_INFO_ID_NAME(&info),
                           level ? level : info.level, -1);
            }
        }
        skill_info_destruct(&info);
    }
}

void ui_new_skill_list_wnd_on_l_btn_up(struct UIWindow *window, int x, int y)
{
    struct UINewSkillListWnd *self = (struct UINewSkillListWnd *)window;
    struct SkillInfo info;

    if (y < self->top_margin) {
        ui_frame_wnd_on_l_btn_up((struct UIWindow *)(&self->frame), x, y);
        ui_new_skill_list_wnd_store_info(WND(self));
        return;
    }
    if (g_session1.simpleSkillWnd || !in_cells(self, x, y))
        return;
    skill_tree_skill_at(self, &info, self->hover, 0);
    if (SKILL_INFO_LEVEL_LIMIT(&info) && info.found && skill_tree_raise_skill(self, info.id)) {
        self->dirty = 1;
        self->pending = 1;
        skill_tree_layout_buttons(self);
        ui_window_invalidate_base(WND(self));
    }
    skill_info_destruct(&info);
}

void ui_new_skill_list_wnd_on_mouse_hover(struct UIWindow *window, int x, int y)
{
    struct UINewSkillListWnd *self = (struct UINewSkillListWnd *)window;
    struct UIWindow *describe;
    int old;

    if (!in_cells(self, x, y) || (g_session1.simpleSkillWnd && x > 0x60)) {
        ui_window_mgr_show_option_window(&g_windowMgr, WND_ID_SKILL_DESCRIBE, 0);
        return;
    }
    old = self->hover;
    self->hover = cell_at(self, x, y);
    if (old != self->hover) {
        if (!self->show_required)
            skill_tree_mark_required(self);
        self->dirty = 1;
        ui_window_invalidate_base(WND(self));
    }
    if (self->hover != -1 && g_session1.showSkillDescript) {
        struct SkillInfo info;

        skill_tree_skill_at(self, &info, self->hover, 0);
        if (info.found) {
            describe = ui_window_open(WND_ID_SKILL_DESCRIBE);
            if (describe) {
                int sx = 0, sy = 0, px, py, top;

                ui_window_send_msg(describe, NULL, SKILL_DESCRIBE_MSG_SHOW, info.id, SKILL_INFO_UPGRADABLE(&info),
                         info.level, 1);
                ui_window_to_screen(WND(self), &sx, &sy);
                px = x + 0x18 + sx;
                py = sy + y;
                if (describe->width + px > g_renderer->width)
                    px += -0x30 - describe->width;
                top = py - describe->height;
                if (describe->height + py <= g_renderer->height)
                    top = py;
                ui_window_move(describe, px, top >= 0 ? top : 0);
                ui_window_mgr_show_option_window(&g_windowMgr, WND_ID_SKILL_DESCRIBE, 1);
            }
        } else {
            describe = ui_window_find(WND_ID_SKILL_DESCRIBE);
            if (describe && describe->visible)
                ui_window_mgr_show_option_window(&g_windowMgr, WND_ID_SKILL_DESCRIBE, 0);
        }
        skill_info_destruct(&info);
        return;
    }
    describe = ui_window_find(WND_ID_SKILL_DESCRIBE);
    if (describe && describe->visible)
        ui_window_mgr_show_option_window(&g_windowMgr, WND_ID_SKILL_DESCRIBE, 0);
}

void ui_new_skill_list_wnd_on_r_btn_up(struct UIWindow *window, int x, int y)
{
    struct UINewSkillListWnd *self = (struct UINewSkillListWnd *)window;
    self->hover = cell_at(self, x, y);
    if (self->hover == -1) {
        int i;

        self->show_required = 0;
        list_clear(&self->required, NULL);
        for (i = 0; i < skill_tree_tab_count(self); i++) {
            ((int *)self->tabs->colours.first)[i] = -1;
            ui_window_invalidate_base(&self->tabs->base);
        }
    } else if (self->required_cell != self->hover) {
        self->show_required = 1;
        skill_tree_mark_required(self);
    } else {
        self->show_required = self->show_required == 0;
    }
    if (self->show_required)
        self->required_cell = self->hover;
    self->dirty = 1;
    ui_window_invalidate_base(WND(self));
}

void ui_new_skill_list_wnd_on_wheel(struct UIWindow *window, int delta)
{
    struct UINewSkillListWnd *self = (struct UINewSkillListWnd *)window;
    int lines;

    if (!self->scrollable)
        return;
    lines = self->rows <= 3 ? self->rows - 1 : 3;
    if (delta >= 0)
        lines = -lines;
    ui_new_skill_list_wnd_send_msg(WND(self), &self->scroll->base, UI_MSG_SCROLL_V, lines, 0, 0, 0);
}

void ui_new_skill_list_wnd_store_info(struct UIWindow *window)
{
    struct UINewSkillListWnd *self = (struct UINewSkillListWnd *)window;
    if (!self->info)
        return;
    self->info->x = WND(self)->x;
    self->info->y = WND(self)->y;
    self->info->width = WND(self)->width;
    self->info->height = WND(self)->height;
}

static void send_view_size(struct UINewSkillListWnd *self)
{
    if (g_session1.simpleSkillWnd)
        ui_new_skill_list_wnd_send_msg(WND(self), NULL, UI_MSG_RESIZE, UI_EDGE_BOTTOM, 0x14a, self->info->height, 0);
    else
        ui_new_skill_list_wnd_send_msg(WND(self), NULL, UI_MSG_RESIZE, UI_EDGE_BOTTOM, 0x230, 0x190, 0);
}

static void relayout(struct UINewSkillListWnd *self)
{
    layout_grid(self);
    skill_tree_layout_buttons(self);
    ui_window_invalidate_base(WND(self));
}

static int on_command(struct UINewSkillListWnd *self, int id)
{
    struct UIWindow *wnd = WND(self);
    struct SkillInfo info;

    switch (id) {
    case UI_ID_CLOSE:
        ui_window_mgr_post_quit1(&g_windowMgr, WND_ID_SKILL_LIST);
        return 0;
    case UI_ID_HELP:
        ui_new_skill_list_wnd_store_info(wnd);
        self->dirty = 1;
        g_session1.simpleSkillWnd = g_session1.simpleSkillWnd == 0;
        send_view_size(self);
        return 0;
    case ID_INFO_CHECK:
        g_session1.showSkillDescript = g_session1.showSkillDescript == 0;
        return 0;
    case ID_NEXT_TAB: {
        int next = self->tab + 1;

        ui_tab_control_select_tab(&self->tabs->base, next != ui_tab_control_tab_count(self->tabs) ? next : 0);
        return 0;
    }
    case ID_APPLY:
        if (ui_window_mgr_message_box(&g_windowMgr, msg_str(MSG_APPLY_QUESTION), UI_MSGBOX_QUESTION, 0, 0, 0,
                                   "메시지", 0x118, 0x78, 0) != UI_MSGBOX_ANSWER_YES)
            return 0;
        skill_tree_apply_points(self);
        ui_new_skill_list_wnd_send_msg(wnd, NULL, MSG_REFRESH, 0, 0, 0, 0);
        if (g_windowMgr.new_chat_wnd)
            ui_window_mgr_set_focus_edit(&g_windowMgr, ui_chat_window_focus_edit(g_windowMgr.new_chat_wnd));
        return 0;
    case ID_RESET:
        skill_tree_reset_points(self);
        ui_new_skill_list_wnd_send_msg(wnd, NULL, MSG_REFRESH, 0, 0, 0, 0);
        return 0;
    }
    if (id >= ID_UP_FIRST && id < ID_UP_FIRST + SKILL_TREE_UP_BUTTONS) {
        if (!self->points_left)
            return 0;
        skill_tree_skill_at(self, &info, self->columns * self->top + id - ID_UP_FIRST, 0);
        if (info.found && SKILL_INFO_UPGRADABLE(&info) && skill_tree_raise_skill(self, info.id)) {
            self->dirty = 1;
            self->pending = 1;
            skill_tree_layout_buttons(self);
            ui_window_invalidate_base(wnd);
        }
        skill_info_destruct(&info);
        return 0;
    }
    {
        unsigned raise = id - ID_RAISE_FIRST;

        skill_tree_skill_at(self, &info,
                            self->columns * self->top + (raise > SKILL_TREE_CELLS - 1 ? id : (int)raise), 0);
        if (info.found) {
            int level = session_get_skill_table_value(&g_session1, info.id);

            level = raise > SKILL_TREE_CELLS - 1 ? level - 1 : level + 1;
            if (level <= 0)
                level = 1;
            if (level > info.level)
                level = info.level;
            player_skill_bundle_set_chosen_level(&g_session1.skills, info.id, level);
            self->dirty = 1;
            ui_window_invalidate_base(wnd);
        }
        skill_info_destruct(&info);
    }
    return 0;
}

int ui_new_skill_list_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1,
                                         int val2, int val3, int val4)
{
    struct UINewSkillListWnd *self = (struct UINewSkillListWnd *)window;
    struct UIWindow *wnd = WND(self);

    switch (msg) {
    case UI_MSG_COMMAND:
        return on_command(self, val1);
    case UI_MSG_SCROLL_V:
    case UI_MSG_PAGE_UP:
    case UI_MSG_PAGE_DOWN: {
        int old = self->top;

        if (msg == UI_MSG_PAGE_UP)
            self->top = old - self->rows + 1;
        else if (msg == UI_MSG_PAGE_DOWN)
            self->top = self->rows - 1 + old;
        else
            self->top = val1 + old;
        if (old != self->top)
            relayout(self);
        return self->top - old;
    }
    case UI_MSG_RESIZE: {
        int height = val3;

        if (val1 != UI_EDGE_BOTTOM)
            return 0;
        if (g_session1.simpleSkillWnd) {
            int least = self->tabs->base.height + 0xc;

            height = height < least ? least : height;
            height = height > 0x190 ? 0x190 : height;
            height = (height - 0x190) / ROW_H * ROW_H + 0x190;
        }
        if (wnd->width != val2 || height != wnd->height) {
            ui_window_resize_base(wnd, val2, height);
            ui_scroll_bar_resize(&self->scroll->base, self->scroll->base.width,
                     wnd->height - self->bottom_margin - self->top_margin);
            relayout(self);
        }
        ui_frame_wnd_move(wnd, self->info->x, self->info->y);
        ui_window_mgr_update_snap_rect(&g_windowMgr, wnd);
        return 0;
    }
    case MSG_TAB_SELECTED: {
        struct List *list = tab_list(self, val1, 0);

        if (!skill_tree_is_last_tab(self, val1) && list->size == 0) {
            self->tabs->selected = self->tab;
            return 0;
        }
        if (self->tab != val1) {
            self->hover = -1;
            self->tab = val1;
            self->dirty = 1;
            if (skill_tree_is_last_tab(self, val1) == 1) {
                ui_tab_control_set_new_mark(self->tabs, val1, 0);
                g_session1.skills.extra_skill_ids.last = g_session1.skills.extra_skill_ids.first;
            }
        }
        relayout(self);
        return 0;
    }
    case MSG_REFRESH:
        skill_tree_reset_points(self);
        player_skill_bundle_number_extra_skills(&g_session1.skills);
        relayout(self);
        return 0;
    case UI_MSG_SET_INFO:
        self->info = (struct WindowInfo *)val1;
        if (!skill_list_info_is_valid(self->info))
            skill_list_info_set_default(self->info);
        send_view_size(self);
        return 0;
    case MSG_USE_SKILL: {
        struct SkillInfo *info = (struct SkillInfo *)val1;

        if (info->level)
            mode_send_message(client_current_mode(), MODE_REQ_USE_SKILL, (int)info, session_get_skill_table_value(&g_session1, info->id), 0,
                          0);
        return 0;
    }
    case MSG_SETUP_TABS:
        setup_tabs(self);
        return 0;
    case MSG_NEW_MARK:
        if (!skill_tree_is_last_tab(self, self->tab)) {
            struct UITabControl *tabs = self->tabs;

            ui_tab_control_set_new_mark(tabs, ui_tab_control_tab_count(tabs) - 1,
                                    g_session1.skills.extra_skill_ids.first != g_session1.skills.extra_skill_ids.last);
            ui_window_invalidate_base(&self->tabs->base);
        }
        return 0;
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
}
