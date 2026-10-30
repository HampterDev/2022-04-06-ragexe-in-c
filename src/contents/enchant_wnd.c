#include "session/state.h"
#include "ui/window_manager_state.h"
#include "contents/connection_api.h"
#include <stdlib.h>
#include <string.h>
#include "contents/client_res.h"
#include "contents/connection.h"
#include "contents/enchant_mgr.h"
#include "contents/enchant_ui.h"
#include "contents/enchant_wnd.h"
#include "contents/game_session.h"
#include "iteminfo/item_api.h"
#include "std/vec.h"
#include "std/std.h"
#include "std/tree.h"
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
#include "effects/effect.h"
#include "core/object_type_tables.h"
#include "core/msg_str.h"
#include "session/session_items.h"

#define ENCHANT_MSG_COMMAND         6
#define ENCHANT_MSG_TAB             0x16
#define INFO_CMD_ENCHANT            0xb0
#define WND_CMD_CLOSE               0xc1
#define ENCHANT_CMD_ITEM_PRESSED    0xc4
#define LIST_CMD_SELECTED           0x148
#define LIST_MSG_REFRESH            0x17
#define LIST_MSG_ADD                0x1f

#define WID_ENCHANT                 0x15b
#define PACKET_ENCHANT_CLOSE        0xba0

#define MSG_ENCHANT_SUCCESS         0xf11
#define MSG_ENCHANT_FAILURE         0xf12
#define MSG_TOO_HEAVY               0xefd

#define INK_HELP    0x503ce6

#pragma pack(push, 1)
struct EnchantRequest {
    short id;
    unsigned __int64 table_id;
    short inventory_index;
};
struct EnchantPerfectRequest {
    short id;
    unsigned __int64 table_id;
    short inventory_index;
    int item_id;
};
struct EnchantUpgradeRequest {
    short id;
    unsigned __int64 table_id;
    short inventory_index;
    short slot;
};
#pragma pack(pop)
SIZE_ASSERT(EnchantRequest, 0xc);
SIZE_ASSERT(EnchantPerfectRequest, 0x10);
SIZE_ASSERT(EnchantUpgradeRequest, 0xe);

static void *bitmap(const char *name)
{
    return res_mgr_get(g_resMgr(), name);
}

static void send_packet(int id, const void *packet)
{
    int len = rag_connection_get_packet_length(rag_connection_instance(), id);
    rag_connection_send_packet(rag_connection_instance(), len, packet);
}

void enchant_wnd_destruct(struct UIEnchantWnd *self)
{
    short packet = PACKET_ENCHANT_CLOSE;

    self->frame.base.ops = &enchant_wnd_ops;
    send_packet(PACKET_ENCHANT_CLOSE, &packet);
    tree_free(&self->pages, NULL);
    ui_frame_wnd_destruct(&self->frame);
}

void enchant_wnd_delete(struct UIWindow *window)
{
    struct UIEnchantWnd *self = (struct UIEnchantWnd *)window;
    enchant_wnd_destruct(self);
    mem_free(self);
}

struct UIEnchantWnd *enchant_wnd_construct(struct UIEnchantWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &enchant_wnd_ops;
    self->targets = 0;
    self->tabs = 0;
    tree_init(&self->pages);
    self->help = 0;
    self->table = 0;
    self->table_id = 0;
    return self;
}

struct UIWindow *enchant_wnd_new(void)
{
    return (struct UIWindow *)enchant_wnd_construct(mem_alloc(sizeof(struct UIEnchantWnd)));
}

static struct UIEnchantTab *new_tab(void)
{
    struct UIEnchantTab *tab = mem_alloc(sizeof *tab);

    if (tab) {
        ui_window_construct(&tab->base, 0);
        tab->base.ops = &enchant_tab_ops;
        string_init(&tab->label);
        tab->selected = 1;
    }
    return tab;
}

static struct UIEnchantInfo *new_page(void)
{
    struct UIEnchantInfo *page = mem_alloc(sizeof *page);

    if (page) {
        ui_window_construct(&page->base, 0);
        page->base.ops = &enchant_info_ops;
        page->target = 0;
        tree_init(&page->sockets);
        tree_init(&page->socket_labels);
        page->materials.first = page->materials.last = page->materials.end = 0;
        page->button = 0;
        page->results = 0;
        page->zeny = 0;
        page->enchant_slot = -1;
    }
    return page;
}

static const int tab_label[PAGES] = { 0xef9, 0xefa, 0xefb, 0xefc };

void enchant_wnd_create(struct UIWindow *window, int width, int height)
{
    struct UIEnchantWnd *self = (struct UIEnchantWnd *)window;
    struct UIEnchantList *targets;
    struct UIEnchantGroup *tabs;
    struct UITagTextViewer *help;
    int page, x = 7;

     (void)width; (void)height;
    ui_window_add_close_button(&self->frame.base, "", 0);

    targets = mem_alloc(sizeof *targets);
    if (targets) {
        enchant_listbox_construct(targets);
        targets->base.ops = &enchant_target_list_ops;
        targets->row_height = 0x32;
    }
    self->targets = targets;
    ui_window_create(&self->targets->base, 0xdc, 0x190);
    ui_window_move_base(&self->targets->base, 0, 0x11);
    ui_window_add_child(&self->frame.base, &self->targets->base);

    tabs = mem_alloc(sizeof *tabs);
    if (tabs) {
        ui_window_construct(&tabs->base, 0);
        tabs->base.ops = &enchant_group_ops;
        tabs->locked = 0;
        tabs->page = -1;
    }
    self->tabs = tabs;
    ui_window_create(&self->tabs->base, 0x104, 0x17);
    ui_window_move_base(&self->tabs->base, 0xf0, 0x11);
    ui_window_add_child(&self->frame.base, &self->tabs->base);

    for (page = 0; page < PAGES; page++, x += 0x3f) {
        struct UIEnchantTab *tab = new_tab();
        struct UIEnchantInfo *info;
        const char *label;

        ui_window_create(&tab->base, 0x38, 0x17);
        ui_window_move_base(&tab->base, x, 0);
        tab->base.id = page;
        label = msg_str(tab_label[page]);
        string_assign_cstr(&tab->label, label);
        ui_window_invalidate_base(&tab->base);
        ui_window_add_child(&self->tabs->base, &tab->base);

        info = new_page();
        PAGE(self, page) = info;
        PAGE(self, page)->base.id = page;
        ui_window_create(&PAGE(self, page)->base, 0x1d5, 0x179);
        ui_window_move_base((&PAGE(self, page)->base), 0xdc, 0x28);
        ui_window_add_child(&self->frame.base, &PAGE(self, page)->base);
    }
    enchant_group_select(self->tabs, PAGE_ENCHANT);

    help = ui_tag_text_viewer_create(0x2a3, 0x2d);
    self->help = help;
    self->help->style.colour = INK_HELP;
    self->help->style.font_height = 0xc;
    self->help->style.bold = 1;
    self->help->bg_argb = ENCHANT_COLORKEY;
    ui_window_invalidate_base(&self->help->base);
    self->help->line_spacing = 2;
    self->help->margin_x = 0xa;
    self->help->margin_y = 3;
    ui_window_move_base(&self->help->base, 0xa, 0x1a6);
    ui_window_add_child(&self->frame.base, &self->help->base);
}

void enchant_wnd_draw(struct UIWindow *window)
{
    struct UIEnchantWnd *self = (struct UIEnchantWnd *)window;
    ui_window_draw_bitmap(&self->frame.base, 0, 0, bitmap(ENCHANT_BMP("bg_enchantUI.bmp")), 0);
}

int enchant_wnd_shown_page(struct UIEnchantWnd *self)
{
    struct TreeNode *node;

    for (node = tree_first(&self->pages); node != self->pages.head; node = tree_next(node))
        if (TREE_VALUE(node, struct EnchantPage)->info->base.visible)
            return TREE_VALUE(node, struct EnchantPage)->page;
    return NO_PAGE;
}

void enchant_wnd_lock(struct UIEnchantWnd *self)
{
    struct TreeNode *node;

    self->targets->locked = 1;
    ui_window_invalidate_base(&self->targets->base);
    self->tabs->locked = 1;
    ui_window_invalidate_base(&self->tabs->base);
    for (node = tree_first(&self->pages); node != self->pages.head; node = tree_next(node))
        enchant_info_lock(TREE_VALUE(node, struct EnchantPage)->info);
    ui_window_invalidate_base(&self->frame.base);
}

void enchant_wnd_open(struct UIEnchantWnd *self, uint64_t table_id)
{
    const int *target;

    self->table = enchant_mgr_find_list(enchant_mgr_instance(), table_id);
    if (!self->table) {
        ui_window_mgr_post_quit1(&g_windowMgr, WID_ENCHANT);
        return;
    }
    self->table_id = table_id;
    ui_tag_text_viewer_append_text(self->help, string_cstr(&self->table->caution));

    for (target = self->table->target_items.first; target != self->table->target_items.last; target++) {
        struct Vec indices;
        const int *index;

        session_inventory_indices(&g_session1, &indices, *target, 1);
        for (index = indices.first; index != indices.last; index++) {
            struct ItemInfo item;
            const struct EnchantList *table;

            if (session_is_in_switch_sets(&g_session1, *index) == 1)
                continue;
            session_get_inventory_item(&g_session1, &item, *index);
            table = self->table;
            if ((unsigned)item.refine >= table->min_refine
                && (unsigned)(int)item.grade >= table->min_grade
                && (table->approve_random_option || item.option_count <= 0))
                ui_window_send(&self->targets->base, &self->frame.base, LIST_MSG_ADD, (int)&item, 0, 0, 0);
            item_info_destruct(&item);
        }
        vec_free(&indices);
    }
}

static void play_effects(int first, int second)
{
    Vec3 at = { 0.0f, 0.0f, 0.0f };
    struct Effect *effect;

    effect_mgr_spawn(g_effectMgr, g_session1.ownAid, first, at, 0, -1, 0, 0);
    effect = effect_mgr_spawn(g_effectMgr, g_session1.ownAid, second, at, 0, -1, 0, 0);
    if (effect)
        effect->ops->set_start_delay(effect, 0xaf0);
}

static const int page_effect[PAGES] = { 0x907, 0x907, 0x90a, 0x90d };

static char apply_success(struct UIEnchantWnd *self, int item_id)
{
    struct ItemInfo item;
    int page = enchant_wnd_shown_page(self);

    if (page == NO_PAGE)
        return 0;
    session_get_inventory_item(&g_session1, &item, PAGE(self, page)->target->item.index);
    if ((unsigned)item.index == 0) {
        item_info_destruct(&item);
        return 0;
    }
    if (page == PAGE_RESET) {
        const int *slot;
        for (slot = self->table->slot_order.first; slot != self->table->slot_order.last; slot++)
            if ((unsigned)*slot <= 4)
                item.slot[*slot] = 0;
    } else {
        unsigned slot = (unsigned)PAGE(self, page)->enchant_slot;
        if (slot > 3) {
            item_info_destruct(&item);
            return 1;
        }
        item.slot[slot] = item_id;
    }
    session_inventory_update(&g_session1, &item, 1);
    ui_window_send(&self->targets->base, &self->frame.base, LIST_MSG_REFRESH, 0, 0, 0, 0);
    enchant_listbox_select(self->targets, (unsigned)self->targets->selected);
    enchant_wnd_lock(self);
    if ((unsigned)page < PAGES)
        play_effects(page_effect[page], page_effect[page] + 1);
    item_info_destruct(&item);
    return 1;
}

void enchant_wnd_result(struct UIEnchantWnd *self, int result, int item_id)
{
    if (result == MSG_ENCHANT_SUCCESS) {
        if (!apply_success(self, item_id))
            return;
    } else if (result == MSG_ENCHANT_FAILURE) {
        int page = enchant_wnd_shown_page(self);

        if (page == NO_PAGE)
            return;
        if ((unsigned)page < PAGES)
            play_effects(page_effect[page], page_effect[page] + 2);
    } else {
        ui_window_mgr_send_msg_(&g_windowMgr, 1, (int)msg_str(result), 0xff, 0, 0);
    }
    ui_window_send(&self->frame.base, &self->frame.base, ENCHANT_MSG_COMMAND, WND_CMD_CLOSE, 0, 0, 0);
}

static void request_enchant(struct UIEnchantWnd *self, struct UIWindow *sender)
{
    if (g_session1.weight * 100 / g_session1.maxWeight >= 70) {
        ui_window_mgr_send_msg_(&g_windowMgr, 1, (int)msg_str(MSG_TOO_HEAVY), 0xff, 0, 0);
        return;
    }
    if (sender == &PAGE(self, PAGE_ENCHANT)->base) {
        struct EnchantRequest req;
        req.id = 0xb9b;
        req.table_id = self->table_id;
        req.inventory_index = (short)PAGE(self, PAGE_ENCHANT)->target->item.index;
        send_packet(req.id, &req);
    } else if (sender == &PAGE(self, PAGE_PERFECT)->base) {
        struct EnchantPerfectRequest req;
        req.id = 0xb9c;
        req.table_id = self->table_id;
        req.inventory_index = (short)PAGE(self, PAGE_PERFECT)->target->item.index;
        req.item_id = enchant_listbox_selected_item_id(PAGE(self, PAGE_PERFECT)->results);
        send_packet(req.id, &req);
    } else if (sender == &PAGE(self, PAGE_UPGRADE)->base) {
        struct EnchantUpgradeRequest req;
        req.id = 0xb9d;
        req.table_id = self->table_id;
        req.inventory_index = (short)PAGE(self, PAGE_UPGRADE)->target->item.index;
        req.slot = (short)PAGE(self, PAGE_UPGRADE)->enchant_slot;
        send_packet(req.id, &req);
    } else if (sender == &PAGE(self, PAGE_RESET)->base) {
        struct EnchantRequest req;
        req.id = 0xb9e;
        req.table_id = self->table_id;
        req.inventory_index = (short)PAGE(self, PAGE_RESET)->target->item.index;
        send_packet(req.id, &req);
    }
    enchant_wnd_lock(self);
}

int enchant_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                    int wparam, int lparam, int extra1, int extra2)
{
    struct UIEnchantWnd *self = (struct UIEnchantWnd *)window;
    struct TreeNode *node;

    if (msg == ENCHANT_MSG_TAB) {
        for (node = tree_first(&self->pages); node != self->pages.head; node = tree_next(node))
            ui_window_set_show_base(&TREE_VALUE(node, struct EnchantPage)->info->base,
                               TREE_VALUE(node, struct EnchantPage)->page == wparam);
        ui_window_invalidate_base(&self->frame.base);
        return 0;
    }
    if (msg != ENCHANT_MSG_COMMAND)
        return ui_window_send_msg_base(&self->frame.base, sender, msg, wparam, lparam, extra1, extra2);

    switch (wparam) {
    case INFO_CMD_ENCHANT:
        request_enchant(self, sender);
        break;
    case WND_CMD_CLOSE:
        ui_window_mgr_post_quit1(&g_windowMgr, WID_ENCHANT);
        break;
    case ENCHANT_CMD_ITEM_PRESSED:
        if (sender == &PAGE(self, PAGE_UPGRADE)->base)
            enchant_wnd_pick_socket(self, (unsigned)lparam, extra1);
        break;
    case LIST_CMD_SELECTED:
        if (sender == &self->targets->base)
            enchant_wnd_select_target(self, (const struct ItemInfo *)lparam);
        else if (sender == &PAGE(self, PAGE_PERFECT)->base)
            enchant_wnd_pick_perfect(self, (const struct ItemInfo *)lparam);
        else if (sender == &PAGE(self, PAGE_UPGRADE)->base)
            enchant_wnd_pick_upgrade(self, (const struct ItemInfo *)lparam);
        break;
    }
    return 0;
}
