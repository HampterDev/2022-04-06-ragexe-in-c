#include "ui/controls/scroll.h"
#include "ui/window_types.h"
#include "render/rp_pool.h"
#include "render/texture_types.h"
#include "ui/window_list.h"
#include "core/object_type_tables.h"
#include "ui/window.h"
#include "ui/controls/controls.h"
#include "ui/render/render.h"
#include "ui/ui_std.h"
#include "ui/controls/controls_std.h"
#include <stdint.h>

#define SCROLL_BAR_THICKNESS 0xd
#define SCROLL_WHEEL_LINES 3

#include "render/cdc.h"
#include "render/render_api.h"
#include "diag/perf.h"

struct UIScrollWnd;
struct UIWindow;
const struct UIWindowOps ui_scroll_wnd_ops = {
    .destroy = ui_scroll_wnd_destroy,
    .resize = ui_scroll_wnd_resize,
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
    .on_create = ui_scroll_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_window_on_draw_base,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_window_on_l_btn_down_base,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_window_on_mouse_move_base,
    .on_mouse_hover = ui_window_on_mouse_hover_base,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_window_on_l_btn_up_base,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_scroll_wnd_on_wheel,
    .snap = ui_window_snap_base,
    .send_msg = ui_scroll_wnd_send_msg,
    .invalidate = ui_window_invalidate_base,
    .render = ui_scroll_wnd_render,
    .has_custom_backdrop = 0,
    .draw_backdrop = ui_window_draw_backdrop_base,
    .get_records = ui_window_get_records_base,
    .type = UI_TYPE_WINDOW,
    .on_hover_enter = ui_window_on_hover_enter_base,
    .on_hover_leave = ui_window_on_hover_leave_base,
    .make_quad = ui_scroll_wnd_make_quad,
    .hit_test = ui_window_hit_test_base,
    .render_overlay = ui_window_render_overlay_base,
};

static int is_bar(struct UIScrollWnd *self, struct UIWindow *wnd)
{
    return wnd == &self->hbar->base || wnd == &self->vbar->base;
}

static int child_less(const void *a, const void *b)
{
    return (uintptr_t)*(struct UIWindow *const *)a < (uintptr_t)*(struct UIWindow *const *)b;
}

static POINT *child_cell(struct UIScrollWnd *self, struct UIWindow *child)
{
    struct ChildPosition *position = tree_get(&self->child_positions, &child, sizeof child, sizeof *position,
                                              child_less);

    return &position->cell;
}

static void place_children(struct UIScrollWnd *self, int dx, int dy)
{
    struct Tree *map = &self->child_positions;
    struct TreeNode *node;

    for (node = tree_first(map); node != map->head; node = tree_next(node)) {
        struct ChildPosition *position = TREE_VALUE(node, struct ChildPosition);

        ui_window_move(position->child, position->cell.x - dx, position->cell.y - dy);
    }
}

static struct TexDrawPrim *texture_quad(int x, int y, int sx, int sy, int w, int h, struct Texture *tex, DWORD argb)
{
    struct TexDrawPrim *quad = renderer_new_prim(g_renderer);
    float tw = (float)tex->base.width, th = (float)tex->base.height;
    int i;

    quad->base.texture = tex;
    quad->quad[0].y = (float)y;
    quad->quad[0].x = (float)x;
    quad->quad[0].u = ((float)sx + g_texelOffsetU) / tw;
    quad->quad[0].v = ((float)sy + g_texelOffsetV) / th;
    quad->quad[1].y = (float)y;
    quad->quad[1].x = (float)(x + w);
    quad->quad[1].u = ((float)(sx + w) + g_texelOffsetU) / tw;
    quad->quad[1].v = ((float)sy + g_texelOffsetV) / th;
    quad->quad[2].x = (float)x;
    quad->quad[2].y = (float)(y + h);
    quad->quad[2].u = ((float)sx + g_texelOffsetU) / tw;
    quad->quad[2].v = ((float)(sy + h) + g_texelOffsetV) / th;
    quad->quad[3].x = (float)(x + w);
    quad->quad[3].y = (float)(y + h);
    quad->quad[3].u = ((float)(sx + w) + g_texelOffsetU) / tw;
    quad->quad[3].v = ((float)(sy + h) + g_texelOffsetV) / th;
    for (i = 0; i < 4; i++) {
        quad->quad[i].diffuse = argb;
        *(DWORD *)&quad->quad[i].z = UI_QUAD_Z_BITS;
        *(DWORD *)&quad->quad[i].rhw = UI_QUAD_RHW_BITS;
    }
    return quad;
}

static int draw_if_dirty(struct UIWindow *wnd, int force)
{
    if (wnd->dirty == 1) {
        ui_window_on_draw(wnd);
        wnd->dirty = 0;
        return 1;
    }
    return force;
}

struct UIScrollWnd *ui_scroll_wnd_construct(struct UIScrollWnd *self, int width,
                                                     int height, int composites)
{
    ui_window_construct(&self->base, composites);
    self->base.ops = &ui_scroll_wnd_ops;
    self->view_width = 0;
    self->view_height = 0;
    self->content_width = 0;
    self->content_height = 0;
    self->hbar = NULL;
    self->vbar = NULL;
    self->vbar_width = SCROLL_BAR_THICKNESS;
    self->hbar_height = SCROLL_BAR_THICKNESS;
    self->hpos = 0;
    self->vpos = 0;
    self->hscroll_on = 0;
    self->vscroll_on = 0;
    tree_init(&self->child_positions);
    ui_window_resize_base(&self->base, width, height);
    ui_window_on_create(&self->base, width, height);
    return self;
}

static void destroy_bars(struct UIScrollWnd *self)
{
    if (self->hbar) {
        ui_scroll_wnd_remove_child(self, &self->hbar->base, 0);
        if (self->hbar)
            ui_window_destroy(&self->hbar->base);
        self->hbar = NULL;
    }
    if (self->vbar) {
        ui_scroll_wnd_remove_child(self, &self->vbar->base, 0);
        if (self->vbar)
            ui_window_destroy(&self->vbar->base);
        self->vbar = NULL;
    }
}

void ui_scroll_wnd_destruct(struct UIScrollWnd *self)
{
    self->base.ops = &ui_scroll_wnd_ops;
    destroy_bars(self);
    tree_free(&self->child_positions, NULL);
    ui_window_destruct(&self->base);
}

void ui_scroll_wnd_destroy(struct UIWindow *window)
{
    struct UIScrollWnd *self = (struct UIScrollWnd *)window;
    ui_scroll_wnd_destruct(self);
    mem_free(self);
}

void ui_scroll_wnd_add_child(struct UIScrollWnd *self, struct UIWindow *child)
{
    struct List children;
    struct ListNode *node;

    if (self->hbar)
        ui_scroll_wnd_remove_child(self, &self->hbar->base, 0);
    if (self->vbar)
        ui_scroll_wnd_remove_child(self, &self->vbar->base, 0);
    ui_window_add_child(&self->base, child);
    if (self->hbar)
        ui_window_add_child(&self->base, &self->hbar->base);
    if (self->vbar)
        ui_window_add_child(&self->base, &self->vbar->base);
    ui_window_list_copy(&children, &self->base.children);
    for (node = children.head->next; node != children.head; node = node->next) {
        struct UIWindow *wnd = *LIST_VALUE(node, struct UIWindow *);
        POINT *home;

        if (is_bar(self, wnd))
            continue;
        home = child_cell(self, wnd);
        home->x = wnd->x;
        home->y = wnd->y;
    }
    ui_scroll_wnd_update_bars(self);
    list_free(&children, NULL);
}

void ui_scroll_wnd_place_child(struct UIScrollWnd *self, struct UIWindow *child, int x, int y)
{
    ui_window_move(child, x, y);
    child_cell(self, child)->x = x;
    child_cell(self, child)->y = y;
    ui_scroll_wnd_update_bars(self);
}

void ui_scroll_wnd_remove_child(struct UIScrollWnd *self, struct UIWindow *child, int destroy)
{
    if (self->hbar && self->vbar)
        tree_erase_key(&self->child_positions, &child, child_less, NULL);
    ui_window_remove_child(&self->base, child, (BYTE)destroy);
}

void ui_scroll_wnd_remove_all(struct UIScrollWnd *self)
{
    self->hpos = 0;
    self->vpos = 0;
    tree_clear(&self->child_positions, NULL);
    ui_window_destroy_children(&self->base);
}

void ui_scroll_wnd_resize(struct UIWindow *window, int width, int height)
{
    struct UIScrollWnd *self = (struct UIScrollWnd *)window;
    ui_window_resize_base(&self->base, width, height);
    if (self->vbar) {
        ui_window_resize(&self->vbar->base, self->vbar_width, height - self->hbar_height);
        ui_window_move(&self->vbar->base, width - self->vbar_width, 0);
    }
    if (self->hbar) {
        ui_scroll_bar_resize(&self->hbar->base, width - self->vbar_width, self->hbar_height);
        ui_window_move_base(&self->hbar->base, 0, height - self->hbar_height);
    }
    ui_scroll_wnd_update_bars(self);
}

void ui_scroll_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UIScrollWnd *self = (struct UIScrollWnd *)window;
    struct UIScrollBar *bar = mem_alloc(sizeof *bar);

    if (bar)
        bar = ui_scroll_bar_construct(bar);
    self->hbar = bar;
    ui_scroll_bar_create(bar, 0, height - self->hbar_height, UI_SCROLL_HORZ, width - self->vbar_width);
    ui_window_add_child(&self->base, &self->hbar->base);
    bar = mem_alloc(sizeof *bar);
    if (bar)
        bar = ui_scroll_bar_construct(bar);
    self->vbar = bar;
    ui_scroll_bar_create(bar, width - self->vbar_width, 0, UI_SCROLL_VERT, height - self->hbar_height);
    ui_window_add_child(&self->base, &self->vbar->base);
}

void ui_scroll_wnd_on_wheel(struct UIWindow *window, int delta)
{
    struct UIScrollWnd *self = (struct UIScrollWnd *)window;
    if (self->vscroll_on != 1)
        return;
    ui_window_send_msg(&self->base, &self->vbar->base, UI_MSG_SCROLL_V, delta < 0 ? SCROLL_WHEEL_LINES : -SCROLL_WHEEL_LINES,
             0, 0, 0);
}

static void set_bar(struct UIScrollBar *bar, int *pos, int content, int view)
{
    ui_scroll_bar_set_range(bar, content - view, view, content);
    if (*pos > content - view)
        *pos = content - view;
    if (*pos < 0)
        *pos = 0;
    ui_scroll_bar_set_pos(bar, *pos);
    ui_window_invalidate_base(&bar->base);
}

void ui_scroll_wnd_update_bars(struct UIScrollWnd *self)
{
    struct Tree *map = &self->child_positions;
    struct TreeNode *node;
    int right = 0, bottom = 0;

    if (!self->hbar || !self->vbar)
        return;
    for (node = tree_first(map); node != map->head; node = tree_next(node)) {
        struct ChildPosition *position = TREE_VALUE(node, struct ChildPosition);
        struct UIWindow *child = position->child;

        if (!child->visible)
            continue;
        if (right < position->cell.x + child->width)
            right = position->cell.x + child->width;
        if (bottom < position->cell.y + child->height)
            bottom = position->cell.y + child->height;
    }
    self->hscroll_on = 0;
    self->vscroll_on = 0;
    self->view_width = self->base.width;
    self->view_height = self->base.height;
    if (right > self->view_width) {
        self->hscroll_on = 1;
        self->view_height = self->base.height - self->hbar_height;
    }
    if (bottom > self->base.height) {
        self->vscroll_on = 1;
        self->view_width = self->base.width - self->vbar_width;
    }
    if (!self->hscroll_on && right > self->view_width) {
        self->hscroll_on = 1;
        self->view_height = self->base.height - self->hbar_height;
    }
    if (!self->vscroll_on && bottom > self->view_height) {
        self->vscroll_on = 1;
        self->view_width = self->base.width - self->vbar_width;
    }
    self->content_width = right > self->view_width ? right : self->view_width;
    self->content_height = bottom > self->view_height ? bottom : self->view_height;
    if (self->hscroll_on == 1) {
        ui_scroll_bar_resize(&self->hbar->base, self->view_width, self->hbar_height);
        set_bar(self->hbar, &self->hpos, self->content_width, self->view_width);
    }
    if (self->vscroll_on == 1) {
        ui_window_resize(&self->vbar->base, self->vbar_width, self->view_height);
        set_bar(self->vbar, &self->vpos, self->content_height, self->view_height);
    }
    if (!self->hscroll_on)
        self->hpos = 0;
    if (!self->vscroll_on)
        self->vpos = 0;
    ui_window_set_show_base(&self->hbar->base, self->hscroll_on);
    ui_window_set_show(&self->vbar->base, self->vscroll_on);
}

static int scroll_by(struct UIScrollWnd *self, struct UIScrollBar *bar, int *pos, int delta, int content, int view)
{
    int old = *pos;

    *pos = old + delta;
    if (*pos < 0)
        *pos = 0;
    if (*pos > content - view)
        *pos = content - view;
    ui_scroll_bar_set_pos(bar, *pos);
    ui_window_invalidate_base(&bar->base);
    ui_window_invalidate_base(&self->base);
    return *pos - old;
}

int ui_scroll_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                   int val1, int val2, int val3, int val4)
{
    struct UIScrollWnd *self = (struct UIScrollWnd *)window;
    switch (msg) {
    case UI_MSG_SCROLL_V:
    case UI_MSG_PAGE_UP:
    case UI_MSG_PAGE_DOWN:
        if (msg == UI_MSG_PAGE_UP)
            val1 = -1;
        else if (msg == UI_MSG_PAGE_DOWN)
            val1 = 1;
        return scroll_by(self, self->vbar, &self->vpos, val1, self->content_height, self->view_height);
    case UI_MSG_SCROLL_H:
    case UI_MSG_PAGE_LEFT:
    case UI_MSG_PAGE_RIGHT:
        if (msg == UI_MSG_PAGE_LEFT)
            val1 = -1;
        else if (msg == UI_MSG_PAGE_RIGHT)
            val1 = 1;
        return scroll_by(self, self->hbar, &self->hpos, val1, self->content_width, self->view_width);
    }
    return ui_window_send_msg_base(&self->base, sender, msg, val1, val2, val3, val4);
}

static BYTE draw_child_clipped(struct UIScrollWnd *self, struct UIWindow *child, int force)
{
    struct List grandchildren;
    struct ListNode *node;
    BYTE changed = 0;
    int dx, dy, sx = 0, sy = 0, w, h;

    force = draw_if_dirty(child, force);
    ui_window_list_copy(&grandchildren, &child->children);
    for (node = grandchildren.head->next; node != grandchildren.head; node = node->next) {
        struct UIWindow *grandchild = *LIST_VALUE(node, struct UIWindow *);

        if (grandchild->visible == 1)
            changed |= ui_window_render(grandchild, force);
    }
    if (force != 1 && changed != 1) {
        list_free(&grandchildren, NULL);
        return 0;
    }
    dx = child->x;
    dy = child->y;
    w = child->width;
    h = child->height;
    if (child->x < 0) {
        dx = 0;
        sx = -child->x;
        if (self->base.width < child->width + child->x)
            w = self->base.width;
        else
            w = child->width + child->x;
    } else if (self->base.width < child->width + child->x) {
        w = self->base.width - child->x;
    }
    if (child->y < 0) {
        dy = 0;
        sy = -child->y;
        if (self->base.height < child->height + child->y)
            h = self->base.height;
        else
            h = child->height + child->y;
    } else if (self->base.height < child->height + child->y) {
        h = self->base.height - child->y;
    }
    dc_bitmap_copy_rect(self->base.canvas, dx, dy, sx, sy, w, h, child->canvas);
    list_free(&grandchildren, NULL);
    return 1;
}

static BYTE render_plain(struct UIScrollWnd *self, int force)
{
    struct List children;
    struct ListNode *node;
    struct UIWindow *parent;
    BYTE changed = 0, shown;

    place_children(self, self->hpos, self->vpos);
    force = draw_if_dirty(&self->base, force);
    ui_window_list_copy(&children, &self->base.children);
    for (node = children.head->next; node != children.head; node = node->next) {
        struct UIWindow *child = *LIST_VALUE(node, struct UIWindow *);

        if (is_bar(self, child)) {
            if (child->visible == 1)
                changed |= ui_window_render(child, force);
        } else if (child->x + child->width > 0 && child->x < self->base.width && child->y + child->height > 0
                   && child->y < self->base.height && child->visible == 1) {
            changed |= draw_child_clipped(self, child, force);
        }
    }
    parent = self->base.parent;
    if (parent && parent->composites == 0 && (force == 1 || changed == 1)) {
        dc_bitmap_copy_bitmap(parent->canvas, self->base.x, self->base.y, self->base.width, self->base.height,
                       self->base.canvas);
        shown = 1;
    } else {
        shown = 0;
    }
    list_free(&children, NULL);
    return shown;
}

static BYTE render_composited(struct UIScrollWnd *self, int force)
{
    struct List children, children2, quads;
    struct ListNode *node;
    BYTE changed = 0;

    place_children(self, 0, 0);
    force = draw_if_dirty(&self->base, force);
    ui_window_list_copy(&children, &self->base.children);
    for (node = children.head->next; node != children.head; node = node->next) {
        struct UIWindow *child = *LIST_VALUE(node, struct UIWindow *);

        if (child->visible == 1)
            changed |= ui_window_render(child, force);
    }
    if (force == 1 || changed == 1) {
        list_init(&quads);
        for (node = self->base.draws.head->next; node != self->base.draws.head; node = node->next)
            list_push_back(&quads, LIST_VALUE(node, void *), sizeof(void *));
        ui_window_list_copy(&children2, &self->base.children);
        for (node = children2.head->next; node != children2.head; node = node->next) {
            struct UIWindow *child = *LIST_VALUE(node, struct UIWindow *);
            struct TexDrawPrim *quad;

            if (is_bar(self, child)) {
                dc_bitmap_to_texture(child->canvas, &child->texture);
                quad = texture_quad(self->hpos + child->x, self->vpos + child->y, 0, 0, child->width,
                                    child->height, child->texture, ui_window_get_color(child));
            } else {
                quad = ui_window_make_quad(child);
            }
            list_push_back(&quads, &quad, sizeof quad);
        }
        render_quads_to_texture(self->vbar_width + self->content_width, self->hbar_height + self->content_height,
                   &quads, &self->base.texture);
        changed = 1;
        list_free(&children2, NULL);
        list_free(&quads, NULL);
    }
    place_children(self, self->hpos, self->vpos);
    list_free(&children, NULL);
    return changed;
}

BYTE ui_scroll_wnd_render(struct UIWindow *window, int force)
{
    struct UIScrollWnd *self = (struct UIScrollWnd *)window;
    if (self->base.composites == 1)
        return render_composited(self, force);
    return render_plain(self, force);
}

struct TexDrawPrim *ui_scroll_wnd_make_quad(struct UIWindow *window)
{
    struct UIScrollWnd *self = (struct UIScrollWnd *)window;

    if (self->base.composites == 0)
        return ui_window_make_quad_base(&self->base);
    return texture_quad(self->base.x, self->base.y, self->hpos, self->vpos, self->base.width, self->base.height,
                        self->base.texture, ui_window_get_color(&self->base));
}
