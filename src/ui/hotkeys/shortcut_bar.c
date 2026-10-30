#include "ui/controls/buttons.h"
#include "world/tooltip.h"
#include "render/rp_pool.h"
#include "skills/skill_info.h"
#include "iteminfo/item_api.h"
#include "session/state.h"
#include "contents/mouse.h"
#include "core/object_type_tables.h"
#include "core/memory.h"
#include "world/mode_manager.h"
#include "game/context.h"
#include <stdio.h>
#include "ui/hotkeys/hotkey_bars.h"
#include "game/drag_info.h"
#include "ui/character/skill_dialogs.h"
#include "core/msg_str.h"
#include "core/skin_path.h"
#include "core/text_util.h"
#include "diag/perf.h"
#include "session/session_items.h"
#include "session/session_jobs.h"
#include "session/session_names.h"
#include "session/session_options.h"
#include "session/session_skills.h"
#include "session/skill_levels.h"
#include "ui/wndmgr/wndmgr.h"
#include "ui/draw_key.h"

struct UIShortCutWnd;
struct UIWindow;
void ui_short_cut_wnd_destroy(struct UIWindow *window);
void ui_short_cut_wnd_move(struct UIWindow *self, int x, int y);
void ui_short_cut_wnd_drag_and_drop(struct UIWindow *window, int x, int y, const struct DragInfo *drag);
void ui_short_cut_wnd_store_info(struct UIWindow *self);
void ui_short_cut_wnd_set_show(struct UIWindow *self, int visible);
void ui_short_cut_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_short_cut_wnd_on_process(struct UIWindow *self);
void ui_short_cut_wnd_on_draw(struct UIWindow *self);
BYTE ui_short_cut_wnd_render(struct UIWindow *self, int force);
void ui_short_cut_wnd_on_l_btn_down(struct UIWindow *self, int x, int y);
void ui_short_cut_wnd_on_l_btn_dbl_clk(struct UIWindow *self, int x, int y);
void ui_short_cut_wnd_on_mouse_hover(struct UIWindow *self, int x, int y);
void ui_short_cut_wnd_on_r_btn_down(struct UIWindow *self, int x, int y);
int ui_short_cut_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_short_cut_wnd_ops = {
    .destroy = ui_short_cut_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 1,
    .move = ui_short_cut_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_short_cut_wnd_drag_and_drop,
    .store_info = ui_short_cut_wnd_store_info,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_short_cut_wnd_set_show,
    .on_create = ui_short_cut_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_short_cut_wnd_on_process,
    .on_draw = ui_short_cut_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_short_cut_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_short_cut_wnd_on_l_btn_dbl_clk,
    .on_mouse_move = ui_frame_wnd_on_mouse_move,
    .on_mouse_hover = ui_short_cut_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_frame_wnd_on_l_btn_up,
    .on_r_btn_down = ui_short_cut_wnd_on_r_btn_down,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_short_cut_wnd_send_msg,
    .invalidate = ui_window_invalidate_base,
    .render = ui_short_cut_wnd_render,
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

#define SHORTCUT_COLOUR_HOVER 0xb4ffb4
#define SHORTCUT_COLOUR_ROW_NUMBER 0x2f4a4a
#define SHORTCUT_FONT_HEIGHT 0xb
#define SHORTCUT_ICON_FALLBACK UI_SKIN "item\\am_berserkpitcher.bmp"
#define SHORTCUT_FRAME UI_SKIN "Shortcut\\shortcut_frame.bmp"
#define GRADE_ICON UI_SKIN "grade_enchant\\grade_icon%d.bmp"
#define DRAG_FROM_STORAGE 6

void ui_short_cut_wnd_destruct(struct UIShortCutWnd *self)
{
    self->frame.base.ops = &ui_short_cut_wnd_ops;
    ui_frame_wnd_destruct(&self->frame);
}

void ui_short_cut_wnd_destroy(struct UIWindow *window)
{
    struct UIShortCutWnd *self = (struct UIShortCutWnd *)window;
    ui_short_cut_wnd_destruct(self);
    mem_free(self);
}

void ui_short_cut_wnd_move(struct UIWindow *window, int x, int y)
{
    struct UIShortCutWnd *self = (struct UIShortCutWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    int sum;

    wnd->x = x;
    wnd->y = y;
    ui_window_mgr_get_snap_delta(&g_windowMgr, wnd, &x, &y, 0);
    wnd->x += x;
    sum = wnd->y + y;
    wnd->y = sum < 0 ? 0 : sum;
    ui_window_mgr_update_snap_rect(&g_windowMgr, wnd);
}

int ui_short_cut_wnd_hit_slot(struct UIShortCutWnd *self, int x, int y)
{
    int column, row;

    self->hover_row = -1;
    if (y <= 3 || y >= self->rows * SHORTCUT_ROW_HEIGHT)
        return -1;
    if ((unsigned)(x - 0xf) > 0x104)
        return -1;
    row = (y - 4) / SHORTCUT_ROW_HEIGHT;
    self->hover_row = row;
    g_session1.shortenSkillPage = (BYTE)row;
    if (y > self->hover_row * SHORTCUT_ROW_HEIGHT + 0x1e)
        self->hover_row = -1;
    column = (x - 0xf) / SHORTCUT_SLOT_WIDTH;
    if (x > column * SHORTCUT_SLOT_WIDTH + 0x29)
        return -1;
    return column;
}

static struct HotkeySlot *slot_at(struct UIShortCutWnd *self, int column)
{
    return self->slots[column + g_session1.shortenSkillPage * HOTKEY_SLOTS_PER_ROW];
}

const struct HotkeySlot *ui_short_cut_wnd_slot(const struct UIWindow *window, int index)
{
    return ((const struct UIShortCutWnd *)window)->slots[index];
}

void ui_short_cut_wnd_drag_and_drop(struct UIWindow *window, int x, int y, const struct DragInfo *drag)
{
    struct UIShortCutWnd *self = (struct UIShortCutWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    int group = session_get_instant_var(&g_session1, SESSION_OPT_BAR_GROUP);
    int from = drag->count;

    if (drag->named) {
        struct SkillInfo info;
        int column;

        column = ui_short_cut_wnd_hit_slot(self, x, y);
        if (column != -1) {
            struct SkillCooldownNode *cd;
            int slot;

            session_get_skill_info(&info, drag->index);
            slot = column + g_session1.shortenSkillPage * HOTKEY_SLOTS_PER_ROW;
            if (info.level == 0 && info.found == 0) {
                mode_send_message(client_current_mode(), GAME_MSG_DROP_DONE, 0, 0, 0, 0);
                skill_info_destruct(&info);
                return;
            }
            session_set_bar_hotkey(&g_session1, 1, info.id, drag->level, slot, group);
            if (from != slot) {
                for (cd = g_session1.skillDelays.head; cd != NULL; cd = cd->next) {
                    struct CooldownTimerNode *t = cooldown_timers_find(&cd->value.timers_bar, slot);

                    if (t != NULL) {
                        cooldown_timers_remove(&cd->value.timers_bar, t);
                        break;
                    }
                }
            }
            cd = hotkey_cooldown_of(info.id);
            if (cd != NULL) {
                unsigned end = cd->value.end_time;

                if (skill_cooldown_running(&cd->value)) {
                    struct CooldownTimerNode *t = cooldown_timers_find(&cd->value.timers_bar, from);
                    int sx = column * SHORTCUT_SLOT_WIDTH + 0x10;
                    int sy = g_session1.shortenSkillPage * SHORTCUT_ROW_HEIGHT + 4;

                    if (t != NULL && t->value.y == sy) {
                        t->value.slot = slot;
                        t->value.x = sx;
                    } else {
                        struct CooldownTimer timer;

                        cooldown_timer_init(&timer, slot, sx, sy, cd->value.total_ms, end - cd->value.total_ms, 0, 0);
                        cooldown_timers_append(&cd->value.timers_bar, &timer);
                    }
                }
            }
        } else {
            struct SkillCooldownNode *cd;

            if (from == -1) {
                mode_send_message(client_current_mode(), GAME_MSG_DROP_DONE, 0, 0, 0, 0);
                return;
            }
            session_get_skill_info(&info, drag->index);
            cd = hotkey_cooldown_of(info.id);
            if (cd != NULL) {
                struct CooldownTimerNode *t;

                t = cooldown_timers_find(&cd->value.timers_bar, drag->count);
                if (t != NULL)
                    cooldown_timers_remove(&cd->value.timers_bar, t);
            }
            session_set_bar_hotkey(&g_session1, 1, 0, 0, from, group);
        }
        ui_short_cut_wnd_send_msg(wnd, NULL, HOTKEY_MSG_REFRESH, 0, 0, 0, 0);
        ui_window_invalidate_base(wnd);
        skill_info_destruct(&info);
    } else if (drag->source != DRAG_FROM_STORAGE) {
        struct ItemInfo item;

        session_get_inventory_item(&g_session1, &item, drag->index);
        if ((unsigned)item.index > 0 && item.identified && (unsigned)item.type <= 0x13) {
            static const BYTE kind_of_type[0x14] = { 0, 0, 0, 2, 1, 1, 2, 2, 1, 1, 0, 1, 1, 1, 1, 1, 0, 0, 2, 0 };
            int kind = kind_of_type[item.type];

            if (kind == 0 || (kind == 1 && item.equip_location != 0)) {
                int column = ui_short_cut_wnd_hit_slot(self, x, y);

                if (column != -1) {
                    int slot = column + g_session1.shortenSkillPage * HOTKEY_SLOTS_PER_ROW;

                    session_set_bar_hotkey(&g_session1, 0, item_info_get_item_id(&item), 0, slot, group);
                    if (from != slot) {
                        struct SkillCooldownNode *cd;

                        for (cd = g_session1.skillDelays.head; cd != NULL; cd = cd->next) {
                            struct CooldownTimerNode *t;

                            t = cooldown_timers_find(&cd->value.timers_bar, slot);
                            if (t != NULL) {
                                cooldown_timers_remove(&cd->value.timers_bar, t);
                                break;
                            }
                        }
                    }
                    ui_short_cut_wnd_send_msg(wnd, NULL, HOTKEY_MSG_REFRESH, 0, 0, 0, 0);
                    ui_window_invalidate_base(wnd);
                }
            }
        }
        item_info_destruct(&item);
    }
    mode_send_message(client_current_mode(), GAME_MSG_DROP_DONE, 0, 0, 0, 0);
}

void ui_short_cut_wnd_store_info(struct UIWindow *window)
{
    struct UIShortCutWnd *self = (struct UIShortCutWnd *)window;
    struct UIWindow *wnd = &self->frame.base;

    if (self->info) {
        self->info->rect.x = wnd->x;
        self->info->rect.y = wnd->y;
        self->info->rect.width = wnd->width;
        self->info->rect.height = wnd->height;
    }
}

void ui_short_cut_wnd_set_show(struct UIWindow *window, int visible)
{
    struct UIShortCutWnd *self = (struct UIShortCutWnd *)window;
    struct UIWindow *wnd = &self->frame.base;

    wnd->visible = visible;
    if (visible) {
        ui_short_cut_wnd_send_msg(wnd, NULL, HOTKEY_MSG_REFRESH, 0, 0, 0, 0);
        ui_window_invalidate_base(wnd);
        ui_window_mgr_update_snap_rect(&g_windowMgr, wnd);
    } else {
        ui_window_mgr_remove_snap_rect(&g_windowMgr, wnd);
    }
}

static struct UIBitmapButton *row_button(struct UIShortCutWnd *self, const char *normal, const char *over,
                                         const char *press, const char *disable, int id, int y, BYTE show)
{
    struct UIBitmapButton *button = mem_alloc(sizeof *button);

    if (button)
        button = ui_bitmap_button_construct(button);
    ui_bitmap_button_set_bitmap(button, normal, UI_BTN_UP, 0);
    ui_bitmap_button_set_bitmap(button, over, UI_BTN_HOVER, 0);
    ui_bitmap_button_set_bitmap(button, press, UI_BTN_DOWN, 0);
    if (disable)
        ui_bitmap_button_set_bitmap(button, disable, UI_BMP_DISABLED, 0);
    ui_window_create(&button->button.base, button->bitmap_width, button->bitmap_height);
    button->button.base.id = id;
    button->button.disabled = 0;
    ui_window_invalidate_base(&button->button.base);
    ui_window_move_base(&button->button.base, 0x114, y);
    ui_button_show((struct UIButton *)(button), show);
    ui_window_add_child(&self->frame.base, &button->button.base);
    return button;
}

void ui_short_cut_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UIShortCutWnd *self = (struct UIShortCutWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct UIWindow *close = ui_window_add_close_button(wnd, "F12", 0);
    struct UIWindow *check;

    ui_window_move_base(close, wnd->width - 10, 0);
    memset(self->slots, 0, sizeof self->slots);

    check = mem_try_alloc(sizeof(struct UICheckBox));
    if (check)
        check = (struct UIWindow *)ui_check_box_construct(check);
    self->group_check = check;
    ui_window_create(check, 10, 0x1a);
    ui_check_box_set_bitmaps(self->group_check, UI_SKIN "shortcut_2_bts.bmp", UI_SKIN "shortcut_1_bts.bmp", "");
    self->group_check->id = SHORTCUT_ID_GROUP;
    ui_window_move(self->group_check, 3, 4);
    ui_check_box_set_check(self->group_check, session_get_instant_var(&g_session1, SESSION_OPT_BAR_GROUP));
    ui_check_box_set_tool_tip(self->group_check, msg_str(MSG_SHORTCUT_GROUP_TIP));
    ui_window_add_child(wnd, self->group_check);

    self->minus = row_button(self, UI_SKIN "Shortcut\\btn_minus_normal.bmp", UI_SKIN "Shortcut\\btn_minus_over.bmp",
                             UI_SKIN "Shortcut\\btn_minus_press.bmp", NULL, SHORTCUT_ID_MINUS, 0, 0);
    self->plus = row_button(self, UI_SKIN "Shortcut\\btn_plus_normal.bmp", UI_SKIN "Shortcut\\btn_plus_over.bmp",
                            UI_SKIN "Shortcut\\btn_plus_press.bmp", UI_SKIN "Shortcut\\btn_plus_disable.bmp",
                            SHORTCUT_ID_PLUS, 0x13, 1);
    ui_short_cut_wnd_send_msg(wnd, NULL, HOTKEY_MSG_REFRESH, 0, 0, 0, 0);
}

void ui_short_cut_wnd_on_process(struct UIWindow *window)
{
    struct UIShortCutWnd *self = (struct UIShortCutWnd *)window;
    struct UIWindow *wnd = &self->frame.base;

    self->hover_column = ui_short_cut_wnd_hit_slot(self, g_mouse.x - wnd->x, g_mouse.y - wnd->y);
}

static unsigned long long short_cut_key(struct UIShortCutWnd *self)
{
    unsigned long long key = DRAW_KEY_START;
    int index;

    key = draw_key_int(key, self->rows);
    key = draw_key_int(key, self->hover_row);
    key = draw_key_int(key, self->hover_column);
    key = draw_key_int(key, g_session1.shortenSkillPage);
    for (index = 0; index < self->rows * HOTKEY_SLOTS_PER_ROW && index < HOTKEY_BAR_SLOTS; index++)
        key = hotkey_slot_key(key, self->slots[index]);
    return draw_key_children(key, &self->frame.base);
}

BYTE ui_short_cut_wnd_render(struct UIWindow *window, int force)
{
    struct UIShortCutWnd *self = (struct UIShortCutWnd *)window;
    BYTE result;

    if (short_cut_key(self) != self->drawn_key)
        ui_window_invalidate_base(window);
    result = ui_window_render_base(window, force);
    hotkey_cooldowns_draw(window, &g_session1.cooldownSlotsBar, 0, self->rows * SHORTCUT_ROW_HEIGHT + 4);
    return result;
}

void ui_short_cut_wnd_on_draw(struct UIWindow *window)
{
    struct UIShortCutWnd *self = (struct UIShortCutWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct BitmapRes *frame = res_mgr_get(g_resMgr(), ui_skin_path(SHORTCUT_FRAME));
    int row, i;

    for (row = 0; row < self->rows; row++)
        ui_window_draw_bitmap(wnd, 0, row * SHORTCUT_ROW_HEIGHT, frame, 0);
    if (self->hover_row != -1 && self->hover_column != -1)
        ui_window_fill_rect(wnd, self->hover_column * SHORTCUT_SLOT_WIDTH + 0x10,
                          g_session1.shortenSkillPage * SHORTCUT_ROW_HEIGHT + 5, 0x18, 0x18, SHORTCUT_COLOUR_HOVER);

    cooldown_slots_clear(&g_session1.cooldownSlotsBar);
    for (row = 0; row < self->rows; row++) {
        int y = 4 + row * SHORTCUT_ROW_HEIGHT;

        for (i = 0; i < HOTKEY_SLOTS_PER_ROW; i++) {
            int index = row * HOTKEY_SLOTS_PER_ROW + i;
            int x = 0x10 + i * SHORTCUT_SLOT_WIDTH;
            struct HotkeySlot *slot;
            char text[0x400];

            if (index >= HOTKEY_BAR_SLOTS)
                continue;
            slot = self->slots[index];
            if (slot->id == 0)
                continue;
            if (slot->is_skill) {
                struct SkillInfo info;

                session_get_skill_info(&info, slot->id);
                if (info.found && info.level && slot->level != 0 && info.level >= slot->level) {
                    struct BitmapRes *icon;

                    session_format_item_bitmap_path(info.id_name, text);
                    icon = res_mgr_get(g_resMgr(), text);
                    if (!icon) {
                        sprintf(text, SHORTCUT_ICON_FALLBACK);
                        icon = res_mgr_get(g_resMgr(), text);
                    }
                    cooldown_slots_add(&g_session1.cooldownSlotsBar, info.id, index, x, y);
                    ui_window_draw_bitmap(wnd, x, y, icon, 1);
                    sprintf(text, "%d", slot->level);
                    ui_window_text_out_outlined_right(wnd, x + 0x1b, y + 0x11, text, 0, 0, 0xffffff, 0,
                                                  SHORTCUT_FONT_HEIGHT, 0);
                }
                skill_info_destruct(&info);
            } else {
                struct ItemInfo item;

                session_get_inventory_item_by_id(&g_session1, &item, slot->id, 1);
                if ((unsigned)item.index > 0) {
                    ui_window_draw_item_icon(wnd, x, y, &item, item.identified);
                    if (item.count != 0) {
                        char count[16];

                        sprintf(count, "%d", item.count);
                        ui_window_text_out_outlined_right(wnd, x + 0x1b, y + 0x11, count, 0, 0, 0xffffff, 0,
                                                      SHORTCUT_FONT_HEIGHT, 0);
                    }
                    if (item.grade > 0) {
                        struct String path;

                        string_init(&path);
                        string_format(&path, GRADE_ICON, (int)item.grade);
                        ui_window_draw_bitmap(wnd, x, y + 0xc,
                                            res_mgr_get(g_resMgr(), string_cstr(&path)), 1);
                        string_free(&path);
                    }
                }
                item_info_destruct(&item);
            }
        }
    }

    for (row = 1; row <= self->rows; row++) {
        struct String number;

        string_init(&number);
        string_format(&number, "%d", row);
        ui_window_text_out(wnd, 6, 0xd + (row - 1) * SHORTCUT_ROW_HEIGHT, string_cstr(&number), 0, 0,
                         SHORTCUT_FONT_HEIGHT, SHORTCUT_COLOUR_ROW_NUMBER, 0, 0);
        string_free(&number);
    }
    self->drawn_key = short_cut_key(self);
}

void ui_short_cut_wnd_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIShortCutWnd *self = (struct UIShortCutWnd *)window;
    struct HotkeySlot *slot;
    int column, index;

    if (x - 0xf >= 0x105 || x < 0xf || y < 4) {
        ui_frame_wnd_on_l_btn_down((struct UIWindow *)(&self->frame), x, y);
        return;
    }
    if (g_replayPlaying)
        return;
    column = ui_short_cut_wnd_hit_slot(self, x, y);
    if (column == -1)
        return;
    index = column + g_session1.shortenSkillPage * HOTKEY_SLOTS_PER_ROW;
    slot = self->slots[index];
    if (slot->id == 0)
        return;
    if (slot->is_skill) {
        struct SkillInfo info;

        session_get_skill_info(&info, slot->id);
        if (info.found && info.level && self->slots[index]->level != 0
            && info.level >= self->slots[index]->level) {
            struct GameMode *mode = game_current_mode();

            if (mode)
                game_mode_drag_by_name(mode, DRAG_FROM_SHORTCUT, info.id, info.id_name,
                                    self->slots[index]->level, index);
        }
        skill_info_destruct(&info);
    } else {
        struct ItemInfo item;

        session_get_inventory_item_by_id(&g_session1, &item, slot->id, 1);
        if ((unsigned)item.index > 0)
            game_mode_drag_item(game_current_mode(), DRAG_FROM_SHORTCUT, &item, item.count,
                              -1);
        item_info_destruct(&item);
    }
}

void ui_short_cut_wnd_on_l_btn_dbl_clk(struct UIWindow *window, int x, int y)
{
    struct UIShortCutWnd *self = (struct UIShortCutWnd *)window;
    int column = ui_short_cut_wnd_hit_slot(self, x, y);
    struct HotkeySlot *slot;

    if (column == -1)
        return;
    slot = slot_at(self, column);
    if (slot->is_skill && slot->id == SKILL_GC_ROLLINGCUTTER) {
        int count = session_get_instant_var(&g_session1, SESSION_OPT_ROLLING_CUTTER_ONE_SHOT) + 1;

        session_set_pair(&g_session1, SESSION_OPT_ROLLING_CUTTER_ONE_SHOT, count);
    } else {
        session_reset_option2(&g_session1);
    }
    ui_short_cut_wnd_send_msg(&self->frame.base, NULL, HOTKEY_MSG_USE, column, g_session1.shortenSkillPage, 0, 0);
}

void ui_short_cut_wnd_on_mouse_hover(struct UIWindow *window, int x, int y)
{
    struct UIShortCutWnd *self = (struct UIShortCutWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct String label, text;
    struct HotKey key, got;
    struct HotkeySlot *slot;
    int column = ui_short_cut_wnd_hit_slot(self, x, y);
    int index, group, tx, ty;

    if (column != self->hover_column) {
        self->hover_column = column;
        ui_window_invalidate_base(wnd);
    }
    if (self->hover_column == -1 || self->hover_row == -1)
        return;
    index = self->hover_column + self->hover_row * HOTKEY_SLOTS_PER_ROW;

    string_init(&label);
    key.code = 0;
    key.modifier = 0;
    string_init(&key.label);
    string_init(&key.action);
    group = session_get_instant_var(&g_session1, SESSION_OPT_BAR_GROUP);
    if (group == 0 || group == 1) {
        session_get_hot_key(&g_session1, &got, group == 0 ? 0 : 3, index);
        hotkey_move_assign(&key, &got);
        string_free(&got.action);
        string_free(&got.label);
        if (key.action.size != 0)
            string_assign(&label, string_cstr(&key.label), key.label.size);
    }

    tx = (x - 0xf) / SHORTCUT_SLOT_WIDTH * SHORTCUT_SLOT_WIDTH + 0xf;
    ty = -0x11;
    ui_window_to_screen(wnd, &tx, &ty);
    string_init(&text);
    slot = self->slots[index];
    if (slot->id == 0) {
        string_assign(&text, string_cstr(&label), label.size);
    } else if (slot->is_skill) {
        struct SkillInfo info;
        int level;

        session_get_skill_info(&info, slot->id);
        level = self->slots[index]->level;
        if (info.level >= level && info.found) {
            struct String line;
            int sp = skill_info_get_sp_amount(&info, level);
            int ap = skill_info_get_ap_amount(&info, level);
            const char *prefix = string_cstr(&label);

            if (session_is_space(&g_session1, info.id)) {
                string_init(&line);
                if (ap)
                    string_format(&line, "%s %s Use Lv %d (Sp : %d%%, Ap : %d)", prefix,
                                  skill_info_get_name(&info), level, sp, ap);
                else
                    string_format(&line, "%s %s Use Lv %d (Sp : %d%%)", prefix, skill_info_get_name(&info),
                                  level, sp);
            } else {
                string_init(&line);
                if (ap)
                    string_format(&line, "%s %s Use Lv %d (Sp : %d, Ap : %d)", prefix,
                                  skill_info_get_name(&info), level, sp, ap);
                else
                    string_format(&line, "%s %s Use Lv %d (Sp : %d)", prefix, skill_info_get_name(&info),
                                  level, sp);
            }
            string_assign(&text, string_cstr(&line), line.size);
            string_free(&line);
        } else {
            string_assign(&text, string_cstr(&label), label.size);
        }
        skill_info_destruct(&info);
    } else {
        struct ItemInfo item;

        session_get_inventory_item_by_id(&g_session1, &item, slot->id, 1);
        if (item.count == 0) {
            string_assign(&text, string_cstr(&label), label.size);
        } else {
            struct String line;
            struct String name = {0};

            item_info_get_display_name(&item, &name);
            string_init(&line);
            string_format(&line, msg_str(MSG_SKILL_SLOT), string_cstr(&label), string_cstr(&name),
                          item.count);
            string_assign(&text, string_cstr(&line), line.size);
            string_free(&line);
            string_free(&name);
        }
        item_info_destruct(&item);
    }
    mode_show_tooltip(client_current_mode(), string_cstr(&text), tx, ty, 0xffffffff, 0);
    string_free(&text);
    string_free(&key.action);
    string_free(&key.label);
    string_free(&label);
}

void ui_short_cut_wnd_on_r_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIShortCutWnd *self = (struct UIShortCutWnd *)window;
    struct HotkeySlot *slot;
    int column = ui_short_cut_wnd_hit_slot(self, x, y);
    int id;

    if (column == -1)
        return;
    slot = slot_at(self, column);
    id = slot->id;
    if (id == 0)
        return;
    if (slot->is_skill) {
        struct SkillInfo info;

        session_get_skill_info(&info, id);
        if (info.level >= slot_at(self, column)->level) {
            struct UIWindow *describe = ui_make_window(WND_SKILL_DESCRIBE);

            if (describe) {
                if (ui_skill_describe_skill(describe) == id)
                    ui_close_window(WND_SKILL_DESCRIBE);
                else
                    ui_window_send(describe, NULL, SKILL_DESCRIBE_MSG_SHOW, id, 0, 0, 0);
            }
            hotkey_place_describe(&self->frame.base, describe, x, y);
        }
        skill_info_destruct(&info);
    } else {
        struct ItemInfo item;

        session_get_inventory_item_by_id(&g_session1, &item, id, 1);
        if ((unsigned)item.index > 0) {
            struct UIWindow *describe = ui_window_open(WND_ITEM_DESCRIBE);

            ui_window_send(describe, NULL, ITEM_DESCRIBE_MSG_SHOW, (int)&item, 0, 0, 0);
        }
        item_info_destruct(&item);
    }
}

static void place_row_buttons(struct UIShortCutWnd *self)
{
    struct UIWindow *wnd = &self->frame.base;
    struct UIWindow *minus = &self->minus->button.base;
    struct UIWindow *plus = &self->plus->button.base;

    ui_window_move_base(minus, wnd->width - minus->width - 2, wnd->height - minus->height - plus->height - 6);
    ui_window_move_base(plus, wnd->width - plus->width - 2, wnd->height - plus->height - 2);
    ui_button_show((struct UIButton *)(self->minus), self->rows != 1);
    ui_button_show((struct UIButton *)(self->plus), 1);
    self->plus->button.disabled = self->rows == SHORTCUT_MAX_ROWS;
    ui_window_invalidate_base(plus);
}

static void use_item(int item_id)
{
    struct ItemInfo item;

    session_get_inventory_item_by_id(&g_session1, &item, item_id, 1);
    if ((unsigned)item.index > 0) {
        struct Mode *mode = game_current_base_mode();

        switch (item.type) {
        case 0: case 1: case 2:
            if (item_info_get_item_id(&item) == ITEM_ID_ASK_FIRST
                && ui_window_mgr_message_box(&g_windowMgr, msg_str(MSG_CONFIRM_OPEN_BOUND_BOX), UI_MSGBOX_QUESTION, 0, 0,
                                          0, "메시지", 0x118, 0x78, 0) == UI_MSGBOX_ANSWER_NO)
                break;
            mode_send_message(mode, GAME_MSG_USE_ITEM, item.index, 0, 0, 0);
            break;
        case 10: case 16: case 17: case 19:
            if (item.worn_location == WEAR_AMMO)
                mode_send_message(mode, GAME_MSG_EQUIP_AMMO, item.index, item.equip_location, 0, 0);
            break;
        case 18:
            if (g_session1.manner >= 0) {
                struct String ask;
                struct String name = {0};
                int answer;

                item_info_get_display_name(&item, &name);
                string_init(&ask);
                string_format(&ask, msg_str(MSG_ASK_USE_ITEM), string_cstr(&name));
                answer = ui_window_mgr_message_box(&g_windowMgr, string_cstr(&ask), UI_MSGBOX_QUESTION, 0, 0, 0,
                                                "메시지", 0x118, 0x78, 0);
                string_free(&ask);
                string_free(&name);
                if (answer == UI_MSGBOX_ANSWER_YES)
                    mode_send_message(mode, GAME_MSG_USE_ITEM, item.index, 0, 0, 0);
            }
            break;
        default:
            if (item.equip_location && item.worn_location == 0 && item.identified)
                mode_send_message(mode, GAME_MSG_EQUIP, item.index, item.equip_location, 0, 0);
            break;
        }
    }
    item_info_destruct(&item);
}

int ui_short_cut_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1, int val2,
                                     int val3, int val4)
{
    struct UIShortCutWnd *self = (struct UIShortCutWnd *)window;
    struct UIWindow *wnd = &self->frame.base;

    switch (msg) {
    case HOTKEY_MSG_USE: {
        struct HotkeySlot *slot;

        if (session_get_instant_var(&g_session1, SESSION_OPT_NO_HOTKEYS) > 0)
            return 0;
        slot = self->slots[val1 + val2 * HOTKEY_SLOTS_PER_ROW];
        if (slot->id == 0)
            return 0;
        if (slot->is_skill) {
            struct SkillInfo info;

            session_get_skill_info(&info, slot->id);
            if (info.found && info.level && slot->level != 0 && info.level >= slot->level)
                mode_send_message(client_current_mode(), GAME_MSG_USE_SKILL, (int)&info, slot->level, 0, 0);
            skill_info_destruct(&info);
        } else {
            use_item(slot->id);
        }
        return 0;
    }

    case HOTKEY_MSG_REFRESH: {
        int group = session_get_instant_var(&g_session1, SESSION_OPT_BAR_GROUP);
        int i;

        for (i = 0; i < HOTKEY_BAR_SLOTS; i++) {
            int at = session_hot_key_page_slot(&g_session1, i, group);
            struct HotkeySlot *slot;
            struct SkillInfo info;

            if (group == 0)
                self->slots[i] = &g_session1.hotkeys0[at];
            else if (group == 1)
                self->slots[i] = &g_session1.hotkeys1[at];
            slot = self->slots[i];
            if (slot->id == 0 || !slot->is_skill)
                continue;
            skill_table_find(&g_session1.skills, &info, slot->id);
            if (info.found == 0) {
                struct SkillCooldownNode *cd;
                struct SkillInfo other;

                for (cd = g_session1.skillDelays.head; cd != NULL; cd = cd->next)
                    if (cd->value.skill_id == self->slots[i]->id)
                        break;
                if (cd != NULL) {
                    struct CooldownTimerNode *t;

                    t = cooldown_timers_find(&cd->value.timers_bar, i);
                    if (t != NULL)
                        cooldown_timers_remove(&cd->value.timers_bar, t);
                }
                if (info.found == 0) {
                    session_get_homun_skill_by_id(&g_session1, &other, self->slots[i]->id);
                    skill_info_assign(&info, &other);
                    skill_info_destruct(&other);
                }
                if (info.found == 0) {
                    session_get_mer_skill_by_id(&g_session1, &other, self->slots[i]->id);
                    skill_info_assign(&info, &other);
                    skill_info_destruct(&other);
                }
                if (info.found == 0) {
                    session_get_skill4_by_id(&g_session1, &other, self->slots[i]->id);
                    skill_info_assign(&info, &other);
                    skill_info_destruct(&other);
                    if (info.found == 0) {
                        skill_info_destruct(&info);
                        continue;
                    }
                }
            }
            if (info.level != 0 && !session_get_skill_table_value(&g_session1, info.id))
                self->slots[i]->level = (short)info.level;
            skill_info_destruct(&info);
        }
        ui_window_invalidate_base(wnd);
        return 0;
    }

    case UI_MSG_SET_INFO:
        self->info = (struct WndInfoShow *)val1;
        if (!short_cut_wnd_info_is_valid(self->info))
            short_cut_wnd_info_make_default(self->info);
        ui_window_resize_base(wnd, wnd->width, self->info->rect.height);
        self->rows = self->info->rect.height / SHORTCUT_ROW_HEIGHT;
        place_row_buttons(self);
        ui_short_cut_wnd_send_msg(wnd, NULL, HOTKEY_MSG_REFRESH, 0, 0, 0, 0);
        ui_short_cut_wnd_move(wnd, self->info->rect.x, self->info->rect.y);
        return 0;

    case UI_MSG_COMMAND:
        switch (val1) {
        case UI_ID_CLOSE:
            ui_window_mgr_show_option_window(&g_windowMgr, WND_SHORTCUT, 0);
            return 0;
        case SHORTCUT_ID_PLUS:
            if (!wnd->visible) {
                ui_window_mgr_show_option_window(&g_windowMgr, WND_SHORTCUT, 1);
                return 0;
            }
            if (++self->rows > SHORTCUT_MAX_ROWS) {
                self->rows = 1;
                ui_window_mgr_show_option_window(&g_windowMgr, WND_SHORTCUT, 0);
            }
            ui_short_cut_wnd_send_msg(wnd, NULL, HOTKEY_MSG_RELAYOUT, 0, 0, 0, 0);
            return 0;
        case SHORTCUT_ID_MINUS:
            if (self->rows > 1)
                self->rows--;
            ui_short_cut_wnd_send_msg(wnd, NULL, HOTKEY_MSG_RELAYOUT, 0, 0, 0, 0);
            return 0;
        case SHORTCUT_ID_GROUP: {
            int group = session_get_instant_var(&g_session1, SESSION_OPT_BAR_GROUP);
            struct SkillCooldownNode *cd;

            if (group == 0)
                session_set_pair(&g_session1, SESSION_OPT_BAR_GROUP, 1);
            else
                session_set_pair(&g_session1, SESSION_OPT_BAR_GROUP, 0);
            self->group_check->state = group == 0;
            ui_window_invalidate(self->group_check);
            for (cd = g_session1.skillDelays.head; cd != NULL; cd = cd->next)
                cooldown_timers_clear(&cd->value.timers_bar);
            ui_short_cut_wnd_send_msg(wnd, NULL, HOTKEY_MSG_REFRESH, 0, 0, 0, 0);
            return 0;
        }
        }
        return 0;

    case HOTKEY_MSG_RELAYOUT:
        if (self->rows * SHORTCUT_ROW_HEIGHT == wnd->height)
            return 0;
        ui_window_resize_base(wnd, wnd->width, self->rows * SHORTCUT_ROW_HEIGHT);
        place_row_buttons(self);
        return 0;

    case HOTKEY_MSG_SCROLL: {
        int turn = val1 < 4 ? val1 : val1 % 4;
        struct SkillCooldownNode *cd;

        if (turn <= 0)
            return 0;
        if (val2 == 0) {
            g_session1.hotkeyPage0 += turn;
            if (g_session1.hotkeyPage0 >= 4)
                g_session1.hotkeyPage0 -= 4;
        } else if (val2 == 1) {
            g_session1.hotkeyPage1 += turn;
            if (g_session1.hotkeyPage1 >= 4)
                g_session1.hotkeyPage1 -= 4;
        }
        cd = g_session1.skillDelays.head;
        while (cd != NULL) {
            if (skill_cooldown_running(&cd->value)) {
                struct CooldownTimerNode *t;

                for (t = cd->value.timers_bar.head; t != NULL; t = t->next) {
                    t->value.slot -= turn * HOTKEY_SLOTS_PER_ROW;
                    t->value.y -= turn * SHORTCUT_ROW_HEIGHT;
                    if (t->value.slot < 0) {
                        t->value.slot += HOTKEY_BAR_SLOTS;
                        t->value.y += SHORTCUT_MAX_ROWS * SHORTCUT_ROW_HEIGHT;
                    }
                }
                cd = cd->next;
            } else {
                cd = skill_cooldowns_remove(&g_session1.skillDelays, cd);
            }
        }
        ui_short_cut_wnd_send_msg(wnd, NULL, HOTKEY_MSG_REFRESH, 0, 0, 0, 0);
        return 0;
    }

    default:
        return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
    }
}

struct UIShortCutWnd *ui_short_cut_wnd_construct(struct UIShortCutWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_short_cut_wnd_ops;
    self->minus = NULL;
    self->plus = NULL;
    self->frame.base.redraws_tree = 1;
    self->hover_column = -1;
    self->hover_row = -1;
    self->rows = 1;
    self->drawn_key = 0;
    return self;
}

struct UIWindow *ui_short_cut_wnd_new(void)
{
    return (struct UIWindow *)ui_short_cut_wnd_construct(mem_alloc(sizeof(struct UIShortCutWnd)));
}
