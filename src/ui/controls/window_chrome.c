#include "ui/window_mgr.h"
#include "ui/game.h"
#include "ui/ui_items.h"
#include "ui/controls/controls.h"
#include "ui/ui_std.h"
#include <mmsystem.h>
#include <stdlib.h>
#include <string.h>

#define UI_ID_MINIMISE 0xc2
#define BIG_CLOSE_OFF UI_SKIN "basic_interface\\big_sys_close_off.bmp"
#define BIG_CLOSE_ON UI_SKIN "basic_interface\\big_sys_close_on.bmp"
#define CLOSE_OFF UI_SKIN "basic_interface\\sys_close_off.bmp"
#define CLOSE_ON UI_SKIN "basic_interface\\sys_close_on.bmp"
#define SYS_OFF UI_SKIN "basic_interface\\sys_base_off.bmp"
#define SYS_ON UI_SKIN "basic_interface\\sys_base_on.bmp"
#define MINI_OFF UI_SKIN "basic_interface\\sys_mini_off.bmp"
#define MINI_ON UI_SKIN "basic_interface\\sys_mini_on.bmp"
#define CHECKBOX_OFF UI_SKIN "checkbox_0.bmp"
#define CHECKBOX_ON UI_SKIN "checkbox_1.bmp"

static struct UIBitmapButton *new_bitmap_button(void)
{
    struct UIBitmapButton *button = mem_alloc(sizeof *button);

    return button ? ui_bitmap_button_construct(button) : NULL;
}

static void title_button_bitmaps(struct UIBitmapButton *button, const char *off, const char *on)
{
    ui_bitmap_button_set_bitmap(button, off, 0, 0);
    ui_bitmap_button_set_bitmap(button, on, 1, 0);
    ui_bitmap_button_set_bitmap(button, on, 2, 0);
    ui_window_create(&button->button.base, button->bitmap_width, button->bitmap_height);
}

struct UIWindow *ui_window_add_sys_button(struct UIWindow *self)
{
    struct UIBitmapButton *button = new_bitmap_button();

    title_button_bitmaps(button, SYS_OFF, SYS_ON);
    ui_window_move_base(&button->button.base, 3, 3);
    ui_window_add_child(self, &button->button.base);
    return &button->button.base;
}

struct UIWindow *ui_window_add_close_button(struct UIWindow *self, const char *tooltip,
                                                    int width)
{
    struct UIBitmapButton *button;

    if (!width)
        width = self->width;
    button = new_bitmap_button();
    title_button_bitmaps(button, CLOSE_OFF, CLOSE_ON);
    ui_window_move_base(&button->button.base, width - 0xe, 3);
    button->button.base.id = UI_ID_CLOSE;
    ui_bitmap_button_set_tool_tip(button, tooltip);
    ui_window_add_child(self, &button->button.base);
    return &button->button.base;
}

struct UIBitmapButton *ui_window_add_big_close_button(struct UIWindow *self, const char *tooltip, int width)
{
    struct UIBitmapButton *button;

    if (!width)
        width = self->width;
    button = new_bitmap_button();
    title_button_bitmaps(button, BIG_CLOSE_OFF, BIG_CLOSE_ON);
    ui_window_move_base(&button->button.base, width - button->bitmap_width, 0);
    button->button.base.id = UI_ID_CLOSE;
    ui_bitmap_button_set_tool_tip(button, tooltip);
    ui_window_add_child(self, &button->button.base);
    return button;
}

struct UIWindow *ui_window_add_mini_button(struct UIWindow *self, const char *tooltip,
                                                   int width)
{
    struct UIBitmapButton *button;

    if (!width)
        width = self->width;
    button = new_bitmap_button();
    title_button_bitmaps(button, MINI_OFF, MINI_ON);
    ui_window_move_base(&button->button.base, width - 0x1c, 3);
    button->button.base.id = UI_ID_MINIMISE;
    ui_bitmap_button_set_tool_tip(button, tooltip);
    ui_window_add_child(self, &button->button.base);
    return &button->button.base;
}

static void suffixed_bitmap(struct UIBitmapButton *button, const struct String *base, const char *suffix,
                            int state)
{
    struct String path;

    string_copy(&path, base);
    string_append_cstr(&path, suffix);
    string_append_cstr(&path, ".bmp");
    ui_bitmap_button_set_bitmap(button, string_cstr(&path), state, 0);
    string_free(&path);
}

static struct UIBitmapButton *suffixed_button(struct UIWindow *self, struct String *base, const char *const *suffixes,
                                              int id, int x, int y, const char *tooltip)
{
    struct UIBitmapButton *button = new_bitmap_button();
    int state;

    for (state = 0; state < 3; state++)
        suffixed_bitmap(button, base, suffixes[state], state);
    ui_window_create(&button->button.base, button->bitmap_width, button->bitmap_height);
    ui_window_move_base(&button->button.base, x, y);
    button->button.base.id = id;
    ui_bitmap_button_set_tool_tip(button, tooltip);
    ui_window_add_child(self, &button->button.base);
    string_free(base);
    return button;
}

struct UIBitmapButton *ui_window_add_bitmap_button(struct UIWindow *self, int id,
                                                           int x, int y, const char *name, const char *tooltip)
{
    static const char *const suffixes[3] = { "_out", "_over", "_press" };
    struct String base;

    if (!name)
        return NULL;
    string_init(&base);
    string_assign(&base, UI_SKIN, 0xf);
    string_append_cstr(&base, name);
    return suffixed_button(self, &base, suffixes, id, x, y, tooltip);
}

struct UIBitmapButton *ui_window_add_numbered_button(struct UIWindow *self, int id,
                                                             int x, int y, const char *name, const char *tooltip,
                                                             BYTE skin)
{
    static const char *const suffixes[3] = { "_1", "_2", "_3" };
    struct String base;

    if (!name)
        return NULL;
    string_init(&base);
    if (skin)
        string_append(&base, UI_SKIN, 0xf);
    string_append_cstr(&base, name);
    return suffixed_button(self, &base, suffixes, id, x, y, tooltip);
}

struct UIWindow *ui_window_add_check_box(struct UIWindow *self, int id, int width,
                                                 int height, const char *label, int x, int y, BYTE checked,
                                                 const char *tooltip, int label_height, COLORREF label_colour,
                                                 const char *checked_bmp, const char *unchecked_bmp)
{
    struct UICheckBox *box = mem_alloc(sizeof(struct UICheckBox));

    box = box ? ui_check_box_construct(&box->base) : NULL;
    ui_window_create(&box->base, width, height);
    ui_check_box_set_bitmaps(&box->base, checked_bmp ? checked_bmp : CHECKBOX_ON,
                          unchecked_bmp ? unchecked_bmp : CHECKBOX_OFF, label);
    ui_window_move_base(&box->base, x, y);
    box->base.id = id;
    ui_check_box_set_check(&box->base, checked);
    ui_check_box_set_tool_tip(&box->base, tooltip);
    box->label_height = label_height;
    box->label_colour = label_colour;
    ui_window_add_child(self, &box->base);
    return &box->base;
}

struct UIWindow *ui_window_add_check_box_plain(struct UIWindow *self, int id, int width,
                                                      int height, const char *label, int x, int y, int checked,
                                                      const char *tooltip)
{
    struct UICheckBox *box = mem_alloc(sizeof(struct UICheckBox));

    box = box ? ui_check_box_construct(&box->base) : NULL;
    ui_window_create(&box->base, width, height);
    ui_check_box_set_bitmaps(&box->base, CHECKBOX_ON, CHECKBOX_OFF, label);
    ui_window_move_base(&box->base, x, y);
    box->base.id = id;
    ui_check_box_set_check(&box->base, checked);
    ui_check_box_set_tool_tip(&box->base, tooltip);
    ui_window_add_child(self, &box->base);
    return &box->base;
}

static COLORREF chip(int x, int y)
{
    int r, g, b;

    ui_skin_color_chip(&g_skinMgr, x, y, &r, &g, &b);
    return (BYTE)r | (BYTE)g << 8 | (DWORD)(BYTE)b << 16;
}

void ui_window_draw_body(struct UIWindow *self, int top, int width, const char *bitmap)
{
    COLORREF colour;

    if (strlen(bitmap)) {
        struct BitmapRes *bmp = ui_skin_bitmap(bitmap);

        if (bmp)
            ui_window_draw_bitmap(self, 0, top, bmp, 0);
        return;
    }
    if (!width)
        width = self->width;
    colour = chip(0xe, 6);
    ui_window_draw_round_frame(self, 0, top, width, self->height - top, colour);
    ui_window_fill_rect(self, 0, top, width, 5, colour);
    colour = chip(2, 2);
    ui_window_fill_rect(self, 1, top, width - 2, self->height - top - 2, colour);
    ui_window_fill_rect(self, 2, self->height - 2, width - 4, 1, colour);
}

static struct BitmapRes *bar_piece(int variant, const char *numbered, const char *plain)
{
    char path[0x104];

    if (variant) {
        sprintf(path, numbered, variant);
        return ui_skin_bitmap(path);
    }
    return ui_skin_bitmap(plain);
}

int ui_window_draw_button_bar(struct UIWindow *self, int variant, int width, int rows)
{
    struct BitmapRes *left, *right, *mid;
    int left_w = 0, right_w = 0, row_h = 0, tiles, i, j;

    if (!width)
        width = self->width;
    left = bar_piece(variant, UI_SKIN "basic_interface\\btnbar_left%d.bmp", UI_SKIN "basic_interface\\btnbar_left.bmp");
    if (left) {
        left_w = left->width;
        row_h = left->height;
        for (i = 1; i <= rows; i++)
            ui_window_draw_bitmap(self, 0, self->height - row_h * i, left, 0);
    }
    right = bar_piece(variant, UI_SKIN "basic_interface\\btnbar_right%d.bmp",
                      UI_SKIN "basic_interface\\btnbar_right.bmp");
    if (right) {
        right_w = right->width;
        for (i = 1; i <= rows; i++)
            ui_window_draw_bitmap(self, width - right_w, self->height - row_h * i, right, 0);
    }
    mid = bar_piece(variant, UI_SKIN "basic_interface\\btnbar_mid%d.bmp", UI_SKIN "basic_interface\\btnbar_mid.bmp");
    if (!mid)
        return 0;
    width -= right_w + left_w;
    tiles = width / mid->width + (width % mid->width != 0);
    for (i = 1; i <= rows; i++)
        for (j = 0; j < tiles; j++)
            ui_window_draw_bitmap(self, left_w + j * mid->width, self->height - row_h * i, mid, 0);
    return mid->height;
}
