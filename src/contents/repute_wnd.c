#include "ui/controls/buttons.h"
#include "core/memory.h"
#include <string.h>
#include "contents/repute_wnd.h"
#include "contents/client_res.h"
#include "core/object_type_tables.h"
#include "core/msg_str.h"
#include "core/text_util.h"

#define REPUTE_UI_DIR "유저인터페이스\\"
#define REPUTE_BMP(file) REPUTE_UI_DIR "Reputation\\" file

#define ROW_COLUMN_STEP 0x13c
#define ROW_STEP        0x56
#define LIGHT_STEP      0xe
#define BAR_WIDTH       299

static void *res_bitmap(const char *name)
{
    return res_mgr_get(g_resMgr(), name);
}

struct UIReputeWnd *repute_wnd_construct(struct UIReputeWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &repute_wnd_ops;
    self->group_combo = NULL;
    self->search_edit = NULL;
    self->rows_per_page = REPUTE_ROWS_PER_PAGE;
    self->page = 1;
    self->page_count = 1;
    self->show_points = 0;
    self->group_points = 0;
    self->entries.first = self->entries.last = self->entries.end = NULL;
    return self;
}

struct UIWindow *repute_wnd_new(void)
{
    return (struct UIWindow *)repute_wnd_construct(mem_alloc(sizeof(struct UIReputeWnd)));
}

void repute_wnd_destruct(struct UIReputeWnd *self)
{
    self->frame.base.ops = &repute_wnd_ops;
    vec_free(&self->entries);
    ui_frame_wnd_destruct(&self->frame);
}

void repute_wnd_delete(struct UIWindow *window)
{
    struct UIReputeWnd *self = (struct UIReputeWnd *)window;
    repute_wnd_destruct(self);
    mem_free(self);
}

struct Vec *repute_wnd_entries_move(struct Vec *self,
                                                     struct Vec *from)
{
    if (self != from) {
        vec_free(self);
        *self = *from;
        from->first = from->last = from->end = NULL;
    }
    return self;
}

static struct UIWindow *new_bitmap_button(struct UIWindow **slot, const char *normal, const char *over,
                                          const char *press)
{
    struct UIWindow *button = (struct UIWindow *)mem_alloc(sizeof(struct UIBitmapButton));

    if (button)
        button = (struct UIWindow *)ui_bitmap_button_construct((struct UIBitmapButton *)button);
    if (slot)
        *slot = button;
    ui_bitmap_button_set_bitmap((struct UIBitmapButton *)button, normal, UI_BTN_UP, 0);
    ui_bitmap_button_set_bitmap((struct UIBitmapButton *)button, over, UI_BTN_HOVER, 0);
    ui_bitmap_button_set_bitmap((struct UIBitmapButton *)button, press, UI_BTN_DOWN, 0);
    ui_window_create(button, ((struct UIBitmapButton *)button)->bitmap_width,
                     ((struct UIBitmapButton *)button)->bitmap_height);
    return button;
}

static void add_arrow(struct UIReputeWnd *self, struct UIWindow **slot, const char *bitmap, int x,
                      int tip, int id)
{
    struct UIWindow *arrow = new_bitmap_button(slot, bitmap, bitmap, bitmap);

    ui_window_move_base(arrow, x, 0x192);
    ui_button_show((struct UIButton *)(arrow), 0);
    ui_bitmap_button_set_tool_tip((struct UIBitmapButton *)arrow, msg_str(tip));
    arrow->id = id;
    ui_window_add_child(&self->frame.base, arrow);
}

static void combo_add(struct UIWindow *combo, const char *text)
{
    struct String item;

    string_init(&item);
    string_assign_cstr(&item, text);
    ui_combo_box_def2_wnd_add_item(combo, item, 0);
}

void repute_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UIReputeWnd *self = (struct UIReputeWnd *)window;
    struct UIWindow *w;
    struct UIComboBoxDefWnd *combo;
    struct UIEditCtrl *edit;
    struct Vec names, entries;
    struct String *name;
    const char *caption;

    (void)width;
    (void)height;
    ui_window_add_sys_button(&self->frame.base);
    ui_window_add_close_button(&self->frame.base, "", 0);

    w = mem_alloc(sizeof(struct UIComboBoxDefWnd));
    self->group_combo = w ? ui_combo_box_def_wnd_construct(w) : NULL;
    combo = (struct UIComboBoxDefWnd *)self->group_combo;
    combo->list_height = 0xa0;
    ui_window_create(self->group_combo, 0x7a, 0x12);
    ui_window_move(self->group_combo, 8, 0x19);
    combo->back_r = combo->back_g = combo->back_b = 0xef;
    combo->border_r = combo->border_g = combo->border_b = 0x63;
    ui_window_add_child(&self->frame.base, self->group_combo);

    w = (struct UIWindow *)mem_alloc(sizeof(struct UIEditCtrl));
    self->search_edit = w ? ui_edit_ctrl_construct(w) : NULL;
    ui_window_create(self->search_edit, 0xc8, 0x12);
    ui_window_move(self->search_edit, 0x88, 0x19);
    edit = (struct UIEditCtrl *)self->search_edit;
    edit->max_chars = 0x14;
    edit->keep_ime_on_enter = 1;
    ui_edit_ctrl_set_back_colour(self->search_edit, 0xef, 0xef, 0xef);
    ui_edit_ctrl_set_border_colour(self->search_edit, 0x63, 0x63, 0x63);
    edit->no_border = 0;
    ui_window_add_child(&self->frame.base, self->search_edit);

    w = new_bitmap_button(NULL, REPUTE_BMP("btn_search.bmp"), REPUTE_BMP("btn_search_over.bmp"),
                          REPUTE_BMP("btn_search_press.bmp"));
    caption = msg_str(0xec1);
    string_assign_cstr(&((struct UIButton *)w)->text, caption);
    ui_window_move(w, 0x150, 0x19);
    w->id = REPUTE_SEARCH_ID;
    ui_window_add_child(&self->frame.base, w);
    self->frame.default_id = REPUTE_SEARCH_ID;

    add_arrow(self, &self->prev_button, REPUTE_BMP("arrow_prev.bmp"), 0x237, 0x41d, REPUTE_PREV_ID);
    add_arrow(self, &self->next_button, REPUTE_BMP("arrow_next.bmp"), 0x26f, 0x41e, REPUTE_NEXT_ID);

    w = (struct UIWindow *)mem_alloc(sizeof(struct UIBasicButton));
    if (w)
        w = (struct UIWindow *)ui_basic_button_construct((struct UIBasicButton *)w);
    ui_basic_button_set_label(w, 0x2a, msg_str(0xbe5));
    ui_window_move(w, 0x24d, 0x1a9);
    w->id = REPUTE_CLOSE_ID;
    ui_window_add_child(&self->frame.base, w);

    combo_add(self->group_combo, msg_str(0xef5));
    repute_mgr_group_names(repute_mgr_instance(), &names);
    for (name = (struct String *)names.first; name != (struct String *)names.last; name++) {
        struct String item;

        string_copy(&item, name);
        ui_combo_box_def2_wnd_add_item(self->group_combo, item, 0);
    }
    ui_combo_box_def_wnd_select(self->group_combo, 0);

    repute_mgr_all_infos(repute_mgr_instance(), &entries);
    repute_wnd_entries_move(&self->entries, &entries);
    vec_free(&entries);
    repute_wnd_set_page(self, 1);
    string_vec_free(&names);
}

static void draw_lights(struct UIReputeWnd *self, int x, int y, const char *bitmap,
                        const long long *span, const long long *value)
{
    unsigned long long i;

    for (i = 0; span ? i < (unsigned long long)*span / REPUTE_POINTS_PER_LIGHT
                     : i < (unsigned long long)(*value / REPUTE_POINTS_PER_LIGHT); i++)
        ui_window_draw_bitmap(&self->frame.base, x + (int)i * LIGHT_STEP, y, res_bitmap(bitmap), 1);
}

static void draw_rest(struct UIReputeWnd *self, int x, int y, unsigned long long rest, unsigned color)
{
    struct String text;

    ui_window_fill_rect(&self->frame.base, x + 0xc, y + 0x64,
                      (int)((float)(double)rest / 1000.0f * (float)BAR_WIDTH), 6, color);
    string_init(&text);
    string_format(&text, msg_str(0xef4), (long long)rest);
    ui_window_text_out_right(&self->frame.base, x + 0x137, y + 0x72, string_cstr(&text), 0, 0, 0xc, 0, 1);
    string_free(&text);
}

static void draw_row(struct UIReputeWnd *self, const struct ReputeInfo *entry, unsigned slot)
{
    int x = (int)(slot & 1) * ROW_COLUMN_STEP;
    int y = (int)(slot >> 1) * ROW_STEP;
    long long value = entry->points;

    ui_window_draw_bitmap(&self->frame.base, x + 7, y + 0x32,
                        res_bitmap(entry->highlighted == 1 ? REPUTE_BMP("bg_highlight.bmp")
                                                         : REPUTE_BMP("bg_info.bmp")), 0);
    ui_window_text_out(&self->frame.base, x + 0xc, y + 0x38, string_cstr(&entry->name), 0, 0, 0xc, 0, 1, 0);

    if (value < 0) {
        long long magnitude = -value;
        long long lights;

        draw_lights(self, x + 0xc, y + 0x48, REPUTE_BMP("light_empty.bmp"), &entry->max_point_negative, NULL);
        lights = magnitude / REPUTE_POINTS_PER_LIGHT;
        draw_lights(self, x + 0xc, y + 0x48, REPUTE_BMP("light_red.bmp"), NULL, &magnitude);
        draw_rest(self, x, y, (unsigned long long)(magnitude - lights * REPUTE_POINTS_PER_LIGHT), 0xff);
    } else {
        draw_lights(self, x + 0xc, y + 0x48, REPUTE_BMP("light_empty.bmp"), &entry->max_point_positive, NULL);
        draw_lights(self, x + 0xc, y + 0x48, REPUTE_BMP("light_blue.bmp"), NULL, &entry->points);
        draw_rest(self, x, y, (unsigned long long)(entry->points % REPUTE_POINTS_PER_LIGHT), 0xcd9279);
    }
}

void repute_wnd_on_draw(struct UIWindow *window)
{
    struct UIReputeWnd *self = (struct UIReputeWnd *)window;
    struct ReputeMgr *mgr;
    struct String text, separator;
    int title, bar, sep_width;
    unsigned i;

    ui_window_clear_canvas(&self->frame.base, 0xffffffff);
    title = ui_window_draw_title_bar(&self->frame.base, 0, msg_str(0xef3), 0);
    bar = ui_window_draw_button_bar(&self->frame.base, 2, 0, 1);
    ui_window_draw_outline(&self->frame.base, 0, title - 1, self->frame.base.width, self->frame.base.height - bar - title + 2, 0xc6c6c6);
    ui_window_draw_outline(&self->frame.base, 0, title, self->frame.base.width, 1, 0);

    mgr = repute_mgr_instance();
    if (mgr->no_info == 1) {
        ui_window_draw_round_frame(&self->frame.base, 0xe1, 0x9b, 0xbe, 0x8c, 0xce9484);
        ui_window_fill_round_rect(&self->frame.base, 0xe2, 0x9c, 0xbc, 0x8a, 0xffe7de);
        ui_window_fill_round_rect(&self->frame.base, 0xe7, 0xa1, 0xb2, 0x80, 0xffffff);
        ui_window_draw_bitmap(&self->frame.base, 0x120, 0xb6,
                            res_bitmap(REPUTE_UI_DIR "basic_interface\\notice_poring.bmp"), 1);
        ui_window_text_out_centred(&self->frame.base, 0xe1, 0x104, msg_str(0xef6), 0xbe, 0, 0xf, 0xc89482, 1);
        return;
    }

    for (i = 0; i < self->rows_per_page; i++) {
        unsigned index = (self->page - 1) * self->rows_per_page + i;
        const struct ReputeInfo *entry;

        if (index >= VEC_COUNT(&self->entries, struct ReputeInfo *))
            break;
        entry = ((struct ReputeInfo **)self->entries.first)[index];
        if (entry)
            draw_row(self, entry, i);
    }

    if (self->show_points == 1) {
        ui_window_fill_rect(&self->frame.base, 0x177, 0x1a, 0x100, 0x10, 0xefefef);
        ui_window_draw_outline(&self->frame.base, 0x176, 0x19, 0x102, 0x12, 0x636363);
        ui_window_text_out(&self->frame.base, 0x179, 0x1d, "GROUP :", 0, 0, 0xc, 0, 1, 0);
        string_init(&text);
        string_format(&text, "%I64d P", self->group_points);
        ui_window_text_out_right(&self->frame.base, 0x273, 0x1d, string_cstr(&text), 0, 0, 0xc, 0, 1);
        string_free(&text);
        ui_window_draw_bitmap(&self->frame.base, 0x1b7, 0x1c,
                            res_bitmap(self->group_points == 0 ? REPUTE_BMP("light_empty.bmp")
                                       : self->group_points > 0 ? REPUTE_BMP("light_blue.bmp")
                                                                : REPUTE_BMP("light_red.bmp")), 1);
    }

    string_init(&separator);
    string_assign_cstr(&separator, " / ");
    sep_width = ui_window_text_width(&self->frame.base, string_cstr(&separator), 0, 0, 0xc, 0, 0);
    ui_window_text_out_centred(&self->frame.base, 0x23e, 0x192, string_cstr(&separator), 0x31, 0, 0xc, 0, 1);
    string_init(&text);
    string_format(&text, "%u", self->page);
    ui_window_text_out_right(&self->frame.base, (int)((double)(unsigned)(0x31 - sep_width) * 0.5 + 574.0), 0x192,
                          string_cstr(&text), 0, 0, 0xc, 0, 1);
    string_free(&text);
    string_format(&text, "%u", self->page_count);
    ui_window_text_out(&self->frame.base, (int)((double)(unsigned)(sep_width + 0x31) * 0.5 + 574.0), 0x192,
                     string_cstr(&text), 0, 0, 0xc, 0, 1, 0);
    string_free(&text);
    string_free(&separator);
}
