#include "game/drag_info.h"
#include "world/tooltip.h"
#include "contents/client_lua_calls.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "world/mode_manager.h"
#include "game/context.h"
#include "ui/window_access.h"
#include "contents/connection_api.h"
#include <stdlib.h>
#include <string.h>
#include "contents/client_res.h"
#include "contents/connection.h"
#include "contents/game_session.h"
#include "iteminfo/item_api.h"
#include "std/vec.h"
#include "contents/lapine_box.h"
#include "contents/lapine_upgrade.h"
#include "contents/lua_script.h"
#include "std/std.h"
#include "ui/controls/buttons.h"
#include "ui/controls/combo_box.h"
#include "ui/controls/scroll.h"
#include "ui/controls/line_info.h"
#include "ui/quests/tag_text_viewer.h"
#include "base.h"
#include "ui/window_types.h"
#include "ui/controls/edit_types.h"
#include "contents/ui_window.h"
#include "contents/ui_window_mgr.h"
#include <mmsystem.h>
#include "core/object_type_tables.h"
#include "core/msg_str.h"
#include "session/session_items.h"

#define UPGRADE_TABLE           "tblLapineUpgradeBox"

#define MSG_NEEDS_LABEL         0xb50
#define MSG_NAME_WITH_OPTIONS   0xa28

#define TARGET_X                0x65
#define TARGET_Y                0xa5
#define TARGET_SIZE             24
#define COLOR_ROW_BG            0xfff7f7

#pragma pack(push, 1)
struct LapineUpgradePacket {
    unsigned short id;
    int box_item;
    unsigned short index;
};
#pragma pack(pop)
SIZE_ASSERT(LapineUpgradePacket, 8);

static int on_target(int x, int y)
{
    return (unsigned)(x - TARGET_X) <= TARGET_SIZE && (unsigned)(y - TARGET_Y) <= TARGET_SIZE;
}

static void clear_target(struct LapineUpgradeBox *self)
{
    struct ItemInfo empty;

    item_info_construct(&empty);
    item_info_assign(&self->target, &empty);
    item_info_destruct(&empty);
}

static void send_upgrade_packet(struct LapineUpgradeBox *self)
{
    struct LapineUpgradePacket packet;
    int len;

    packet.index = (unsigned short)self->target.index;
    packet.id = PACKET_LAPINE_UPGRADE;
    packet.box_item = self->box_item;
    len = rag_connection_get_packet_length(rag_connection_instance(), PACKET_LAPINE_UPGRADE);
    rag_connection_send_packet(rag_connection_instance(), len, &packet);
    self->waiting = 1;
    ((struct UIButton *)self->make_button)->disabled = 1;
    ui_window_invalidate(self->make_button);
    ui_window_invalidate_base(&self->frame.base);
}

static void set_box(struct LapineUpgradeBox *self, int box_item)
{
    struct ItemNameRuns runs = {0};
    struct ItemInfo item;

    int min_refine, max_refine;
    const char *need_source;

    self->box_item = box_item;

    item_info_construct(&item);
    item_info_set_item_id(&item, box_item);
    item_info_name_runs(&self->frame.base, &item, &runs, 1, 0);
    string_assign_cstr(&self->title, string_cstr(&runs.text));
    item_info_destruct(&item);
    string_free(&runs.text);

    min_refine = 0;

    client_lua_call_table(g_session1.luaState, UPGRADE_TABLE, "get_min_need_refine", "d>d", box_item, &min_refine);
    self->list->min_refine = min_refine;

    max_refine = 0;

    client_lua_call_table(g_session1.luaState, UPGRADE_TABLE, "get_max_need_refine", "d>d", box_item, &max_refine);
    self->list->max_refine = max_refine;

    client_lua_call_table(g_session1.luaState, UPGRADE_TABLE, "get_min_need_option_num", "d>d", box_item, &self->min_option_num);
    self->list->min_option_num = self->min_option_num;

    client_lua_call_table(g_session1.luaState, UPGRADE_TABLE, "set_resource_item_list", "d", box_item);
    ui_window_send(&self->list->scroll.base, &self->frame.base, LAPINE_MSG_FILL, 0, 0, 0, 0);

    need_source = NULL;

    client_lua_call_table(g_session1.luaState, UPGRADE_TABLE, "get_need_source_string", "d>s", box_item, &need_source);
    string_assign_cstr(&self->need_source, need_source);
}

static void set_target(struct LapineUpgradeBox *self, int index)
{
    struct ItemInfo item;

    if ((unsigned)self->target.index > 0 || lapine_upgrade_list_take(self->list, index) != 1)
        return;
    session_get_inventory_item(&g_session1, &item, index);
    if ((unsigned)item.index > 0) {
        struct ItemNameRuns runs = {0};

        item_info_name_runs(&self->frame.base, &item, &runs, 1, 0);
        if (item.identified == 1 && item.option_count)
            string_format(&runs.text, msg_str(MSG_NAME_WITH_OPTIONS), string_cstr(&runs.text), item.option_count);
        item_info_set_name(&item, string_cstr(&runs.text));
        string_free(&runs.text);
        item_info_assign(&self->target, &item);
        ui_window_invalidate_base(&self->frame.base);
    }
    item_info_destruct(&item);
}

static struct LapineUpgradeList *upgrade_list_new(void)
{
    struct LapineUpgradeList *self = (struct LapineUpgradeList *)mem_alloc(sizeof *self);

    if (self)
        lapine_upgrade_list_construct(self, 0x96, 0xcc);
    return self;
}

struct LapineUpgradeBox *lapine_upgrade_construct(struct LapineUpgradeBox *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &lapine_upgrade_box_ops;
    self->cancel_button = NULL;
    self->make_button = NULL;
    self->move_button = NULL;
    self->list = NULL;
    self->box_item = 0;
    string_init(&self->title);
    item_info_construct(&self->target);
    string_init(&self->need_source);
    string_init(&self->notice);
    self->notice_time = 0;
    self->notice_duration = 3000;
    self->waiting = 0;
    return self;
}

struct UIWindow *lapine_upgrade_new(void)
{
    return (struct UIWindow *)lapine_upgrade_construct(mem_alloc(sizeof(struct LapineUpgradeBox)));
}

void lapine_upgrade_delete(struct UIWindow *window)
{
    struct LapineUpgradeBox *self = (struct LapineUpgradeBox *)window;
    lapine_upgrade_destruct(self);
    mem_free(self);
}

void lapine_upgrade_drop(struct UIWindow *window, int x, int y, const struct DragInfo *drop)
{
    struct LapineUpgradeBox *self = (struct LapineUpgradeBox *)window;

    if ((unsigned)self->target.index == 0 && drop->source == LAPINE_DRAG_CANDIDATE)
        ui_window_send(&self->frame.base, &self->frame.base, LAPINE_UPGRADE_MSG_SET_TARGET, drop->index, 0, 0, 0);
    game_send_message(MODE_MSG_DROPPED, 0, 0, 0, 0);
}

void lapine_upgrade_on_create(struct UIWindow *window, int w, int h)
{
    struct LapineUpgradeBox *self = (struct LapineUpgradeBox *)window;
    ui_window_add_sys_button(&self->frame.base);
    ui_window_add_close_button(&self->frame.base, "", 0);
    self->list = upgrade_list_new();
    ui_window_move_base(&self->list->scroll.base, 0xf1, 0x11);
    ui_window_add_child(&self->frame.base, &self->list->scroll.base);
    lapine_add_buttons(&self->frame.base, &self->cancel_button, &self->make_button, &self->move_button,
                       LAPINE_UPGRADE_MAKE_BUTTON_ID);
}

void lapine_upgrade_on_frame(struct UIWindow *window)
{
    struct LapineUpgradeBox *self = (struct LapineUpgradeBox *)window;
    struct LapineIngredient *row = lapine_upgrade_list_selected(self->list);
    struct UIWindow *button = self->move_button;
    char disabled = ((struct UIButton *)button)->disabled;
    int changed = 0;

    if (row) {
        if (disabled == 1 && (unsigned)self->target.index == 0) {
            ((struct UIButton *)button)->disabled = 0;
            changed = 1;
        }
    } else if (disabled == 0) {
        ((struct UIButton *)button)->disabled = 1;
        changed = 1;
    }
    if (changed) {
        ui_window_invalidate(button);
        ui_window_invalidate_base(&self->frame.base);
    }

    if (!self->waiting) {
        button = self->make_button;
        disabled = ((struct UIButton *)button)->disabled;
        changed = 0;
        if ((unsigned)self->target.index > 0) {
            if (disabled == 1) {
                ((struct UIButton *)button)->disabled = 0;
                changed = 1;
            }
        } else if (disabled == 0) {
            ((struct UIButton *)button)->disabled = 1;
            changed = 1;
        }
        if (changed) {
            ui_window_invalidate(button);
            ui_window_invalidate_base(&self->frame.base);
        }
    }
    if (self->notice_time + self->notice_duration < timeGetTime())
        string_clear(&self->notice);
}

void lapine_upgrade_on_draw(struct UIWindow *window)
{
    struct LapineUpgradeBox *self = (struct LapineUpgradeBox *)window;
    ui_window_draw_bitmap(&self->frame.base, 0, 0, res_mgr_get(g_resMgr(), LAPINE_BMP("bg_lapineUpgrade.bmp")), 1);
    if (self->title.size > 0)
        ui_window_draw_title_bar(&self->frame.base, 0, string_cstr(&self->title), 0);
    ui_window_text_out_centred(&self->frame.base, 0x23, 0x2f, msg_str(MSG_NEEDS_LABEL), 0x9c, 0, 0xb, 0xcd7a51, 0);
    ui_window_text_out_centred(&self->frame.base, 0x23, 0x3f, string_cstr(&self->need_source), 0x9c, 0, 0xb, 0x844a42, 0);
    if ((unsigned)self->target.index > 0)
        ui_window_draw_item_icon(&self->frame.base, TARGET_X, TARGET_Y, &self->target, 0);
    if (self->notice_time + self->notice_duration > timeGetTime()) {
        ui_window_draw_bitmap(&self->frame.base, 6, 0xe5, res_mgr_get(g_resMgr(), LAPINE_BMP("icon_notify.bmp")), 1);
        ui_window_text_out(&self->frame.base, 0x19, 0xe6, string_cstr(&self->notice), 0, 0, 0xc, 0xff0000, 0, 0);
    }
}

void lapine_upgrade_press(struct UIWindow *window, int x, int y)
{
    struct LapineUpgradeBox *self = (struct LapineUpgradeBox *)window;
    if (y < 0x11)
        ui_frame_wnd_on_l_btn_down((struct UIWindow *)(&self->frame), x, y);
    if (on_target(x, y) && (unsigned)self->target.index > 0)
        game_mode_drag_item(game_current_mode(), LAPINE_DRAG_CHOSEN, &self->target, 1, -1);
}

void lapine_upgrade_double_click(struct UIWindow *window, int x, int y)
{
    struct LapineUpgradeBox *self = (struct LapineUpgradeBox *)window;
    if (on_target(x, y) && self->target.index)
        ui_window_send(&self->frame.base, &self->frame.base, LAPINE_MSG_REMOVE, self->target.index, 0, 0, 0);
}

void lapine_upgrade_hover(struct UIWindow *window, int x, int y)
{
    struct LapineUpgradeBox *self = (struct LapineUpgradeBox *)window;
    ui_frame_wnd_on_mouse_move(&self->frame.base, x, y);
    if (on_target(x, y) && (unsigned)self->target.index > 0) {
        int bx = TARGET_X, by = TARGET_Y - 20;

        ui_window_to_screen(&self->frame.base, &bx, &by);
        mode_show_tooltip(client_current_mode(), string_cstr(&self->target.name), bx, by, -1, 0);
    }
}

void lapine_upgrade_show_info(struct UIWindow *window, int x, int y)
{
    struct LapineUpgradeBox *self = (struct LapineUpgradeBox *)window;
    if (on_target(x, y) && (unsigned)self->target.index > 0) {
        struct UIWindow *desc = ui_window_open(ITEM_DESC_WND_ID);

        ui_window_send(desc, NULL, UI_MSG_SHOW_ITEM, (int)&self->target, 0, 0, 0);
    }
}

int lapine_upgrade_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                       int a, int b, int c, int d)
{
    struct LapineUpgradeBox *self = (struct LapineUpgradeBox *)window;
    switch (msg) {
    case UI_MSG_BUTTON:
        if (a == LAPINE_CANCEL_BUTTON_ID) {
            ui_window_mgr_post_quit1(&g_windowMgr, LAPINE_UPGRADE_WND_ID);
        } else if (a == LAPINE_MOVE_BUTTON_ID) {
            struct LapineIngredient *row = lapine_upgrade_list_selected(self->list);

            if (row)
                ui_window_send(&self->frame.base, &self->frame.base, LAPINE_UPGRADE_MSG_SET_TARGET, row->item.index, 0, 0, 0);
        } else if (a == LAPINE_UPGRADE_MAKE_BUTTON_ID) {
            if (self->target.index)
                send_upgrade_packet(self);
        }
        return 0;
    case LAPINE_UPGRADE_MSG_SET_BOX:
        set_box(self, a);
        return 0;
    case LAPINE_UPGRADE_MSG_SET_TARGET:
        set_target(self, a);
        return 0;
    case LAPINE_MSG_REMOVE:
        if (self->target.index && self->target.index == a) {
            clear_target(self);
            ui_window_invalidate_base(&self->frame.base);
            lapine_upgrade_list_add(self->list, a);
        }
        return 0;
    case LAPINE_MSG_NOTICE:
        self->notice_time = timeGetTime();
        string_assign_cstr(&self->notice, (const char *)a);
        ui_window_invalidate_base(&self->frame.base);
        return 0;
    default:
        return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, a, b, c, d);
    }
}

void lapine_upgrade_list_delete(struct UIWindow *window)
{
    struct LapineUpgradeList *self = (struct LapineUpgradeList *)window;
    lapine_upgrade_list_destruct(self);
    mem_free(self);
}

void lapine_upgrade_list_drop(struct UIWindow *window, int x, int y, const struct DragInfo *drop)
{
    struct LapineUpgradeList *self = (struct LapineUpgradeList *)window;

    if (drop->source == LAPINE_DRAG_CHOSEN)
        ui_window_send(&self->scroll.base, &self->scroll.base, LAPINE_UPGRADE_MSG_SET_TARGET, drop->index, 0, 0, 0);
    game_send_message(MODE_MSG_DROPPED, 0, 0, 0, 0);
}

static void fill(struct LapineUpgradeList *self)
{
    struct ItemInfo *r;
    struct LapineIngredient **it;

    for (r = (struct ItemInfo *)self->resources.first; r != (struct ItemInfo *)self->resources.last; r++) {
        struct Vec indices;
        int *p;

        session_inventory_indices(&g_session1, &indices, atoi(string_cstr(&r->id_text)), 1);
        for (p = (int *)indices.first; p != (int *)indices.last; p++)
            lapine_upgrade_list_add(self, *p);
        vec_free(&indices);
    }
    for (it = (struct LapineIngredient **)self->rows.first; it != (struct LapineIngredient **)self->rows.last; it++) {
        struct ItemInfo item;

        session_get_inventory_item(&g_session1, &item, (*it)->item.index);
        if ((unsigned)item.index == 0)
            ui_window_set_show_base(&(*it)->frame.base, 0);
        item_info_destruct(&item);
    }
    ui_window_send(&self->scroll.base, &self->scroll.base, LAPINE_MSG_LAYOUT, 0, 0, 0, 0);
}

int lapine_upgrade_list_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                            int a, int b, int c, int d)
{
    struct LapineUpgradeList *self = (struct LapineUpgradeList *)window;
    struct LapineIngredient **it;

    switch (msg) {
    case UI_MSG_BUTTON:
        for (it = (struct LapineIngredient **)self->rows.first; it != (struct LapineIngredient **)self->rows.last;
             it++) {
            if (&(*it)->frame.base != sender) {
                (*it)->selected = 0;
                ui_window_invalidate_base(&(*it)->frame.base);
            }
        }
        ui_window_invalidate_base(&self->scroll.base);
        break;
    case LAPINE_MSG_FILL:
        fill(self);
        break;
    case LAPINE_UPGRADE_MSG_SET_TARGET:
        ui_window_send(self->scroll.base.parent, &self->scroll.base, LAPINE_MSG_REMOVE, a, 0, 0, 0);
        break;
    case LAPINE_MSG_LAYOUT:
        lapine_list_layout(&self->scroll, &self->rows);
        break;
    case LAPINE_MSG_REMOVE:
        ui_window_send(self->scroll.base.parent, &self->scroll.base, LAPINE_UPGRADE_MSG_SET_TARGET, a, 0, 0, 0);
        break;
    }
    return ui_scroll_wnd_send_msg((struct UIWindow *)(&self->scroll), sender, msg, a, b, c, d);
}

void lapine_upgrade_row_delete(struct UIWindow *window)
{
    struct LapineIngredient *self = (struct LapineIngredient *)window;
    lapine_upgrade_row_destruct(self);
    mem_free(self);
}

void lapine_upgrade_row_drop(struct UIWindow *window, int x, int y, const struct DragInfo *drop)
{
    struct LapineIngredient *self = (struct LapineIngredient *)window;
    struct UIWindow *list = self->frame.base.parent;

    if (list)
        ui_window_drag_and_drop(list, x + self->frame.base.x, y + self->frame.base.y, drop);
}

void lapine_upgrade_row_on_draw(struct UIWindow *window)
{
    struct LapineIngredient *self = (struct LapineIngredient *)window;
    struct String cut = {0};

    ui_window_clear_canvas(&self->frame.base, COLOR_ROW_BG);
    if (self->selected == 1)
        ui_window_draw_bitmap(&self->frame.base, 0x20, 0, res_mgr_get(g_resMgr(), LAPINE_BMP("list_selected_item.bmp")), 0);
    ui_window_draw_item_icon(&self->frame.base, 4, 3, &self->item, 0);
    ui_window_fit_text(&self->frame.base, &cut, 100, string_cstr(&self->item.name), 0, 0xc, 0);
    ui_window_draw_shadow_text(&self->frame.base, 0x24, 9, string_cstr(&cut), 0, 0, 0xe0e0e0, 0, 0xc, 0);
    string_free(&cut);
}

void lapine_upgrade_row_press(struct UIWindow *window, int x, int y)
{
    struct LapineIngredient *self = (struct LapineIngredient *)window;
    if ((unsigned)self->item.index > 0)
        game_mode_drag_item(game_current_mode(), LAPINE_DRAG_CANDIDATE, &self->item,
                            self->count, -1);
}

void lapine_upgrade_row_double_click(struct UIWindow *window, int x, int y)
{
    struct LapineIngredient *self = (struct LapineIngredient *)window;
    if ((unsigned)self->item.index > 0)
        ui_window_send(self->frame.base.parent, &self->frame.base, LAPINE_MSG_REMOVE, self->item.index, 0, 0, 0);
}

void lapine_upgrade_row_release(struct UIWindow *window, int x, int y)
{
    struct LapineIngredient *self = (struct LapineIngredient *)window;
    self->selected = 1;
    ui_window_invalidate_base(&self->frame.base);
    if (self->frame.base.parent)
        ui_window_send(self->frame.base.parent, &self->frame.base, UI_MSG_BUTTON, 0, 0, 0, 0);
}

void lapine_upgrade_row_show_info(struct UIWindow *window, int x, int y)
{
    struct LapineIngredient *self = (struct LapineIngredient *)window;
    if ((unsigned)self->item.index > 0) {
        struct UIWindow *desc = ui_window_open(ITEM_DESC_WND_ID);

        ui_window_send(desc, &self->frame.base, UI_MSG_SHOW_ITEM, (int)&self->item, 0, 0, 0);
    }
}
