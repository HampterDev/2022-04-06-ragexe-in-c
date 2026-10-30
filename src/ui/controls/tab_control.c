#include "ui/controls/edit.h"
#include "ui/controls/tabs.h"
#include "ui/window_access.h"
#include "render/rp_pool.h"
#include "core/object_type_tables.h"
#include "ui/controls/controls.h"
#include "ui/window_mgr.h"
#include "ui/ui_std.h"
#include "ui/controls/controls_std.h"
#include "core/msg_str.h"
#include "diag/perf.h"

struct UITabControl;
struct UIWindow;
const struct UIWindowOps ui_tab_control_ops = {
    .destroy = ui_tab_control_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 0,
    .refreshes_children = 0,
    .move = ui_window_move_base,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_tab_control_drag_and_drop,
    .store_info = ui_window_store_info_base,
    .save_pos = ui_window_save_pos_base,
    .move_delta = ui_window_move_delta_base,
    .set_show = ui_window_set_show_base,
    .on_create = ui_window_on_create_base,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_tab_control_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_tab_control_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_tab_control_on_l_btn_dbl_clk,
    .on_mouse_move = ui_tab_control_on_mouse_move,
    .on_mouse_hover = ui_window_on_mouse_hover_base,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_tab_control_on_l_btn_up,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_window_snap_base,
    .send_msg = ui_tab_control_send_msg,
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

#define NAME_COUNT(tc) VEC_COUNT(&(tc)->names, struct String)
#define NAME(tc, i) (&((struct String *)(tc)->names.first)[i])
#define LENGTH(tc, i) (((int *)(tc)->lengths.first)[i])
#define EDIT_HIDDEN (-100)
#define MSG_NAME_TOO_LONG 0x629

static int vertical(const struct UITabControl *self)
{
    return self->side == UI_TABS_LEFT || self->side == UI_TABS_RIGHT;
}

static int tab_offset(const struct UITabControl *self, int i)
{
    int offset = 0, j;

    for (j = 0; j < i; j++)
        offset += LENGTH(self, j) - 1;
    return offset;
}

static int name_extent(struct UITabControl *self, const char *name)
{
    if (vertical(self))
        return ui_window_vertical_text_length(&self->base, name, 0, 0, self->font_height, 0);
    return ui_window_text_width(&self->base, name, 0, 0, self->font_height, 0, 0);
}

struct UIWindow *ui_tab_control_construct(struct UIWindow *wnd)
{
    struct UITabControl *self = (struct UITabControl *)wnd;

    ui_window_construct(&self->base, 0);
    self->base.ops = &ui_tab_control_ops;
    self->names.first = self->names.last = self->names.end = NULL;
    self->colours.first = self->colours.last = self->colours.end = NULL;
    self->lengths.first = self->lengths.last = self->lengths.end = NULL;
    self->font_height = 12;
    self->new_marks.words.first = self->new_marks.words.last = self->new_marks.words.end = NULL;
    self->new_marks.size = 0;
    self->selected = -1;
    self->drag_x = 0;
    self->drag_y = 0;
    return wnd;
}

void ui_tab_control_destroy(struct UIWindow *window)
{
    struct UITabControl *self = (struct UITabControl *)window;
    self->base.ops = &ui_tab_control_ops;
    vec_free(&self->new_marks.words);
    vec_free(&self->lengths);
    vec_free(&self->colours);
    string_vec_free(&self->names);
    ui_window_destruct(&self->base);
    mem_free(self);
}

void ui_tab_control_fit_size(struct UITabControl *self)
{
    int total = tab_offset(self, (int)NAME_COUNT(self));

    if (vertical(self))
        ui_window_resize_base(&self->base, self->thickness + 1, total + 1);
    else
        ui_window_resize_base(&self->base, total + 1, self->thickness + 1);
    ui_window_invalidate_base(&self->base);
}

void ui_tab_control_relayout(struct UITabControl *self)
{
    if (self->equal_lengths) {
        int longest = 0;
        unsigned i;

        for (i = 0; i < NAME_COUNT(self); i++) {
            int length = name_extent(self, string_cstr(NAME(self, i))) + self->label_margin;

            if (length > longest)
                longest = length;
        }
        for (i = 0; i < NAME_COUNT(self); i++)
            LENGTH(self, i) = longest;
    }
    ui_tab_control_fit_size(self);
}

void ui_tab_control_setup(struct UIWindow *wnd, int side, int thickness, int padding,
                                   BYTE equal_lengths, BYTE no_fill, BYTE renamable, DWORD fill_selected,
                                   DWORD fill, DWORD frame_selected, DWORD frame, int text_colour_selected,
                                   int text_colour)
{
    struct UITabControl *self = (struct UITabControl *)wnd;
    struct String *name;
    struct StdBitIter it;
    void *mem;

    ui_window_on_create_base(&self->base, 0, 0);
    self->side = side;
    self->thickness = thickness;
    self->label_margin = padding;
    self->equal_lengths = equal_lengths;
    self->no_fill = no_fill;
    self->renamable = renamable;
    self->fill_selected = fill_selected ? fill_selected : 0xffffff;
    self->fill = fill ? fill : 0xffffff;
    self->frame_selected = frame_selected ? frame_selected : 0xc3c3c3;
    self->frame = frame ? frame : 0xc3c3c3;
    self->text_colour_selected = text_colour_selected;
    self->text_colour = text_colour;
    for (name = self->names.first; name != (struct String *)self->names.last; name++)
        string_free(name);
    self->names.last = self->names.first;
    self->colours.last = self->colours.first;
    self->lengths.last = self->lengths.first;
    std_vector_bool_erase(&self->new_marks, &it, self->new_marks.words.first, 0,
                          (unsigned *)self->new_marks.words.first + self->new_marks.size / 32,
                          self->new_marks.size % 32);
    self->selected = 0;
    self->font_height = 12;
    ui_tab_control_fit_size(self);

    mem = mem_alloc(sizeof(struct UIEditCtrl));
    self->edit = mem ? ui_edit_ctrl2_construct(mem) : NULL;
    ui_window_create(self->edit, 100, 0x13);
    ui_window_move(self->edit, EDIT_HIDDEN, EDIT_HIDDEN);
    ui_window_add_child(&self->base, self->edit);
}

void ui_tab_control_add_tab(struct UIWindow *wnd, const char *name)
{
    struct UITabControl *self = (struct UITabControl *)wnd;
    struct StdBitIter it;
    BYTE marked = 0;

    self->selected = 0;
    string_vec_push_text(&self->names, name, strlen(name));
    *(int *)vec_add(&self->colours, 1, sizeof(int)) = -1;
    *(int *)vec_add(&self->lengths, 1, sizeof(int)) = name_extent(self, name) + self->label_margin;
    std_vector_bool_insert(&self->new_marks, &it,
                           (unsigned *)self->new_marks.words.first + self->new_marks.size / 32,
                           self->new_marks.size % 32, 1, &marked);
    ui_tab_control_relayout(self);
}

static void fill_tab(struct UITabControl *self, int i, int x, int y, int width, int height, DWORD colour)
{
    struct UIWindow *wnd = &self->base;
    int own;

    if (self->no_fill)
        return;
    own = ((int *)self->colours.first)[i];
    if (own != -1)
        colour = own;
    switch (self->side) {
    case UI_TABS_TOP:
        ui_window_fill_rect(wnd, x + 1, y + 1, width - 2, height - 1, colour);
        break;
    case UI_TABS_BOTTOM:
        ui_window_fill_rect(wnd, x + 1, y, width - 2, height - 1, colour);
        break;
    case UI_TABS_LEFT:
        ui_window_fill_rect(wnd, x + 1, y + 1, width - 1, height - 2, colour);
        break;
    case UI_TABS_RIGHT:
        ui_window_fill_rect(wnd, x, y + 1, width - 1, height - 2, colour);
        break;
    }
}

static void frame_tab(struct UITabControl *self, int x, int y, int width, int height, DWORD colour)
{
    struct UIWindow *wnd = &self->base;

    switch (self->side) {
    case UI_TABS_TOP:
        ui_window_fill_rect(wnd, x + 2, y, width - 4, 1, colour);
        ui_window_fill_rect(wnd, x + 1, y + 1, 1, 1, colour);
        ui_window_fill_rect(wnd, x, y + 2, 1, height - 2, colour);
        ui_window_fill_rect(wnd, x + width - 2, y + 1, 1, 1, colour);
        ui_window_fill_rect(wnd, x + width - 1, y + 2, 1, height - 2, colour);
        break;
    case UI_TABS_BOTTOM:
        ui_window_fill_rect(wnd, x, y, 1, height - 2, colour);
        ui_window_fill_rect(wnd, x + 1, y + height - 2, 1, 1, colour);
        ui_window_fill_rect(wnd, x + width - 1, y, 1, height - 2, colour);
        ui_window_fill_rect(wnd, x + 2, y + height - 1, width - 4, 1, colour);
        ui_window_fill_rect(wnd, x + width - 2, y + height - 2, 1, 1, colour);
        break;
    case UI_TABS_LEFT:
        ui_window_fill_rect(wnd, x + 2, y, width - 2, 1, colour);
        ui_window_fill_rect(wnd, x + 1, y + 1, 1, 1, colour);
        ui_window_fill_rect(wnd, x, y + 2, 1, height - 4, colour);
        ui_window_fill_rect(wnd, x + 1, y + height - 2, 1, 1, colour);
        ui_window_fill_rect(wnd, x + 2, y + height - 1, width - 2, 1, colour);
        break;
    case UI_TABS_RIGHT:
        ui_window_fill_rect(wnd, x, y, width - 2, 1, colour);
        ui_window_fill_rect(wnd, x + width - 2, y + 1, 1, 1, colour);
        ui_window_fill_rect(wnd, x + width - 1, y + 2, 1, height - 4, colour);
        ui_window_fill_rect(wnd, x, y + height - 1, width - 2, 1, colour);
        ui_window_fill_rect(wnd, x + width - 2, y + height - 2, 1, 1, colour);
        break;
    }
}

static int centre(int room)
{
    return room / 2 + room % 2;
}

static void draw_tab_down(struct UITabControl *self, int i)
{
    struct UIWindow *wnd = &self->base;
    const char *name = string_cstr(NAME(self, i));
    int length = LENGTH(self, i);
    int extent = ui_window_vertical_text_length(wnd, name, 0, 0, self->font_height, 0);
    int top = tab_offset(self, i);
    int text_x = (wnd->width - 12) / 2;
    int text_y = centre(length - extent) + top;

    if (self->selected == i) {
        fill_tab(self, i, 0, top, wnd->width, length, self->fill_selected);
        frame_tab(self, 0, top, wnd->width, length, self->frame_selected);
        ui_window_text_out_down(wnd, text_x, text_y, string_cstr(NAME(self, i)), 0, 0, self->font_height,
                             self->text_colour_selected);
        return;
    }
    if (self->side == UI_TABS_LEFT) {
        fill_tab(self, i, 1, top, wnd->width, length, self->fill);
        frame_tab(self, 1, top, wnd->width, length, self->frame);
        ui_window_fill_rect(wnd, wnd->width - 1, top, 1, length, self->frame_selected);
    } else if (self->side == UI_TABS_RIGHT) {
        fill_tab(self, i, 0, top, wnd->width - 1, length, self->fill);
        frame_tab(self, 0, top, wnd->width - 1, length, self->frame);
        ui_window_fill_rect(wnd, 0, top, 1, length, self->frame_selected);
    }
    ui_window_text_out_down(wnd, text_x + 1, text_y, string_cstr(NAME(self, i)), 0, 0, self->font_height,
                         self->text_colour);
}

static void draw_tab_across(struct UITabControl *self, int i)
{
    struct UIWindow *wnd = &self->base;
    const char *name = string_cstr(NAME(self, i));
    int length = LENGTH(self, i);
    int extent = ui_window_text_width(wnd, name, 0, 0, self->font_height, 0, 0);
    int left = tab_offset(self, i);
    int text_x = centre(length - extent) + left;
    int text_y = (wnd->height - 12) / 2;

    if (self->selected == i) {
        fill_tab(self, i, left, 0, length, wnd->height, self->fill_selected);
        frame_tab(self, left, 0, length, wnd->height, self->frame_selected);
        ui_window_text_out(wnd, text_x, text_y, string_cstr(NAME(self, i)), 0, 0, self->font_height,
                         self->text_colour_selected, 0, 0);
        return;
    }
    if (self->side == UI_TABS_TOP) {
        fill_tab(self, i, left, 1, length, wnd->height, self->fill);
        frame_tab(self, left, 1, length, wnd->height, self->frame);
        ui_window_fill_rect(wnd, left, wnd->height - 1, length, 1, self->frame_selected);
    } else if (self->side == UI_TABS_BOTTOM) {
        fill_tab(self, i, left, 0, length, wnd->height - 1, self->fill);
        frame_tab(self, left, 0, length, wnd->height - 1, self->frame);
        ui_window_fill_rect(wnd, left, 0, length, 1, self->frame_selected);
    }
    ui_window_text_out(wnd, text_x, text_y + 1, string_cstr(NAME(self, i)), 0, 0, self->font_height,
                     self->text_colour, 0, 0);
}

void ui_tab_control_on_draw(struct UIWindow *window)
{
    struct UITabControl *self = (struct UITabControl *)window;
    struct UIWindow *wnd = &self->base;
    int i;

    ui_window_clear_canvas(wnd, UI_TRANSPARENT);
    if (wnd->parent)
        ui_window_send_msg(wnd->parent, wnd, UI_MSG_TAB_RENAMED, 0, 0, 0, 0);
    for (i = 0; i < (int)NAME_COUNT(self); i++) {
        switch (self->side) {
        case UI_TABS_TOP:
        case UI_TABS_BOTTOM:
            draw_tab_across(self, i);
            break;
        case UI_TABS_LEFT:
        case UI_TABS_RIGHT:
            draw_tab_down(self, i);
            break;
        }
        if (self->new_marks.size >= NAME_COUNT(self)
            && (((unsigned *)self->new_marks.words.first)[(unsigned)i >> 5] & (1u << (i & 31))))
            ui_window_draw_bitmap(wnd, 1, (WORD)tab_offset(self, i) + 2,
                                ui_skin_bitmap(UI_SKIN "basic_interface\\icon_new.bmp"), 0);
    }
}

void ui_tab_control_drag_and_drop(struct UIWindow *window, int x, int y, const struct DragInfo *drag)
{
    struct UITabControl *self = (struct UITabControl *)window;
    if (self->base.parent)
        ui_window_drag_and_drop(self->base.parent, x, y, drag);
}

static void close_edit(struct UITabControl *self)
{
    ui_edit_ctrl_set_text((struct UIEditCtrl *)self->edit, "");
    ui_window_move(self->edit, EDIT_HIDDEN, EDIT_HIDDEN);
    if (ui_window_mgr_get_focus_edit(&g_windowMgr) == self->edit)
        ui_window_mgr_focus_edit_lost(&g_windowMgr);
}

void ui_tab_control_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UITabControl *self = (struct UITabControl *)window;
    int along = vertical(self) ? y : x;
    int count = (int)NAME_COUNT(self);
    int i;

    for (i = 0; i < count; i++) {
        along -= LENGTH(self, i);
        if (along < 0)
            break;
    }
    if (i == count || i == -1)
        return;
    if (i < count && self->selected != i)
        self->selected = i;
    ui_edit_ctrl_set_text((struct UIEditCtrl *)self->edit, "");
    ui_window_move(self->edit, EDIT_HIDDEN, EDIT_HIDDEN);
    if (self->base.parent) {
        ui_window_send_msg(self->base.parent, &self->base, UI_MSG_TAB_CHANGED, self->selected, 0, 0, 0);
        ui_window_invalidate(self->base.parent);
    }
    ui_window_invalidate_base(&self->base);
    ui_window_mgr_set_capture(&g_windowMgr, &self->base);
}

void ui_tab_control_on_l_btn_dbl_clk(struct UIWindow *window, int x_unused, int y_unused)
{
    struct UITabControl *self = (struct UITabControl *)window;
    int sel = self->selected;

    if (vertical(self) || !self->renamable)
        return;
    ui_edit_ctrl_set_text((struct UIEditCtrl *)self->edit, string_cstr(NAME(self, sel)));
    ui_window_resize(self->edit, LENGTH(self, sel) - 4, self->base.height - 2);
    ui_window_move(self->edit, tab_offset(self, self->selected) + 2, 2);
    ui_window_mgr_set_focus_edit(&g_windowMgr, self->edit);
}

void ui_tab_control_on_mouse_move(struct UIWindow *window, int x, int y)
{
    struct UITabControl *self = (struct UITabControl *)window;
    if (ui_window_mgr_get_capture(&g_windowMgr) != &self->base)
        return;
    if ((self->drag_x || self->drag_y) && self->drag_x - x <= 5 && self->drag_y - y <= 5)
        return;
    if (self->base.parent)
        ui_window_send_msg(self->base.parent, &self->base, UI_MSG_TAB_DRAGGED, self->selected, 0, 0, 0);
    self->drag_x = x;
    self->drag_y = y;
}

void ui_tab_control_on_l_btn_up(struct UIWindow *self, int x_unused, int y_unused)
{
    ui_window_mgr_release_capture(&g_windowMgr);
}

int ui_tab_control_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1, int val2,
                                    int val3, int val4)
{
    struct UITabControl *self = (struct UITabControl *)window;
    struct String name;

    if (msg != UI_MSG_DEFAULT_BUTTON)
        return 0;
    if (sender != self->edit || self->edit->x == EDIT_HIDDEN) {
        if (self->base.parent)
            ui_window_send_msg(self->base.parent, &self->base, UI_MSG_DEFAULT_BUTTON, 0, 0, 0, 0);
        return 0;
    }
    if (val1 != 1) {
        string_init(&name);
        string_assign_cstr(&name, ui_edit_ctrl_get_text(self->edit));
        if (!name.size) {
            string_free(&name);
            return 0;
        }
        if ((ui_window_text_width(&self->base, string_cstr(&name), 0, 0, self->font_height, 0, 0)
             + self->label_margin) * (int)NAME_COUNT(self) >= self->base.parent->width - 0x5a) {
            ui_message_box(msg_str(MSG_NAME_TOO_LONG));
        } else {
            int old_width;

            string_assign(NAME(self, self->selected), string_cstr(&name), name.size);
            old_width = self->base.width;
            ui_tab_control_relayout(self);
            if (self->base.parent)
                ui_window_send_msg(self->base.parent, &self->base, UI_MSG_TAB_RENAMED, (int)string_cstr(&name),
                         self->selected, old_width > self->base.width ? old_width : 0, 0);
        }
        string_free(&name);
    }
    close_edit(self);
    ui_window_invalidate_base(&self->base);
    return 0;
}

void ui_tab_control_select_tab(struct UIWindow *wnd, int tab)
{
    struct UITabControl *self = (struct UITabControl *)wnd;

    if (tab < (int)NAME_COUNT(self) && self->selected != tab)
        self->selected = tab;
    ui_edit_ctrl_set_text((struct UIEditCtrl *)self->edit, "");
    ui_window_move(self->edit, EDIT_HIDDEN, EDIT_HIDDEN);
    if (self->base.parent) {
        ui_window_send_msg(self->base.parent, &self->base, UI_MSG_TAB_CHANGED, self->selected, 0, 0, 0);
        ui_window_invalidate(self->base.parent);
    }
    ui_window_invalidate_base(&self->base);
}

int ui_tab_control_tab_count(struct UITabControl *self)
{
    return (int)NAME_COUNT(self);
}

int ui_tab_control_tab_at(struct UITabControl *self, int x, int y)
{
    int along = vertical(self) ? y : x;
    int count = (int)NAME_COUNT(self), i;

    for (i = 0; i < count; i++) {
        along -= LENGTH(self, i);
        if (along < 0)
            return i;
    }
    return -1;
}

void ui_tab_control_set_new_mark(struct UITabControl *self, int tab, BYTE on)
{
    unsigned *word;
    unsigned bit;

    if (tab < 0 || tab >= (int)NAME_COUNT(self))
        return;
    word = (unsigned *)self->new_marks.words.first + ((unsigned)tab >> 5);
    bit = 1u << (tab & 0x1f);
    *word = on ? (*word | bit) : (*word & ~bit);
}

void ui_tab_control_remove_tab(struct UIWindow *wnd, int index)
{
    struct UITabControl *self = (struct UITabControl *)wnd;
    int count, i;
    int *colours, *lengths;
    struct String *names;
    struct StdBitIter it;

    close_edit(self);
    count = (int)NAME_COUNT(self);
    for (i = 0; i < count && i != index; i++)
        ;
    if (i >= count)
        return;
    colours = self->colours.first;
    memmove(colours + i, colours + i + 1, (BYTE *)self->colours.last - (BYTE *)(colours + i + 1));
    self->colours.last = (int *)self->colours.last - 1;
    names = self->names.first;
    string_vec_erase(&self->names, names + i, names + i + 1);
    lengths = self->lengths.first;
    memmove(lengths + i, lengths + i + 1, (BYTE *)self->lengths.last - (BYTE *)(lengths + i + 1));
    self->lengths.last = (int *)self->lengths.last - 1;
    std_vector_bool_erase_at(&self->new_marks, &it, (unsigned *)self->new_marks.words.first + (i >> 5),
                             i & 0x1f);
    self->selected = 0;
    ui_tab_control_fit_size(self);
}
