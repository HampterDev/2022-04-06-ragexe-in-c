#include "game/drag.h"
#include "render/rp_pool.h"
#include "skills/skill_bundle.h"
#include "std/list.h"
#include "skills/skill_info.h"
#include "contents/guild_info.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "contents/mouse.h"
#include "core/object_type_tables.h"
#include "world/mode_manager.h"
#include "game/context.h"
#include <stdio.h>
#include <string.h>
#include "ui/social/social.h"
#include "ui/social/guild_wnd.h"
#include "core/msg_str.h"
#include "diag/perf.h"
#include "session/session_names.h"
#include "session/session_skills.h"
#include "session/skill_levels.h"

#define GUILD_SKILL_ROWS_SHOWN 7
#define GUILD_SKILL_ROW_HEIGHT 0x22
#define GUILD_SKILL_HIT_HEIGHT 0x20
#define HIDDEN (-200)

#define ID_USE 0xb0
#define ID_UP_FIRST 0xe7
#define ID_DOWN_FIRST 0xf5
#define ID_RAISE_FIRST 0xfc

#define GUILD_SKILL_MSG_REBUILD 0x17
#define GUILD_SKILL_MSG_MENU 0x31
#define GUILD_SKILL_MSG_USE 0x3a

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

#define MODE_REQ_GUILD_SKILL_UP 0x43
#define MODE_REQ_USE_SKILL 0x71
#define DRAG_KIND_SKILL 8

#define MSG_PASSIVE 0x11e

#define GUILD_PAGE_SKILLS 3

struct UIGuildSkillWnd {
    struct UIGuildWnd base;
    struct UIScrollBar *bar;
    struct UIBitmapButton *use_button;
    struct UIBitmapButton *close_button;
    struct UIBitmapButton *raise_buttons[GUILD_SKILL_ROWS_SHOWN];
    struct UIBitmapButton *lower_buttons[GUILD_SKILL_ROWS_SHOWN];
    struct UIBitmapButton *up_buttons[GUILD_SKILL_ROWS_SHOWN];
    int selected;
    int top;
    int per_row;
    int rows;
    int scrollable;
    struct List skills;
    struct Vec menu;
};
void ui_guild_skill_wnd_destroy(struct UIWindow *window);
void ui_guild_skill_wnd_on_create(struct UIWindow *self, int width_unused, int height);
void ui_guild_skill_wnd_on_draw(struct UIWindow *self);
void ui_guild_skill_wnd_on_l_btn_down(struct UIWindow *self, int x, int y);
void ui_guild_skill_wnd_on_r_btn_down(struct UIWindow *self, int x, int y);
void ui_guild_skill_wnd_on_r_btn_up(struct UIWindow *self, int x_unused, int y_unused);
void ui_guild_skill_wnd_on_wheel(struct UIWindow *self, int delta);
int ui_guild_skill_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);

const struct UIGuildWndOps ui_guild_skill_wnd_ops = {
    .base = {
        .destroy = ui_guild_skill_wnd_destroy,
        .resize = ui_window_resize_base,
        .is_frame = 1,
        .refreshes_children = 0,
        .move = ui_frame_wnd_move,
        .is_focusable = ui_window_is_focusable_base,
        .get_backdrop_box = ui_window_get_backdrop_box_base,
        .is_click_through = ui_window_is_click_through_base,
        .is_hit_testable = ui_window_is_hit_testable_base,
        .drag_and_drop = ui_window_drag_and_drop_base,
        .store_info = ui_guild_wnd_store_info,
        .save_pos = ui_frame_wnd_save_pos,
        .move_delta = ui_frame_wnd_move_delta,
        .set_show = ui_window_set_show_base,
        .on_create = ui_guild_skill_wnd_on_create,
        .on_destroy = ui_window_on_destroy_base,
        .on_process = ui_window_on_process_base,
        .on_draw = ui_guild_skill_wnd_on_draw,
        .on_focus = ui_window_on_focus_base,
        .on_focus_lost = ui_window_on_focus_lost_base,
        .on_l_btn_down = ui_guild_skill_wnd_on_l_btn_down,
        .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
        .on_mouse_move = ui_frame_wnd_on_mouse_move,
        .on_mouse_hover = ui_frame_wnd_on_mouse_hover,
        .track_mouse = ui_window_track_mouse_base,
        .on_mouse_shape = ui_window_on_mouse_shape_base,
        .on_l_btn_up = ui_frame_wnd_on_l_btn_up,
        .on_r_btn_down = ui_guild_skill_wnd_on_r_btn_down,
        .on_r_btn_up = ui_guild_skill_wnd_on_r_btn_up,
        .on_wheel = ui_guild_skill_wnd_on_wheel,
        .snap = ui_frame_wnd_snap,
        .send_msg = ui_guild_skill_wnd_send_msg,
        .invalidate = ui_window_invalidate_base,
        .render = ui_window_render_base,
        .has_custom_backdrop = 0,
        .draw_backdrop = ui_window_draw_backdrop_base,
        .get_records = ui_frame_wnd_get_records,
        .type = UI_TYPE_FRAME,
        .on_hover_enter = ui_window_on_hover_enter_base,
        .on_hover_leave = ui_window_on_hover_leave_base,
        .make_quad = ui_window_make_quad_base,
        .hit_test = ui_window_hit_test_base,
        .render_overlay = ui_window_render_overlay_base,
    },
    .tab = GUILD_TAB_SKILLS,
};

#define WND(self) (&(self)->base.frame.base)

static int hit_row(struct UIGuildSkillWnd *self, int x, int y)
{
    struct ListNode *node;
    int row, index, shown;

    if (x < 0x28 || y < 0x29 || y >= self->rows * GUILD_SKILL_HIT_HEIGHT + 0x29)
        return -1;
    row = (y - 0x29) / GUILD_SKILL_HIT_HEIGHT;
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

static struct SkillInfo *get_entry(struct UIGuildSkillWnd *self, struct SkillInfo *out, int index)
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

static struct SkillInfo *get_row_entry(struct UIGuildSkillWnd *self, struct SkillInfo *out, int i)
{
    struct ListNode *node;
    int index = 0;

    for (node = self->skills.head->next; node != self->skills.head; node = node->next, index++)
        if (index == self->top + i)
            return skill_info_copy(out, LIST_VALUE(node, struct SkillInfo));
    return skill_info_construct_unknown(out);
}

static void refresh_range(struct UIGuildSkillWnd *self)
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

static void place_buttons(struct UIGuildSkillWnd *self)
{
    int i, y;

    for (i = 0, y = GUILD_SKILL_ROW_HEIGHT; y < 0x110; i++, y += GUILD_SKILL_ROW_HEIGHT) {
        struct SkillInfo info;

        get_row_entry(self, &info, i);
        if (g_guildInfo.is_master && i < self->rows && (unsigned)(self->top + i) < self->skills.size && g_session1.guildSkillPoints &&
            info.found && SKILL_INFO_UPGRADABLE(&info))
            ui_window_move_base(&self->up_buttons[i]->button.base, 0x4e, y + 0xb);
        else
            ui_window_move_base(&self->up_buttons[i]->button.base, HIDDEN, y - 0x11);
        ui_window_move_base(&self->lower_buttons[i]->button.base, HIDDEN, y);
        ui_window_move_base(&self->raise_buttons[i]->button.base, HIDDEN, y);
        skill_info_destruct(&info);
    }
}

struct UIGuildSkillWnd *ui_guild_skill_wnd_construct(struct UIGuildSkillWnd *self)
{
    ui_guild_wnd_construct(&self->base);
    WND(self)->ops = &ui_guild_skill_wnd_ops.base;
    list_init(&self->skills);
    self->menu.first = self->menu.last = self->menu.end = NULL;
    self->use_button = NULL;
    self->close_button = NULL;
    self->selected = -1;
    self->per_row = 1;
    self->base.info = NULL;
    self->top = 0;
    return self;
}

struct UIWindow *ui_guild_skill_wnd_new(void)
{
    return (struct UIWindow *)ui_guild_skill_wnd_construct(mem_alloc(sizeof(struct UIGuildSkillWnd)));
}

void ui_guild_skill_wnd_destroy(struct UIWindow *window)
{
    struct UIGuildSkillWnd *self = (struct UIGuildSkillWnd *)window;
    WND(self)->ops = &ui_guild_skill_wnd_ops.base;
    ui_menu_entry_vec_free(&self->menu);
    list_free(&self->skills, skill_info_destroy);
    WND(self)->ops = &ui_guild_wnd_ops.base;
    ui_frame_wnd_destruct(&self->base.frame);
    mem_free(self);
}

static struct UIBitmapButton *add_button(struct UIWindow *wnd, const char *up, const char *hover, const char *down,
                                         int x, int y, int id)
{
    struct UIBitmapButton *button = mem_alloc(sizeof(struct UIBitmapButton));

    if (button)
        button = ui_bitmap_button_construct(button);
    ui_bitmap_button_set_bitmap(button, up, UI_BTN_UP, 0);
    ui_bitmap_button_set_bitmap(button, hover, UI_BTN_HOVER, 0);
    ui_bitmap_button_set_bitmap(button, down, UI_BTN_DOWN, 0);
    ui_window_create(&button->button.base, button->bitmap_width, button->bitmap_height);
    ui_window_move_base(&button->button.base, x, y);
    button->button.base.id = id;
    ui_window_add_child(wnd, &button->button.base);
    return button;
}

void ui_guild_skill_wnd_on_create(struct UIWindow *window, int width_unused, int height)
{
    struct UIGuildSkillWnd *self = (struct UIGuildSkillWnd *)window;
    static const char *const names[2][3] = {
        { "btn_use", "btn_use_a", "btn_use_b" },
        { "btn_close", "btn_close_a", "btn_close_b" },
    };
    struct UIWindow *wnd = WND(self);
    struct UIScrollBar *bar;
    int i, y;

    ui_guild_wnd_init_tab_text(&self->base);
    if (strcmp(string_cstr(&g_guildInfo.master_name), "") == 0)
        mode_send_message(client_current_mode(), MODE_REQ_GUILD_INFO, 0, 0, 0, 0);
    g_guildInfo.page_asked[GUILD_PAGE_SKILLS] = 1;
    mode_send_message(client_current_mode(), MODE_REQ_GUILD_INFO, GUILD_PAGE_SKILLS, 0, 0, 0);

    bar = mem_alloc(sizeof(struct UIScrollBar));
    if (bar)
        bar = ui_scroll_bar_construct(bar);
    self->bar = bar;
    ui_scroll_bar_create(bar, wnd->width - 0xf, 0x29, UI_SCROLL_VERT, wnd->height - 0x46);
    ui_window_add_child(wnd, &self->bar->base);

    self->per_row = 1;
    self->rows = (height - 0x3e) / 32;
    for (i = 0, y = 0x2d; y < 0x11b; i++, y += GUILD_SKILL_ROW_HEIGHT)
        self->up_buttons[i] = add_button(wnd, UI_SKIN "basic_interface\\skill_up_a.bmp",
                                         UI_SKIN "basic_interface\\skill_up_b.bmp",
                                         UI_SKIN "basic_interface\\skill_up_c.bmp", 0x4e, y, ID_UP_FIRST + i);
    for (i = 0, y = 0x3a; y < 0x128; i++, y += GUILD_SKILL_ROW_HEIGHT)
        self->lower_buttons[i] = add_button(wnd, UI_SKIN "basic_interface\\arw_left.bmp",
                                            UI_SKIN "basic_interface\\arw_left.bmp",
                                            UI_SKIN "basic_interface\\arw_left.bmp", 0x70, y, ID_DOWN_FIRST + i);
    for (i = 0, y = 0x3a; y < 0x128; i++, y += GUILD_SKILL_ROW_HEIGHT)
        self->raise_buttons[i] = add_button(wnd, UI_SKIN "basic_interface\\arw_right.bmp",
                                            UI_SKIN "basic_interface\\arw_right.bmp",
                                            UI_SKIN "basic_interface\\arw_right.bmp", 0xc0, y, ID_RAISE_FIRST + i);

    {
        struct UIBitmapButton **slots[2] = { &self->use_button, &self->close_button };
        static const int xs[2] = { 0x2e, 0x5c };
        static const int ids[2] = { ID_USE, UI_ID_CLOSE };
        int b, state;

        for (b = 0; b < 2; b++) {
            struct UIBitmapButton *button = mem_alloc(sizeof(struct UIBitmapButton));

            if (button)
                button = ui_bitmap_button_construct(button);
            *slots[b] = button;
            for (state = 0; state < 3; state++) {
                struct String path;

                string_init(&path);
                string_assign(&path, UI_SKIN, sizeof UI_SKIN - 1);
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
    }
    ui_window_send_msg(&self->base.frame.base, NULL, GUILD_SKILL_MSG_REBUILD, 0, 0, 0, 0);
}

void ui_guild_skill_wnd_on_draw(struct UIWindow *window)
{
    struct UIGuildSkillWnd *self = (struct UIGuildSkillWnd *)window;
    struct UIWindow *wnd = WND(self);
    struct ListNode *node;
    char buf[0x50];
    int index, drawn;

    ui_guild_wnd_on_draw((struct UIWindow *)(&self->base));
    drawn = 0;
    index = 0;
    for (node = self->skills.head->next; node != self->skills.head; node = node->next, index++) {
        struct SkillInfo *info = LIST_VALUE(node, struct SkillInfo);
        int x, y, level_y;
        COLORREF name_colour;

        if (index < self->top * self->per_row)
            continue;
        x = drawn % self->per_row * 32;
        y = drawn / self->per_row * GUILD_SKILL_ROW_HEIGHT;
        if (self->selected == index) {
            COLORREF colour = ui_skin_colour(6, 2);

            if (info->level == 0)
                colour = ui_skin_colour(0xa, 2);
            else if (info->sp_cost == 0)
                colour = ui_skin_colour(0xe, 2);
            ui_window_fill_rect(wnd, x + 0x68, y + 0x2b, 0xa4, 0x1c, colour);
        }
        session_format_item_bitmap_path(SKILL_INFO_NAME(info), buf);
        ui_window_draw_bitmap(wnd, x + 0x2c, y + 0x2d, res_mgr_get(g_resMgr(), buf), 1);
        if (info->level) {
            sprintf(buf, g_serviceType != 0xc ? "Lv : %d" : "Nv : %d", info->level);
            level_y = y + 0x3a;
            ui_window_text_out(wnd, x + 0x71, level_y, buf, 0, 0, 0xc, 0, 0, 0);
            if (info->type == 0)
                sprintf(buf, msg_str(MSG_PASSIVE));
            else
                sprintf(buf, "Sp : %d", info->sp_cost);
            ui_window_text_out(wnd, x + 0xd4, level_y, buf, 0, 0, 0xc, 0, 0, 0);
            name_colour = RGB(0, 0, 0);
        } else {
            name_colour = RGB(0x78, 0x78, 0x78);
        }
        ui_window_text_out(wnd, x + 0x70, y + 0x2e, skill_info_get_name(info), 0, 0, 0xc, name_colour, 0,
                         0);
        if (++drawn == self->rows * self->per_row)
            break;
    }
    sprintf(buf, "SkillPoint : %d", g_session1.guildSkillPoints);
    ui_window_text_out(wnd, wnd->width - 0x64, wnd->height - 0x14, buf, 0, 0, 0xc, 0, 0, 0);
}

void ui_guild_skill_wnd_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIGuildSkillWnd *self = (struct UIGuildSkillWnd *)window;
    int old = self->selected;
    struct SkillInfo info;
    struct GameMode *game_mode;

    self->selected = hit_row(self, x, y);
    if (old != self->selected)
        ui_window_invalidate_base(&self->base.frame.base);
    if (g_replayPlaying)
        return;
    get_entry(self, &info, self->selected);
    if (info.found && info.type && info.level) {
        game_mode = game_current_mode();
        if (game_mode) {
            int level = session_get_skill_table_value(&g_session1, info.id);

            game_mode_drag_by_name(game_mode, DRAG_KIND_SKILL, info.id, SKILL_INFO_NAME(&info),
                       level ? level : info.level, -1);
        }
    }
    ui_guild_wnd_on_l_btn_down((struct UIWindow *)(&self->base), x, y);
    skill_info_destruct(&info);
}

void ui_guild_skill_wnd_on_r_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIGuildSkillWnd *self = (struct UIGuildSkillWnd *)window;
    int old = self->selected;

    self->selected = hit_row(self, x, y);
    if (old != self->selected)
        ui_window_invalidate_base(&self->base.frame.base);
}

void ui_guild_skill_wnd_on_r_btn_up(struct UIWindow *window, int x_unused, int y_unused)
{
    struct UIGuildSkillWnd *self = (struct UIGuildSkillWnd *)window;
    ui_window_send_msg(&self->base.frame.base, NULL, GUILD_SKILL_MSG_MENU, 0, 0, 0, 0);
}

void ui_guild_skill_wnd_on_wheel(struct UIWindow *window, int delta)
{
    struct UIGuildSkillWnd *self = (struct UIGuildSkillWnd *)window;
    int step;

    if (!self->scrollable)
        return;
    step = self->rows <= 3 ? self->rows - 1 : 3;
    if (delta >= 0)
        step = -step;
    ui_window_send_msg(&self->base.frame.base, &self->bar->base, UI_MSG_SCROLL_V, step, 0, 0, 0);
}

static void rebuild(struct UIGuildSkillWnd *self)
{
    int count = ((int)g_session1.guildSkills.size);
    int i;

    list_clear(&self->skills, skill_info_destroy);
    for (i = 0; i < count; i++) {
        struct SkillInfo info;

        session_get_skill_at(&g_session1, &info, i);
        if (info.found && (SKILL_INFO_UPGRADABLE(&info) || info.level)) {
            struct SkillInfo copy;

            skill_info_copy(&copy, &info);
            list_push_back(&self->skills, &copy, sizeof copy);
        }
        skill_info_destruct(&info);
    }
    ui_window_invalidate_base(&self->base.frame.base);
    refresh_range(self);
    place_buttons(self);
}

static void open_menu(struct UIGuildSkillWnd *self)
{
    struct UIMenuEntry *entry, *end;
    struct UIWindow *menu;
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
    end = self->menu.last;
    for (entry = self->menu.first; entry != end; entry++)
        ui_window_send_msg(menu, NULL, MSG_MENU_ADD_ITEM, (int)string_cstr(&entry->text), 0, 0, 0);
    ui_window_send_msg(menu, NULL, MSG_MENU_SET_OWNER, GUILD_TAB_WND_ID_FIRST + GUILD_TAB_SKILLS, 0, 0, 0);
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

static void menu_choice(struct UIGuildSkillWnd *self, int choice)
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

static void step_level(struct UIGuildSkillWnd *self, int row, int step)
{
    struct SkillInfo info;

    get_entry(self, &info, self->top + row);
    if (info.found) {
        int level = session_get_skill_table_value(&g_session1, info.id) + step;

        if (level <= 0)
            level = 1;
        if (level > info.level)
            level = info.level;
        player_skill_bundle_set_chosen_level(&g_session1.skills, info.id, level);
        ui_window_invalidate_base(&self->base.frame.base);
    }
    skill_info_destruct(&info);
}

static void command(struct UIGuildSkillWnd *self, int id)
{
    struct SkillInfo info;

    if (id == ID_USE) {
        if (self->selected == -1)
            return;
        get_entry(self, &info, self->selected);
        if (info.level)
            ui_window_send_msg(&self->base.frame.base, NULL, GUILD_SKILL_MSG_USE, (int)&info, 0, 0, 0);
        skill_info_destruct(&info);
    } else if (id == UI_ID_CLOSE) {
        ui_close_window(GUILD_WND_ID);
    } else if (id >= ID_UP_FIRST && id < ID_UP_FIRST + GUILD_SKILL_ROWS_SHOWN) {
        if (!g_session1.guildSkillPoints)
            return;
        get_entry(self, &info, self->top + id - ID_UP_FIRST);
        if (info.found)
            mode_send_message(client_current_mode(), MODE_REQ_GUILD_SKILL_UP, info.id, 0, 0, 0);
        skill_info_destruct(&info);
    } else if (id >= ID_DOWN_FIRST && id < ID_DOWN_FIRST + GUILD_SKILL_ROWS_SHOWN) {
        step_level(self, id - ID_DOWN_FIRST, -1);
    } else if (id >= ID_RAISE_FIRST && id < ID_RAISE_FIRST + GUILD_SKILL_ROWS_SHOWN) {
        step_level(self, id - ID_RAISE_FIRST, 1);
    }
}

int ui_guild_skill_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1,
                                       int val2, int val3, int val4)
{
    struct UIGuildSkillWnd *self = (struct UIGuildSkillWnd *)window;
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
            self->top = val1 + old;
        if (old != self->top) {
            refresh_range(self);
            place_buttons(self);
            ui_window_invalidate_base(&self->base.frame.base);
        }
        return self->top - old;
    }
    case GUILD_SKILL_MSG_REBUILD:
        rebuild(self);
        return 0;
    case MSG_MENU_CHOICE:
        menu_choice(self, val1);
        return 0;
    case GUILD_SKILL_MSG_MENU:
        open_menu(self);
        return 0;
    case GUILD_SKILL_MSG_USE: {
        struct SkillInfo *info = (struct SkillInfo *)val1;

        if (info->level)
            mode_send_message(client_current_mode(), MODE_REQ_USE_SKILL, (int)info, session_get_skill_table_value(&g_session1, info->id),
                          0, 0);
        return 0;
    }
    }
    return ui_guild_wnd_send_msg((struct UIWindow *)(&self->base), sender, msg, val1, val2, val3, val4);
}
