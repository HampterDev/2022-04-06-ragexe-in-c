#include "world/tooltip.h"
#include "contents/player_names.h"
#include "render/rp_pool.h"
#include "core/object_type_tables.h"
#include "world/mode_manager.h"
#include "game/context.h"
#include <stdio.h>
#include "ui/shops/shops.h"
#include "ui/ui_std.h"
#include "ui/shops/shops_std.h"
#include "core/msg_str.h"
#include "core/skin_path.h"
#include "core/text_util.h"
#include "diag/perf.h"
#include "session/session_items.h"
#include "ui/items/items.h"

#define LOG_WND_ID_0 0x101
#define LOG_WND_ID_1 0x102
#define MSG_LOG_TITLE_0 0x9ff
#define MSG_LOG_TITLE_1 0xa00
#define MSG_LOG_NAME_PENDING 0x245
#define MSG_LOG_TIP 0xb7
#define MSG_LOG_TIP_OPTIONS 0x9f7
#define LOG_ID_CLOSE 0xc1
#define LOG_MSG_ADD 0x17
#define LOG_MIN_HEIGHT 0x78
#define LOG_MAX_HEIGHT 0x150

struct Vec;
struct UIMerchantItemLogWnd;
struct UIWindow;
void ui_merchant_item_log_wnd_destroy(struct UIWindow *window);
void ui_merchant_item_log_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_merchant_item_log_wnd_on_process(struct UIWindow *self);
void ui_merchant_item_log_wnd_on_draw(struct UIWindow *self);
void ui_merchant_item_log_wnd_on_mouse_hover(struct UIWindow *self, int x, int y);
void ui_merchant_item_log_wnd_on_r_btn_down(struct UIWindow *self, int x, int y);
int ui_merchant_item_log_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_merchant_item_log_wnd_ops = {
    .destroy = ui_merchant_item_log_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_item_base_wnd_drag_and_drop,
    .store_info = ui_item_base_wnd_store_info,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_merchant_item_log_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_merchant_item_log_wnd_on_process,
    .on_draw = ui_merchant_item_log_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_frame_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_frame_wnd_on_mouse_move,
    .on_mouse_hover = ui_merchant_item_log_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_frame_wnd_on_l_btn_up,
    .on_r_btn_down = ui_merchant_item_log_wnd_on_r_btn_down,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_item_base_wnd_on_wheel,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_merchant_item_log_wnd_send_msg,
    .invalidate = ui_window_invalidate_base,
    .render = ui_window_render_base,
    .has_custom_backdrop = 0,
    .draw_backdrop = ui_window_draw_backdrop_base,
    .get_records = ui_frame_wnd_get_records,
    .type = UI_TYPE_FRAME,
    .on_hover_enter = ui_window_on_hover_enter_base,
    .on_hover_leave = ui_window_on_hover_leave_base,
    .make_quad = ui_window_make_quad_base,
    .hit_test = ui_window_hit_test_base,
    .render_overlay = ui_window_render_overlay_base,
};

struct UIMerchantItemLogWnd {
    struct UIItemBaseWnd base;
    struct Vec entries;
    int variant;
};
OFFSET_ASSERT(UIMerchantItemLogWnd, entries, 0xd0);
SIZE_ASSERT(UIMerchantItemLogWnd, 0xe0);

void merchant_log_entry_destruct(struct MerchantLogEntry *self)
{
    string_free(&self->item);
    string_free(&self->name);
}

struct UIMerchantItemLogWnd *ui_merchant_item_log_wnd_construct(struct UIMerchantItemLogWnd *self, int variant)
{
    ui_item_base_wnd_construct(&self->base);
    self->base.frame.base.ops = &ui_merchant_item_log_wnd_ops;
    self->entries.first = self->entries.last = self->entries.end = NULL;
    self->variant = variant;
    return self;
}

struct UIWindow *ui_merchant_item_log_wnd_new(int arg)
{
    return (struct UIWindow *)ui_merchant_item_log_wnd_construct(mem_alloc(sizeof(struct UIMerchantItemLogWnd)), arg);
}

void ui_merchant_item_log_wnd_destruct(struct UIMerchantItemLogWnd *self)
{
    struct MerchantLogEntry *entry;

    self->base.frame.base.ops = &ui_merchant_item_log_wnd_ops;
    list_clear(&self->base.items, item_info_destroy);
    for (entry = self->entries.first; entry != self->entries.last; entry++)
        merchant_log_entry_destruct(entry);
    vec_free(&self->entries);
    ui_item_base_wnd_destruct(&self->base);
}

void ui_merchant_item_log_wnd_destroy(struct UIWindow *window)
{
    struct UIMerchantItemLogWnd *self = (struct UIMerchantItemLogWnd *)window;
    ui_merchant_item_log_wnd_destruct(self);
    mem_free(self);
}

static void place_controls(struct UIMerchantItemLogWnd *self)
{
    struct UIWindow *wnd = &self->base.frame.base;

    ui_window_move_base(&self->base.close->button.base, wnd->width - 0x3d, wnd->height - 0x18);
}

void ui_merchant_item_log_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UIMerchantItemLogWnd *self = (struct UIMerchantItemLogWnd *)window;
    struct UIWindow *wnd = &self->base.frame.base;
    struct UIWindow *title = mem_alloc(sizeof(struct UIStaticText));
    struct UIResizer *resizer;
    struct UIScrollBar *bar;
    struct UIBitmapButton *close;

    if (title)
        title = ui_static_text_construct(title);
    ui_window_create(title, 0xa0, 0xf);
    ui_static_text_set_text(title, msg_str(self->variant ? MSG_LOG_TITLE_1 : MSG_LOG_TITLE_0), 0);
    ui_window_move(title, 0x10, 2);
    ((struct UIStaticText *)title)->white_shadow = 1;
    ui_window_add_child(wnd, title);

    resizer = mem_alloc(sizeof *resizer);
    if (resizer)
        resizer = ui_resizer_construct(resizer);
    self->base.resizer = resizer;
    ui_window_create(&resizer->base, 0x10, 0x10);
    ui_resizer_set_edge(self->base.resizer, 3);
    ui_resizer_set_bitmap(self->base.resizer, UI_SKIN "btn_resize.bmp");
    ui_window_move_base(&self->base.resizer->base, wnd->width - 0xf, wnd->height - 0xf);
    ui_resizer_set_colours(self->base.resizer, 0xffff00ff, 0xffff00ff);
    ui_window_add_child(wnd, &self->base.resizer->base);

    bar = mem_alloc(sizeof *bar);
    if (bar)
        bar = ui_scroll_bar_construct(bar);
    self->base.scroll_bar = bar;
    ui_scroll_bar_create(bar, wnd->width - 0xf, 0xe, 1, wnd->height - 0x26);
    ui_window_add_child(wnd, &self->base.scroll_bar->base);

    close = mem_alloc(sizeof *close);
    if (close)
        close = ui_bitmap_button_construct(close);
    self->base.close = close;
    ui_bitmap_button_set_bitmap(close, UI_SKIN "btn_close.bmp", UI_BTN_UP, 0);
    ui_bitmap_button_set_bitmap(self->base.close, UI_SKIN "btn_close_a.bmp", UI_BTN_HOVER, 0);
    ui_bitmap_button_set_bitmap(self->base.close, UI_SKIN "btn_close_b.bmp", UI_BTN_DOWN, 0);
    close = self->base.close;
    ui_window_create(&close->button.base, close->bitmap_width, close->bitmap_height);
    place_controls(self);
    self->base.close->button.base.id = LOG_ID_CLOSE;
    ui_window_add_child(wnd, &self->base.close->button.base);

    self->base.columns = 1;
    self->base.rows = (height - 0x26) / ITEM_GRID_CELL;
}

void ui_merchant_item_log_wnd_on_draw(struct UIWindow *window)
{
    struct UIMerchantItemLogWnd *self = (struct UIMerchantItemLogWnd *)window;
    struct UIWindow *wnd = &self->base.frame.base;
    struct ListNode *node;
    struct MerchantLogEntry *entry;
    int strips, rows, i, shown, x, y;

    ui_window_draw_title_bar(wnd, 0, "", 0);
    if (wnd->height == ITEM_GRID_Y)
        return;
    ui_window_fill_rect(wnd, 0, ITEM_GRID_Y, wnd->width, wnd->height - 0x26, ui_skin_colour(2, 2));
    strips = (wnd->height - 0x26) / 8 + ((wnd->height - 0x26) % 8 != 0);
    for (i = 0, y = ITEM_GRID_Y; i < strips; i++, y += 8)
        ui_window_draw_bitmap(wnd, 0, y,
                            res_mgr_get(g_resMgr(), ui_skin_path(UI_SKIN "basic_interface\\itemwin_left.bmp")),
                            0);
    for (i = 0, y = ITEM_GRID_Y; i < strips; i++, y += 8) {
        struct BitmapRes *right = res_mgr_get(g_resMgr(),
                                                ui_skin_path(UI_SKIN "basic_interface\\itemwin_right.bmp"));

        ui_window_draw_bitmap(wnd, wnd->width - right->width, y, right, 0);
    }
    rows = (wnd->height - 0x26) / ITEM_GRID_CELL;
    for (i = 0, y = ITEM_GRID_Y; i < rows; i++, y += ITEM_GRID_CELL)
        ui_window_draw_item_win_mid(wnd, ITEM_GRID_X, y);

    node = self->base.items.head->next;
    entry = self->entries.first;
    shown = 0;
    if (self->base.rows >= 1) {
        for (i = 0;; i++, node = node->next, entry++) {
            if (i >= self->base.scroll) {
                char name[0x1a], money[0x40], digits[0x40], when[0x14];
                struct ItemInfo *item;
                struct tm *tm;
                int len;

                if ((unsigned)self->base.items.size < (unsigned)(i + 1))
                    break;
                item = LIST_VALUE(node, struct ItemInfo);
                if (!item_grid_hidden(item)) {
                    y = shown / self->base.columns * ITEM_GRID_CELL;
                    x = shown % self->base.columns * ITEM_GRID_CELL + 0x2c;
                    ui_window_draw_item_icon_count(wnd, x, y + 0x15, item, 0);
                    if (item->grade > 0) {
                        struct String path;

                        string_init(&path);
                        string_format(&path, UI_SKIN "grade_enchant\\grade_icon%d.bmp", (int)item->grade);
                        ui_window_draw_bitmap(wnd, x, y + 0x21,
                                            res_mgr_get(g_resMgr(), string_cstr(&path)), 1);
                        string_free(&path);
                    }
                    x = wnd->width - 0x172;
                    y += 0x1d;
                    memset(name, 0, sizeof name);
                    _snprintf_s(name, sizeof name, (size_t)-1, "%s", string_cstr(&entry->name));
                    ui_window_text_out(wnd, x, y, name, (int)strlen(name), 0, 12, 0, 0, 0);
                    x += 0x96;
                    memset(money, 0, sizeof money);
                    memset(digits, 0, sizeof digits);
                    len = sprintf(money, " %10s Zeny", format_money(item->price, digits));
                    ui_window_text_out(wnd, x, y, money, len, 0, 12, 0, 0, 0);
                    x += 0x6e;
                    memset(when, 0, sizeof when);
                    tm = _localtime32(&entry->time);
                    _snprintf_s(when, sizeof when, (size_t)-1, "%02d/%02d - %02d:%02d:%02d", tm->tm_mon + 1,
                                   tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
                    ui_window_text_out(wnd, x, y, when, (int)strlen(when), 0, 12, 0, 0, 0);
                    shown++;
                }
            }
            if (shown + 1 > self->base.rows)
                break;
        }
    }
    ui_window_draw_button_bar(wnd, 2, 0, 1);
}

void ui_merchant_item_log_wnd_on_mouse_hover(struct UIWindow *window, int x, int y)
{
    struct UIMerchantItemLogWnd *self = (struct UIMerchantItemLogWnd *)window;
    struct ItemInfo info;
    struct String text;
    struct String name = {0};
    int tip_x, tip_y;

    ui_item_base_wnd_item_at(&self->base, &info, x, y);
    if (info.index) {
        tip_x = (x - ITEM_GRID_X) / ITEM_GRID_CELL * ITEM_GRID_CELL + ITEM_GRID_X;
        tip_y = (y - ITEM_GRID_Y) / ITEM_GRID_CELL * ITEM_GRID_CELL + 1;
        ui_window_to_screen(&self->base.frame.base, &tip_x, &tip_y);
        item_info_get_display_name(&info, &name);
        string_init(&text);
        if (info.identified == 1 && info.option_count)
            string_format(&text, msg_str(MSG_LOG_TIP_OPTIONS), string_cstr(&name), info.option_count,
                          info.count);
        else
            string_format(&text, msg_str(MSG_LOG_TIP), string_cstr(&name), info.count);
        mode_show_tooltip(client_current_mode(), string_cstr(&text), tip_x, tip_y,
                          session_get_item_dis_color(&info), 0);
        string_free(&text);
        string_free(&name);
    }
    item_info_destruct(&info);
}

void ui_merchant_item_log_wnd_on_r_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIMerchantItemLogWnd *self = (struct UIMerchantItemLogWnd *)window;
    struct ItemInfo info;
    struct UIWindow *wnd;

    ui_item_base_wnd_item_at(&self->base, &info, x, y);
    if (info.index) {
        game_current_base_mode();
        wnd = ui_window_open(ITEM_WND_ID);
        ui_window_send(wnd, NULL, UI_MSG_SHOW_ITEM, (int)&info, 0, 0, 0);
    }
    item_info_destruct(&info);
}

void ui_merchant_item_log_wnd_on_process(struct UIWindow *window)
{
    struct UIMerchantItemLogWnd *self = (struct UIMerchantItemLogWnd *)window;
    struct MerchantLogEntry *entry;
    BYTE asked = 0;

    for (entry = self->entries.first; entry != self->entries.last; entry++) {
        const char *pending = msg_str(MSG_LOG_NAME_PENDING);
        struct TextView name;

        if (!string_equal_cstr(&entry->name, pending))
            continue;
        name = player_name_lookup(entry->char_id);
        string_assign(&entry->name, name.data, name.size);
        asked = 1;
    }
    if (asked)
        ui_window_invalidate_base(&self->base.frame.base);
}

int ui_merchant_item_log_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                            int val1, int val2, int val3, int val4)
{
    struct UIMerchantItemLogWnd *self = (struct UIMerchantItemLogWnd *)window;
    struct UIWindow *wnd = &self->base.frame.base;

    switch (msg) {
    case UI_MSG_COMMAND:
        if (val1 == LOG_ID_CLOSE)
            ui_window_mgr_post_quit1(&g_windowMgr, self->variant ? LOG_WND_ID_1 : LOG_WND_ID_0);
        return 0;

    case UI_MSG_RESIZE:
        if (val1 == 3) {
            int height = val3 > LOG_MAX_HEIGHT ? LOG_MAX_HEIGHT : val3;

            if (height < LOG_MIN_HEIGHT)
                height = LOG_MIN_HEIGHT;
            height = (height - LOG_MIN_HEIGHT) / ITEM_GRID_CELL * ITEM_GRID_CELL + LOG_MIN_HEIGHT;
            if (height != wnd->height) {
                ui_window_resize_base(wnd, wnd->width, height);
                self->base.columns = 1;
                self->base.rows = (height - 0x26) / ITEM_GRID_CELL;
                ui_window_move_base(&self->base.resizer->base, wnd->width - 0xf, wnd->height - 0xf);
                ui_window_move_base(&self->base.scroll_bar->base, wnd->width - 0xf, 0xe);
                ui_scroll_bar_resize(&self->base.scroll_bar->base, self->base.scroll_bar->base.width, wnd->height - 0x26);
                ui_item_base_wnd_update_scroll(&self->base);
            }
            place_controls(self);
            ui_window_mgr_update_snap_rect(&g_windowMgr, wnd);
        }
        return 0;

    case LOG_MSG_ADD: {
        struct ItemInfo info;
        struct MerchantLogEntry entry;
        struct String text = {0};

        item_info_copy(&info, (const struct ItemInfo *)val1);
        item_info_copy(list_push_back(&self->base.items, NULL, sizeof(struct ItemInfo)), &info);
        string_init(&entry.name);
        string_init(&entry.item);
        {
            struct TextView name = player_name_lookup(val2);
            string_assign(&entry.name, name.data, name.size);
        }
        entry.time = (__time32_t)val3;
        item_info_get_display_name(&info, &text);
        string_assign(&entry.item, string_cstr(&text), text.size);
        string_free(&text);
        entry.char_id = val2;
        vec_push(&self->entries, &entry, sizeof entry);
        ui_window_invalidate_base(wnd);
        ui_item_base_wnd_update_scroll(&self->base);
        item_info_destruct(&info);
        return 0;
    }
    }
    return ui_item_base_wnd_send_msg((struct UIWindow *)(&self->base), sender, msg, val1, val2, val3, val4);
}
