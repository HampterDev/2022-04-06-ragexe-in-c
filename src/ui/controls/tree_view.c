#include "ui/controls/edit.h"
#include "ui/controls/scroll.h"
#include "ui/controls/listbox.h"
#include "ui/window_access.h"
#include "render/rp_pool.h"
#include "core/object_type_tables.h"
#include "world/mode_manager.h"
#include "ui/window_mgr.h"
#include "ui/game.h"
#include "ui/ui_items.h"
#include "ui/controls/controls.h"
#include "ui/ui_std.h"
#include <mmsystem.h>
#include <stdlib.h>
#include <string.h>
#include "core/skin_path.h"
#include "diag/perf.h"

struct UITreeNode {
    int id;
    int expanded;
    int icon;
    char name[0x15e];
    BYTE draggable;
    BYTE drop_inside;
    int key;
    struct UITreeNode *parent;
    struct UITVBitmapButton *expand_btn;
    struct UITVBitmapButton *collapse_btn;
    struct UITVTextButton *label;
    struct List children;
    struct Vec icons;
};
OFFSET_ASSERT(UITreeNode, draggable, 0x16a);
OFFSET_ASSERT(UITreeNode, parent, 0x170);
OFFSET_ASSERT(UITreeNode, children, 0x180);
SIZE_ASSERT(UITreeNode, 0x194);
#define TREE_NODE(it) (*LIST_VALUE(it, struct UITreeNode *))

struct UITreeView {
    struct UIWindow base;
    struct UIScrollBar *vbar;
    int has_vscroll;
    struct UITreeNode *root;
    struct UITreeNode *selected;
    struct UITreeNode *dragged;
    struct UITreeNode *drop;
    struct UITreeNode *editing;
    struct UIWindow *edit;
    int indent;
    int row_height;
    int row_count;
    int top;
    BYTE hover;
    COLORREF drop_colour;
};
OFFSET_ASSERT(UITreeView, root, 0x84);
OFFSET_ASSERT(UITreeView, drop_colour, 0xb0);
SIZE_ASSERT(UITreeView, 0xb4);
const struct UIWindowOps ui_tree_view_ops = {
    .destroy = ui_tree_view_destroy,
    .resize = ui_tree_view_resize,
    .is_frame = 0,
    .refreshes_children = 0,
    .move = ui_window_move_base,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_window_store_info_base,
    .save_pos = ui_window_save_pos_base,
    .move_delta = ui_window_move_delta_base,
    .set_show = ui_window_set_show_base,
    .on_create = ui_tree_view_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_tree_view_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_tree_view_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_tree_view_on_mouse_move,
    .on_mouse_hover = ui_window_on_mouse_hover_base,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_tree_view_on_l_btn_up,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_tree_view_on_wheel,
    .snap = ui_window_snap_base,
    .send_msg = ui_tree_view_send_msg,
    .invalidate = ui_window_invalidate_base,
    .render = ui_window_render_base,
    .has_custom_backdrop = 0,
    .draw_backdrop = ui_window_draw_backdrop_base,
    .get_records = ui_window_get_records_base,
    .type = UI_TYPE_WINDOW,
    .on_hover_enter = ui_window_on_hover_enter_base,
    .on_hover_leave = ui_window_on_hover_leave_base,
    .make_quad = ui_window_make_quad_base,
    .hit_test = ui_window_hit_test_base,
    .render_overlay = ui_window_render_overlay_base,
};

#define OFF_SCREEN (-100)
#define BAR_INSET 10
#define MAX_NAME 0x169

enum {
    TV_MSG_SELECT = 0x61,
    TV_MSG_TOGGLE = 0x62,
    TV_MSG_ACTIVATE = 0x63,
    TV_MSG_TOGGLE_BTN = 0x64,
    TV_MSG_DRAG = 0x6b,
    TV_MSG_DRAG_OVER = 0x6c,
    TV_MSG_DROP = 0x6d,
    TV_MSG_RENAME = 0x6e,
};

static void move_off(void *wnd)
{
    ui_window_move(wnd, OFF_SCREEN, OFF_SCREEN);
}

static void node_delete(struct UITreeNode *node)
{
    string_vec_free(&node->icons);
    list_free(&node->children, NULL);
    mem_free(node);
}

void ui_tree_view_hide_rows(struct UITreeView *self, struct List *list)
{
    struct ListNode *head, *it;

    if ((int)list->size <= 0)
        return;
    head = list->head;
    for (it = head->next; it != head; it = it->next) {
        struct UITreeNode *n = TREE_NODE(it);

        if (n->collapse_btn)
            move_off(n->collapse_btn);
        if (n->expand_btn)
            move_off(n->expand_btn);
        move_off(n->label);
        if ((int)TREE_NODE(it)->children.size > 0)
            ui_tree_view_hide_rows(self, &TREE_NODE(it)->children);
    }
}

BYTE ui_tree_view_clear_children(struct UITreeView *self, struct UITreeNode *node)
{
    struct ListNode *it;
    int left;

    if (!node)
        return 0;
    left = (int)node->children.size;
    if (left <= 0)
        return 0;
    it = node->children.head->next;
    if (it == node->children.head)
        return 0;
    for (;;) {
        struct UITreeNode *child = TREE_NODE(it);

        if (child->children.size)
            ui_tree_view_clear_children(self, child);
        if (child->collapse_btn) {
            move_off(child->collapse_btn);
            TREE_NODE(it)->collapse_btn = NULL;
            child = TREE_NODE(it);
        }
        if (child->expand_btn) {
            move_off(child->expand_btn);
            TREE_NODE(it)->expand_btn = NULL;
            child = TREE_NODE(it);
        }
        move_off(child->label);
        TREE_NODE(it)->label = NULL;
        left--;
        if (!node->children.size) {
            if (node->collapse_btn) {
                move_off(node->collapse_btn);
                node->collapse_btn = NULL;
            }
            if (node->expand_btn) {
                move_off(node->expand_btn);
                node->expand_btn = NULL;
            }
        }
        if (TREE_NODE(it))
            node_delete(TREE_NODE(it));
        if (!left)
            break;
        it = it->next;
        if (it == node->children.head)
            return 1;
    }
    list_clear(&node->children, NULL);
    return 1;
}

BYTE ui_tree_view_remove(struct UITreeView *self, struct UITreeNode *parent, int id, BYTE keep_children)
{
    struct ListNode *head, *it;
    struct UITreeNode *child;

    if (!parent)
        parent = self->root;
    if ((int)parent->children.size <= 0)
        return 0;
    head = parent->children.head;
    for (it = head->next;; it = it->next) {
        if (it == head)
            return 0;
        if (TREE_NODE(it)->id == id)
            break;
    }
    child = TREE_NODE(it);
    if (!keep_children) {
        if (child->children.size) {
            ui_tree_view_clear_children(self, child);
            child = TREE_NODE(it);
        }
        if (child->collapse_btn) {
            move_off(child->collapse_btn);
            TREE_NODE(it)->collapse_btn = NULL;
            child = TREE_NODE(it);
        }
        if (child->expand_btn) {
            move_off(child->expand_btn);
            TREE_NODE(it)->expand_btn = NULL;
            child = TREE_NODE(it);
        }
    }
    move_off(child->label);
    TREE_NODE(it)->label = NULL;
    child = TREE_NODE(it);
    list_erase(&parent->children, it, NULL);
    if (child)
        node_delete(child);
    if (!parent->children.size) {
        if (parent->collapse_btn) {
            move_off(parent->collapse_btn);
            parent->collapse_btn = NULL;
        }
        if (parent->expand_btn) {
            move_off(parent->expand_btn);
            parent->expand_btn = NULL;
        }
    }
    return 1;
}

int ui_tree_view_layout(struct UITreeView *self, struct List *list, int x, int y)
{
    struct ListNode *head, *it;
    int first = self->top;
    int end = self->base.height / self->row_height + self->top;
    int gap = self->indent - 2;
    int step = 0, bottom;

    if ((int)list->size <= 0)
        return y;
    head = list->head;
    it = head->next;
    if (it == head)
        return y;
    for (; it != head; self->row_count++, it = it->next) {
        struct UITreeNode *node = TREE_NODE(it);
        struct UITVTextButton *label;
        int label_x = x, label_y;

        if (node->collapse_btn)
            label_x = node->collapse_btn->bmp.bitmap_width + gap + x;
        bottom = y;
        if (self->row_count >= first && self->row_count < end) {
            int room, label_height;

            if (node->icon > -1) {
                struct String *path = (struct String *)node->icons.first + node->icon;
                struct BitmapRes *bmp = res_mgr_get(g_resMgr(), ui_skin_path(string_cstr(path)));

                bottom = bmp->height + y;
                if (bottom >= self->base.height)
                    continue;
                ui_window_draw_bitmap(&self->base, label_x, y, bmp, 1);
                label_x += bmp->width + gap;
            }
            if (bottom >= self->base.height)
                continue;

            label = TREE_NODE(it)->label;
            label->boxed = 0;
            ui_window_invalidate_base(&label->button.base);
            if (self->selected && self->selected == TREE_NODE(it)) {
                label = TREE_NODE(it)->label;
                label->boxed = 1;
                ui_window_invalidate_base(&label->button.base);
            }
            label = TREE_NODE(it)->label;
            label->alt_text = 0;
            label->alt_text_colour = 0;
            ui_window_invalidate_base(&label->button.base);
            if (self->hover && self->dragged && self->drop && self->drop_colour) {
                struct UITreeNode *n = TREE_NODE(it);

                if (self->drop == n) {
                    label = n->label;
                    label->alt_text = 1;
                    label->alt_text_colour = self->drop_colour;
                    ui_window_invalidate_base(&label->button.base);
                    n = TREE_NODE(it);
                }
                if (n->parent && n->parent->label && n->parent->label->alt_text) {
                    label = n->label;
                    label->alt_text_colour = self->drop_colour;
                    label->alt_text = 1;
                    ui_window_invalidate_base(&label->button.base);
                }
            }

            room = self->base.width - label_x - 2;
            if (self->has_vscroll)
                room -= self->vbar->base.width;
            label_y = y + 2;
            ui_window_move_base(&TREE_NODE(it)->label->button.base, label_x, label_y);
            label = TREE_NODE(it)->label;
            label_height = label->button.base.height;
            if (room != label->button.base.width && strcmp(string_cstr(&label->button.text), "")) {
                int w;

                string_assign(&label->label, string_cstr(&label->button.text), label->button.text.size);
                if (label->button.base.width > room) {
                    struct String shortened = {0};
                    const char *cut = ui_window_shorten_text(&label->button.base, &shortened,
                                                           string_cstr(&label->button.text), room - 2,
                                                           label->font_face, label->font_height, label->outlined);

                    string_assign_cstr(&label->label, cut);
                    string_free(&shortened);
                    w = room;
                } else {
                    w = ui_window_text_width(&label->button.base, string_cstr(&label->button.text), 0,
                                           label->font_face, label->font_height, label->outlined, 0) + 2;
                }
                ui_window_resize_base(&label->button.base, label->framed ? w + 2 : w, label_height);
            }
            if (self->editing == TREE_NODE(it)) {
                move_off(TREE_NODE(it)->label);
                label = TREE_NODE(it)->label;
                ui_window_resize(self->edit, label->button.base.width, label->button.base.height);
                ui_window_move(self->edit, label_x, label_y);
            }
            step = TREE_NODE(it)->label->button.base.height + self->indent;
            if (self->row_height <= step)
                self->row_height = step;
        }

        if (bottom >= self->base.height)
            continue;
        node = TREE_NODE(it);
        if (node->expanded == 0) {
            if (self->row_count < first || self->row_count >= end)
                continue;
            if (node->expand_btn)
                ui_window_move_base(&node->expand_btn->bmp.button.base, x, y + 4);
            y += step;
        } else if (node->expanded == 1) {
            if (self->row_count >= first && self->row_count < end) {
                if (node->collapse_btn) {
                    ui_window_move_base(&node->collapse_btn->bmp.button.base, x, y + 4);
                    node = TREE_NODE(it);
                }
                y += step;
                bottom += step;
            }
            if ((int)node->children.size <= 0 || bottom >= self->base.height)
                continue;
            if (node->collapse_btn)
                x += node->collapse_btn->bmp.bitmap_width + gap;
            {
                int below = ui_tree_view_layout(self, &node->children, x, y);

                if (self->row_count >= first && self->row_count < end)
                    y = below;
            }
            if (TREE_NODE(it)->collapse_btn)
                x -= TREE_NODE(it)->collapse_btn->bmp.bitmap_width + gap;
        }
    }
    return y;
}

static void tree_view_destruct(struct UITreeView *self)
{
    struct UITreeNode *root;

    self->base.ops = &ui_tree_view_ops;
    ui_tree_view_clear_children(self, self->root);
    root = self->root;
    if (root)
        node_delete(root);
    ui_window_destruct(&self->base);
}

void ui_tree_view_destroy(struct UIWindow *window)
{
    struct UITreeView *self = (struct UITreeView *)window;
    tree_view_destruct(self);
    mem_free(self);
}

void ui_tree_view_resize(struct UIWindow *window, int width, int height)
{
    struct UITreeView *self = (struct UITreeView *)window;
    ui_window_resize_base(&self->base, width, height);
    ui_window_move_base(&self->vbar->base, self->base.width - BAR_INSET, 0);
    ui_scroll_bar_resize(&self->vbar->base, self->vbar->base.width, height);
    ui_tree_view_update_scroll_bar(self);
}

void ui_tree_view_on_create(struct UIWindow *window, int width, int height)
{
    struct UITreeView *self = (struct UITreeView *)window;
    struct UIScrollBar *bar = mem_alloc(sizeof *bar);
    struct UIWindow *edit;

    if (bar) {
        ui_scroll_bar_construct(bar);
        bar->base.ops = &ui_scroll_bar3_ops;
        bar->button_size = 9;
    }
    self->vbar = bar;
    bar->orientation = 1;
    ui_window_resize_base(&bar->base, bar->thickness, height);
    ui_window_move_base(&bar->base, width - BAR_INSET, 0);
    ui_window_on_create_base(&bar->base, bar->thickness, height);
    ui_window_add_child(&self->base, &self->vbar->base);

    edit = mem_alloc(sizeof(struct UIEditCtrl));
    edit = edit ? ui_edit_ctrl2_construct(edit) : NULL;
    self->edit = edit;
    ui_window_create(edit, 100, 0x13);
    move_off(self->edit);
    ui_window_add_child(&self->base, self->edit);
}

void ui_tree_view_on_draw(struct UIWindow *window)
{
    struct UITreeView *self = (struct UITreeView *)window;
    ui_window_clear_canvas(&self->base, UI_TRANSPARENT);
    if (self->base.parent)
        ui_window_invalidate(self->base.parent);
    ui_tree_view_hide_rows(self, &self->root->children);
    self->row_count = 0;
    ui_tree_view_layout(self, &self->root->children, 0, 0);
    ui_tree_view_update_scroll_bar(self);
}

void ui_tree_view_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UITreeView *self = (struct UITreeView *)window;
    struct UIWindow *parent = self->base.parent;

    if (parent)
        ui_window_on_l_btn_down(parent, self->base.x + x, self->base.y + y);
}

void ui_tree_view_on_mouse_move(struct UIWindow *window, int x, int y)
{
    struct UITreeView *self = (struct UITreeView *)window;
    self->hover = x >= 0 && x < self->base.width && y >= 0 && y < self->base.height;
}

void ui_tree_view_on_l_btn_up(struct UIWindow *window, int x, int y)
{
    struct UITreeView *self = (struct UITreeView *)window;
    if (x >= 0 && x < self->base.width && y >= 0 && y < self->base.height)
        self->selected = NULL;
    self->dragged = NULL;
    self->drop = NULL;
    self->hover = 0;
    ui_window_invalidate_base(&self->base);
    if (self->base.parent)
        ui_window_on_l_btn_up(self->base.parent, self->base.x + x, self->base.y + y);
}

void ui_tree_view_on_wheel(struct UIWindow *window, int delta)
{
    struct UITreeView *self = (struct UITreeView *)window;
    if (!self->has_vscroll)
        return;
    if (delta < 0)
        ui_window_send_msg(&self->base, &self->vbar->base, UI_MSG_SCROLL_V, 3, 0, 0, 0);
    else
        ui_window_send_msg(&self->base, &self->vbar->base, UI_MSG_SCROLL_V, -3, 0, 0, 0);
}

void ui_tree_view_update_scroll_bar(struct UITreeView *self)
{
    int page = self->base.height / self->row_height;
    int extra = self->row_count - page < 0 ? 0 : self->row_count - page;
    struct UIScrollBar *bar;

    self->has_vscroll = extra != 0;
    bar = self->vbar;
    bar->total = self->row_count;
    bar->page = page;
    if (extra) {
        bar->max_pos = extra;
        ui_window_invalidate_base(&bar->base);
        if (bar->pos > bar->max_pos)
            bar->pos = bar->max_pos;
    } else {
        bar->max_pos = 0;
        ui_window_invalidate_base(&bar->base);
        if (bar->pos > bar->max_pos)
            bar->pos = bar->max_pos;
        self->top = 0;
        self->row_count = 0;
    }
    ui_window_invalidate_base(&self->vbar->base);
}

int ui_tree_view_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1, int val2,
                                  int val3, int val4)
{
    struct UITreeView *self = (struct UITreeView *)window;
    struct UITreeNode *node;

    switch (msg) {
    case UI_MSG_DEFAULT_BUTTON:
        if (sender == self->edit && self->editing)
            ui_window_send_msg(&self->base, NULL, TV_MSG_RENAME, 0, 1, 0, 0);
        return 0;

    case UI_MSG_SCROLL_V:
    case UI_MSG_PAGE_UP:
    case UI_MSG_PAGE_DOWN: {
        int old, top, page = 0, last;

        ui_tree_view_hide_rows(self, &self->root->children);
        old = self->top;
        if (msg == UI_MSG_PAGE_UP) {
            page = self->base.height;
            top = old - page / self->row_height + 1;
        } else if (msg == UI_MSG_PAGE_DOWN) {
            page = self->base.height;
            top = old - 1 + page / self->row_height;
        } else {
            page = self->base.height;
            top = old + val1;
        }
        self->top = top;
        if (top < 0) {
            self->top = 0;
            top = 0;
        }
        last = self->row_count - page / self->row_height;
        if (top > last) {
            self->top = last;
            top = last;
        }
        self->vbar->pos = top;
        ui_window_invalidate_base(&self->vbar->base);
        ui_window_invalidate_base(&self->base);
        if (self->base.parent)
            ui_window_invalidate(self->base.parent);
        return self->top - old;
    }

    case UI_MSG_BAR_PAGE_UP:
        if (self->has_vscroll)
            ui_window_send_msg(&self->base, &self->vbar->base, UI_MSG_SCROLL_V, -1, 0, 0, 0);
        return 0;
    case UI_MSG_BAR_PAGE_DOWN:
        if (self->has_vscroll)
            ui_window_send_msg(&self->base, &self->vbar->base, UI_MSG_SCROLL_V, 1, 0, 0, 0);
        return 0;

    case TV_MSG_SELECT:
        self->selected = (struct UITreeNode *)val1;
        if (val1)
            ui_window_send_msg(self->base.parent, &self->base, TV_MSG_SELECT, val1, sender->x + val2,
                     sender->height / 2 + sender->y, 0);
        return 0;

    case TV_MSG_TOGGLE:
    case TV_MSG_ACTIVATE:
    case TV_MSG_TOGGLE_BTN:
        self->dragged = NULL;
        self->drop = NULL;
        self->selected = (struct UITreeNode *)val1;
        if (!val1)
            return 0;
        if (msg == TV_MSG_ACTIVATE) {
            ui_window_send_msg(self->base.parent, &self->base, TV_MSG_ACTIVATE, val1, sender->x + val2,
                     sender->height / 2 + sender->y, 0);
            return 0;
        }
        node = (struct UITreeNode *)val1;
        if (node->children.size) {
            int bx, by;
            struct UITVBitmapButton *btn;

            ui_tree_view_hide_rows(self, &self->root->children);
            bx = sender->x;
            by = msg == TV_MSG_TOGGLE ? sender->y - 6 : sender->y;
            if (self->selected->expanded == 0) {
                self->selected->expanded = 1;
                btn = self->selected->collapse_btn;
            } else {
                self->selected->expanded = 0;
                btn = self->selected->expand_btn;
            }
            ui_window_move_base(&btn->bmp.button.base, bx, by);
            ui_window_invalidate_base(&self->base);
        }
        if (msg == TV_MSG_TOGGLE)
            ui_window_send_msg(self->base.parent, &self->base, TV_MSG_TOGGLE, (int)self->selected, sender->x + val2,
                     sender->height / 2 + sender->y, 0);
        self->dragged = NULL;
        self->drop = NULL;
        return 0;

    case TV_MSG_DRAG:
        node = (struct UITreeNode *)val1;
        self->dragged = node;
        self->selected = node;
        if (!node->draggable)
            self->dragged = NULL;
        return 0;

    case TV_MSG_DRAG_OVER:
        mode_set_cursor_action(client_current_mode(), 0);
        if (!self->dragged || self->dragged->id == val1)
            return 0;
        node = (struct UITreeNode *)val1;
        self->drop = node;
        if (!node->drop_inside) {
            node = node->parent;
            self->drop = node;
        }
        node->expanded = 1;
        ui_window_invalidate_base(&self->base);
        return 0;

    case TV_MSG_DROP:
        if (sender == self->base.parent)
            self->drop = NULL;
        if (self->dragged) {
            struct UITVTextButton *label;
            struct UITreeNode *old_parent;
            struct List *list;

            if (self->dragged->parent == self->drop)
                return 0;
            if (self->drop && self->drop->parent == self->dragged)
                return 0;
            mode_set_cursor_action(client_current_mode(), 2);
            label = self->dragged->label;
            ui_tree_view_remove(self, self->dragged->parent, self->dragged->id, 1);
            old_parent = self->dragged->parent;
            self->dragged->parent = self->drop;
            list = self->drop ? &self->drop->children : &self->root->children;
            list_push_back(list, &self->dragged, sizeof self->dragged);
            self->dragged->label = label;
            if (self->base.parent)
                ui_window_send_msg(self->base.parent, &self->base, TV_MSG_DROP, (int)self->dragged, (int)self->drop,
                         (int)old_parent, 0);
            ui_tree_view_hide_rows(self, &self->root->children);
        }
        self->selected = NULL;
        self->dragged = NULL;
        self->drop = NULL;
        ui_window_invalidate_base(&self->base);
        return 0;

    case TV_MSG_RENAME:
        if (val2 == 0) {
            struct UIEditCtrl *edit = (struct UIEditCtrl *)self->edit;

            self->editing = (struct UITreeNode *)val1;
            edit->max_chars = val3 > MAX_NAME ? MAX_NAME : val3;
            ui_edit_ctrl_set_text((struct UIEditCtrl *)self->edit, self->editing->name);
            ui_window_mgr_set_focus_edit(&g_windowMgr, self->edit);
            ui_window_invalidate_base(&self->base);
        } else if (val2 == 1) {
            struct UIWindow *edit = self->edit;
            size_t len;

            if (ui_window_mgr_get_focus_edit(&g_windowMgr) != edit)
                return 0;
            len = strlen(ui_edit_ctrl_get_text(edit));
            if (self->base.parent)
                ui_window_send_msg(self->base.parent, &self->base, TV_MSG_RENAME,
                         (int)ui_edit_ctrl_get_text(self->edit), (int)len + 1, 0, 0);
        } else {
            int x = self->edit->x, y = self->edit->y;

            move_off(self->edit);
            if (val3) {
                strcpy(self->editing->name, ui_edit_ctrl_get_text(self->edit));
                uitv_text_button_set_text(self->editing->label, text_view_cstr(ui_edit_ctrl_get_text(self->edit)));
                uitv_text_button_fit(self->editing->label);
            }
            ui_window_move_base(&self->editing->label->button.base, x, y);
            self->editing = NULL;
        }
        return 0;

    default:
        return 0;
    }
}
