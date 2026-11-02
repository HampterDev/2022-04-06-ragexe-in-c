#include "game/drag_info.h"
#include "world/tooltip.h"
#include "world/mode_manager.h"
#include "ui/window_access.h"
#include "game/context.h"
#include "ui/items/refining_internal.h"
#include "core/msg_str.h"
#include "session/session_items.h"

static int in_item_box(int x, int y)
{
    return (unsigned)(x - 0x6d) <= 0x2e && (unsigned)(y - 0xc9) <= 0x2e;
}

static int in_icon(int x, int y, int x0)
{
    return x >= x0 && x <= x0 + 0x18 && y >= 0x96 && y <= 0xae;
}

static int material_slot_at(int x, int y)
{
    int i;

    if ((unsigned)(x - 0xb) > 0xf2 || (unsigned)(y - 0x37) > 0x34)
        return -1;
    i = (x - 0xb) / 0x31;
    if (x > i * 0x31 + 0x39 || i >= 5)
        return -1;
    return i;
}

void ui_refining_wnd_drag_and_drop(struct UIWindow *window, int x, int y, const struct DragInfo *drag)
{
    struct UIRefiningWnd *self = (struct UIRefiningWnd *)window;
    if (in_item_box(x, y) && drag->source == 1)
        ui_refining_wnd_send_msg(&self->frame.base, WND(self), MSG_ITEM_DROPPED, drag->index, 0, 0, 0);
    game_send_message(MODE_MSG_DRAG_END, 0, 0, 0, 0);
}

void ui_refining_wnd_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIRefiningWnd *self = (struct UIRefiningWnd *)window;
    if (y < 0x11) {
        ui_frame_wnd_on_l_btn_down((struct UIWindow *)(&self->frame), x, y);
        return;
    }
    if (self->state == REFINE_READY && self->item.index && in_item_box(x, y))
        game_mode_drag_item(game_current_mode(), 0x1c, &self->item, 1, -1);
}

void ui_refining_wnd_on_l_btn_dbl_clk(struct UIWindow *window, int x, int y)
{
    struct UIRefiningWnd *self = (struct UIRefiningWnd *)window;
    if (self->item.index && in_item_box(x, y))
        ui_refining_wnd_reset(self);
}

static int material_hint(int item_id)
{
    switch (item_id) {
    case 0x3d8:
    case 0x3d9:
    case 0x3f2:
    case 0x3f3:
    case 0x1dc3:
    case 0x1dc4:
    case 0xf438f:
    case 0xf4390:
    case 0xf43b0:
    case 0xf43b1:
    case 0xf43b2:
    case 0xf43b3:
        return MSG_HINT_ORE;
    case 0x184f:
    case 0x1850:
        return MSG_HINT_HD;
    case 0x1851:
    case 0x1852:
    case 0x1860:
    case 0x1861:
    case 0x18b7:
    case 0x1afa:
    case 0x1afe:
    case 0x1aff:
    case 0xf438b:
    case 0xf438c:
    case 0xf438d:
    case 0xf438e:
        return MSG_HINT_ENRICHED;
    }
    return 0;
}

void ui_refining_wnd_on_l_btn_up(struct UIWindow *window, int x, int y)
{
    struct UIRefiningWnd *self = (struct UIRefiningWnd *)window;
    int i;

    if (y < 0x11) {
        ui_frame_wnd_on_l_btn_up((struct UIWindow *)(&self->frame), x, y);
        return;
    }
    if (self->state != REFINE_READY)
        return;
    i = material_slot_at(x, y);
    if (i < 0)
        return;
    if ((unsigned)i < MATERIAL_COUNT(&self->materials)) {
        struct RefineMaterial *m = MATERIAL(&self->materials, i);
        int hint;

        if (refining_carried(m->item_id) <= 0)
            return;
        self->selected = i;
        self->btn_refine->price = refining_selected_zeny(self);
        ui_window_invalidate_base(&self->btn_refine->button.button.base);
        m = MATERIAL(&self->materials, i);
        if (m->chance < 100 && !self->blessing && (hint = material_hint(m->item_id)) != 0)
            ui_refining_notice_set_text(self->notice, msg_str(hint), 0, 0);
    } else if (i == 4 && self->blessing_count > 0) {
        if (self->blessing_count > refining_carried(self->blessing_item)) {
            ui_refining_notice_set_text(self->notice, msg_str(MSG_NO_BLESSINGS), NOTICE_TIME_SHORT, 1);
        } else if (self->blessing == 1) {
            self->blessing = 0;
            ui_refining_notice_set_text(self->notice, "", 0, 0);
        } else {
            self->blessing = 1;
            ui_refining_notice_set_text(self->notice, msg_str(MSG_BLESSING_ON), 0, 0);
        }
    } else {
        return;
    }
    ui_window_invalidate_base(&self->frame.base);
}

static void show_name_tooltip(struct UIRefiningWnd *self, const char *name, int x, int y, int lift)
{
    ui_window_to_screen(WND(self), &x, &y);
    mode_show_tooltip(client_current_mode(), name, x, y - lift, 0xffffffff, 0);
}

static const char *item_name(int item_id)
{
    struct ItemInfo record;
    const char *name;

    item_info_construct(&record);
    item_info_set_item_id(&record, item_id);
    name = item_info_get_name(&record);
    item_info_destruct(&record);
    return name;
}

void ui_refining_wnd_on_mouse_hover(struct UIWindow *window, int x, int y)
{
    struct UIRefiningWnd *self = (struct UIRefiningWnd *)window;
    const char *name = "";
    int i;

    if (self->state != REFINE_READY)
        return;
    if (self->selected != -1 && in_icon(x, y, 0x51)) {
        name = item_name(MATERIAL(&self->materials, self->selected)->item_id);
        show_name_tooltip(self, name, 0x51, 0x96, 0x14);
        return;
    }
    if (self->blessing == 1 && in_icon(x, y, 0xa1)) {
        name = item_name(self->blessing_item);
        show_name_tooltip(self, name, 0xa1, 0x96, 0x14);
        return;
    }
    i = material_slot_at(x, y);
    if (i < 0)
        return;
    if ((unsigned)i < MATERIAL_COUNT(&self->materials))
        name = item_name(MATERIAL(&self->materials, i)->item_id);
    else if (i == 4 && self->blessing_count > 0)
        name = item_name(self->blessing_item);
    if (strlen(name))
        show_name_tooltip(self, name, i * 0x31 + 0xb, 0x23, 0);
}

void ui_refining_wnd_on_r_btn_down(struct UIWindow *self, int x, int y)
{
}

static void describe_carried(int item_id)
{
    struct ItemInfo inv;
    struct UIWindow *describe;

    session_get_inventory_item_by_id(&g_session1, &inv, item_id, 0);
    if (inv.index) {
        describe = ui_window_open(WID_ITEM_DESCRIBE);
        if (describe)
            ui_window_send_msg(describe, NULL, MSG_DESCRIBE_ITEM, (intptr_t)&inv, 0, 0, 0);
    }
    item_info_destruct(&inv);
}

void ui_refining_wnd_on_r_btn_up(struct UIWindow *window, int x, int y)
{
    struct UIRefiningWnd *self = (struct UIRefiningWnd *)window;
    struct UIWindow *describe;
    int i;

    if (y < 0x11)
        return;
    if (self->state == REFINE_READY) {
        if (self->selected != -1 && in_icon(x, y, 0x51)) {
            struct ItemInfo inv;

            describe = ui_window_open(WID_ITEM_DESCRIBE);
            if (!describe)
                return;
            session_get_inventory_item_by_id(&g_session1, &inv,
                                          MATERIAL(&self->materials, self->selected)->item_id, 0);
            ui_window_send_msg(describe, NULL, MSG_DESCRIBE_ITEM, (intptr_t)&inv, 0, 0, 0);
            item_info_destruct(&inv);
            return;
        }
        if (self->blessing == 1 && in_icon(x, y, 0xa1)) {
            struct ItemInfo inv;

            describe = ui_window_open(WID_ITEM_DESCRIBE);
            if (!describe)
                return;
            session_get_inventory_item_by_id(&g_session1, &inv, self->blessing_item, 0);
            ui_window_send_msg(describe, NULL, MSG_DESCRIBE_ITEM, (intptr_t)&inv, 0, 0, 0);
            item_info_destruct(&inv);
            return;
        }
        i = material_slot_at(x, y);
        if (i >= 0) {
            if ((unsigned)i < MATERIAL_COUNT(&self->materials)) {
                describe_carried(MATERIAL(&self->materials, i)->item_id);
                return;
            }
            if (i == 4) {
                if (self->blessing_count > 0)
                    describe_carried(self->blessing_item);
                return;
            }
        }
    }
    if (self->item.index && in_item_box(x, y)) {
        describe = ui_window_open(WID_ITEM_DESCRIBE);
        if (describe)
            ui_window_send_msg(describe, NULL, MSG_DESCRIBE_ITEM, (intptr_t)&self->item, 0, 0, 0);
    }
}
