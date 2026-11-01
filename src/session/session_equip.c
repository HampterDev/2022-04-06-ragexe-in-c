#include "effects/rag/rag_effect.h"
#include "ui/window_manager_state.h"
#include "game/context.h"
#include "session/csession.h"
#include "session/session_items.h"
#include "session/session_jobs.h"
#include "session/session_windows.h"

#include <string.h>
#include "game/gamemode.h"
#include "actor/render_ops.h"
#include "ui/wndmgr/wndmgr.h"

#define LOCATION_ARROW      0x8000
#define MSG_PUT_ON          0xaa
#define MSG_TAKEN_OFF       0xab
#define STATUS_ARROW        0x2b7

static void say_item(const struct ItemInfo *item, int msg, int color)
{
    struct String line;

    struct String name = {0};

    item_info_get_display_name(item, &name);
    string_init(&line);
    string_format(&line, "%s%s", string_cstr(&name), msg_str(msg));
    ui_window_mgr_send_msg_(&g_windowMgr, 1, (int)string_cstr(&line), color, 6, 0);
    string_free(&line);
    string_free(&name);
}

static struct Player *mode_player(struct GameMode *gm)
{
    return gm->world ? gm->world->player : NULL;
}

BYTE session_take_off_equip(struct Session *self, unsigned location, int index)
{
    if (location == LOCATION_ARROW) {
        struct ItemInfo arrow;
        struct GameMode *gm;

        session_get_inventory_item(&g_session1, &arrow, index);
        if ((unsigned)arrow.index > 0)
            say_item(&arrow, MSG_TAKEN_OFF, 0xff);
        self->wornArrowIndex = 0;
        ui_window_mgr_invalidate_once_per_frame(&g_windowMgr);
        gm = game_current_mode();
        if (gm) {
            struct Player *player = mode_player(gm);

            own_states_set_present(STATUS_ARROW, 0);
            if (player && player->status_icons)
                rag_effect_send_msg(player->status_icons, NULL, 0x2c, 1, 0, 0);
        }
        string_free(&arrow.name);
        string_free(&arrow.id_text);
    } else {
        int slots[3];

        equip_location_slots(slots, equip_location_to_base(location));
        if (slots[2] < 10) {
            if (slots[2] != -1) {
                struct ItemInfo *slot = &self->equipSlots[slots[2]];
                say_item(slot, MSG_TAKEN_OFF, 0xff);
                slot->worn_location = 0;
                session_add_inventory_item(self, slot, 1);
                item_info_clear(slot);
            }
            if (slots[1] != -1 && slots[1] < 10)
                item_info_clear(&self->equipSlots[slots[1]]);
            if (slots[0] != -1 && slots[0] < 10)
                item_info_clear(&self->equipSlots[slots[0]]);
        }
        session_refresh_equip_wnd(self);
    }
    ui_window_mgr_invalidate_once_per_frame(&g_windowMgr);
    return 1;
}

BYTE session_set_worn_costume(struct Session *self, const struct ItemInfo *item)
{
    int out[3], slots[3], i;

    session_equip_slots_of(out, item->worn_location);
    slots[0] = out[2];
    slots[1] = out[1];
    slots[2] = out[0];
    for (i = 0; i < 3; i++) {
        struct ItemInfo *slot;
        if (slots[i] == -1)
            continue;
        slot = &self->costumeSlots[slots[i]];
        item_info_assign(slot, item);
        slot->count = 1;
        slot->expire_time = item->expire_time;
        slot->bind_on_equip_type = item->bind_on_equip_type;
        slot->view = item->view;
    }
    session_invalidate_windows(self);
    return 1;
}

BYTE session_put_on_equip(struct Session *self, struct ItemInfo *item)
{
    say_item(item, MSG_PUT_ON, 0xc8ff00);
    if (item->bind_on_equip_type == 1)
        item->bind_on_equip_type = 2;
    if (session_is_costume_or_shadow_location(&g_session1, (int)item->worn_location)) {
        session_set_worn_costume(&g_session1, item);
    } else {
        int slots[3], i;

        if (item->worn_location == LOCATION_ARROW) {
            slots[0] = 0xf;
            slots[1] = -1;
            slots[2] = -1;
        } else {
            equip_location_slots(slots, equip_location_to_base(item->worn_location));
        }
        for (i = 0; i < 3; i++)
            if (slots[i] != -1 && slots[i] < 10) {
                item_info_assign(&self->equipSlots[slots[i]], item);
                self->equipSlots[slots[i]].count = 1;
            }
        session_refresh_equip_wnd(self);
        ui_window_mgr_invalidate_once_per_frame(&g_windowMgr);
    }
    if (item->worn_location != LOCATION_ARROW)
        session_remove_inventory_item(self, item->index, 1);
    return 1;
}

void session_set_costume_slot(struct Session *self, unsigned i,
                                        const struct ItemInfo *item)
{
    if (i <= 9)
        item_info_assign(&self->costumeSlots[i], item);
}

void session_set_equip_slot(struct Session *self, unsigned i,
                                      const struct ItemInfo *item)
{
    if (i <= 9)
        item_info_assign(&self->equipSlots[i], item);
}

void session_set_arrow(struct Session *self, int index)
{
    struct GameMode *gm;
    struct Player *player;

    self->wornArrowIndex = index;
    ui_window_mgr_invalidate_once_per_frame(&g_windowMgr);
    gm = game_current_mode();
    if (!gm)
        return;
    own_states_set_present(STATUS_ARROW, index != 0);
    player = mode_player(gm);
    if (player && player->status_icons)
        rag_effect_send_msg(player->status_icons, NULL, 0x2c, 1, 0, 0);
}

BYTE session_set_viewed_costume(struct Session *self, const struct ItemInfo *item)
{
    int out[3], slots[3], i;

    session_equip_slots_of(out, item->worn_location);
    slots[0] = out[2];
    slots[1] = out[1];
    slots[2] = out[0];
    for (i = 0; i < 3; i++) {
        struct ItemInfo *slot;
        if (slots[i] == -1)
            continue;
        slot = &self->viewedCostumes[slots[i]];
        item_info_assign(slot, item);
        slot->count = 1;
        slot->expire_time = item->expire_time;
        slot->bind_on_equip_type = item->bind_on_equip_type;
        slot->view = item->view;
    }
    return 1;
}

void session_clear_worn_costumes(struct Session *self)
{
    int i;

    for (i = 0; i < 10; i++)
        item_info_clear(&self->costumeSlots[i]);
}

void session_clear_viewed_costumes(struct Session *self)
{
    int i;

    for (i = 0; i < 10; i++)
        item_info_clear(&self->viewedCostumes[i]);
}

struct ItemInfo *session_get_worn_costume(struct Session *self, struct ItemInfo *out, int slot)
{
    if (slot == 15)
        item_info_construct_with_arg(out);
    else if ((unsigned)slot <= 9)
        item_info_copy(out, &self->costumeSlots[slot]);
    else
        item_info_construct_with_arg(out);
    return out;
}

BYTE session_take_off_costume(struct Session *self, unsigned location)
{
    int slots[3];

    if (location == LOCATION_ARROW) {
        session_set_arrow(self, 0);
    } else {
        session_equip_slots_of(slots, location);
        if (slots[2] < 10) {
            if (slots[2] != -1) {
                struct ItemInfo *slot = &self->costumeSlots[slots[2]];
                say_item(slot, MSG_TAKEN_OFF, 0xff);
                slot->worn_location = 0;
                session_add_inventory_item(self, slot, 1);
                item_info_clear(slot);
            }
            if (slots[1] != -1 && slots[1] < 10)
                item_info_clear(&self->costumeSlots[slots[1]]);
            if (slots[0] != -1 && slots[0] < 10)
                item_info_clear(&self->costumeSlots[slots[0]]);
        }
    }
    session_invalidate_windows(self);
    return 1;
}

void session_set_arrow_status(struct Session *self, int index)
{
    struct GameMode *gm;
    struct Player *player;

    self->switchArrowIndex = index;
    session_invalidate_windows(self);
    gm = game_current_mode();
    if (!gm)
        return;
    own_states_set_present(STATUS_ARROW, index != 0);
    player = mode_player(gm);
    if (!player || !player->status_icons) {
        hideui_skip_count++;
        return;
    }
    rag_effect_send_msg(player->status_icons, NULL, 0x2c, 1, 0, 0);
}
