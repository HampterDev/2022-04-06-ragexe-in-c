#include "ui/items/refining_internal.h"
#include "ui/draw.h"
#include "core/msg_str.h"
#include "core/text_util.h"

static void material_record(struct ItemInfo *out, int item_id)
{
    item_info_construct(out);
    item_info_set_item_id(out, item_id);
    out->count = refining_carried(item_id);
}

static void draw_count(struct UIRefiningWnd *self, int x, const char *fmt, BYTE short_of, int count, int need)
{
    struct String line;

    string_init(&line);
    string_format(&line, fmt, short_of ? "^CA152E" : "", count, need);
    ui_window_text_out_line(WND(self), x, 0x6b, &line, 0x30, 6, 0, 0xc, 0, 0);
    string_free(&line);
}

static void draw_centred_caption(struct UIRefiningWnd *self, const char *text)
{
    int w = ui_window_text_width(WND(self), text, 0, 0, 0xc, 1, 0);

    ui_window_text_out(WND(self), (int)((double)(WND(self)->width - w) * 0.5), 0x1f, text, 0, 0, 0xc,
                     COLOUR_CHANCE, 1, 0);
}

void ui_refining_wnd_on_draw(struct UIWindow *window)
{
    struct UIRefiningWnd *self = (struct UIRefiningWnd *)window;
    struct UIWindow *wnd = WND(self);
    struct String bg, path;
    struct ItemInfo record;
    BYTE have_path = 1;

    ui_window_clear_canvas(wnd, UI_TRANSPARENT);
    ui_window_draw_outline(wnd, 0, 0x11, 0x108, 0x12e, COLOUR_FRAME);
    string_init(&bg);
    switch (self->state) {
    case REFINE_WAIT:
        string_init(&path);
        string_format(&path, REFINE_DIR "bg_refining_wait_%02d.bmp", self->anim_frame);
        break;
    case REFINE_READY:
        string_init(&path);
        string_format(&path, REFINE_DIR "bg_refining%s_ready_%02d.bmp", self->blessing_count > 0 ? "A" : "B",
                      self->anim_frame);
        break;
    case REFINE_PROCESS:
        string_init(&path);
        if (self->anim_frame <= 8)
            string_format(&path, REFINE_DIR "bg_refining%s_process_%02d.bmp", self->blessing == 1 ? "A" : "B",
                          self->anim_frame);
        else
            string_format(&path, REFINE_DIR "bg_refining_process_%02d.bmp", self->anim_frame);
        break;
    case REFINE_SUCCESS:
        string_init(&path);
        string_format(&path, REFINE_DIR "bg_refining_success_%02d.bmp", self->anim_frame);
        break;
    case REFINE_FAILURE:
    case REFINE_DOWNGRADE:
    case REFINE_FAILURE_KEPT:
        string_init(&path);
        string_format(&path, REFINE_DIR "bg_refining_fail_%02d.bmp", self->anim_frame);
        break;
    default:
        have_path = 0;
        break;
    }
    if (have_path) {
        string_assign(&bg, string_cstr(&path), path.size);
        string_free(&path);
    }
    ui_window_draw_bitmap(wnd, 1, 0x11, res_mgr_get(g_resMgr(), string_cstr(&bg)), 0);

    if (self->state == REFINE_READY) {
        struct RefineMaterial *m;
        int i = 0, x = 0xa;

        if (self->selected >= 0) {
            string_init(&path);
            string_format(&path, REFINE_DIR "slot_select_blue%d.bmp", self->selected);
            ui_window_draw_bitmap(wnd, self->selected * 0x31 + 0xb, 0x37,
                                res_mgr_get(g_resMgr(), string_cstr(&path)), 0);
            string_free(&path);
        }
        if (self->blessing == 1)
            ui_window_draw_bitmap(wnd, 0xcf, 0x37,
                                res_mgr_get(g_resMgr(), REFINE_DIR "slot_select_red.bmp"), 0);

        for (m = self->materials.first; m != self->materials.last; m++, i++, x += 0x31) {
            material_record(&record, m->item_id);
            ui_window_draw_item_icon(wnd, x + 0xc, 0x45, &record, 0);
            draw_count(self, x, "%s%d^000000/1", record.count == 0, record.count, 0);
            if (i == self->selected &&
                (self->state == REFINE_READY || (self->state == REFINE_PROCESS && self->anim_frame == 0)))
                ui_window_draw_item_icon(wnd, 0x51, 0x96, &record, 0);
            item_info_destruct(&record);
        }

        if (self->blessing_item > 0 && self->blessing_count > 0) {
            material_record(&record, self->blessing_item);
            ui_window_draw_item_icon(wnd, 0xda, 0x45, &record, 0);
            draw_count(self, 0xce, "%s%d^000000/%d", self->blessing_count > record.count, record.count,
                       self->blessing_count);
            if (self->blessing == 1 &&
                (self->state == REFINE_READY || (self->state == REFINE_PROCESS && self->anim_frame < 0xd)))
                ui_window_draw_item_icon(wnd, 0xa1, 0x96, &record, 0);
            item_info_destruct(&record);
        }
    }

    if (self->item.index) {
        ui_window_draw_item_icon(wnd, 0x78, 0xd4, &self->item, 0);
        if (self->item.grade > 0) {
            string_init(&path);
            string_format(&path, UI_SKIN "grade_enchant\\grade_icon%d.bmp", self->item.grade);
            ui_window_draw_bitmap(wnd, 0x78, 0xe0, res_mgr_get(g_resMgr(), string_cstr(&path)), 1);
            string_free(&path);
        }
    }

    switch (self->state) {
    case REFINE_READY:
    case REFINE_SUCCESS:
    case REFINE_DOWNGRADE:
    case REFINE_FAILURE_KEPT:
        string_copy(&path, &self->item.name);
        ui_window_text_out_centred(wnd, 0, self->state == REFINE_READY ? 0xf6 : 0xf9, string_cstr(&path),
                                0x108, 0, 0xc, COLOUR_NAME, 1);
        string_free(&path);
        break;
    case REFINE_FAILURE:
        ui_window_text_out_centred(wnd, 0, 0xf9, msg_str(MSG_DESTROYED), 0x108, 0, 0xc, COLOUR_NAME, 1);
        break;
    }

    switch (self->state) {
    case REFINE_WAIT:
    case REFINE_READY:
    case REFINE_PROCESS:
        if (self->state != REFINE_PROCESS || self->anim_frame <= 3) {
            struct String line, percent;
            int line_width, percent_width, x;

            string_init(&line);
            string_format(&line, "%s %d%%", msg_str(MSG_CHANCE), refining_selected_chance(self));
            line_width = ui_window_text_width(wnd, string_cstr(&line), 0, 0, 0xc, 1, 0);
            string_init(&percent);
            string_format(&percent, "%d%%", refining_selected_chance(self));
            percent_width = ui_window_text_width(wnd, string_cstr(&percent), 0, 0, 0xc, 1, 0);
            x = (int)((double)(wnd->width - line_width) * 0.5);
            ui_window_text_out(wnd, x, 0x1f, msg_str(MSG_CHANCE), 0, 0, 0xc, COLOUR_CHANCE, 1, 0);
            ui_window_text_out(wnd, x - percent_width + line_width, 0x1f, string_cstr(&percent), 0, 0, 0xc,
                             COLOUR_PERCENT, 1, 0);
            string_free(&percent);
            string_free(&line);
        }
        break;
    case REFINE_SUCCESS:
        draw_centred_caption(self, msg_str(MSG_SUCCESS));
        break;
    case REFINE_FAILURE:
    case REFINE_DOWNGRADE:
    case REFINE_FAILURE_KEPT:
        draw_centred_caption(self, msg_str(MSG_FAILURE));
        break;
    }

    ui_window_draw_button_bar(wnd, 2, 0, 1);
    ui_window_draw_title_bar(wnd, 0, msg_str(MSG_TITLE), 0);
    string_free(&bg);
}
