#include "render/rp_pool.h"
#include "render/texture_types.h"
#include "ui/window.h"
#include "ui/draw.h"
#include "ui/skin.h"
#include "ui/controls/controls_std.h"
#include "ui/window_mgr.h"
#include "ui/render/render.h"
#include <mmsystem.h>
#include "core/object_type_tables.h"
#include "render/render_api.h"
#include "ui/ui_std.h"
#include "ui/wndmgr/wndmgr.h"

int g_uiWindowSerial;

struct UIWindow *ui_window_construct(struct UIWindow *self, int composites)
{
    list_init(&self->draws);
    self->texture = NULL;
    self->ops = &ui_window_ops;
    self->parent = NULL;
    self->width = 0;
    self->height = 0;
    self->x = 0;
    self->y = 0;
    self->canvas = NULL;
    self->visible = 1;
    self->id = UI_ID_NONE;
    self->state = 0;
    self->state_count = 0;
    self->alpha_target = 0xff;
    self->alpha_time = timeGetTime();
    self->alpha = 0xff;
    self->text_bk_on = 0;
    self->text_bk_colour = 0;
    self->list_rank = 4;
    list_init(&self->children);
    self->dirty = 1;
    self->quads.first = self->quads.last = self->quads.end = NULL;
    self->sprite_quads.first = self->sprite_quads.last = self->sprite_quads.end = NULL;
    self->serial = ++g_uiWindowSerial;
    self->composites = (BYTE)composites;
    self->redraws_tree = 0;
    return self;
}

void ui_window_create(struct UIWindow *self, int width, int height)
{
    if (self->canvas)
        dc_bitmap_destroy(self->canvas);
    self->canvas = dc_bitmap_create(width, height);
    self->width = width;
    self->height = height;
    ui_window_invalidate(self);
    ui_window_on_create(self, width, height);
}

void ui_window_add_child(struct UIWindow *self, struct UIWindow *child)
{
    child->parent = self;
    list_push_back(&self->children, &child, sizeof child);
}

void ui_window_prepend_child(struct UIWindow *self, struct UIWindow *child)
{
    child->parent = self;
    list_push_front(&self->children, &child, sizeof child);
}

void ui_window_destroy_children(struct UIWindow *self)
{
    struct ListNode *node;

    for (node = self->children.head->next; node != self->children.head; node = node->next) {
        struct UIWindow *child = *LIST_VALUE(node, struct UIWindow *);

        if (child)
            ui_window_destroy(child);
    }
    list_clear(&self->children, NULL);
}

void ui_window_remove_child(struct UIWindow *self, struct UIWindow *child, BYTE destroy)
{
    struct ListNode *node;

    if (!child)
        return;
    for (node = self->children.head->next; node != self->children.head; node = node->next)
        if (*LIST_VALUE(node, struct UIWindow *) == child)
            break;
    if (node == self->children.head)
        return;
    ui_window_mgr_drop_window_refs(&g_windowMgr, child);
    if (destroy == 1)
        ui_window_destroy(child);
    list_erase(&self->children, node, NULL);
}

struct UIWindow *ui_window_find_child_by_serial(struct UIWindow *self, int serial)
{
    struct ListNode *node;

    for (node = self->children.head->next; node != self->children.head; node = node->next) {
        struct UIWindow *child = *LIST_VALUE(node, struct UIWindow *);

        if (child->serial == serial)
            return child;
    }
    return NULL;
}

struct UIWindow *ui_window_next_focusable(struct UIWindow *self, struct UIWindow *child)
{
    struct ListNode *node;

    for (node = self->children.head->next; node != self->children.head; node = node->next)
        if (*LIST_VALUE(node, struct UIWindow *) == child && ui_window_is_focusable(child))
            break;
    if (node == self->children.head)
        return NULL;
    for (node = node->next; node != self->children.head; node = node->next) {
        struct UIWindow *next = *LIST_VALUE(node, struct UIWindow *);

        if (ui_window_is_focusable(next))
            return next;
    }
    return NULL;
}

struct UIWindow *ui_window_prev_focusable(struct UIWindow *self, struct UIWindow *child)
{
    struct ListNode *head = self->children.head, *node = head;

    if (node == node->next)
        return NULL;
    for (;;) {
        struct UIWindow *w = *LIST_VALUE(node->prev, struct UIWindow *);

        if (w == child && ui_window_is_focusable(w))
            break;
        node = node->prev;
        if (node == head->next)
            return NULL;
    }
    node = node->prev;
    while (node != head->next) {
        struct UIWindow *w = *LIST_VALUE(node->prev, struct UIWindow *);

        if (ui_window_is_focusable(w))
            return w;
        node = node->prev;
    }
    return NULL;
}

BYTE ui_window_has_ancestor(struct UIWindow *self, struct UIWindow *ancestor)
{
    struct UIWindow *p;

    for (p = self->parent; p; p = p->parent)
        if (p == ancestor)
            return 1;
    return 0;
}

static void send_msg_to_tree(struct UIWindow *self, int msg)
{
    struct ListNode *node;

    ui_window_send_msg(self, NULL, msg, 0, 0, 0, 0);
    for (node = self->children.head->next; node != self->children.head; node = node->next) {
        struct UIWindow *child = *LIST_VALUE(node, struct UIWindow *);

        if (child)
            ui_window_send_msg(child, NULL, msg, 0, 0, 0, 0);
    }
}

void ui_window_send_msg3_to_tree(struct UIWindow *self)
{
    send_msg_to_tree(self, 3);
}

void ui_window_send_msg4_to_tree(struct UIWindow *self)
{
    send_msg_to_tree(self, 4);
}

void ui_window_flush_quads(struct UIWindow *self)
{
    void **quad;

    for (quad = self->quads.first; quad != self->quads.last; quad++)
        renderer_add_prim(g_renderer, *quad, 0x201);
    self->quads.last = self->quads.first;
}

BYTE ui_window_render_base(struct UIWindow *self, int force)
{
    struct ListNode *node;
    BYTE redrawn = 0;

    if (self->dirty == 1) {
        ui_ondraw(self);
        force = 1;
    }
    for (node = self->children.head->next; node != self->children.head; node = node->next) {
        struct UIWindow *child = *LIST_VALUE(node, struct UIWindow *);

        if (child->visible == 1 && (ui_window_render(child, force) || redrawn))
            redrawn = 1;
    }

    if (self->composites == 1) {
        BYTE scope;
        struct List quads;
        BYTE result;

        if (force != 1 && redrawn != 1)
            return 0;
        ui_list_iterator_self(&scope);
        list_init(&quads);
        for (node = self->draws.head->next; node != self->draws.head; node = node->next)
            list_push_back(&quads, LIST_VALUE(node, void *), sizeof(void *));
        for (node = self->children.head->next; node != self->children.head; node = node->next) {
            struct UIWindow *child = *LIST_VALUE(node, struct UIWindow *);

            if (child->visible == 1) {
                void *quad = ui_window_make_quad(child);

                list_push_back(&quads, &quad, sizeof quad);
            }
        }
        render_quads_to_texture(self->width, self->height, &quads, (struct Texture **)&self->texture);
        result = self->parent != NULL;
        list_free(&quads, NULL);
        ui_empty_method(&scope);
        return result;
    }

    if (!self->parent)
        return 0;
    if (self->parent->composites == 0) {
        if (force != 1 && redrawn != 1)
            return 0;
        dc_bitmap_copy_bitmap(self->parent->canvas, self->x, self->y, self->width, self->height, self->canvas);
        return 1;
    }
    return force == 1 || redrawn == 1;
}
