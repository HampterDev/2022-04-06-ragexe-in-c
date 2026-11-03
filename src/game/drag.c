#include "game/gamemode.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "contents/mouse.h"
#include <stddef.h>
#include <string.h>
#include "game/drag.h"
#include "game/client.h"

#include "contents/barter_item.h"
#include "core/msg_str.h"
#include "session/session_items.h"
#include "session/session_names.h"
#include "ui/ui_std.h"
#include "ui/wndmgr/wndmgr.h"

#define WND_ITEM_TOOLTIP 0xf

void game_mode_reset_drag(struct GameMode *self)
{
    struct DragInfo *drag = &self->drag;

    drag->identified = 0;
    drag->index = 0;
    string_clear(&drag->name);
    drag->price = 0;
    string_clear(&drag->display_name);
    drag->source = 0;
    drag->drop_count = 0;
    drag->count = -1;
    drag->level = 0;
    drag->refine = 0;
    drag->slots[0] = 0;
    drag->slots[1] = 0;
    drag->slots[2] = 0;
    drag->slots[3] = 0;
    drag->shop_index = -1;
}

void game_mode_set_drag_source(struct GameMode *self, int source, int index)
{
    if (index == -1)
        return;
    self->drag.source = source;
    self->drag.index = index;
}

static void begin_pickup(struct GameMode *self)
{
    ui_window_mgr_show_option_window(&g_windowMgr, WND_ITEM_TOOLTIP, 0);
    game_mode_reset_drag(self);
}

static void set_names(struct DragInfo *drag, const char *name, int identified)
{
    string_assign_cstr(&drag->display_name, session_get_item_display_name(name, identified));
    string_assign_cstr(&drag->name, name);
}

void game_mode_drag_barter_item(struct GameMode *self, int source, const struct BarterGoods *item,
                                        int arg3, int count)
{
    struct DragInfo *drag = &self->drag;

    begin_pickup(self);
    drag->source = source;
    drag->named = 0;
    drag->drop_count = arg3;
    set_names(drag, string_cstr(&item->id_text), (unsigned char)item->identified);
    drag->price = item->price;
    drag->type = item->type;
    drag->identified = item->identified;
    drag->count = count;
    drag->shop_index = item->key;
}

void game_mode_drag_expanded_barter_item(struct GameMode *self, int source,
                                                const struct BarterGoodsEx *item, int arg3, int count)
{
    struct DragInfo *drag = &self->drag;

    begin_pickup(self);
    drag->source = source;
    drag->named = 0;
    drag->drop_count = arg3;
    set_names(drag, string_cstr(&item->id_text), (unsigned char)item->identified);
    drag->price = item->price;
    drag->type = item->type;
    drag->identified = item->identified;
    drag->count = count;
    drag->refine = item->refine;
    drag->shop_index = item->key;
}

void game_mode_drag_item(struct GameMode *self, int source, const struct ItemInfo *item, int arg3,
                                  int count)
{
    struct DragInfo *drag = &self->drag;
    int i;

    begin_pickup(self);
    if ((unsigned)item->index == 0)
        return;
    drag->source = source;
    drag->named = 0;
    drag->index = item->index;
    drag->drop_count = arg3;
    set_names(drag, string_cstr(&item->id_text), (unsigned char)item->identified);
    drag->price = item->price;
    drag->charged_price = item->charged_price;
    drag->type = item->type;
    drag->identified = item->identified;
    drag->count = count;
    drag->refine = item->refine;
    memcpy(drag->slots, item->slot, sizeof drag->slots);
    drag->option_count = item->option_count;
    for (i = 0; i < drag->option_count; i++)
        drag->options[i] = item->options[i];
    drag->by_index = item->by_index;
    drag->equip_location = item->equip_location;
    drag->view = item->view;
}

void game_mode_drag_by_name(struct GameMode *self, int source, int index, const char *name, int level,
                                    int count)
{
    struct DragInfo *drag = &self->drag;

    begin_pickup(self);
    drag->source = source;
    drag->index = index;
    drag->identified = 0;
    string_assign_cstr(&drag->display_name, session_get_item_act_name(name));
    string_assign_cstr(&drag->name, name);
    string_assign_cstr(&drag->name2, name);
    drag->level = level;
    drag->named = 1;
    drag->count = count;
}

#define MODE_MSG_END_DRAG 0x12

char game_mode_refuse_held_item_drag(struct GameMode *self)
{
    struct ItemInfo item;
    char refused = 0;

    if (g_session1.wornArrowIndex != self->drag.index)
        return 0;
    session_get_inventory_item(&g_session1, &item, self->drag.index);
    if ((unsigned)item.index > 0) {
        mode_send_msg(self, MODE_MSG_END_DRAG, 0, 0, 0, 0);
        ui_window_mgr_message_box(&g_windowMgr, msg_str(0x291), 0, 1, 0, 0, UI_MSGBOX_TITLE, 0x118, 0x78, 0);
        refused = 1;
    }
    string_free(&item.name);
    string_free(&item.id_text);
    return refused;
}

enum { DRAG_FROM_EQUIP = 2, DRAG_FROM_COSTUME = 0x18 };

const char *game_mode_count_item_id_text(struct GameMode *self)
{
    struct ItemInfo item;
    const char *id_text;

    if (self->drag.source != DRAG_FROM_EQUIP && self->drag.source != DRAG_FROM_COSTUME)
        return self->drag.name.size != 0 ? string_cstr(&self->drag.name) : 0;
    session_get_equipped_item(&g_session1, &item, self->drag.count);
    id_text = (unsigned)item.index > 0 ? string_cstr(&item.id_text) : 0;
    string_free(&item.name);
    string_free(&item.id_text);
    return id_text;
}

#include "render/sprite_draw.h"
#include "ui/window_types.h"
enum { DRAG_FROM_WINDOW_MGR = 0x1d };

void game_mode_draw_dragged(struct GameMode *self)
{
    struct String name, spr_name, act_name;
    struct SprRes *spr;
    struct ActRes *act;

    if (self->drag.source == 0)
        return;
    if (g_windowMgr.item_drop_cnt_wnd && g_windowMgr.item_drop_cnt_wnd->visible != 0)
        return;
    if (self->drag.source == DRAG_FROM_WINDOW_MGR) {
        if (ui_window_mgr_is_dragging_member_plate(&g_windowMgr) == 1)
            ui_window_mgr_move_dragged_plate(&g_windowMgr, g_mouse.x, g_mouse.y);
        return;
    }
    if (self->drag.display_name.size == 0) {
        game_mode_reset_drag(self);
        return;
    }
    string_copy(&name, &self->drag.display_name);
    string_copy(&spr_name, &name);
    string_append_cstr(&spr_name, ".spr");
    string_copy(&act_name, &name);
    string_append_cstr(&act_name, ".act");
    spr = res_get(string_cstr(&spr_name));
    act = res_get(string_cstr(&act_name));
    screen_sprite_frame(g_renderer, g_mouse.x, g_mouse.y, act, spr, 0, 0, 1.0f, 0, 0xfaffffff, 0x201);
    string_free(&act_name);
    string_free(&spr_name);
    string_free(&name);
}
