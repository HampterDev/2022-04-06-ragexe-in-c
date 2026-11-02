#include "ui/controls/buttons.h"
#include "ui/social/guild_windows.h"
#include "contents/player_names.h"
#include "session/character_name.h"
#include "ui/social/guild_requests.h"
#include "contents/guild_info.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "contents/mouse.h"
#include "world/mode_manager.h"
#include "core/markup_tags.h"
#include "game/context.h"
#include <stdio.h>
#include <string.h>
#include "ui/social/guild_manage.h"
#include "core/msg_str.h"
#include "core/text_util.h"
#include "diag/perf.h"
#include "std/tree.h"
#include "platform/clock.h"

#define MEMBER_TAB_WND_ID (GUILD_TAB_WND_ID_FIRST + GUILD_TAB_MEMBERS)

#define MEMBER_LIST_X 5
#define MEMBER_LIST_WIDTH 0x179
#define MEMBER_HEADER_TOP 0x2f
#define MEMBER_ROW_TOP 0x42
#define MEMBER_ROW_HEIGHT 0x24
#define MEMBER_TEXT_TOP 0x3f
#define MEMBER_BUTTON_X 0x8e
#define MEMBER_BUTTON_TOP 0x52
#define MEMBER_LIST_HEIGHT 0xd9

static const int g_memberHeaders[GUILD_MEMBER_COLUMNS] = { 0x197, 0x1f7, 0x1f8, 0x198, 0x1fa, 0x1fb };

#define MENU_LEAVE 0x11
#define MENU_EXPEL 0x10
#define MENU_POSITION_0 0x3b
#define MENU_INVITE 5
#define MENU_WHISPER 0x14
#define MENU_SEND_MAIL 0x3a
#define MENU_MEMBER_INFO 0x12
#define WND_ID_GUILD_LEAVE 0x4b
#define WND_ID_GUILD_EXPEL 0x4c
#define MSG_NAMELESS_MEMBER 0xbe1
#define UIWM_MSG_WHISPER 0xe
#define MSG_LABEL_ON_LIST 0x66

void guild_open_menu(struct Vec *entries, int owner_id)
{
    struct UIMenuEntry *entry;
    struct UIWindow *menu;
    unsigned i;
    int x, y, max;

    if (VEC_COUNT(entries, struct UIMenuEntry) == 0)
        return;
    menu = ui_make_window(WND_ID_MENU);
    for (i = 0; i < VEC_COUNT(entries, struct UIMenuEntry); i++) {
        entry = &VEC_AT(entries, struct UIMenuEntry, i);
        ui_window_send_msg(menu, NULL, MENU_MSG_ADD_ITEM, (int)string_cstr(&entry->text), 0, 0, 0);
    }
    ui_window_send_msg(menu, NULL, MENU_MSG_SET_OWNER, owner_id, 0, 0, 0);
    ui_window_send_msg(menu, NULL, MENU_MSG_FIT, 0, 0, 0, 0);
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

struct UIBitmapButton *guild_add_ok_button(void)
{
    struct UIBitmapButton *ok = mem_alloc(sizeof(struct UIBitmapButton));

    if (ok)
        ok = ui_bitmap_button_construct(ok);
    ui_bitmap_button_set_bitmap(ok, UI_SKIN "btn_ok.bmp", UI_BTN_UP, 0);
    ui_bitmap_button_set_bitmap(ok, UI_SKIN "btn_ok_a.bmp", UI_BTN_HOVER, 0);
    ui_bitmap_button_set_bitmap(ok, UI_SKIN "btn_ok_b.bmp", UI_BTN_DOWN, 0);
    ui_bitmap_button_set_bitmap(ok, UI_SKIN "btn_ok_dis.bmp", UI_BMP_DISABLED, 0);
    ui_window_create(&ok->button.base, ok->bitmap_width, ok->bitmap_height);

    return ok;
}

void guild_update_scroll(struct UIScrollBar *bar, unsigned count, int per_row, int rows, int *top, int *scrollable)
{
    int lines = (int)(count / (unsigned)per_row + (count % (unsigned)per_row != 0));
    int excess = lines - rows;

    if (excess < 0)
        excess = 0;
    *scrollable = excess != 0;
    if (excess) {
        ui_scroll_bar_set_range(bar, excess, rows, lines);
        if (*top < 0)
            *top = 0;
        if (*top > lines - rows)
            *top = lines - rows;
        ui_scroll_bar_set_pos(bar, *top);
    } else {
        *top = 0;
        ui_scroll_bar_set_range(bar, 0, rows, lines);
    }
    ui_window_invalidate_base(&bar->base);
}

static void menu_add(struct Vec *menu, int text_id, int value)
{
    struct UIMenuEntry entry;

    ui_menu_entry_construct(&entry, msg_str(text_id), value);
    ui_menu_entry_vec_push_move(menu, &entry);
    string_free(&entry.text);
}

static void member_copy(struct GuildMember *dst, const struct GuildMember *src)
{
    dst->aid = src->aid;
    dst->gid = src->gid;
    string_copy(&dst->name, &src->name);
    dst->hair_style = src->hair_style;
    dst->hair_color = src->hair_color;
    dst->job = src->job;
    dst->sex = src->sex;
    dst->position = src->position;
    string_copy(&dst->position_name, &src->position_name);
    dst->level = src->level;
    string_copy(&dst->memo, &src->memo);
    dst->exp_share = src->exp_share;
    dst->exp = src->exp;
    dst->online = src->online;
    dst->last_login = src->last_login;
}

static void destroy_member(void *value)
{
    guild_member_destruct(value);
}

static void copy_members(struct UIGuildMemberManageWnd *self)
{
    struct ListNode *node;

    if (&self->members == &g_guildInfo.members)
        return;
    list_clear(&self->members, destroy_member);
    for (node = g_guildInfo.members.head->next; node != g_guildInfo.members.head; node = node->next) {
        struct GuildMember copy;

        member_copy(&copy, LIST_VALUE(node, struct GuildMember));
        list_push_back(&self->members, &copy, sizeof copy);
    }
}

static void hand_members_to_guild(struct UIGuildMemberManageWnd *self)
{
    struct ListNode *node;

    guild_info_clear_members(&g_guildInfo);
    for (node = self->members.head->next; node != self->members.head; node = node->next) {
        struct GuildMember copy;

        member_copy(&copy, LIST_VALUE(node, struct GuildMember));
        guild_info_add_member(&g_guildInfo, &copy);
        string_free(&copy.memo);
        string_free(&copy.position_name);
        string_free(&copy.name);
    }
}

struct SortDir {
    int column;
    int dir;
};

static int *current_sort_dir(struct UIGuildMemberManageWnd *self)
{
    struct SortDir *entry = tree_get(&self->sort_dirs, &self->sort_column, sizeof self->sort_column, sizeof *entry,
                                     tree_less_int);

    return &entry->dir;
}

static struct String *position_name(int position)
{
    struct GuildPositionName *entry = tree_get(&g_guildInfo.position_names, &position, sizeof position,
                                               sizeof *entry, tree_less_int);

    return &entry->name;
}

static void set_guild_member_edits(struct UIGuildMemberManageWnd *self)
{
    tree_free(&g_guildInfo.member_position_edits, NULL);
    tree_copy(&g_guildInfo.member_position_edits, &self->changes, sizeof(struct GuildMemberEdit), NULL);
}

static void flip_sort_dir(struct UIGuildMemberManageWnd *self)
{
    int *dir = current_sort_dir(self);

    if (*dir == GUILD_SORT_UP)
        *dir = GUILD_SORT_DOWN;
    else if (*dir == GUILD_SORT_DOWN)
        *dir = GUILD_SORT_UP;
}

void ui_guild_member_manage_wnd_update_scroll(struct UIGuildMemberManageWnd *self)
{
    if (!self->bar)
        return;
    self->rows = GUILD_MEMBER_ROWS - GUILD_MEMBER_ROWS * self->row_gap / 36;
    guild_update_scroll(self->bar, self->members.size, self->per_row, self->rows, &self->top, &self->scrollable);
}

void ui_guild_member_manage_wnd_sort_by(struct UIGuildMemberManageWnd *self, int column)
{
    static const GuildMemberSort sorts[GUILD_MEMBER_COLUMNS][2] = {
        { guild_member_sort_name_up, guild_member_sort_name_down },
        { guild_member_sort_position_up, guild_member_sort_position_down },
        { guild_member_sort_job_up, guild_member_sort_job_down },
        { guild_member_sort_level_up, guild_member_sort_level_down },
        { guild_member_sort_share_up, guild_member_sort_share_down },
        { guild_member_sort_exp_up, guild_member_sort_exp_down },
    };
    int dir;

    if (self->sort_column != column) {
        self->sort_column = column;
        *current_sort_dir(self) = GUILD_SORT_UP;
    } else {
        int *current = current_sort_dir(self);

        *current = *current == GUILD_SORT_UP ? GUILD_SORT_DOWN : GUILD_SORT_UP;
    }
    if ((unsigned)column < GUILD_MEMBER_COLUMNS) {
        dir = *current_sort_dir(self);
        if (dir == GUILD_SORT_UP || dir == GUILD_SORT_DOWN)
            sorts[column][dir - 1](&self->members);
    }
    ui_window_invalidate_base(&self->base.frame.base);
}

void ui_guild_member_manage_wnd_on_create(struct UIWindow *window, int width_unused,
                                                int height_unused)
{
    struct UIGuildMemberManageWnd *self = (struct UIGuildMemberManageWnd *)window;
    struct UIWindow *wnd = GUILD_WND(self);
    struct UICheckBox *check;
    int i, y;

    ui_guild_wnd_init_tab_text(&self->base);
    guild_request_page(GUILD_PAGE_MEMBERS);
    copy_members(self);
    self->row_gap += 8;
    for (i = 0; i < GUILD_MEMBER_ROWS; i++) {
        y = (self->row_gap + MEMBER_ROW_HEIGHT) * i + MEMBER_TEXT_TOP;
        self->col_name[i] = mem_alloc(sizeof(struct UIStaticText));
        if (self->col_name[i])
            self->col_name[i] = ui_static_text_construct(self->col_name[i]);
        ui_window_create(self->col_name[i], g_guildLayout.member_position_cols[0] / 2 + 2, 0xf);
        ui_static_text_set_text_ellipsis(self->col_name[i], "");
        ui_window_move(self->col_name[i], 8, y);
        ui_window_add_child(wnd, self->col_name[i]);

        self->col_2[i] = mem_alloc(sizeof(struct UIStaticText));
        if (self->col_2[i])
            self->col_2[i] = ui_static_text_construct(self->col_2[i]);
        ui_window_create(self->col_2[i], g_guildLayout.member_position_cols[1] - 0x10, 0xf);
        ui_static_text_set_text_ellipsis(self->col_2[i], "");
        ui_window_move(self->col_2[i], g_guildLayout.member_position_cols[0] + 8, y);
        ui_window_add_child(wnd, self->col_2[i]);

        self->col_3[i] = mem_alloc(sizeof(struct UIStaticText));
        if (self->col_3[i])
            self->col_3[i] = ui_static_text_construct(self->col_3[i]);
        ui_window_create(self->col_3[i], 0x27, 0xf);
        ui_static_text_set_text_ellipsis(self->col_3[i], "");
        ui_window_move(self->col_3[i], g_guildLayout.member_position_cols[0] + 0x3a, y);
        ui_window_add_child(wnd, self->col_3[i]);

        self->col_position[i] = mem_alloc(sizeof(struct UIStaticText));
        if (self->col_position[i])
            self->col_position[i] = ui_static_text_construct(self->col_position[i]);
        ui_window_create(self->col_position[i], 0x27, 0xf);
        ui_static_text_set_text_ellipsis(self->col_position[i], "");
        ui_window_move(self->col_position[i], g_guildLayout.member_position_cols[0] + 0x65, y);
        ui_window_add_child(wnd, self->col_position[i]);
    }
    if (g_guildInfo.is_master) {
        for (i = 0, y = MEMBER_BUTTON_TOP; y < 0x12a; i++, y += MEMBER_ROW_HEIGHT) {
            struct UIBitmapButton *button = mem_alloc(sizeof(struct UIBitmapButton));

            if (button)
                button = ui_bitmap_button_construct(button);
            self->pos_buttons[i] = button;
            ui_bitmap_button_set_bitmap(button, UI_SKIN "basic_interface\\txtbox_btn_a.bmp", UI_BTN_UP, 0);
            ui_bitmap_button_set_bitmap(button, UI_SKIN "basic_interface\\txtbox_btn_b.bmp", UI_BTN_HOVER, 0);
            ui_bitmap_button_set_bitmap(button, UI_SKIN "basic_interface\\txtbox_btn_c.bmp", UI_BTN_DOWN, 0);
            ui_window_create(&button->button.base, button->bitmap_width, button->bitmap_height);
            ui_window_move_base(&button->button.base, MEMBER_BUTTON_X, y);
            button->button.base.id = GUILD_ID_TEXT_BUTTON;
            ui_window_add_child(wnd, &button->button.base);
        }
    }
    self->bar = mem_alloc(sizeof(struct UIScrollBar));
    if (self->bar)
        self->bar = ui_scroll_bar_construct(self->bar);
    ui_scroll_bar_create(self->bar, wnd->width - 0x10, 0x2b, UI_SCROLL_VERT, self->row_gap + 0xf1);
    ui_window_add_child(wnd, &self->bar->base);

    self->ok_button = guild_add_ok_button();
    guild_place_ok(wnd, self->ok_button, self->changes.size != 0);
    self->ok_button->button.base.id = UI_ID_OK;
    ui_window_add_child(wnd, &self->ok_button->button.base);
    self->base.frame.default_id = UI_ID_OK;

    check = mem_alloc(sizeof(struct UICheckBox));
    if (check)
        check = ui_check_box_construct((struct UIWindow *)check);
    self->sort_check = check;
    ui_window_create(&check->base, 0xf0, 0xc);
    ui_check_box_set_bitmaps(&check->base, UI_SKIN "checkbox_1.bmp", UI_SKIN "checkbox_0.bmp", msg_str(0xb30));
    ui_check_box_fit_to_label(check);
    ui_window_move_base(&check->base, 0xc, wnd->height - 0x12);
    ui_check_box_set_check(&check->base, g_session1.guildMemberSort);
    ui_window_add_child(wnd, &check->base);
    ui_guild_member_manage_wnd_update_scroll(self);
}

void ui_guild_member_manage_wnd_on_draw(struct UIWindow *window)
{
    struct UIGuildMemberManageWnd *self = (struct UIGuildMemberManageWnd *)window;
    struct UIWindow *wnd = GUILD_WND(self);
    struct ListNode *node;
    COLORREF frame;
    const char *titles[GUILD_MEMBER_COLUMNS];
    int i, lines, x, row, index;

    ui_guild_wnd_on_draw((struct UIWindow *)(&self->base));
    for (i = 0; i < GUILD_MEMBER_ROWS; i++) {
        ui_window_move(self->col_name[i], GUILD_OFF_SCREEN, GUILD_OFF_SCREEN);
        ui_window_move(self->col_2[i], GUILD_OFF_SCREEN, GUILD_OFF_SCREEN);
        ui_window_move(self->col_3[i], GUILD_OFF_SCREEN, GUILD_OFF_SCREEN);
        ui_window_move(self->col_position[i], GUILD_OFF_SCREEN, GUILD_OFF_SCREEN);
        if (self->pos_buttons[i])
            ui_window_move_base(&self->pos_buttons[i]->button.base, GUILD_OFF_SCREEN, GUILD_OFF_SCREEN);
    }
    if (g_session1.guildMemberSort) {
        guild_member_sort_online_first(&self->members);
    }
    frame = ui_skin_colour(0xe, 6);
    ui_window_draw_outline(wnd, MEMBER_LIST_X, 0x2e, MEMBER_LIST_WIDTH, self->row_gap / 2 + 0xed, frame);
    ui_window_fill_rect(wnd, MEMBER_LIST_X, MEMBER_ROW_TOP, MEMBER_LIST_WIDTH, 1, frame);
    lines = GUILD_MEMBER_ROWS - GUILD_MEMBER_ROWS * self->row_gap / 36;
    for (i = 1; i <= lines; i++)
        ui_window_fill_rect(wnd, MEMBER_LIST_X, (self->row_gap + MEMBER_ROW_HEIGHT) * i + MEMBER_ROW_TOP,
                          MEMBER_LIST_WIDTH, 1, frame);
    if (self->sort_column == 0)
        ui_window_fill_rect(wnd, 6, MEMBER_HEADER_TOP, g_guildLayout.member_position_cols[0], 0x13,
                          GUILD_SORTED_COLOUR);
    x = 0;
    for (i = 1; i < GUILD_MEMBER_COLUMNS; i++) {
        x += g_guildLayout.member_position_cols[i - 1] + 1;
        ui_window_fill_rect(wnd, x + 5, 0x2e, 1, self->row_gap / 2 + 0xed, frame);
        if (self->sort_column == i)
            ui_window_fill_rect(wnd, x + 6, MEMBER_HEADER_TOP, g_guildLayout.member_position_cols[i], 0x13,
                              GUILD_SORTED_COLOUR);
    }
    for (i = 0; i < GUILD_MEMBER_COLUMNS; i++)
        titles[i] = msg_str(g_memberHeaders[i]);
    x = 10;
    for (i = 0; i < GUILD_MEMBER_COLUMNS; i++) {
        ui_window_text_out(wnd, x, 0x33, titles[i], 0, 0, 12, 0, 0, 0);
        if (i == self->sort_column) {
            int arrow_x = ui_window_text_width(wnd, titles[i], 0, 0, 12, 0, 0) + x;
            const char *arrow = NULL;
            int dir = *current_sort_dir(self);

            if (dir == GUILD_SORT_UP)
                arrow = UI_SKIN "guild\\guild_arrow_up.bmp";
            else if (dir == GUILD_SORT_DOWN)
                arrow = UI_SKIN "guild\\guild_arrow_down.bmp";
            if (arrow)
                ui_window_draw_bitmap(wnd, arrow_x, 0x35, ui_skin_bitmap(arrow), 1);
        }
        x += 1 + g_guildLayout.member_position_cols[i];
    }
    row = 0;
    index = 0;
    for (node = self->members.head->next; node != self->members.head; node = node->next) {
        if (ui_guild_member_manage_wnd_draw_row(self, LIST_VALUE(node, struct GuildMember), &row, index)) {
            row++;
            index++;
        }
    }
}

static int hit_member(struct UIGuildMemberManageWnd *self, int x, int y)
{
    int index;

    if ((unsigned)(x - MEMBER_LIST_X) > MEMBER_LIST_WIDTH || (unsigned)(y - MEMBER_ROW_TOP) > MEMBER_LIST_HEIGHT)
        return -1;
    index = (y - MEMBER_ROW_TOP) / (self->row_gap + MEMBER_ROW_HEIGHT) + self->per_row * self->top;
    if ((unsigned)index >= self->members.size)
        return -1;
    return index;
}

static void select_at(struct UIGuildMemberManageWnd *self, int x, int y)
{
    int old = self->selected;

    self->selected = hit_member(self, x, y);
    if (old != self->selected)
        ui_window_invalidate_base(&self->base.frame.base);
}

void ui_guild_member_manage_wnd_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIGuildMemberManageWnd *self = (struct UIGuildMemberManageWnd *)window;
    ui_guild_wnd_on_l_btn_down((struct UIWindow *)(&self->base), x, y);
    if (x >= 6 && x <= 0x17d && (unsigned)(y - MEMBER_HEADER_TOP) <= 0x13) {
        int column, left = 6;

        for (column = 0; column < GUILD_MEMBER_COLUMNS; column++) {
            int right = g_guildLayout.member_position_cols[column] + left;

            if (left <= x && x <= right) {
                ui_guild_member_manage_wnd_sort_by(self, column);
                return;
            }
            left = right + 1;
        }
        return;
    }
    select_at(self, x, y);
}

void ui_guild_member_manage_wnd_on_r_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIGuildMemberManageWnd *self = (struct UIGuildMemberManageWnd *)window;
    select_at(self, x, y);
}

void ui_guild_member_manage_wnd_on_r_btn_up(struct UIWindow *window, int x, int y)
{
    struct UIGuildMemberManageWnd *self = (struct UIGuildMemberManageWnd *)window;
    ui_guild_member_manage_wnd_send_msg(&self->base.frame.base, NULL, GUILD_MSG_OPEN_MENU, 0, 0, 0, 0);
}

void ui_guild_member_manage_wnd_on_wheel(struct UIWindow *window, int delta)
{
    struct UIGuildMemberManageWnd *self = (struct UIGuildMemberManageWnd *)window;
    int step;

    if (!self->scrollable)
        return;
    step = self->rows <= 3 ? self->rows - 1 : 3;
    if (delta >= 0)
        step = -step;
    ui_guild_member_manage_wnd_send_msg(&self->base.frame.base, &self->bar->base, UI_MSG_SCROLL_V, step, 0, 0, 0);
}

void ui_guild_member_manage_wnd_on_process(struct UIWindow *window)
{
    struct UIGuildMemberManageWnd *self = (struct UIGuildMemberManageWnd *)window;
    if (self->ok_time && platform_ticks_ms() > self->ok_time + GUILD_OK_REENABLE_MS) {
        self->ok_time = 0;
        self->ok_button->button.disabled = 0;
        ui_window_invalidate_base(&self->ok_button->button.base);
    }
}

static void send_changes(struct UIGuildMemberManageWnd *self)
{
    struct Mode *mode;

    if (!self->changes.size)
        return;
    mode = client_current_mode();
    set_guild_member_edits(self);
    mode_send_message(mode, MODE_REQ_GUILD_MEMBER_CHANGES, 0, 0, 0, 0);
    tree_clear(&self->changes, NULL);
    self->ok_button->button.disabled = 1;
    ui_window_invalidate_base(&self->ok_button->button.base);
    self->ok_time = platform_ticks_ms();
}

static void open_positions(struct UIGuildMemberManageWnd *self, struct UIWindow *sender)
{
    struct UIWindow *combo;
    int row = 0, i, count, n, height, x, y;

    if (!self->pos_buttons[0])
        return;
    for (i = 0; i < GUILD_MEMBER_ROWS; i++) {
        if (&self->pos_buttons[i]->button.base == sender) {
            row = i;
            break;
        }
    }
    self->menu_row = self->selected = self->top + row;
    ui_window_invalidate_base(&self->base.frame.base);
    if (g_windowMgr.combo_box_wnd)
        ui_close_window(WND_ID_COMBO);
    combo = ui_make_window(WND_ID_COMBO);
    count = (int)g_guildInfo.position_names.size;
    n = 0;
    for (; n < count; n++) {
        char text[0x40];

        sprintf(text, "%s", string_cstr(position_name(n)));
        ui_window_send_msg(combo, NULL, COMBO_MSG_ADD_LINE, (int)text, 0, 0, 0);
    }
    height = n * 16;
    if (height >= 0x50)
        height = 0x50;
    ui_window_send_msg(combo, NULL, COMBO_MSG_SELECT, 0, 0, 0, 0);
    ui_window_resize(combo, 0x96, height);
    ui_window_send_msg(combo, NULL, COMBO_MSG_OPENER_ID, MEMBER_TAB_WND_ID, 0, 0, 0);
    x = 0x58;
    y = (self->row_gap + MEMBER_ROW_HEIGHT) * row + self->row_gap + 0x65;
    ui_window_to_screen(GUILD_WND(self), &x, &y);
    ui_window_move(combo, x + 1, y + 1);
}

static void position_chosen(struct UIGuildMemberManageWnd *self, int position)
{
    struct ListNode *node;
    int index = 0;

    for (node = self->members.head->next; node != self->members.head; node = node->next, index++) {
        if (self->selected == index) {
            struct GuildMember *m = LIST_VALUE(node, struct GuildMember);

            if (m->position != position && position && m->position) {
                struct GuildMemberEdit *edit;
                const struct String *name;

                m->position = position;
                name = position_name(m->position);
                string_assign(&m->position_name, string_cstr(name), name->size);
                edit = tree_get(&self->changes, &m->gid, sizeof m->gid, sizeof *edit, tree_less_int);
                edit->change.aid = m->aid;
                edit->change.gid = m->gid;
                edit->change.position = m->position;
            }
            break;
        }
    }
    guild_place_ok(GUILD_WND(self), self->ok_button, self->changes.size != 0);
    ui_window_invalidate_base(&self->base.frame.base);
}

static void open_menu(struct UIGuildMemberManageWnd *self)
{
    struct UIMenuEntry *entry;
    struct GuildMember m;

    for (entry = self->menu.first; entry != self->menu.last; entry++)
        string_free(&entry->text);
    self->menu.last = self->menu.first;
    if (g_windowMgr.menu_wnd)
        ui_close_window(WND_ID_MENU);
    if (self->selected != -1) {
        guild_member_construct(&m);
        if (ui_guild_member_manage_wnd_get_member(self, self->selected, &m) == 1) {
            int master = g_guildInfo.master_gid;

            if (m.gid == g_session1.ownGid) {
                if (master ? m.gid != master : !string_equal(&m.name, &g_guildInfo.master_name))
                    menu_add(&self->menu, 0x1fc, MENU_LEAVE);
                guild_member_destruct(&m);
                goto open;
            }
            if (master) {
                if (master == g_session1.ownGid) {
                    menu_add(&self->menu, 0x1fd, MENU_EXPEL);
                    menu_add(&self->menu, 0xb6b, MENU_POSITION_0);
                } else if (g_guildInfo.can_expel && m.gid != master) {
                    menu_add(&self->menu, 0x1fd, MENU_EXPEL);
                }
            } else {
                const char *me = session_character_name(&g_session1);

                if (string_equal_cstr(&g_guildInfo.master_name, me)) {
                    menu_add(&self->menu, 0x1fd, MENU_EXPEL);
                    menu_add(&self->menu, 0xb6b, MENU_POSITION_0);
                } else if (g_guildInfo.can_expel && !string_equal(&m.name, &g_guildInfo.master_name)) {
                    menu_add(&self->menu, 0x1fd, MENU_EXPEL);
                }
            }
            if (m.online == 1) {
                int invite = 0;

                if (g_session1.partyLeader) {
                    struct PartyMember member;

                    if (session_get_party_member_by_name(&g_session1, &member, string_cstr(&m.name))
                            ->found == 0)
                        invite = 1;
                    string_free(&member.map_name);
                    string_free(&member.name);
                }
                if (invite)
                    menu_add(&self->menu, 0x532, MENU_INVITE);
                menu_add(&self->menu, 0x7fb, MENU_WHISPER);
            }
            menu_add(&self->menu, 0xaf8, MENU_SEND_MAIL);
        }
        guild_member_destruct(&m);
    }
open:
    guild_open_menu(&self->menu, MEMBER_TAB_WND_ID);
}

static void menu_choice(struct UIGuildMemberManageWnd *self, int choice)
{
    struct GuildMember outer, m;

    guild_member_construct(&outer);
    if ((ui_guild_member_manage_wnd_get_member(self, self->selected, &outer)
         && text_view_find(string_view(&outer.name), string_view(&text_tag_NAMELESS), 0) != STRING_NPOS)
        || !player_names_has(outer.gid)) {
        ui_chat_line(msg_str(MSG_NAMELESS_MEMBER), 0xff);
        guild_member_destruct(&outer);
        return;
    }
    switch (VEC_AT(&self->menu, struct UIMenuEntry, choice).id) {
    case MENU_INVITE:
        guild_member_construct(&m);
        if (ui_guild_member_manage_wnd_get_member(self, self->selected, &m))
            mode_send_message(client_current_mode(), MODE_REQ_PARTY_INVITE, (int)string_cstr(&m.name), 0, 0, 0);
        break;
    case MENU_EXPEL:
        guild_member_construct(&m);
        if (ui_guild_member_manage_wnd_get_member(self, self->selected, &m)) {
            struct GuildBanTarget target;

            target.guild_id = g_guildInfo.guild_id;
            target.aid = m.aid;
            target.char_id = m.gid;
            ui_window_send(ui_window_open(WND_ID_GUILD_EXPEL), NULL, UI_MSG_SET_INFO, (int)&target, 0, 0, 0);
        }
        break;
    case MENU_LEAVE:
        guild_member_construct(&m);
        if (ui_guild_member_manage_wnd_get_member(self, self->selected, &m) && m.aid == g_session1.ownAid) {
            g_session1.guildLeaveGid = (float)(unsigned)m.gid;
            ui_make_window(WND_ID_GUILD_LEAVE);
        }
        break;
    case MENU_MEMBER_INFO:
        guild_member_construct(&m);
        if (ui_guild_member_manage_wnd_get_member(self, self->selected, &m))
            mode_send_message(client_current_mode(), MODE_REQ_OPEN_GUILD_MEMBER_INFO, m.aid, 0, 0, 0);
        break;
    case MENU_WHISPER:
        guild_member_construct(&m);
        if (ui_guild_member_manage_wnd_get_member(self, self->selected, &m) == 1) {
            struct String gid_text;
            const char *label;

            if (ui_window_mgr_is_friend(&g_windowMgr, string_cstr(&m.name)) == 1)
                label = msg_str(MSG_LABEL_ON_LIST);
            else
                label = session_is_guild_member_char_id(&g_session1, m.gid) == 1 ? "GuildMember" : NULL;
            session_scramble_number(&g_session1, &gid_text, m.gid);
            ui_window_mgr_send_msg_(&g_windowMgr, UIWM_MSG_WHISPER, (int)string_cstr(&m.name), (int)label,
                                  (int)string_cstr(&gid_text), 0);
            string_free(&gid_text);
        }
        break;
    case MENU_SEND_MAIL:
        guild_member_construct(&m);
        if (ui_guild_member_manage_wnd_get_member(self, self->selected, &m) == 1) {
            game_send_message(MODE_REQ_OPEN_WRITE_RODEX, (int)string_cstr(&m.name), 0, 0, 0);
        }
        break;
    case MENU_POSITION_0:
        guild_member_construct(&m);
        if (ui_guild_member_manage_wnd_get_member(self, self->selected, &m) == 1) {
            struct String text;

            string_init(&text);
            string_format(&text, msg_str(0xb6c), string_cstr(&m.name), string_cstr(&m.name));
            if (ui_window_mgr_message_box(&g_windowMgr, string_cstr(&text), UI_MSGBOX_QUESTION, 0, 0, 0,
                                       "메시지", 0x118, 0x78, 0) != UI_MSGBOX_ANSWER_NO) {
                struct GuildMemberEdit *edit;
                const struct String *name;

                m.position = 0;
                name = position_name(m.position);
                string_assign(&m.position_name, string_cstr(name), name->size);
                edit = tree_get(&self->changes, &m.gid, sizeof m.gid, sizeof *edit, tree_less_int);
                edit->change.aid = m.aid;
                edit->change.gid = m.gid;
                edit->change.position = m.position;
                set_guild_member_edits(self);
                game_send_message(MODE_REQ_GUILD_MEMBER_CHANGES, 0, 0, 0, 0);
            }
            string_free(&text);
        }
        break;
    default:
        guild_member_destruct(&outer);
        return;
    }
    guild_member_destruct(&m);
    guild_member_destruct(&outer);
}

static void refresh(struct UIGuildMemberManageWnd *self)
{
    tree_clear(&self->changes, NULL);
    copy_members(self);
    ui_window_invalidate_children(GUILD_WND(self));
    guild_place_ok(GUILD_WND(self), self->ok_button, self->changes.size != 0);
    ui_guild_member_manage_wnd_update_scroll(self);
}

static void rebuild(struct UIGuildMemberManageWnd *self)
{
    struct UIWindow *wnd = GUILD_WND(self);
    BYTE master;
    int i;

    copy_members(self);
    master = g_guildInfo.is_master != 0;
    for (i = 0; i < GUILD_MEMBER_ROWS; i++) {
        struct UIBitmapButton *button = self->pos_buttons[i];

        if (button) {
            ui_button_show((struct UIButton *)(button), master);
            continue;
        }
        button = mem_alloc(sizeof(struct UIBitmapButton));
        if (button)
            button = ui_bitmap_button_construct(button);
        self->pos_buttons[i] = button;
        ui_bitmap_button_set_bitmap(button, UI_SKIN "basic_interface\\txtbox_btn_a.bmp", UI_BTN_UP, 0);
        ui_bitmap_button_set_bitmap(button, UI_SKIN "basic_interface\\txtbox_btn_b.bmp", UI_BTN_HOVER, 0);
        ui_bitmap_button_set_bitmap(button, UI_SKIN "basic_interface\\txtbox_btn_c.bmp", UI_BTN_DOWN, 0);
        ui_window_create(&button->button.base, button->bitmap_width, button->bitmap_height);
        ui_window_move_base(&button->button.base, MEMBER_BUTTON_X, (self->row_gap + MEMBER_ROW_HEIGHT) * i + self->row_gap + 0x52);
        button->button.base.id = GUILD_ID_TEXT_BUTTON;
        ui_window_add_child(wnd, &button->button.base);
    }
    flip_sort_dir(self);
    ui_guild_member_manage_wnd_sort_by(self, 1);
    hand_members_to_guild(self);
}

#define MEMBER_MSG_REBUILD 0x6a

int ui_guild_member_manage_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                              int val1, int val2, int val3, int val4)
{
    struct UIGuildMemberManageWnd *self = (struct UIGuildMemberManageWnd *)window;
    switch (msg) {
    case UI_MSG_COMMAND:
        switch (val1) {
        case UI_ID_OK:
            send_changes(self);
            break;
        case UI_ID_CLOSE:
            ui_close_window(GUILD_WND_ID);
            break;
        case UI_ID_CHECKBOX:
            g_session1.guildMemberSort = g_session1.guildMemberSort == 0;
            ui_window_invalidate_base(&self->base.frame.base);
            break;
        case GUILD_ID_TEXT_BUTTON:
            open_positions(self, sender);
            break;
        }
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
            ui_guild_member_manage_wnd_update_scroll(self);
            ui_window_invalidate_base(&self->base.frame.base);
        }
        return self->top - old;
    }
    case COMBO_MSG_CHOSEN:
        position_chosen(self, val1);
        return 0;
    case GUILD_MSG_MENU_CHOICE:
        menu_choice(self, val1);
        return 0;
    case GUILD_MSG_OPEN_MENU:
        open_menu(self);
        return 0;
    case GUILD_MSG_REFRESH:
        refresh(self);
        return 0;
    case MEMBER_MSG_REBUILD:
        rebuild(self);
        return 0;
    }
    return ui_guild_wnd_send_msg((struct UIWindow *)(&self->base), sender, msg, val1, val2, val3, val4);
}
