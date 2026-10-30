#include "world/tooltip.h"
#include "render/rp_pool.h"
#include "skills/skill_info.h"
#include "iteminfo/item_api.h"
#include "session/state.h"
#include "core/object_type_tables.h"
#include "world/mode_manager.h"
#include "game/context.h"
#include <stdio.h>
#include "ui/hotkeys/hotkey_bars.h"
#include "game/drag_info.h"
#include "ui/character/skill_dialogs.h"
#include "core/msg_str.h"
#include "core/text_util.h"
#include "diag/perf.h"
#include "session/session_items.h"
#include "session/session_jobs.h"
#include "session/session_names.h"
#include "session/session_options.h"
#include "session/session_skills.h"
#include "session/session_windows.h"
#include "session/skill_levels.h"

struct UIQuikSlotWnd;
struct UIWindow;
void ui_quik_slot_wnd_destroy(struct UIWindow *window);
void ui_quik_slot_wnd_drag_and_drop(struct UIWindow *window, int x, int y, const struct DragInfo *drag);
void ui_quik_slot_wnd_store_info(struct UIWindow *self);
void ui_quik_slot_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_quik_slot_wnd_on_draw(struct UIWindow *self);
void ui_quik_slot_wnd_on_l_btn_down(struct UIWindow *self, int x, int y);
void ui_quik_slot_wnd_on_l_btn_dbl_clk(struct UIWindow *self, int x, int y);
void ui_quik_slot_wnd_on_mouse_hover(struct UIWindow *self, int x, int y);
void ui_quik_slot_wnd_on_r_btn_down(struct UIWindow *self, int x, int y);
int ui_quik_slot_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_quik_slot_wnd_ops = {
    .destroy = ui_quik_slot_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_quik_slot_wnd_drag_and_drop,
    .store_info = ui_quik_slot_wnd_store_info,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_quik_slot_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_quik_slot_wnd_on_process,
    .on_draw = ui_quik_slot_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_quik_slot_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_quik_slot_wnd_on_l_btn_dbl_clk,
    .on_mouse_move = ui_frame_wnd_on_mouse_move,
    .on_mouse_hover = ui_quik_slot_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_frame_wnd_on_l_btn_up,
    .on_r_btn_down = ui_quik_slot_wnd_on_r_btn_down,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_quik_slot_wnd_send_msg,
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
};

#define QUICK_SKIN UI_SKIN "basic_interface\\quickslot.bmp"
#define GRADE_ICON UI_SKIN "grade_enchant\\grade_icon%d.bmp"
#define QUICK_COLOUR_HOVER 0xb4ffb4
#define QUICK_FONT_HEIGHT 0xb
#define DRAG_FROM_STORAGE 6

void ui_quik_slot_wnd_destruct(struct UIQuikSlotWnd *self)
{
    self->frame.base.ops = &ui_quik_slot_wnd_ops;
    ui_frame_wnd_destruct(&self->frame);
}

void ui_quik_slot_wnd_destroy(struct UIWindow *window)
{
    struct UIQuikSlotWnd *self = (struct UIQuikSlotWnd *)window;
    ui_quik_slot_wnd_destruct(self);
    mem_free(self);
}

int ui_quik_slot_wnd_hit_slot(struct UIQuikSlotWnd *self, int x, int y)
{
    int column;

    if ((unsigned)y > 0x17)
        return -1;
    if (x > 0x3a && x < self->frame.base.width)
        return -1;
    column = (x - 4) / SHORTCUT_SLOT_WIDTH;
    if (x > column * SHORTCUT_SLOT_WIDTH + 0x1e)
        return -1;
    return column;
}

int ui_quik_slot_wnd_slot_x(int slot)
{
    int i = 0, x = 5;

    while (i != slot) {
        x += SHORTCUT_SLOT_WIDTH;
        i++;
        if (x >= 0x3f)
            return -1;
    }
    return x;
}

void ui_quik_slot_wnd_drag_and_drop(struct UIWindow *window, int x, int y, const struct DragInfo *drag)
{
    struct UIQuikSlotWnd *self = (struct UIQuikSlotWnd *)window;
    struct UIWindow *wnd = &self->frame.base;

    if (drag->named) {
        struct SkillInfo info;
        int column;

        column = ui_quik_slot_wnd_hit_slot(self, x, y);
        if (column != -1) {
            session_get_skill_info(&info, drag->index);
            if (info.level != 0 && info.found != 0) {
                struct SkillCooldownNode *cd;
                int from = drag->count;

                session_set_quick_slot(&g_session1, 1, info.id, drag->level, column);
                if (from != column) {
                    for (cd = g_session1.skillDelays.head; cd != NULL; cd = cd->next) {
                        struct CooldownTimerNode *t = cooldown_timers_find(&cd->value.timers_quick, column);

                        if (t != NULL) {
                            cooldown_timers_remove(&cd->value.timers_quick, t);
                            break;
                        }
                    }
                }
                cd = hotkey_cooldown_of(info.id);
                if (cd != NULL && skill_cooldown_running(&cd->value)) {
                    struct CooldownTimerList *timers = &cd->value.timers_quick;
                    struct CooldownTimerNode *t = cooldown_timers_find(timers, from);

                    if (t != NULL) {
                        t->value.slot = column;
                        t->value.x = ui_quik_slot_wnd_slot_x(column);
                    } else {
                        int sx = ui_quik_slot_wnd_slot_x(column);

                        for (t = timers->head; t != NULL; t = t->next)
                            if (t->value.x == sx) {
                                cooldown_timers_remove(timers, t);
                                break;
                            }
                    }
                }
                ui_quik_slot_wnd_send_msg(wnd, NULL, HOTKEY_MSG_REFRESH, 0, 0, 0, 0);
                ui_window_invalidate_base(wnd);
            }
            skill_info_destruct(&info);
        } else if (drag->count != -1) {
            struct SkillCooldownNode *cd;

            session_get_skill_info(&info, drag->index);
            cd = hotkey_cooldown_of(info.id);
            if (cd != NULL) {
                struct CooldownTimerNode *t = cooldown_timers_find(&cd->value.timers_quick, drag->count);

                if (t != NULL)
                    cooldown_timers_remove(&cd->value.timers_quick, t);
            }
            session_set_quick_slot(&g_session1, 1, 0, 0, drag->count);
            ui_quik_slot_wnd_send_msg(wnd, NULL, HOTKEY_MSG_REFRESH, 0, 0, 0, 0);
            ui_window_invalidate_base(wnd);
            skill_info_destruct(&info);
        }
    } else if (drag->source != DRAG_FROM_STORAGE) {
        struct ItemInfo item;

        session_get_inventory_item(&g_session1, &item, drag->index);
        if ((unsigned)item.index > 0 && item.identified && (unsigned)item.type <= 0x13) {
            static const BYTE kind_of_type[0x14] = { 0, 0, 0, 2, 1, 1, 2, 2, 1, 1, 0, 1, 1, 1, 1, 1, 0, 0, 2, 0 };
            int kind = kind_of_type[item.type];

            if (kind == 0 || (kind == 1 && item.equip_location != 0)) {
                int column = ui_quik_slot_wnd_hit_slot(self, x, y);

                if (column != -1) {
                    struct SkillCooldownNode *cd;

                    session_set_quick_slot(&g_session1, 0, item_info_get_item_id(&item), 0, column);
                    if (drag->count != column) {
                        for (cd = g_session1.skillDelays.head; cd != NULL; cd = cd->next) {
                            struct CooldownTimerNode *t = cooldown_timers_find(&cd->value.timers_quick, column);

                            if (t != NULL) {
                                cooldown_timers_remove(&cd->value.timers_quick, t);
                                break;
                            }
                        }
                    }
                    ui_quik_slot_wnd_send_msg(wnd, NULL, HOTKEY_MSG_REFRESH, 0, 0, 0, 0);
                    ui_window_invalidate_base(wnd);
                }
            }
        }
        item_info_destruct(&item);
    }
    mode_send_message(client_current_mode(), GAME_MSG_DROP_DONE, 0, 0, 0, 0);
}

void ui_quik_slot_wnd_store_info(struct UIWindow *window)
{
    struct UIQuikSlotWnd *self = (struct UIQuikSlotWnd *)window;
    struct UIWindow *wnd = &self->frame.base;

    if (self->info) {
        self->info->x = wnd->x;
        self->info->y = wnd->y;
        self->info->width = wnd->width;
        self->info->height = wnd->height;
    }
}

void ui_quik_slot_wnd_on_create(struct UIWindow *self, int width, int height)
{
}

void ui_quik_slot_wnd_on_process(struct UIWindow *window)
{
    struct UIQuikSlotWnd *self = (struct UIQuikSlotWnd *)window;
    ui_window_invalidate_base(&self->frame.base);
}

void ui_quik_slot_wnd_on_draw(struct UIWindow *window)
{
    struct UIQuikSlotWnd *self = (struct UIQuikSlotWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    int i;

    ui_window_draw_body(wnd, 0, 0, QUICK_SKIN);
    if (self->hover != -1)
        ui_window_fill_rect(wnd, self->hover * SHORTCUT_SLOT_WIDTH + 5, 5, 0x18, 0x18, QUICK_COLOUR_HOVER);

    cooldown_slots_clear(&g_session1.cooldownSlotsQuick);
    for (i = 0; i < HOTKEY_QUICK_SLOTS; i++) {
        struct HotkeySlot *slot = &self->slots[i];
        int x = 5 + i * SHORTCUT_SLOT_WIDTH;
        char text[0x400];

        if (slot->id == 0)
            continue;
        if (slot->is_skill) {
            struct SkillInfo info;

            session_get_skill_info(&info, slot->id);
            if (info.found) {
                struct BitmapRes *icon;
                int tx = x + 0xe;

                session_format_item_bitmap_path(info.id_name, text);
                icon = res_mgr_get(g_resMgr(), text);
                cooldown_slots_add(&g_session1.cooldownSlotsQuick, info.id, i, x, 5);
                ui_window_draw_bitmap(wnd, x, 5, icon, 1);
                if (slot->level != 0) {
                    sprintf(text, "%d", (int)slot->level);
                    ui_window_text_out(wnd, tx + 1, 0x14, text, 0, 0, QUICK_FONT_HEIGHT, 0xffffff, 0, 0);
                    ui_window_text_out(wnd, tx - 1, 0x14, text, 0, 0, QUICK_FONT_HEIGHT, 0xffffff, 0, 0);
                    ui_window_text_out(wnd, tx, 0x15, text, 0, 0, QUICK_FONT_HEIGHT, 0xffffff, 0, 0);
                    ui_window_text_out(wnd, tx, 0x13, text, 0, 0, QUICK_FONT_HEIGHT, 0xffffff, 0, 0);
                    ui_window_text_out(wnd, tx, 0x14, text, 0, 0, QUICK_FONT_HEIGHT, 0, 0, 0);
                }
            }
            skill_info_destruct(&info);
        } else {
            struct ItemInfo item;

            session_get_inventory_item_by_id(&g_session1, &item, slot->id, 1);
            if ((unsigned)item.index > 0) {
                ui_window_draw_item_icon_count(wnd, x, 5, &item, item.identified, 1);
                if (item.grade > 0) {
                    struct String path;

                    string_init(&path);
                    string_format(&path, GRADE_ICON, (int)item.grade);
                    ui_window_draw_bitmap(wnd, x, 0x11, res_mgr_get(g_resMgr(), string_cstr(&path)),
                                        1);
                    string_free(&path);
                }
            }
            item_info_destruct(&item);
        }
    }
    hotkey_cooldowns_draw(wnd, &g_session1.cooldownSlotsQuick, 1, 0);
}

void ui_quik_slot_wnd_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIQuikSlotWnd *self = (struct UIQuikSlotWnd *)window;
    struct HotkeySlot slot;
    int column;

    if (x > 0x3b && x < self->frame.base.width) {
        ui_frame_wnd_on_l_btn_down((struct UIWindow *)(&self->frame), x, y);
        return;
    }
    if (g_replayPlaying)
        return;
    column = ui_quik_slot_wnd_hit_slot(self, x, y);
    if (column == -1)
        return;
    slot = self->slots[column];
    if (slot.is_skill) {
        struct SkillInfo info;

        session_get_skill_info(&info, slot.id);
        if (info.found && info.level) {
            struct GameMode *mode = game_current_mode();

            if (mode)
                game_mode_drag_by_name(mode, DRAG_FROM_QUICK_SLOT, info.id, info.id_name,
                                    slot.level, column);
        }
        skill_info_destruct(&info);
    } else if (slot.id != 0) {
        struct ItemInfo item;

        session_get_inventory_item_by_id(&g_session1, &item, slot.id, 1);
        if ((unsigned)item.index > 0)
            game_mode_drag_item(game_current_mode(), DRAG_FROM_QUICK_SLOT, &item,
                              item.count, -1);
        item_info_destruct(&item);
    }
}

void ui_quik_slot_wnd_on_l_btn_dbl_clk(struct UIWindow *window, int x, int y)
{
    struct UIQuikSlotWnd *self = (struct UIQuikSlotWnd *)window;
    struct HotkeySlot *slot;
    int column;

    if (session_get_instant_var(&g_session1, SESSION_OPT_NO_HOTKEYS) > 0)
        return;
    column = ui_quik_slot_wnd_hit_slot(self, x, y);
    if (column == -1)
        return;
    slot = &self->slots[column];
    if (slot->is_skill && slot->id == SKILL_GC_ROLLINGCUTTER) {
        int count = session_get_instant_var(&g_session1, SESSION_OPT_ROLLING_CUTTER_ONE_SHOT) + 1;

        session_set_pair(&g_session1, SESSION_OPT_ROLLING_CUTTER_ONE_SHOT, count);
    } else {
        session_reset_option2(&g_session1);
    }
    ui_quik_slot_wnd_send_msg(&self->frame.base, NULL, HOTKEY_MSG_USE, column, 0, 0, 0);
}

void ui_quik_slot_wnd_on_mouse_hover(struct UIWindow *window, int x, int y)
{
    struct UIQuikSlotWnd *self = (struct UIQuikSlotWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct String text;
    struct HotkeySlot *slot;
    int column = ui_quik_slot_wnd_hit_slot(self, x, y);
    int tx, ty;

    if (column != self->hover) {
        self->hover = column;
        ui_window_invalidate_base(wnd);
    }
    if (self->hover == -1)
        return;
    ty = -0x11;
    tx = (x - 4) / SHORTCUT_SLOT_WIDTH * SHORTCUT_SLOT_WIDTH + 4;
    ui_window_to_screen(wnd, &tx, &ty);
    string_init(&text);
    slot = &self->slots[self->hover];
    if (slot->id == 0) {
        string_clear(&text);
    } else if (slot->is_skill) {
        struct SkillInfo info;
        struct String line;
        int level, sp, ap;

        session_get_skill_info(&info, slot->id);
        level = self->slots[self->hover].level;
        sp = skill_info_get_sp_amount(&info, level);
        ap = skill_info_get_ap_amount(&info, info.level);
        if (session_is_space(&g_session1, info.id)) {
            string_init(&line);
            if (ap)
                string_format(&line, "%s Use Lv %d (Sp : %d%%, Ap: %d)", skill_info_get_name(&info), level,
                              sp, ap);
            else
                string_format(&line, "%s Use Lv %d (Sp : %d%%)", skill_info_get_name(&info), level, sp);
        } else {
            string_init(&line);
            if (ap)
                string_format(&line, "%s Use Lv %d (Sp : %d, Ap: %d)", skill_info_get_name(&info), level,
                              sp, ap);
            else
                string_format(&line, "%s Use Lv %d (Sp : %d)", skill_info_get_name(&info), level, sp);
        }
        string_assign(&text, string_cstr(&line), line.size);
        string_free(&line);
        skill_info_destruct(&info);
    } else {
        struct ItemInfo item;
        struct String line;
        struct String name = {0};

        session_get_inventory_item_by_id(&g_session1, &item, slot->id, 1);
        item_info_get_display_name(&item, &name);
        string_init(&line);
        string_format(&line, msg_str(MSG_ITEM_SLOT), string_cstr(&name), item.count);
        string_assign(&text, string_cstr(&line), line.size);
        string_free(&line);
        string_free(&name);
        item_info_destruct(&item);
    }
    if (text.size != 0)
        mode_show_tooltip(client_current_mode(), string_cstr(&text), tx, ty, 0xffffffff, 0);
    string_free(&text);
}

void ui_quik_slot_wnd_on_r_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIQuikSlotWnd *self = (struct UIQuikSlotWnd *)window;
    struct HotkeySlot *slot;
    int column;

    if (x > 0x3b && x < self->frame.base.width)
        return;
    column = ui_quik_slot_wnd_hit_slot(self, x, y);
    if (column == -1)
        return;
    slot = &self->slots[column];
    if (slot->is_skill) {
        int id = slot->id;
        struct UIWindow *describe = ui_make_window(WND_SKILL_DESCRIBE);

        if (describe) {
            if (ui_skill_describe_skill(describe) == id)
                ui_close_window(WND_SKILL_DESCRIBE);
            else
                ui_window_send(describe, NULL, SKILL_DESCRIBE_MSG_SHOW, id, 0, 0, 0);
        }
        hotkey_place_describe(&self->frame.base, describe, x, y);
    } else if (slot->id != 0) {
        struct ItemInfo item;

        session_get_inventory_item_by_id(&g_session1, &item, slot->id, 1);
        if ((unsigned)item.index > 0) {
            struct UIWindow *describe = ui_window_open(WND_ITEM_DESCRIBE);

            ui_window_send(describe, NULL, ITEM_DESCRIBE_MSG_SHOW, (int)&item, 0, 0, 0);
        }
        item_info_destruct(&item);
    }
}

static void use_item(int item_id)
{
    struct ItemInfo item;

    session_get_inventory_item_by_id(&g_session1, &item, item_id, 1);
    if ((unsigned)item.index > 0) {
        struct Mode *mode = game_current_base_mode();

        switch (item.type) {
        case 0: case 1: case 2:
            mode_send_message(mode, GAME_MSG_USE_ITEM, item.index, 0, 0, 0);
            break;
        case 10: case 16: case 17: case 19:
            if (item.worn_location == WEAR_AMMO) {
                mode_send_message(mode, GAME_MSG_TAKEOFF_EQUIP, 0xf, 0, 0, 0);
                mode_send_message(mode, GAME_MSG_EQUIP_AMMO, item.index, item.equip_location, 0, 0);
            }
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

int ui_quik_slot_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1, int val2,
                                     int val3, int val4)
{
    struct UIQuikSlotWnd *self = (struct UIQuikSlotWnd *)window;
    struct UIWindow *wnd = &self->frame.base;

    switch (msg) {
    case HOTKEY_MSG_USE: {
        struct HotkeySlot slot = self->slots[val1];

        if (slot.is_skill) {
            struct SkillInfo info;

            session_get_skill_info(&info, slot.id);
            if (info.level && info.found)
                mode_send_message(client_current_mode(), GAME_MSG_USE_SKILL, (int)&info, slot.level, 0, 0);
            skill_info_destruct(&info);
        } else if (slot.id != 0) {
            use_item(slot.id);
        }
        return 0;
    }

    case UI_MSG_SET_INFO:
        self->info = (struct WindowInfo *)val1;
        if (!quik_slot_wnd_info_is_valid(self->info))
            quik_slot_wnd_info_make_default(self->info);
        ui_quik_slot_wnd_send_msg(wnd, NULL, HOTKEY_MSG_REFRESH, 0, 0, 0, 0);
        ui_frame_wnd_move(wnd, self->info->x, self->info->y);
        return 0;

    case HOTKEY_MSG_REFRESH: {
        int i;

        for (i = 0; i < HOTKEY_QUICK_SLOTS; i++) {
            struct HotkeySlot from = g_session1.quickSlots[i];
            struct HotkeySlot *slot = &self->slots[i];

            slot->id = 0;
            slot->is_skill = 0;
            slot->level = 0;
            if (from.id == 0)
                continue;
            if (from.is_skill) {
                struct SkillInfo info, other;

                skill_table_find(&g_session1.skills, &info, from.id);
                if (info.found == 0) {
                    struct SkillCooldownNode *cd;

                    for (cd = g_session1.skillDelays.head; cd != NULL; cd = cd->next)
                        if (cd->value.skill_id == from.id)
                            break;
                    if (cd != NULL) {
                        struct CooldownTimerNode *t = cooldown_timers_find(&cd->value.timers_quick, i);

                        if (t != NULL)
                            cooldown_timers_remove(&cd->value.timers_quick, t);
                    }
                }
                if (info.found == 0) {
                    session_get_homun_skill_by_id(&g_session1, &other, from.id);
                    skill_info_assign(&info, &other);
                    skill_info_destruct(&other);
                }
                if (info.found == 0) {
                    session_get_mer_skill_by_id(&g_session1, &other, from.id);
                    skill_info_assign(&info, &other);
                    skill_info_destruct(&other);
                }
                if (info.found == 0) {
                    session_get_skill4_by_id(&g_session1, &other, from.id);
                    skill_info_assign(&info, &other);
                    skill_info_destruct(&other);
                }
                if (info.found != 0 && info.level != 0) {
                    slot->id = info.id;
                    slot->is_skill = 1;
                    slot->level = session_get_skill_table_value(&g_session1, info.id) ? from.level : (short)info.level;
                }
                skill_info_destruct(&info);
            } else {
                struct ItemInfo item;

                session_get_inventory_item_by_id(&g_session1, &item, from.id, 1);
                if ((unsigned)item.index > 0 && item.identified)
                    slot->id = from.id;
                item_info_destruct(&item);
            }
        }
        ui_window_invalidate_base(wnd);
        return 0;
    }

    default:
        return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
    }
}

struct UIQuikSlotWnd *ui_quik_slot_wnd_construct(struct UIQuikSlotWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_quik_slot_wnd_ops;
    self->hover = -1;
    memset(self->slots, 0, sizeof self->slots);
    return self;
}

struct UIWindow *ui_quik_slot_wnd_new(void)
{
    return (struct UIWindow *)ui_quik_slot_wnd_construct(mem_alloc(sizeof(struct UIQuikSlotWnd)));
}
