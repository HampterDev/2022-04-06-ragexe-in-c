#include "ui/controls/edit.h"
#include "ui/social/guild_windows.h"
#include "contents/guild_info.h"
#include "world/mode_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ui/social/guild_manage.h"
#include "core/object_type_tables.h"
#include "core/msg_str.h"
#include "core/text_util.h"
#include "diag/perf.h"
#include "std/tree.h"
#include "platform/clock.h"

#define POS_COL(i) (g_guildLayout.member_position_cols[6 + (i)])
#define POS_COLUMNS 6
#define POS_GRID_ROWS 12
#define POS_ROW_TOP 0x42
#define POS_TEXT_TOP 0x44
#define POS_EDIT_GREY 0xf2
#define POS_NAME_MAX_CHARS 0x1e
#define POS_TAX_MAX_CHARS 2
#define MSG_TAX_RANGE 0xd9e

static const int g_positionHeaders[POS_COLUMNS] = { 0x1fe, 0x1ff, 0x200, 0x201, 0x9c3, 0x202 };

static struct UICheckBox *add_right_box(struct UIGuildPositionManageWnd *self, int x, int y)
{
    struct UICheckBox *box = mem_alloc(sizeof(struct UICheckBox));

    if (box)
        box = ui_check_box_construct((struct UIWindow *)box);
    ui_window_create(&box->base, 0xc, 0xc);
    ui_check_box_set_bitmaps(&box->base, UI_SKIN "checkbox_1.bmp", UI_SKIN "checkbox_0.bmp", "");
    ui_window_move_base(&box->base, x, y);
    box->base.state = 0;
    ui_window_invalidate_base(&box->base);
    ui_window_add_child(GUILD_WND(self), &box->base);
    return box;
}

static struct UIWindow *add_edit(struct UIGuildPositionManageWnd *self, int width, int x, int y, int max_chars,
                                 int id)
{
    struct UIWindow *edit = mem_alloc(sizeof(struct UIEditCtrl));

    if (edit) {
        ui_edit_ctrl_construct(edit);
        edit->ops = &ui_edit_ctrl_var_id_ops.base;
    }
    ui_window_create(edit, width, 0x10);
    ui_window_move_base(edit, x, y);
    UI_EDIT_MAX_CHARS(edit) = max_chars;
    ui_edit_ctrl_set_back_colour(edit, POS_EDIT_GREY, POS_EDIT_GREY, POS_EDIT_GREY);
    edit->id = id;
    ui_window_add_child(GUILD_WND(self), edit);
    return edit;
}

static struct UIWindow *add_text(struct UIGuildPositionManageWnd *self, int width, int x, int y)
{
    struct UIWindow *text = mem_alloc(sizeof(struct UIStaticText));

    if (text)
        text = ui_static_text_construct(text);
    ui_window_create(text, width, 0xf);
    ui_static_text_set_text_ellipsis(text, "");
    ui_window_move(text, x, y);
    ui_window_add_child(GUILD_WND(self), text);
    return text;
}

static void destroy_position(void *value)
{
    guild_position_destruct(value);
}

static void copy_positions(struct UIGuildPositionManageWnd *self)
{
    struct ListNode *node;

    if (&self->positions == &g_guildInfo.positions)
        return;
    list_clear(&self->positions, destroy_position);
    for (node = g_guildInfo.positions.head->next; node != g_guildInfo.positions.head; node = node->next) {
        struct GuildPosition copy;

        guild_position_copy(&copy, LIST_VALUE(node, struct GuildPosition));
        list_push_back(&self->positions, &copy, sizeof copy);
    }
}

void ui_guild_position_manage_wnd_update_scroll(struct UIGuildPositionManageWnd *self)
{
    if (self->bar)
        guild_update_scroll(self->bar, self->positions.size, self->per_row, self->rows, &self->top,
                            &self->scrollable);
}

void ui_guild_position_manage_wnd_on_create(struct UIWindow *window, int width_unused,
                                                  int height_unused)
{
    struct UIGuildPositionManageWnd *self = (struct UIGuildPositionManageWnd *)window;
    struct UIWindow *wnd = GUILD_WND(self);
    int i;

    ui_guild_wnd_init_tab_text(&self->base);
    copy_positions(self);
    if (strcmp(string_cstr(&g_guildInfo.master_name), "") == 0)
        mode_send_message(client_current_mode(), MODE_REQ_GUILD_INFO, 0, 0, 0, 0);
    g_guildInfo.page_asked[GUILD_PAGE_POSITIONS] = 1;
    mode_send_message(client_current_mode(), MODE_REQ_GUILD_INFO, GUILD_PAGE_POSITIONS, 0, 0, 0);
    self->top = 0;
    self->selected = 0;
    self->per_row = 1;
    self->rows = GUILD_POSITION_ROWS;
    self->scrollable = 0;
    self->bar = mem_alloc(sizeof(struct UIScrollBar));
    if (self->bar)
        self->bar = ui_scroll_bar_construct(self->bar);
    ui_scroll_bar_create(self->bar, wnd->width - 0xf, 0x29, UI_SCROLL_VERT, wnd->height - 0x46);
    ui_window_add_child(wnd, &self->bar->base);
    if (g_guildInfo.is_master) {
        for (i = 0; i < GUILD_POSITION_ROWS; i++)
            self->invite_checks[i] = add_right_box(self, wnd->width - 0x94, g_guildLayout.position_row_height * i + POS_ROW_TOP);
        for (i = 0; i < GUILD_POSITION_ROWS; i++)
            self->expel_checks[i] = add_right_box(self, wnd->width - 0x30, g_guildLayout.position_row_height * i + POS_ROW_TOP);
        for (i = 0; i < GUILD_POSITION_ROWS; i++)
            self->storage_checks[i] = add_right_box(self, wnd->width - 0x7c, g_guildLayout.position_row_height * i + POS_ROW_TOP);
    }
    if (g_guildInfo.is_master) {
        for (i = 0; i < GUILD_POSITION_ROWS; i++)
            self->name_edits[i] = add_edit(self, POS_COL(1) - 4, POS_COL(0) + 9, g_guildLayout.position_row_height * i + POS_ROW_TOP,
                                           POS_NAME_MAX_CHARS, GUILD_ID_POSITION_NAME);
        for (i = 0; i < GUILD_POSITION_ROWS; i++)
            self->tax_edits[i] = add_edit(self, POS_COL(4) / 2 - 4,
                                          POS_COL(1) + 0xc + POS_COL(3) + POS_COL(2) + POS_COL(0),
                                          g_guildLayout.position_row_height * i + POS_ROW_TOP, POS_TAX_MAX_CHARS, GUILD_ID_POSITION_TAX);
    } else {
        for (i = 0; i < GUILD_POSITION_ROWS; i++)
            self->name_texts[i] = add_text(self, POS_COL(1) - 4, POS_COL(0) + 9, g_guildLayout.position_row_height * i + POS_ROW_TOP);
        for (i = 0; i < GUILD_POSITION_ROWS; i++)
            self->tax_texts[i] = add_text(self, POS_COL(4) / 2 - 4,
                                          POS_COL(1) + 0xc + POS_COL(3) + POS_COL(2) + POS_COL(0),
                                          g_guildLayout.position_row_height * i + POS_ROW_TOP);
    }
    self->ok_button = guild_add_ok_button();
    guild_place_ok(wnd, self->ok_button, self->changes.size != 0);
    self->ok_button->button.base.id = UI_ID_OK;
    ui_window_add_child(wnd, &self->ok_button->button.base);
    self->base.frame.default_id = UI_ID_OK;
    ui_guild_position_manage_wnd_update_scroll(self);
}

static void hide(struct UIWindow *control)
{
    if (control)
        ui_window_move(control, GUILD_OFF_SCREEN, GUILD_OFF_SCREEN);
}

static void show_right(struct UIGuildPositionManageWnd *self, struct UICheckBox *box, int value, int x, int y)
{
    if (box) {
        box->base.state = value;
        ui_window_invalidate_base(&box->base);
        ui_window_move_base(&box->base, x, y);
    } else {
        char path[0x80];

        sprintf(path, UI_SKIN "checkbox_%d.bmp", value != 0);
        ui_window_draw_bitmap(GUILD_WND(self), x, y, res_mgr_get(g_resMgr(), path), 0);
    }
}

void ui_guild_position_manage_wnd_on_draw(struct UIWindow *window)
{
    struct UIGuildPositionManageWnd *self = (struct UIGuildPositionManageWnd *)window;
    struct UIWindow *wnd = GUILD_WND(self);
    struct ListNode *node;
    const char *titles[POS_COLUMNS];
    int cols[POS_COLUMNS];
    COLORREF grid;
    int i, row, x, index, shown;

    ui_guild_wnd_on_draw((struct UIWindow *)(&self->base));
    for (i = 0; i < GUILD_POSITION_ROWS; i++) {
        hide(&self->invite_checks[i]->base);
        hide(&self->expel_checks[i]->base);
        hide(self->tax_texts[i]);
        hide(self->name_edits[i]);
        hide(self->name_texts[i]);
        hide(self->tax_edits[i]);
        hide(&self->storage_checks[i]->base);
    }
    ui_window_fill_rect(wnd, 4, 0x2d, wnd->width - 0x16, wnd->height - g_guildLayout.position_row_height - 0x45,
                      ui_skin_colour(0xe, 6));
    for (i = 0; i < POS_COLUMNS; i++)
        cols[i] = POS_COL(i);
    grid = ui_skin_colour(2, 2);
    for (row = 0; row < POS_GRID_ROWS; row++) {
        x = 0;
        for (i = 0; i < POS_COLUMNS; i++) {
            ui_window_fill_rect(wnd, x + 5, g_guildLayout.position_row_height * row + 0x2e, cols[i], g_guildLayout.position_row_height - 1, grid);
            x += 1 + cols[i];
        }
    }
    for (i = 0; i < POS_COLUMNS; i++)
        titles[i] = msg_str(g_positionHeaders[i]);
    x = 0;
    for (i = 0; i < POS_COLUMNS; i++) {
        ui_window_text_out(wnd, x + 8, 0x31, titles[i], 0, 0, 12, 0, 0, 0);
        x += 1 + cols[i];
    }
    index = 0;
    shown = 0;
    for (node = self->positions.head->next; node != self->positions.head; node = node->next, index++) {
        struct GuildPosition *p = LIST_VALUE(node, struct GuildPosition);
        char text[0x200];
        int y;

        if (index < self->top * self->per_row)
            continue;
        y = shown / self->per_row * g_guildLayout.position_row_height + POS_TEXT_TOP;
        if (self->selected == index)
            ui_window_fill_rect(wnd, 5, y - 3, wnd->width - 0x18, 0x12, ui_skin_colour(6, 2));
        sprintf(text, "%d", p->ranking);
        ui_window_text_out(wnd, 8, y, text, 0, 0, 12, 0, 0, 0);
        if (self->name_edits[shown]) {
            ui_edit_ctrl_set_text((struct UIEditCtrl *)self->name_edits[shown], string_cstr(&p->name));
            ui_window_move(self->name_edits[shown], POS_COL(0) + 9, y - 2);
        } else if (self->name_texts[shown]) {
            ui_static_text_set_text_ellipsis(self->name_texts[shown], string_cstr(&p->name));
            ui_window_move(self->name_texts[shown], POS_COL(0) + 9, y - 2);
        }
        show_right(self, self->invite_checks[shown], p->can_invite, POS_COL(0) + 0xa + POS_COL(1), y);
        show_right(self, self->expel_checks[shown], p->can_expel, POS_COL(0) + 0xb + POS_COL(1) + POS_COL(2), y);
        show_right(self, self->storage_checks[shown], p->can_use_storage,
                   POS_COL(0) + 0xc + POS_COL(1) + POS_COL(2) + POS_COL(3), y);
        x = POS_COL(0) + 0xd + POS_COL(1) + POS_COL(2) + POS_COL(3) + POS_COL(4);
        if (self->tax_edits[shown]) {
            sprintf(text, "%d", p->exp_tax);
            ui_edit_ctrl_set_text((struct UIEditCtrl *)self->tax_edits[shown], text);
            ui_window_move(self->tax_edits[shown], x, y - 2);
            ui_window_text_out(wnd, x + POS_COL(5) / 2, y, "%", 0, 0, 12, 0, 0, 0);
        } else if (self->tax_texts[shown]) {
            sprintf(text, "%d %s", p->exp_tax, "%");
            ui_static_text_set_text(self->tax_texts[shown], text, 0);
            ui_window_move(self->tax_texts[shown], x, y - 2);
        }
        if (++shown == self->rows * self->per_row)
            break;
    }
}

void ui_guild_position_manage_wnd_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIGuildPositionManageWnd *self = (struct UIGuildPositionManageWnd *)window;
    int old;

    ui_guild_wnd_on_l_btn_down((struct UIWindow *)(&self->base), x, y);
    old = self->selected;
    self->selected = ui_guild_position_manage_wnd_hit_test(self, x, y);
    if (old != self->selected)
        ui_guild_position_manage_wnd_invalidate(&self->base.frame.base);
}

void ui_guild_position_manage_wnd_on_r_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIGuildPositionManageWnd *self = (struct UIGuildPositionManageWnd *)window;
    int old = self->selected;

    self->selected = ui_guild_position_manage_wnd_hit_test(self, x, y);
    if (old != self->selected)
        ui_guild_position_manage_wnd_invalidate(&self->base.frame.base);
}

void ui_guild_position_manage_wnd_on_wheel(struct UIWindow *window, int delta)
{
    struct UIGuildPositionManageWnd *self = (struct UIGuildPositionManageWnd *)window;
    int step;

    if (!self->scrollable)
        return;
    step = self->rows <= 3 ? self->rows - 1 : 3;
    if (delta >= 0)
        step = -step;
    ui_guild_position_manage_wnd_send_msg(&self->base.frame.base, &self->bar->base, UI_MSG_SCROLL_V, step, 0, 0, 0);
}

void ui_guild_position_manage_wnd_on_process(struct UIWindow *window)
{
    struct UIGuildPositionManageWnd *self = (struct UIGuildPositionManageWnd *)window;
    if (self->ok_time && platform_ticks_ms() > self->ok_time + GUILD_OK_REENABLE_MS) {
        self->ok_time = 0;
        self->ok_button->button.disabled = 0;
        ui_window_invalidate_base(&self->ok_button->button.base);
    }
}

static void keep_change(struct UIGuildPositionManageWnd *self, struct GuildPosition *p)
{
    struct GuildPositionEdit *edit = tree_get(&self->changes, &p->position_id, sizeof p->position_id, sizeof *edit,
                                              tree_less_int);

    guild_position_assign(&edit->position, p);
}

static void send_changes(struct UIGuildPositionManageWnd *self)
{
    struct Tree *to = &g_guildInfo.position_edits;
    struct Mode *mode;

    if (!self->changes.size)
        return;
    mode = client_current_mode();
    tree_free(to, guild_position_edit_destroy);
    tree_copy(to, &self->changes, sizeof(struct GuildPositionEdit), guild_position_edit_copy);
    mode_send_message(mode, MODE_REQ_GUILD_POSITION_CHANGES, 0, 0, 0, 0);
    tree_clear(&self->changes, guild_position_edit_destroy);
    ui_window_move_base(&self->ok_button->button.base, GUILD_OFF_SCREEN, GUILD_OFF_SCREEN);
    self->ok_button->button.disabled = 1;
    ui_window_invalidate_base(&self->ok_button->button.base);
    self->ok_time = platform_ticks_ms();
    ui_window_invalidate_children(GUILD_WND(self));
}

static int right_changed(struct UIGuildPositionManageWnd *self, struct UIWindow *sender, int value)
{
    int i, found;

    if (!self->invite_checks[0] || !self->expel_checks[0] || !self->storage_checks[0])
        return 0;
    for (i = 0; i < GUILD_POSITION_ROWS; i++) {
        struct GuildPosition *p;

        if (&self->invite_checks[i]->base == sender) {
            p = ui_guild_position_manage_wnd_get_position(self, self->top + i, &found);
            if (found) {
                p->can_invite = value;
                keep_change(self, p);
            }
        }
        if (&self->expel_checks[i]->base == sender) {
            p = ui_guild_position_manage_wnd_get_position(self, self->top + i, &found);
            if (found) {
                p->can_expel = value;
                keep_change(self, p);
            }
        }
        if (&self->storage_checks[i]->base == sender) {
            p = ui_guild_position_manage_wnd_get_position(self, self->top + i, &found);
            if (found) {
                p->can_use_storage = value;
                keep_change(self, p);
            }
        }
    }
    return 1;
}

static int edit_row(struct UIWindow **edits, struct UIWindow *sender, int fallback)
{
    int i;

    for (i = 0; i < GUILD_POSITION_ROWS; i++)
        if (edits[i] == sender)
            return i;
    return fallback;
}

static int name_changed(struct UIGuildPositionManageWnd *self, struct UIWindow *sender, int id)
{
    struct GuildPosition *p;
    int row, found;

    if (!self->name_edits[0])
        return 0;
    row = edit_row(self->name_edits, sender, id - GUILD_ID_POSITION_NAME);
    p = ui_guild_position_manage_wnd_get_position(self, self->top + row, &found);
    if (found) {
        string_assign_cstr(&p->name, ui_edit_ctrl_get_text(self->name_edits[row]));
        keep_change(self, p);
    }
    return 1;
}

static int tax_changed(struct UIGuildPositionManageWnd *self, struct UIWindow *sender, int id)
{
    struct GuildPosition *p;
    int row, found;

    if (!self->tax_edits[0])
        return 0;
    row = edit_row(self->tax_edits, sender, id - GUILD_ID_POSITION_TAX);
    p = ui_guild_position_manage_wnd_get_position(self, self->top + row, &found);
    if (found) {
        p->exp_tax = atoi(ui_edit_ctrl_get_text(self->tax_edits[row]));
        if (p->exp_tax < 0 || p->exp_tax > GUILD_TAX_MAX) {
            struct String text;

            string_init(&text);
            string_format(&text, msg_str(MSG_TAX_RANGE), GUILD_TAX_MAX, 0);
            ui_chat_line(string_cstr(&text), 0xff);
            string_free(&text);
            if (p->exp_tax < 0)
                p->exp_tax = 0;
            else if (p->exp_tax > GUILD_TAX_MAX)
                p->exp_tax = GUILD_TAX_MAX;
        }
        keep_change(self, p);
    }
    return 1;
}

intptr_t ui_guild_position_manage_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender,
                                                int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4)
{
    struct UIGuildPositionManageWnd *self = (struct UIGuildPositionManageWnd *)window;
    int changed;

    switch (msg) {
    case UI_MSG_COMMAND:
        switch (val1) {
        case UI_ID_OK:
            send_changes(self);
            return 0;
        case UI_ID_CLOSE:
            ui_close_window(GUILD_WND_ID);
            return 0;
        case UI_ID_CHECKBOX:
            changed = right_changed(self, sender, val2);
            break;
        case GUILD_ID_POSITION_NAME:
            changed = name_changed(self, sender, val1);
            break;
        case GUILD_ID_POSITION_TAX:
            changed = tax_changed(self, sender, val1);
            break;
        default:
            return 0;
        }
        if (changed) {
            guild_place_ok(GUILD_WND(self), self->ok_button, self->changes.size != 0);
            ui_window_invalidate_children(GUILD_WND(self));
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
            ui_guild_position_manage_wnd_update_scroll(self);
            ui_guild_position_manage_wnd_invalidate(&self->base.frame.base);
        }
        return self->top - old;
    }
    case GUILD_MSG_REFRESH:
        copy_positions(self);
        ui_window_invalidate_children(GUILD_WND(self));
        guild_place_ok(GUILD_WND(self), self->ok_button, self->changes.size != 0);
        ui_guild_position_manage_wnd_update_scroll(self);
        return 0;
    }
    return ui_guild_wnd_send_msg((struct UIWindow *)(&self->base), sender, msg, val1, val2, val3, val4);
}
