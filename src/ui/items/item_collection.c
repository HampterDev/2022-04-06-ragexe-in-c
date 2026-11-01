#include "ui/controls/listbox.h"
#include "world/tooltip.h"
#include "ui/window_manager_state.h"
#include "core/object_type_tables.h"
#include "world/mode_manager.h"
#include "ui/items/item_collection_internal.h"
#include "ui/items/pet_windows.h"
#include <string.h>
#include "core/msg_str.h"
#include "diag/perf.h"
#include "iteminfo/iteminfo.h"
#include "std/tree.h"

const struct UIWindowOps ui_item_collection_wnd_ops = {
    .destroy = ui_item_collection_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_item_collection_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_item_collection_wnd_store_info,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_item_collection_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_item_collection_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_item_collection_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_frame_wnd_on_mouse_move,
    .on_mouse_hover = ui_item_collection_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_item_collection_wnd_on_l_btn_up,
    .on_r_btn_down = ui_item_collection_wnd_on_r_btn_down,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_item_collection_wnd_send_msg,
    .invalidate = ui_window_invalidate_base,
    .render = ui_window_render_base,
    .has_custom_backdrop = 0,
    .draw_backdrop = ui_window_draw_backdrop_base,
    .type = UI_TYPE_FRAME,
    .on_hover_enter = ui_window_on_hover_enter_base,
    .on_hover_leave = ui_window_on_hover_leave_base,
    .make_quad = ui_window_make_quad_base,
    .hit_test = ui_window_hit_test_base,
    .render_overlay = ui_window_render_overlay_base,
};

struct UIItemCollectionWnd *ui_item_collection_wnd_construct(struct UIItemCollectionWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_item_collection_wnd_ops;
    item_info_construct(&self->item);
    string_init(&self->picture);
    string_init(&self->card_picture);
    tree_init(&self->options);
    self->info = NULL;
    item_info_clear(&self->item);
    self->pet_renamed = 0;
    self->pet_level = -1;
    self->pet_hunger = -1;
    self->pet_intimacy = -1;
    self->pet_accessory = -1;
    self->close_btn = NULL;
    self->show_cards = 0;
    self->option_count = 0;
    self->move_info = NULL;
    self->preview_btn = NULL;
    self->options_height = 0;
    return self;
}

struct UIWindow *ui_item_collection_wnd_new(void)
{
    return (struct UIWindow *)ui_item_collection_wnd_construct(mem_alloc(sizeof(struct UIItemCollectionWnd)));
}

void ui_item_collection_wnd_destruct(struct UIItemCollectionWnd *self)
{
    self->frame.base.ops = &ui_item_collection_wnd_ops;
    ui_window_mgr_post_quit1(&g_windowMgr, WND_ITEM_MOVE_INFO);
    tree_free(&self->options, option_lines_destroy);
    string_free(&self->card_picture);
    string_free(&self->picture);
    item_info_destruct(&self->item);
    ui_frame_wnd_destruct(&self->frame);
}

void ui_item_collection_wnd_destroy(struct UIWindow *window)
{
    struct UIItemCollectionWnd *self = (struct UIItemCollectionWnd *)window;
    ui_item_collection_wnd_destruct(self);
    mem_free(self);
}

void item_collection_place_left_of(struct UIWindow *wnd, int keep_left_of)
{
    struct UIWindow *other = ui_get_window(keep_left_of);

    if (other)
        ui_frame_wnd_move(wnd, other->x - other->width, other->y);
}

void ui_item_collection_wnd_move(struct UIWindow *window, int x, int y)
{
    struct UIItemCollectionWnd *self = (struct UIItemCollectionWnd *)window;
    ui_frame_wnd_move((struct UIWindow *)(&self->frame), x, y);
    if (g_windowMgr.item_collection_comparison_wnd)
        item_collection_place_left_of(g_windowMgr.item_collection_comparison_wnd, WND_ITEM_COLLECTION);
}

void ui_item_collection_wnd_store_info(struct UIWindow *window)
{
    struct UIItemCollectionWnd *self = (struct UIItemCollectionWnd *)window;
    if (!self->info)
        return;
    self->info->x = self->frame.base.x;
    self->info->y = self->frame.base.y;
    self->info->width = self->frame.base.width;
    self->info->height = self->frame.base.height;
}

void ui_item_collection_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UIItemCollectionWnd *self = (struct UIItemCollectionWnd *)window;
    struct UIWindow *wnd = COLLECTION_WND(self);
    struct UIWindow *text;
    struct UIResizer *resizer;

    self->wnd_id = WND_ITEM_COLLECTION;

    text = mem_alloc(sizeof(struct UITextViewer));
    if (text)
        text = ui_text_viewer_construct(text);
    self->text = text;
    ui_list_box_create_window((struct UIListBox *)text, 0x5b, 0x1f, width - 0x62, height - 0x28, 0);
    UI_TEXT_VIEWER_BACKGROUND(self->text) = ui_skin_argb(2, 2);
    ui_window_invalidate(self->text);
    ui_window_add_child(wnd, self->text);

    self->close_btn = ui_window_add_close_button(wnd, "ESC", 0);
    self->view_btn = ui_add_skin_button(wnd, "btn_view", 6, 6, ID_VIEW_BUTTON);

    resizer = mem_alloc(sizeof *resizer);
    if (resizer)
        resizer = ui_resizer_construct(resizer);
    self->resizer = resizer;
    ui_window_create(&resizer->base, 0x10, 0x10);
    ui_resizer_set_edge(self->resizer, UI_EDGE_BOTTOM_RIGHT);
    ui_resizer_set_bitmap(self->resizer,
                        self->wnd_id == WND_ITEM_COLLECTION_CMP
                            ? UI_SKIN "basic_interface\\btn_comparison_resize.bmp"
                            : UI_SKIN "btn_resize.bmp");
    ui_resizer_set_colours(self->resizer, UI_TRANSPARENT, UI_TRANSPARENT);
    ui_window_move_base(&self->resizer->base, width - 0x11, height - 0x11);
    ui_window_add_child(wnd, &self->resizer->base);

    if (!self->move_info) {
        struct UIItemMoveInfo *move_info = mem_alloc(sizeof *move_info);
        int text_height;

        if (move_info)
            move_info = ui_item_move_info_construct(move_info);
        self->move_info = move_info;
        ui_text_button_set_text(&move_info->button.button.base, msg_str(MSG_MOVE_INFO), 0, 0, 0xc, 0, 1);
        self->move_info->button.button.base.id = ID_MOVE_INFO;
        text_height = ui_window_text_height(wnd, msg_str(MSG_MOVE_INFO), 0, 0, 0xc, 0, 0);
        ui_text_button_fit_width(&self->move_info->button,
                              ui_window_text_width(wnd, msg_str(MSG_MOVE_INFO), 0, 0, 0xc, 0, 0) + 1,
                              text_height);
        ui_window_add_child(wnd, &self->move_info->button.button.base);
    }
}

static const char *collection_box_prefix(const struct UIItemCollectionWnd *self)
{
    return self->wnd_id == WND_ITEM_COLLECTION_CMP ? UI_SKIN "basic_interface\\sysbox_comparison_"
                                                   : UI_SKIN "sysbox_";
}

void ui_item_collection_wnd_on_draw(struct UIWindow *window)
{
    struct UIItemCollectionWnd *self = (struct UIItemCollectionWnd *)window;
    struct UIWindow *wnd = COLLECTION_WND(self);
    int height, base, y;

    height = wnd->height;
    if (self->item.identified == 1 && self->option_count != 0)
        height -= self->options_height;
    base = self->show_cards == 1 ? height - 0x1d : height;

    if (self->wnd_id == WND_ITEM_COLLECTION_CMP || self->wnd_id == WND_COMBINED_CARD_COLLECTION_CMP) {
        ui_window_draw_box_bitmaps(wnd, UI_SKIN "basic_interface\\sysbox_comparison_", 0, 0, wnd->width, base,
                                1);
        ui_window_draw_bitmap(wnd, 0, 0, ui_skin_bitmap(UI_SKIN "basic_interface\\collection_Comparison_bg.bmp"),
                            1);
    } else if (self->item.grade > 0) {
        char path[MAX_PATH];

        sprintf(path, UI_SKIN "sysbox_g%d_", self->item.grade);
        ui_window_draw_box_bitmaps(wnd, path, 0, 0, wnd->width, base, 0);
        sprintf(path, UI_SKIN "basic_interface\\collection_bg_g%d.bmp", self->item.grade);
        ui_window_draw_bitmap(wnd, 0, 0, ui_skin_bitmap(path), 1);
    } else {
        ui_window_draw_box_bitmaps(wnd, UI_SKIN "sysbox_", 0, 0, wnd->width, base, 1);
        ui_window_draw_bitmap(wnd, 0, 0, ui_skin_bitmap(UI_SKIN "basic_interface\\collection_bg.bmp"), 1);
    }
    if (self->picture.size > 0)
        ui_window_draw_bitmap(wnd, 10, 11,
                            (struct BitmapRes *)res_mgr_get(g_resMgr(), string_cstr(&self->picture)), 1);
    ui_window_draw_item_name(wnd, 0x5a, 0xa, &self->item, self->item.identified ? 0 : 0x808080, 0);

    if (self->show_cards == 1) {
        y = 0;
        if (self->item.identified == 1 && self->option_count > 0) {
            struct TreeNode *node;

            for (node = tree_first(&self->options); node != self->options.head; node = tree_next(node)) {
                const struct List *lines = &TREE_VALUE(node, struct OptionLines)->lines;
                struct ListNode *line;

                ui_window_draw_box_bitmaps(wnd, collection_box_prefix(self), 0, base + y, wnd->width,
                                        (int)lines->size * 15 + 0xe, 1);
                y += 8;
                for (line = lines->head->next;; ) {
                    const struct String *text = LIST_VALUE(line, struct String);
                    const char *title = msg_str(MSG_OPTION_TITLE);

                    if (string_equal_cstr(text, title)) {
                        ui_window_text_out_centred(wnd, 0, base + y, string_cstr(text), wnd->width, 0,
                                                0xc, RGB(0xff, 0, 0), 0);
                        y += 0xb;
                        break;
                    }
                    ui_window_text_out(wnd, 0xc, base + y, string_cstr(text), 0, 0, 0xc, 0, 0, 0);
                    y += 0xb;
                    line = line->next;
                    if (line == lines->head)
                        break;
                    y += 4;
                }
                y += 0xa;
            }
        }
        if (!item_info_has_maker_slot(&self->item)) {
            int i, x;

            y += base;
            ui_window_draw_box_bitmaps(wnd, collection_box_prefix(self), 0, y, wnd->width, 0x1d, 1);
            for (i = 0, x = 4; x < 0x68; i++, x += 0x19) {
                if (self->item.slot[i] > 0) {
                    char id_text[0x10];

                    sprintf(id_text, "%d", self->item.slot[i]);
                    ui_window_draw_item_icon_by_name(wnd, x, y + 2, id_text, 0);
                } else {
                    struct BitmapRes *slot = ui_skin_bitmap(UI_SKIN "empty_card_slot.bmp");

                    if (self->wnd_id == WND_ITEM_COLLECTION_CMP)
                        slot = ui_skin_bitmap(i >= item_info_get_slot_count(&self->item)
                                                  ? UI_SKIN "basic_interface\\comparison_disable_card_slot.bmp"
                                                  : UI_SKIN "basic_interface\\comparison_empty_card_slot.bmp");
                    else if (i >= item_info_get_slot_count(&self->item))
                        slot = ui_skin_bitmap(UI_SKIN "disable_card_slot.bmp");
                    ui_window_draw_bitmap(wnd, x, y + 2, slot, 1);
                }
            }
        }
    }

    if (item_move_info_get(COLLECTION_ITEM_ID(self))->item_id > 0)
        ui_window_move_base(&self->move_info->button.button.base, self->text->x, self->text->y + self->text->height);
    else
        ui_window_move_base(&self->move_info->button.button.base, -1000, -1000);
}

int item_collection_card_at(struct UIItemCollectionWnd *self, int x, int y, int *slot_x, int *slot_y)
{
    struct UIWindow *wnd = COLLECTION_WND(self);
    int rel = x - 3;
    int count, i, left;

    *slot_x = rel / 25 * 25 + 4;
    *slot_y = wnd->height - 0x32;
    ui_window_to_screen(wnd, slot_x, slot_y);
    if (y <= wnd->height - 0x1a)
        return -1;
    count = item_info_get_slot_count(&self->item);
    if (item_info_is_equipment_type(&self->item) && !item_info_is_pet_range_id(&self->item)
        && !item_info_has_maker_slot(&self->item))
        count = 4;
    for (i = 0, left = 0; i < count; i++, left += 25) {
        if (left < rel && rel < left + 25)
            return self->item.slot[i] ? i : -1;
    }
    return -1;
}

void item_collection_card_item(struct ItemInfo *card, int card_id)
{
    item_info_construct(card);
    card->identified = 1;
    card->type = 6;
    card->refine = 0;
    item_info_set_item_id(card, card_id);
}

void ui_item_collection_wnd_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIItemCollectionWnd *self = (struct UIItemCollectionWnd *)window;
    struct UIWindow *other;

    if (x <= 0x3c || y >= 0x11)
        return;
    ui_frame_wnd_on_l_btn_down((struct UIWindow *)(&self->frame), x, y);
    if (self->wnd_id == WND_ITEM_COLLECTION) {
        other = g_windowMgr.item_collection_comparison_wnd;
        if (!other)
            return;
        other->alpha_target = self->frame.base.alpha_target;
        other->alpha_time = self->frame.base.alpha_time;
    }
    if (self->wnd_id == WND_COMBINED_CARD_COLLECTION) {
        other = g_windowMgr.combined_card_item_collection_comparison_wnd;
        if (other) {
            other->alpha_target = self->frame.base.alpha_target;
            other->alpha_time = self->frame.base.alpha_time;
        }
    }
}

void ui_item_collection_wnd_on_mouse_hover(struct UIWindow *window, int x, int y)
{
    struct UIItemCollectionWnd *self = (struct UIItemCollectionWnd *)window;
    struct ItemInfo card;
    struct String name = {0};
    struct Mode *mode;
    COLORREF colour;
    int slot_x, slot_y, slot;

    slot = item_collection_card_at(self, x, y, &slot_x, &slot_y);
    if (slot == -1 || self->show_cards != 1)
        return;
    item_collection_card_item(&card, self->item.slot[slot]);
    mode = client_current_mode();
    colour = (COLORREF)-1;
    if (slot >= item_info_get_slot_count(&self->item))
        colour = item_info_get_name_color(&self->item);
    item_info_get_display_name(&card, &name);
    mode_show_tooltip(mode, string_cstr(&name), slot_x, slot_y, colour, 0);
    string_free(&name);
    item_info_destruct(&card);
}

void ui_item_collection_wnd_on_l_btn_up(struct UIWindow *window, int x, int y)
{
    struct UIItemCollectionWnd *self = (struct UIItemCollectionWnd *)window;
    ui_frame_wnd_on_l_btn_up((struct UIWindow *)(&self->frame), x, y);
    if (g_windowMgr.item_collection_comparison_wnd) {
        g_windowMgr.item_collection_comparison_wnd->alpha_target = self->frame.base.alpha_target;
        g_windowMgr.item_collection_comparison_wnd->alpha_time = self->frame.base.alpha_time;
    }
}

void item_collection_show_card(int wnd_id, struct ItemInfo *card, int past_slots)
{
    struct UIWindow *wnd = ui_make_window(wnd_id);

    ui_window_send_msg(wnd, NULL, UI_MSG_COLLECTION_IDENTIFIED, card->identified, 0, 0, 0);
    ui_window_send_msg(wnd, NULL, UI_MSG_COLLECTION_ITEM, (int)card, past_slots, 0, 0);
}

void ui_item_collection_wnd_on_r_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIItemCollectionWnd *self = (struct UIItemCollectionWnd *)window;
    struct UIItemCollectionWnd *cmp;
    struct ItemInfo card;
    int slot_x, slot_y, slot;

    slot = item_collection_card_at(self, x, y, &slot_x, &slot_y);
    if (slot == -1)
        return;
    item_collection_card_item(&card, self->item.slot[slot]);
    item_collection_show_card(WND_COMBINED_CARD_COLLECTION, &card,
                         slot >= item_info_get_slot_count(&self->item) ? 1 : 0);
    cmp = (struct UIItemCollectionWnd *)g_windowMgr.item_collection_comparison_wnd;
    if (cmp && cmp->item.slot[slot] != 0 && cmp->item.slot[slot] != 0xfe && cmp->item.slot[slot] != 0xff) {
        struct ItemInfo cmp_card;

        item_collection_card_item(&cmp_card, cmp->item.slot[slot]);
        item_collection_show_card(WND_COMBINED_CARD_COLLECTION_CMP, &cmp_card, 1);
        item_info_destruct(&cmp_card);
    }
    item_info_destruct(&card);
}

static int collection_info_valid(const struct WindowInfo *info)
{
    return info->width == 0x118 && info->height == 0x78 && info->x >= 0
           && info->x <= g_renderer->width - 0x118 && info->y >= 0
           && info->y <= g_renderer->height - 0x78;
}

void item_collection_take_info(struct UIWindow *wnd, struct WindowInfo *info, int default_y)
{
    if (collection_info_valid(info)) {
        ui_window_move(wnd, info->x, info->y);
        return;
    }
    info->x = 0x118;
    info->y = default_y;
    info->width = 0x118;
    info->height = 0x78;
    ui_window_move(wnd, (g_renderer->width - 640) / 2 + info->x,
             info->y * g_renderer->width / 640);
}

int ui_item_collection_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                           int val1, int val2, int val3, int val4)
{
    struct UIItemCollectionWnd *self = (struct UIItemCollectionWnd *)window;
    switch (msg) {
    case UI_MSG_COMMAND:
        switch (val1) {
        case UI_ID_CLOSE:
            ui_window_mgr_post_quit1(&g_windowMgr, self->wnd_id);
            break;
        case ID_VIEW_BUTTON:
            ui_window_send_msg(ui_make_window(UI_WND_CARD_ILLUST), NULL, UI_MSG_SHOW_CARD,
                     (int)string_cstr(&self->item.id_text), self->item.identified, 0, 0);
            break;
        case ID_PREVIEW_BUTTON: {
            struct UIWindow *preview = ui_make_window(WND_EQUIP_PREVIEW);

            if (preview)
                ui_window_send_msg(preview, COLLECTION_WND(self), UI_MSG_PREVIEW_ITEM, (int)&self->item, 0, 0, 0);
            break;
        }
        }
        return 0;

    case UI_MSG_RESIZE:
        if (val1 == UI_EDGE_BOTTOM_RIGHT)
            item_collection_set_height(self, val3);
        return 0;

    case UI_MSG_COLLECTION_ITEM:
        return item_collection_set_item(self, (struct ItemInfo *)val1, val2, val4);

    case UI_MSG_COLLECTION_REFRESH:
        item_collection_refresh(self);
        return 0;

    case UI_MSG_SET_INFO:
        self->info = (struct WindowInfo *)val1;
        item_collection_take_info(COLLECTION_WND(self), self->info, 0x7e);
        return 0;
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
}

void item_collection_set_height(struct UIItemCollectionWnd *self, int height)
{
    struct UIWindow *wnd = COLLECTION_WND(self);
    struct UIWindow *text = self->text;
    int min_height = (self->show_cards == 1 ? 0xaf : 0x78) + self->options_height;
    int move_id, text_bottom;

    if (height < min_height)
        height = min_height;
    if (height > 0x1c2)
        height = 0x1c2;
    if (height == wnd->height)
        return;
    move_id = item_move_info_get(COLLECTION_ITEM_ID(self))->item_id;
    if (move_id) {
        ui_window_resize_base(wnd, wnd->width, self->move_info->button.button.base.height + 3 + height);
        self->move_info->item_id = COLLECTION_ITEM_ID(self);
    } else {
        ui_window_resize_base(wnd, wnd->width, height);
    }
    height -= self->options_height;
    if (self->show_cards) {
        ui_window_resize(text, wnd->width - 0x62, height - 0x45);
        ui_window_move_base(&self->resizer->base, wnd->width - 0x11, wnd->height - self->options_height - 0x2e);
    } else {
        ui_window_resize(text, wnd->width - 0x62, height - 0x28);
        ui_window_move_base(&self->resizer->base, wnd->width - 0x11, wnd->height - self->options_height - 0x11);
    }
    text_bottom = text->y + text->height;
    if (move_id)
        ui_window_move_base(&self->move_info->button.button.base, text->x, self->show_cards ? text_bottom - 0x1d : text_bottom);
    else
        ui_window_move_base(&self->move_info->button.button.base, -1000, -1000);
    ui_window_send_msg(text, NULL, UI_MSG_SCROLL_V, 0, 0, 0, 0);
}

void ui_item_collection_set_pet(struct UIWindow *wnd, const struct PetEggDetails *pet)
{
    struct UIItemCollectionWnd *self = (struct UIItemCollectionWnd *)wnd;

    self->pet_level = pet->level;
    self->pet_hunger = pet->hunger;
    self->pet_intimacy = pet->intimacy;
    self->pet_accessory = pet->accessory;
    self->pet_renamed = pet->renamed;
    strncpy_s(self->pet_name, sizeof self->pet_name, pet->name, _TRUNCATE);
}
