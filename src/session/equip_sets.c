#include "ui/window_manager_state.h"
#include "session/csession.h"
#include "session/session_items.h"
#include "session/session_jobs.h"
#include "ui/character/equip_wnd.h"
#include "ui/wndmgr/wndmgr.h"

static void item_destroy(struct ItemInfo *item)
{
    string_free(&item->name);
    string_free(&item->id_text);
}

BYTE session_put_switch_costume(struct Session *self, const struct ItemInfo *item)
{
    int packed[3], slots[3], i;

    session_equip_slots_of(packed, item->worn_location);
    slots[0] = packed[2];
    slots[1] = packed[1];
    slots[2] = packed[0];
    for (i = 0; i < 3; i++) {
        struct ItemInfo *slot;

        if (slots[i] == -1)
            continue;
        slot = &self->switchCostumes[slots[i]];
        item_info_assign(slot, item);
        slot->count = 1;
        slot->expire_time = item->expire_time;
        slot->bind_on_equip_type = item->bind_on_equip_type;
        slot->view = item->view;
    }
    if (g_windowMgr.equip_wnd)
        ui_equip_wnd_refresh_swap_wnd(g_windowMgr.equip_wnd);
    ui_window_mgr_refresh_item_windows(&g_windowMgr);
    return 1;
}

BYTE session_put_switch_equip(struct Session *self, const struct ItemInfo *item)
{
    int packed[3], slots[3], i;

    session_equip_slots_of(packed, item->worn_location);
    slots[0] = packed[2];
    slots[1] = packed[1];
    slots[2] = packed[0];
    if (item->worn_location == 0x8000)
        session_set_arrow_status(&g_session1, item->index);
    for (i = 0; i < 3; i++) {
        if (slots[i] == -1 || slots[i] >= 10)
            break;
        item_info_assign(&self->switchEquips[slots[i]], item);
        self->switchEquips[slots[i]].count = 1;
    }
    if (g_windowMgr.equip_wnd)
        ui_equip_wnd_refresh_swap_wnd(g_windowMgr.equip_wnd);
    ui_window_mgr_refresh_item_windows(&g_windowMgr);
    return 1;
}

void session_clear_switch_costumes(struct Session *self)
{
    int i;

    for (i = 0; i < 10; i++)
        item_info_clear(&self->switchCostumes[i]);
    ui_window_mgr_refresh_item_windows(&g_windowMgr);
}

void session_clear_switch_equips(struct Session *self)
{
    int i;

    for (i = 0; i < 10; i++)
        item_info_clear(&self->switchEquips[i]);
    ui_window_mgr_refresh_item_windows(&g_windowMgr);
}

struct ItemInfo *session_get_switch_costume(struct Session *self, struct ItemInfo *out, int slot)
{
    if (slot == 0xf)
        item_info_construct_with_arg(out);
    else if ((unsigned)slot <= 9)
        item_info_copy(out, &self->switchCostumes[slot]);
    else
        item_info_construct_with_arg(out);
    return out;
}

struct ItemInfo *session_get_switch_equip(struct Session *self, struct ItemInfo *out, int slot)
{
    if (slot == 0xf) {
        session_get_inventory_item(self, out, self->switchArrowIndex);
        return out;
    }
    if ((unsigned)slot <= 9)
        item_info_copy(out, &self->switchEquips[slot]);
    else
        item_info_construct_with_arg(out);
    return out;
}

struct ItemInfo *session_find_item_for_switch(struct Session *self, struct ItemInfo *out, int index,
                                       unsigned location)
{
    struct ItemInfo found, slotItem;
    int packed[3], slots[3], i;

    if (index == 0) {
        item_info_construct_with_arg(out);
        return out;
    }
    session_get_inventory_item(self, &found, index);
    if ((unsigned)found.index > 0) {
        item_info_copy(out, &found);
        item_destroy(&found);
        return out;
    }
    session_equip_slots_of(packed, location);
    slots[0] = packed[2];
    slots[1] = packed[1];
    slots[2] = packed[0];
    for (i = 0; i < 3; i++) {
        if (slots[i] == -1)
            continue;
        if (session_is_costume_or_shadow_location(self, (int)location))
            session_get_worn_costume(self, &slotItem, slots[i]);
        else
            session_get_equipped_item(self, &slotItem, slots[i]);
        item_info_assign(&found, &slotItem);
        item_destroy(&slotItem);
        if (found.index == index)
            break;
    }
    if (found.index != 0)
        item_info_copy(out, &found);
    else
        item_info_construct_with_arg(out);
    item_destroy(&found);
    return out;
}

int session_is_in_switch_sets(struct Session *self, int index)
{
    int i;

    if (index == 0)
        return 0;
    for (i = 0; i < 10; i++)
        if (index == self->switchEquips[i].index || index == self->switchCostumes[i].index)
            return 1;
    return 0;
}

void session_take_out_switch_costume(struct Session *self, const struct ItemInfo *item)
{
    int slots[3];

    if (item->worn_location == 0x8000) {
        session_set_arrow_status(self, 0);
    } else {
        session_equip_slots_of(slots, item->worn_location);
        if (slots[2] < 10) {
            if (slots[2] != -1) {
                self->switchCostumes[slots[2]].worn_location = 0;
                item_info_clear(&self->switchCostumes[slots[2]]);
            }
            if (slots[1] != -1 && slots[1] < 10)
                item_info_clear(&self->switchCostumes[slots[1]]);
            if (slots[0] != -1 && slots[0] < 10)
                item_info_clear(&self->switchCostumes[slots[0]]);
        }
    }
    if (g_windowMgr.equip_wnd)
        ui_equip_wnd_refresh_swap_wnd(g_windowMgr.equip_wnd);
    ui_window_mgr_refresh_item_windows(&g_windowMgr);
}

void session_take_out_switch_equip(struct Session *self, const struct ItemInfo *item)
{
    int slots[3];

    if (item->worn_location == 0x8000) {
        struct ItemInfo found;

        session_find_item_for_switch(&g_session1, &found, item->index, item->worn_location);
        if ((unsigned)found.index > 0) {
            struct String line;
            struct String name = {0};

            item_info_get_display_name(&found, &name);
            string_init(&line);
            string_format(&line, "%s%s", string_cstr(&name), msg_str(0xab));
            ui_window_mgr_send_msg_(&g_windowMgr, 1, (intptr_t)string_cstr(&line), 0xff, 6, 0);
            string_free(&line);
            string_free(&name);
        }
        session_set_arrow_status(self, 0);
        item_destroy(&found);
    } else {
        session_equip_slots_of(slots, item->worn_location);
        if (slots[2] < 10) {
            if (slots[2] != -1) {
                self->switchEquips[slots[2]].worn_location = 0;
                item_info_clear(&self->switchEquips[slots[2]]);
            }
            if (slots[1] != -1 && slots[1] < 10)
                item_info_clear(&self->switchEquips[slots[1]]);
            if (slots[0] != -1 && slots[0] < 10)
                item_info_clear(&self->switchEquips[slots[0]]);
        }
    }
    if (g_windowMgr.equip_wnd)
        ui_equip_wnd_refresh_swap_wnd(g_windowMgr.equip_wnd);
    ui_window_mgr_refresh_item_windows(&g_windowMgr);
}
