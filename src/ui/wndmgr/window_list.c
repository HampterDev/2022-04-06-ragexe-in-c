#include "render/renderer.h"
#include "replay/replay.h"
#include "navi/navi_manager.h"
#include "ui/guild_windows.h"
#include "ui/window_manager_state.h"
#include "ui/window_access.h"
#include "ui/wndmgr/wndmgr.h"
#include "ui/draw.h"
#include "render/device.h"
#include "actor/types.h"
#include "login/merc_ai_types.h"
#include "game/drag_info.h"
#include "ui/chat/chat_window_parts.h"
#include "ui/chat/sub_chat.h"
#include "ui/chat/sub_chat_map.h"
#include "session/session_party.h"
#include "ui/quests/navigation.h"
#include "ui/shops/roulette.h"
#include "resources/bitmap_types.h"

void ui_window_mgr_add_window(struct UIWindowMgr *self, struct UIWindow *wnd)
{
    list_push_back(&self->windows, &wnd, sizeof wnd);
}

void ui_window_mgr_add_to_window_list(struct UIWindowMgr *self, struct UIWindow *wnd)
{
    list_push_front(&self->windows, &wnd, sizeof wnd);
}

void ui_window_mgr_send_key_down(struct UIWindowMgr *self)
{
    struct UIWindow *target = self->choose_wnd;
    struct ListNode *node;

    if (target == NULL)
        target = self->say_dialog_wnd;
    if (target == NULL)
        target = self->combo_box_wnd;
    if (target == NULL)
        target = self->combo_box_idwnd;
    if (target == NULL)
        target = self->choose_warp_wnd;
    if (target == NULL)
        target = self->make_target_list_wnd;
    if (target == NULL)
        target = self->focus_edit;
    if (target != NULL) {
        ui_window_send_msg(target, NULL, 0x13, 0, 0, 0, 0);
        return;
    }
    for (node = self->windows.head->next; node != self->windows.head; node = node->next) {
        struct UIWindow *wnd = *LIST_VALUE(node, struct UIWindow *);

        if (wnd->ops->is_frame)
            ui_window_send_msg(wnd, NULL, 0x13, 0, 0, 0, 0);
    }
}

void ui_window_mgr_show_shop_titles(struct UIWindowMgr *self, BYTE show)
{
    struct ListNode *node;

    for (node = self->windows.head->next; node != self->windows.head; node = node->next) {
        struct UIWindow *wnd = *LIST_VALUE(node, struct UIWindow *);

        if (wnd->id != 0xbd)
            continue;
        if (show == 1)
            ui_window_set_show(wnd, g_windowMgr.show_shop_titles ? 1 : 0);
        else
            ui_window_set_show(wnd, 0);
    }
}

void ui_window_mgr_drop_window_refs(struct UIWindowMgr *self, struct UIWindow *wnd)
{
    if (self->capture && (self->capture == wnd || ui_window_has_ancestor(self->capture, wnd)))
        self->capture = NULL;
    if (self->focus_edit && (self->focus_edit == wnd || ui_window_has_ancestor(self->focus_edit, wnd)))
        self->focus_edit = NULL;
    if (self->modal_wnd && (self->modal_wnd == wnd || ui_window_has_ancestor(self->modal_wnd, wnd)))
        self->modal_wnd = NULL;
    if (self->mouse_wnd && (self->mouse_wnd == wnd || ui_window_has_ancestor(self->mouse_wnd, wnd)))
        self->mouse_wnd = NULL;
}

void window_mgr_clear_backdrop(struct UIWindowMgr *self)
{
    if (self->backdrop) {
        surface_destroy(self->backdrop);
        self->backdrop = NULL;
    }
}

int ui_window_mgr_get_window_ids(struct UIWindowMgr *self, int *ids)
{
    struct ListNode *node;

    if (ids == NULL)
        return 0;
    for (node = self->windows.head->next; node != self->windows.head; node = node->next)
        *ids++ = (*LIST_VALUE(node, struct UIWindow *))->id;
    return self->windows.size;
}

void ui_window_mgr_hide_chat_wnd(struct UIWindowMgr *self)
{
    if (self->new_chat_wnd)
        ui_window_set_show(self->new_chat_wnd, 0);
}

BYTE actor_homun_ai_running(void *actor)
{
    const struct MercenaryAI *ai = ((const struct GameActor *)actor)->homun_ai;

    return ai != NULL && ai->L != NULL;
}

struct UIWindow *window_mgr_guild_window(struct UIWindowMgr *self)
{
    if (self->guild_info_manage_wnd)
        return self->guild_info_manage_wnd;
    if (self->guild_member_manage_wnd)
        return self->guild_member_manage_wnd;
    if (self->guild_position_manage_wnd)
        return self->guild_position_manage_wnd;
    if (self->guild_skill_wnd)
        return self->guild_skill_wnd;
    if (self->guild_banished_member_wnd)
        return self->guild_banished_member_wnd;
    if (self->guild_notice_wnd)
        return self->guild_notice_wnd;
    return self->guild_total_info_wnd;
}

struct UIWindow *ui_window_mgr_get_focus_edit(struct UIWindowMgr *self)
{
    return self->focus_edit;
}

struct UIWindow *ui_window_mgr_focus_root(struct UIWindowMgr *self)
{
    struct UIWindow *root;

    if (!self->focus_edit)
        return NULL;
    root = ui_window_get_ancestor(self->focus_edit);
    return root == self->guild_position_manage_wnd ? NULL : root;
}

struct UIWindow *ui_window_mgr_get_capture(struct UIWindowMgr *self)
{
    return self->capture;
}

void ui_window_mgr_focus_edit_lost(struct UIWindowMgr *self)
{
    if (self->focus_edit)
        ui_window_on_focus_lost(self->focus_edit);
}

void ui_window_mgr_focus_first_edit(struct UIWindowMgr *self)
{
    struct ListNode *node, *child;

    for (node = self->windows.head->next; node != self->windows.head; node = node->next) {
        struct UIWindow *wnd = *LIST_VALUE(node, struct UIWindow *);

        if (!wnd->visible)
            continue;
        for (child = wnd->children.head->next; child != wnd->children.head; child = child->next) {
            struct UIWindow *edit = *LIST_VALUE(child, struct UIWindow *);

            if (ui_window_is_focusable(edit) && edit->x > 0 && edit->y > 0) {
                if (self->focus_edit)
                    ui_window_on_focus_lost(self->focus_edit);
                self->focus_edit = edit;
                ui_window_on_focus(edit);
                return;
            }
        }
    }
}

void ui_window_mgr_edit_focus_to_chat_wnd(struct UIWindowMgr *self)
{
    struct UIWindow *edit;

    if (self->new_chat_wnd == NULL)
        return;
    edit = ui_chat_window_edit(self->new_chat_wnd);
    if (self->focus_edit)
        ui_window_on_focus_lost(self->focus_edit);
    self->focus_edit = edit;
    if (edit)
        ui_edit_ctrl_on_focus(edit);
}

struct UIWindow *ui_window_mgr_find_by_serial(struct UIWindowMgr *self, int serial)
{
    struct ListNode *node;

    for (node = self->windows.head->next; node != self->windows.head; node = node->next) {
        struct UIWindow *wnd = *LIST_VALUE(node, struct UIWindow *), *found;

        if (wnd->serial == serial)
            return wnd;
        found = ui_window_find_child_by_serial(wnd, serial);
        if (found)
            return found;
    }
    return NULL;
}

struct UIWindow *ui_window_mgr_window_at(struct UIWindowMgr *self, int x, int y)
{
    struct ListNode *node = self->windows.head;
    struct UIWindow *hit, *old;

    while (node != self->windows.head->next) {
        node = node->prev;
        hit = ui_window_hit_test(*LIST_VALUE(node, struct UIWindow *), x, y);
        if (hit && hit->visible) {
            self->hover_prev = self->hover_serial;
            self->hover_serial = hit->serial;
            if (self->hover_prev != self->hover_serial) {
                ui_window_on_hover_enter(hit);
                old = ui_window_mgr_find_by_serial(self, self->hover_prev);
                if (old)
                    ui_window_on_hover_leave(old);
            }
            return hit;
        }
    }
    self->hover_prev = self->hover_serial;
    self->hover_serial = 0;
    if (self->hover_prev) {
        old = ui_window_mgr_find_by_serial(self, self->hover_prev);
        if (old)
            ui_window_on_hover_leave(old);
    }
    return NULL;
}

BYTE ui_window_mgr_drop_on(struct UIWindowMgr *self, int x, int y, struct DragInfo *drag)
{
    struct UIWindow *wnd = ui_window_mgr_window_at(self, x, y), *w;
    int ox = 0, oy = 0;

    if (wnd == NULL)
        return 0;
    for (w = wnd; w; w = w->parent) {
        ox += w->x;
        oy += w->y;
    }
    x -= ox;
    y -= oy;
    ui_window_record_drag(wnd, x, y, drag, -1);
    drag->drop_window = wnd;
    ui_window_drag_and_drop(wnd, x, y, drag);
    return 1;
}

void ui_window_mgr_draw_picture(struct UIWindowMgr *self, int x, int y, struct BitmapRes *bitmap)
{
    struct DcBitmap *canvas;
    int w, h;

    (void)self;
    if (bitmap == NULL)
        return;
    w = bitmap->width;
    h = bitmap->height;
    canvas = dc_bitmap_create(w, h);
    if (!canvas) return;
    dc_bitmap_update(canvas, 0, 0, w, h, bitmap->pixels, 1);
    dc_bitmap_draw(canvas, x, y, w, h, 0xffffffff);
    dc_bitmap_destroy(canvas);
}

void ui_window_mgr_close_sub_chat(struct UIWindowMgr *self, int key)
{
    struct SubChatWnd *sub = tree_find(&self->sub_chat_wnds, &key, tree_less_int);

    if (!sub)
        return;
    ui_window_mgr_post_quit2(self, sub->wnd);
    tree_erase(&self->sub_chat_wnds, TREE_NODE_OF(sub), NULL);
}

void ui_window_mgr_remove_snap_rect(struct UIWindowMgr *self, struct UIWindow *wnd)
{
    if (!g_session1.windowSnap)
        return;
    tree_erase_key(&self->snap_rects, &wnd, tree_less_uint, NULL);
}

struct WindowInfo *ui_window_mgr_get_closed_rect(struct UIWindowMgr *self, int window_id)
{
    struct WmClosedRect *closed = tree_find(&self->closed_rects, &window_id, tree_less_int);

    return closed ? closed->rect : NULL;
}

static void move_to_top(struct UIWindowMgr *self, struct ListNode *node)
{
    struct UIWindow *wnd = *LIST_VALUE(node, struct UIWindow *);

    list_erase(&self->windows, node, NULL);
    list_push_back(&self->windows, &wnd, sizeof wnd);
}

static struct ListNode *find_node(struct UIWindowMgr *self, const struct UIWindow *wnd)
{
    struct ListNode *node;

    for (node = self->windows.head->next; node != self->windows.head; node = node->next)
        if (*LIST_VALUE(node, struct UIWindow *) == wnd)
            return node;
    return NULL;
}

static void raise_companion(struct UIWindowMgr *self, const struct UIWindow *wnd)
{
    struct ListNode *node;
    int companion = wnd->id == 0x52 ? 0xfa : wnd->id == 0xc ? 0xea : -1;

    if (companion == -1)
        return;
    for (node = self->windows.head->next; node != self->windows.head; node = node->next) {
        if ((*LIST_VALUE(node, struct UIWindow *))->id == companion) {
            move_to_top(self, node);
            return;
        }
    }
}

void ui_window_mgr_raise_window(struct UIWindowMgr *self, struct UIWindow *wnd)
{
    struct ListNode *node;

    if (wnd == NULL)
        return;
    if (self->kafra_move_map_service_info_wnd && self->navigation_v4_wnd != wnd)
        return;
    if (ui_window_mgr_get_window(self, 0x116) || ui_window_mgr_get_window(self, 4)
        || ui_window_mgr_get_window(self, 0xc7) || ui_window_mgr_get_window(self, 0xc8))
        return;
    raise_companion(self, wnd);
    node = find_node(self, wnd);
    if (node)
        move_to_top(self, node);
}

void ui_window_mgr_invalidate_all(struct UIWindowMgr *self)
{
    struct ListNode *node;

    for (node = self->windows.head->next; node != self->windows.head; node = node->next)
        ui_window_invalidate_children(*LIST_VALUE(node, struct UIWindow *));
}

void ui_window_mgr_invalidate_once_per_frame(struct UIWindowMgr *self)
{
    struct ListNode *node;

    if (self->invalidated)
        return;
    self->invalidated = 1;
    for (node = self->windows.head->next; node != self->windows.head; node = node->next) {
        struct UIWindow *wnd = *LIST_VALUE(node, struct UIWindow *);

        if (wnd->ops->refreshes_children)
            ui_window_invalidate_children(wnd);
    }
}

static void refresh(struct UIWindow *wnd)
{
    if (wnd)
        ui_window_send_msg(wnd, NULL, 0x3c, 0, 0, 0, 0);
}

void ui_window_mgr_refresh_clan_wnd(struct UIWindowMgr *self)
{
    refresh(self->clan_info_manage_wnd);
}

void session_refresh_merchant_mirror_item_wnd(struct UIWindowMgr *self)
{
    refresh(self->roullette_wnd);
}

void ui_window_mgr_allow_roulette_spin(struct UIWindowMgr *self)
{
    if (self->roullette_wnd)
        ui_roulette_allow_spin(self->roullette_wnd, 1);
}

void window_mgr_invalidate_guild_wnd(struct UIWindowMgr *self)
{
    refresh(self->guild_info_manage_wnd);
    refresh(self->guild_member_manage_wnd);
    refresh(self->guild_position_manage_wnd);
    refresh(self->guild_skill_wnd);
    refresh(self->guild_banished_member_wnd);
    refresh(self->guild_notice_wnd);
    refresh(self->guild_total_info_wnd);
}

static void send_to_text_window(struct UIWindowMgr *self, int msg)
{
    struct UIWindow *target = self->change_name_wnd;
    struct ListNode *node;

    if (target == NULL)
        target = self->book_wnd;
    if (target) {
        ui_window_send_msg(target, NULL, msg, 0, 0, 0, 0);
        return;
    }
    for (node = self->windows.head->next; node != self->windows.head; node = node->next) {
        struct UIWindow *wnd = *LIST_VALUE(node, struct UIWindow *);

        if (wnd->ops->is_frame)
            ui_window_send_msg(wnd, NULL, msg, 0, 0, 0, 0);
    }
}

void ui_window_mgr_send_key_left(struct UIWindowMgr *self)
{
    send_to_text_window(self, 0x14);
}

void ui_window_mgr_send_key_right(struct UIWindowMgr *self)
{
    send_to_text_window(self, 0x15);
}

BYTE ui_window_mgr_is_barter_open(struct UIWindowMgr *self)
{
    return self->bartermarket_wnd || self->barter_item_purchase_wnd || self->expanded_bartermarket_wnd
        || self->expanded_barter_item_purchase_wnd;
}

int ui_window_mgr_is_friend(struct UIWindowMgr *self, const char *name)
{
    (void)self;
    return session_has_friend_named(&g_session1, name) != 0;
}

BYTE ui_minimap_zoom_wnd_claims_mouse(struct UIWindow *self)
{
    (void)self;

    return 1;
}

struct UIWindow *ui_window_mgr_open_sub_chat(struct UIWindowMgr *self, int key, int x, int y, int width,
                                                    int height)
{
    struct SubChatWnd *sub;
    struct UIWindow *wnd;

    if (x == -1 && y == -1) {
        x = (key + 10) * 10;
        y = (key + 5) * 20;
    }
    if (width == 0 || width > g_renderer->width)
        width = 300;
    if (height == 0 || height > g_renderer->height)
        height = 0x5f;
    if (x > g_renderer->width)
        x = g_renderer->width - width;
    if (y > g_renderer->height)
        y = 100;
    sub = tree_get(&self->sub_chat_wnds, &key, sizeof key, sizeof *sub, tree_less_int);
    wnd = sub->wnd;
    if (wnd == NULL) {
        wnd = (struct UIWindow *)mem_alloc(sizeof(struct UISubChatHisWnd));
        wnd = wnd ? (struct UIWindow *)ui_sub_chat_his_wnd_construct((struct UISubChatHisWnd *)wnd) : NULL;
        ((struct UISubChatHisWnd *)wnd)->key = key;
        ui_window_create(wnd, width, height);
        ui_window_move(wnd, x, y);
        list_push_back(&self->windows, &wnd, sizeof wnd);
        sub->wnd = wnd;
    }
    ui_window_set_show(wnd, 1);
    return wnd;
}

BYTE actor_merc_ai_running(void *actor)
{
    const struct MercenaryAI *ai = ((const struct GameActor *)actor)->merc_ai;

    return ai != NULL && ai->L != NULL;
}

void ui_minimap_zoom_wnd_on_l_btn_down(struct UIWindow *self, int x, int y)
{
    (void)self;

    (void)x;
    (void)y;
}

void ui_window_mgr_add_to_name_waiting_list(struct UIWindowMgr *self, struct UIWindow *wnd)
{
    struct ListNode *node;

    for (node = self->name_waiting.head->next; node != self->name_waiting.head; node = node->next)
        if (*LIST_VALUE(node, struct UIWindow *) == wnd)
            return;
    list_push_back(&self->name_waiting, &wnd, sizeof wnd);
}

void ui_window_mgr_remove_from_name_waiting_list(struct UIWindowMgr *self, struct UIWindow *wnd)
{
    struct ListNode *node;

    for (node = self->name_waiting.head->next; node != self->name_waiting.head; node = node->next) {
        if (*LIST_VALUE(node, struct UIWindow *) == wnd) {
            list_erase(&self->name_waiting, node, NULL);
            return;
        }
    }
}

void ui_window_mgr_tick(struct UIWindowMgr *self)
{
    struct ListNode *node;

    for (node = self->name_waiting.head->next; node != self->name_waiting.head; node = node->next)
        ui_window_invalidate(*LIST_VALUE(node, struct UIWindow *));
    while (self->name_waiting.head->next != self->name_waiting.head)
        list_erase(&self->name_waiting, self->name_waiting.head->next, NULL);
}

void ui_window_mgr_release_capture(struct UIWindowMgr *self)
{
    self->capture = NULL;
}

void ui_window_mgr_no_op(struct UIWindowMgr *self)
{
    (void)self;
}

void ui_window_mgr_refresh_item_windows(struct UIWindowMgr *self)
{
    struct UIWindow *wnd;

    if (self->item_wnd)
        ui_window_send_msg(self->item_wnd, NULL, 0x17, 0, 0, 0, 0);
    if (self->short_cut_wnd)
        ui_window_send_msg(self->short_cut_wnd, NULL, 0x17, 0, 0, 0, 0);
    wnd = ui_window_mgr_get_window(self, 0x9f);
    if (wnd)
        ui_window_send_msg(wnd, NULL, 0x17, 0, 0, 0, 0);
    if (self->bartermarket_wnd)
        ui_window_send_msg(self->bartermarket_wnd, NULL, 0x17, 0, 0, 0, 0);
}

void ui_window_mgr_render(struct UIWindowMgr *self)
{
    self->invalidated = 0;
    if (self->backdrop)
        surface_stretch(self->backdrop, (g_3dDevice.width - self->screen_width) / 2,
                        (g_3dDevice.height - self->screen_height) / 2, self->screen_width, self->screen_height);
}

void ui_window_mgr_set_backdrop(struct UIWindowMgr *self, struct BitmapRes *bitmap)
{
    if (bitmap == NULL) {
        if (self->backdrop) {
            surface_destroy(self->backdrop);
            self->backdrop = NULL;
        }
        return;
    }
    if (self->backdrop == NULL) {
        self->backdrop = c3d_device_new_wallpaper_surface(&g_3dDevice, bitmap->width, bitmap->height);
        if (self->backdrop == NULL)
            return;
    }
    surface_upload(self->backdrop, 0, 0, bitmap->width, bitmap->height, bitmap->pixels, 0, 0);
}

void ui_window_mgr_replay_close_window(struct UIWindowMgr *self, int window_id)
{
    if (window_id != 0xfb)
        ui_window_mgr_post_quit1(self, window_id);
}

static void give_focus(struct UIWindowMgr *self, struct UIWindow *edit)
{
    if (self->focus_edit)
        ui_window_on_focus_lost(self->focus_edit);
    self->focus_edit = edit;
    if (edit)
        ui_window_on_focus(edit);
}

void ui_window_mgr_set_focus_edit(struct UIWindowMgr *self, struct UIWindow *edit)
{
    give_focus(self, edit);
}

void ui_window_mgr_set_focus_edit_first(struct UIWindowMgr *self)
{
    struct ListNode *head, *node;

    if (self->focus_edit) {
        head = self->focus_edit->parent->children.head;
        for (node = head->next; node != head; node = node->next) {
            struct UIWindow *wnd = *LIST_VALUE(node, struct UIWindow *);

            if (ui_window_is_focusable(wnd)) {
                give_focus(self, wnd);
                return;
            }
        }
    }
    give_focus(self, NULL);
}

void ui_window_mgr_set_focus_edit_last(struct UIWindowMgr *self)
{
    struct ListNode *head, *node;

    if (self->focus_edit) {
        head = self->focus_edit->parent->children.head;
        for (node = head->prev; node != head; node = node->prev) {
            struct UIWindow *wnd = *LIST_VALUE(node, struct UIWindow *);

            if (ui_window_is_focusable(wnd)) {
                give_focus(self, wnd);
                return;
            }
        }
    }
    give_focus(self, NULL);
}

static void step_focus(struct UIWindowMgr *self, struct UIWindow *edit)
{
    if (edit == NULL)
        give_focus(self, NULL);
    else if (ui_window_is_focusable(edit))
        give_focus(self, edit);
}

void ui_window_mgr_focus_next_edit(struct UIWindowMgr *self)
{
    if (self->focus_edit)
        step_focus(self, ui_window_next_focusable(self->focus_edit->parent, self->focus_edit));
}

void ui_window_mgr_focus_prev_edit(struct UIWindowMgr *self)
{
    if (self->focus_edit)
        step_focus(self, ui_window_prev_focusable(self->focus_edit->parent, self->focus_edit));
}

void ui_window_mgr_set_capture(struct UIWindowMgr *self, struct UIWindow *wnd)
{
    self->capture = wnd;
}

void ui_window_mgr_set_active_top_window(struct UIWindowMgr *self, struct UIWindow *wnd)
{
    while (wnd && wnd->parent)
        wnd = wnd->parent;
    self->active_top_wnd = wnd;
}

void ui_window_mgr_show_windows(struct UIWindowMgr *self, int window_id, BYTE show)
{
    struct ListNode *node;

    for (node = self->windows.head->next; node != self->windows.head; node = node->next) {
        struct UIWindow *wnd = *LIST_VALUE(node, struct UIWindow *);

        if (wnd->id == window_id)
            ui_window_set_show(wnd, show);
    }
}

void ui_window_mgr_set_shop_titles_shown(struct UIWindowMgr *self, BYTE show)
{
    self->show_shop_titles = show;
    ui_window_mgr_show_windows(self, 0xbd, show);
}

static void show_or_store(struct UIWindow *wnd, int show)
{
    if (show)
        ui_window_mgr_raise_window(&g_windowMgr, wnd);
    else
        ui_window_store_info(wnd);
}

BYTE ui_window_mgr_show_option_window(struct UIWindowMgr *self, int window_id, int show)
{
    struct UIWindow *wnd;

    switch (window_id) {
    case 0x1:
        if (g_replayPlaying && !replay_recorder_instance()->info.options[6])
            return 0;
        self->chat_wnd_show = show;
        if (self->new_chat_wnd == NULL)
            ui_window_open(0x1);
        ui_window_set_show(self->new_chat_wnd, show);
        show_or_store(self->new_chat_wnd, show);
        return 1;
    case 0xf:
        ui_window_open(0xf);
        if (self->item_drop_cnt_wnd->visible != show)
            ui_window_set_show(self->item_drop_cnt_wnd, show);
        if (show)
            ui_window_mgr_raise_window(&g_windowMgr, self->item_drop_cnt_wnd);
        return 0;
    case 0x24:
        if (g_replayPlaying && !replay_recorder_instance()->info.options[7])
            return 0;
        self->shortcut_info.show = show;
        ui_window_open(0x24);
        ui_window_set_show(self->short_cut_wnd, show);
        if (show)
            ui_window_mgr_raise_window(&g_windowMgr, self->short_cut_wnd);
        ui_window_open(0x9f);
        wnd = ui_window_find(0x9f);
        if (wnd) {
            ui_window_set_show(wnd, show);
            if (show)
                ui_window_mgr_raise_window(&g_windowMgr, wnd);
        }
        return 1;
    case 0x2e:
        self->skill_describe_show = show;
        if (self->skill_describe_wnd == NULL) {
            if (!show)
                return 1;
            ui_window_open(0x2e);
            if (self->skill_describe_wnd == NULL)
                return 1;
        }
        ui_window_set_show(self->skill_describe_wnd, show);
        show_or_store(self->skill_describe_wnd, show);
        return 1;
    case 0x111:
        wnd = ui_window_mgr_get_window(self, 0x111);
        ui_window_show(wnd, show);
        return 1;
    case 0x114:
        self->round_box_tooltip_show = show;
        if (self->round_box_tooltip_wnd == NULL) {
            if (!show)
                return 1;
            ui_window_open(0x114);
            if (self->round_box_tooltip_wnd == NULL)
                return 1;
        }
        ui_window_set_show(self->round_box_tooltip_wnd, show);
        return 1;
    }
    return 0;
}

void ui_window_mgr_send_key_up(struct UIWindowMgr *self)
{
    struct UIWindow *const dialogs[] = {
        self->choose_wnd, self->say_dialog_wnd, self->combo_box_wnd, self->combo_box_idwnd,
        self->choose_warp_wnd, self->make_target_list_wnd, self->focus_edit,
    };
    struct ListNode *node;
    size_t i;

    for (i = 0; i < sizeof dialogs / sizeof dialogs[0]; i++) {
        if (dialogs[i]) {
            ui_window_send_msg(dialogs[i], NULL, 0x12, 0, 0, 0, 0);
            return;
        }
    }
    for (node = self->windows.head->next; node != self->windows.head; node = node->next) {
        struct UIWindow *wnd = *LIST_VALUE(node, struct UIWindow *);

        if (wnd->ops->is_frame)
            ui_window_send_msg(wnd, NULL, 0x12, 0, 0, 0, 0);
    }
}

void ui_window_mgr_navigation_help(struct UIWindowMgr *self)
{
    struct UIWindow *nav = self->navigation_v4_wnd;

    if (nav && !g_navi.windowCollapsed && ui_navigation_v4_wnd_minimize_checked((struct UINavigationV4Wnd *)nav))
        ui_window_send_msg(self->navigation_v4_wnd, NULL, UI_MSG_COMMAND, UI_ID_HELP, 0, 0, 0);
}

int ui_scale_x640(int v)
{
    return g_renderer->width * v / 640;
}

int ui_center_x640(int x)
{
    return (g_renderer->width - 640) / 2 + x;
}

int ui_center_y480(int y)
{
    return (g_renderer->height - 480) / 2 + y;
}
