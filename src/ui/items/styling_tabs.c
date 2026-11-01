#include "ui/controls/scroll.h"
#include "ui/controls/buttons.h"
#include "render/rp_pool.h"
#include "core/object_type_tables.h"
#include "core/memory.h"
#include "ui/items/styling_shop.h"
#include "core/msg_str.h"
#include "diag/perf.h"

#define ID_SHOP_APPLY 0xc0

static void styling_draw_dotted_line(struct UIWindow *wnd, int x)
{
    struct BitmapRes *line = ui_skin_bitmap(STYLE_SKIN "line_dotted_2px.bmp");

    if (line)
        ui_window_draw_bitmap(wnd, x, 0, line, 0);
}

static struct UIDisplayCharacter *styling_add_display(struct UIWindow *wnd)
{
    struct UIDisplayCharacter *display = mem_try_alloc(sizeof *display);

    if (display)
        display = ui_display_character_construct(display);
    return display;
}

void ui_stylingshop_tab_parent_wnd_destroy(struct UIWindow *window)
{
    struct UIStylingTab *self = (struct UIStylingTab *)window;
    self->base.ops = &ui_stylingshop_tab_parent_wnd_ops;
    ui_window_destruct(&self->base);
    mem_free(self);
}

struct UICheckBox;
void ui2nd_costume_check_box_destroy(struct UIWindow *window);
void ui2nd_costume_check_box_on_l_btn_down(struct UIWindow *self, int x, int y);
const struct UIWindowOps ui2nd_costume_check_box_ops = {
    .destroy = ui2nd_costume_check_box_destroy,
    .resize = ui_window_resize_base,
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
    .on_create = ui_window_on_create_base,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_check_box_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui2nd_costume_check_box_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_window_on_mouse_move_base,
    .on_mouse_hover = ui_check_box_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_check_box_on_mouse_shape,
    .on_l_btn_up = ui_window_on_l_btn_up_base,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_window_snap_base,
    .send_msg = ui_window_send_msg_base,
    .invalidate = ui_window_invalidate_base,
    .render = ui_window_render_base,
    .has_custom_backdrop = 0,
    .draw_backdrop = ui_window_draw_backdrop_base,
    .type = UI_TYPE_WINDOW,
    .on_hover_enter = ui_window_on_hover_enter_base,
    .on_hover_leave = ui_window_on_hover_leave_base,
    .make_quad = ui_window_make_quad_base,
    .hit_test = ui_window_hit_test_base,
    .render_overlay = ui_window_render_overlay_base,
};

void ui2nd_costume_check_box_destroy(struct UIWindow *window)
{
    struct UICheckBox *self = (struct UICheckBox *)window;
    ui_check_box_destruct(self);
    mem_free(self);
}

void ui2nd_costume_check_box_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UICheckBox *self = (struct UICheckBox *)window;
    if (self->disabled == 0 && self->base.state != 1)
        ui_check_box_on_l_btn_down((struct UIWindow *)(self), x, y);
}

#define MSG_2ND_COSTUME_SHOW 0xb15
#define MSG_2ND_COSTUME_HIDE 0xb10
#define ID_CHECK_BOX 0xcd

void ui2nd_costume_wnd_destroy(struct UIWindow *window)
{
    struct UI2ndCostumeWnd *self = (struct UI2ndCostumeWnd *)window;
    self->tab.base.ops = &ui2nd_costume_wnd_ops;
    self->tab.base.ops = &ui_stylingshop_tab_parent_wnd_ops;
    ui_window_destruct(&self->tab.base);
    mem_free(self);
}

static struct UICheckBox *costume_add_box(struct UI2ndCostumeWnd *self, int y, int label_y, int msg_id)
{
    struct UIWindow *wnd = &self->tab.base;
    struct UICheckBox *box = mem_try_alloc(sizeof *box);
    struct UIWindow *label;

    if (box) {
        ui_check_box_construct(&box->base);
        box->base.ops = &ui2nd_costume_check_box_ops;
    }
    ui_window_create(&box->base, 0x11, 0xd);
    ui_check_box_set_bitmaps_fit(box, STYLE_SKIN "check_able.bmp", STYLE_SKIN "Notcheck.bmp");
    ui_check_box_set_check(&box->base, 0);
    ui_window_move_base(&box->base, 6, y);
    ui_window_add_child(wnd, &box->base);

    label = mem_try_alloc(sizeof(struct UIStaticText));
    if (label)
        label = ui_static_text_construct(label);
    ui_window_create(label, 0x64, 0xc);
    ui_window_move(label, 0x1a, label_y);
    ui_static_text_set_text(label, msg_str(msg_id), 0);
    ui_static_text_set_font(label, 0, 0xb, 0, 0);
    ui_window_add_child(wnd, label);
    return box;
}

void ui2nd_costume_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UI2ndCostumeWnd *self = (struct UI2ndCostumeWnd *)window;
    struct UIWindow *wnd = &self->tab.base;
    struct UIShowInfoWnd *info;
    int kind;

    (void)width;
    (void)height;
    self->tab.display = styling_add_display(wnd);
    ui_window_create(&self->tab.display->base, (int)(wnd->width * 0.33), (int)(wnd->height * 0.6));
    ui_window_move_base(&self->tab.display->base, 0, 0);
    ui_window_add_child(wnd, &self->tab.display->base);

    self->show_box = costume_add_box(self, 0x8c, 0x8f, MSG_2ND_COSTUME_SHOW);
    self->hide_box = costume_add_box(self, 0xaa, 0xad, MSG_2ND_COSTUME_HIDE);

    info = mem_try_alloc(sizeof *info);
    if (info)
        info = ui_show_info_wnd_construct(info);
    self->tab.info = info;
    ui_window_create(&info->base, 0xc2, 0xb6);
    kind = STYLE_TAB_2ND_COSTUME;
    ui_show_info_wnd_setup(self->tab.info, &kind);
    ui_window_move_base(&self->tab.info->base, 0x8e, 0xd);
    self->tab.info->display = self->tab.display;
    ui_window_add_child(wnd, &self->tab.info->base);

    ui_check_box_set_check(&self->show_box->base, self->tab.display->body_style == 0);
    ui_check_box_set_check(&self->hide_box->base, self->show_box->base.state == 0);
}

void ui2nd_costume_wnd_on_draw(struct UIWindow *window)
{
    struct UI2ndCostumeWnd *self = (struct UI2ndCostumeWnd *)window;
    struct UIWindow *wnd = &self->tab.base;
    int y;

    ui_window_clear_canvas(wnd, 0xffffffff);
    for (y = 0x8c; y < 0xc8; y += 0x1e) {
        struct BitmapRes *frame = ui_skin_bitmap(STYLE_SKIN "checkbox.bmp");

        if (frame)
            ui_window_draw_bitmap(wnd, 5, y, frame, 0);
    }
    styling_draw_dotted_line(wnd, 0x77);
}

int ui2nd_costume_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1,
                                       int val2, int val3, int val4)
{
    struct UI2ndCostumeWnd *self = (struct UI2ndCostumeWnd *)window;
    struct UIDisplayCharacter *display = self->tab.display;

    if (msg != UI_MSG_COMMAND)
        return ui_window_send_msg_base(&self->tab.base, sender, msg, val1, val2, val3, val4);
    if (val1 == ID_CHECK_BOX) {
        if (sender == &self->hide_box->base) {
            if (display && self->show_box && self->hide_box->base.state != 0) {
                display->body_style = 1;
                ui_check_box_set_check(&self->show_box->base, 0);
                ui_window_invalidate_base(&self->tab.info->base);
            }
        } else if (sender == &self->show_box->base) {
            if (display && self->hide_box && self->show_box->base.state != 0) {
                display->body_style = 0;
                ui_check_box_set_check(&self->hide_box->base, 0);
                ui_window_invalidate_base(&self->tab.info->base);
            }
        } else {
            ui_window_invalidate_base(&self->tab.info->base);
        }
    }
    ui_window_invalidate_base(&self->tab.base);
    return 0;
}

void ui_accessory_show_case_destroy(struct UIWindow *window)
{
    struct UIAccessoryShowCase *self = (struct UIAccessoryShowCase *)window;
    self->scroll.base.ops = &ui_accessory_show_case_ops;
    vec_free(&self->shadows);
    ui_scroll_wnd_destruct(&self->scroll);
    mem_free(self);
}

void ui_accessory_show_case_on_draw(struct UIWindow *window)
{
    struct UIAccessoryShowCase *self = (struct UIAccessoryShowCase *)window;
    ui_window_clear_canvas(&self->scroll.base, 0xffffffff);
}

void ui_accessory_cus_wnd_destroy(struct UIWindow *window)
{
    struct UIStylingTab *self = (struct UIStylingTab *)window;
    self->base.ops = &ui_accessory_cus_wnd_ops;
    self->base.ops = &ui_stylingshop_tab_parent_wnd_ops;
    ui_window_destruct(&self->base);
    mem_free(self);
}

void ui_accessory_cus_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UIStylingTab *self = (struct UIStylingTab *)window;
    struct UIWindow *wnd = &self->base;
    struct UIShowInfoWnd *info;
    struct UIAccessoryShowCase *show_case;
    int kind;

    (void)width;
    (void)height;
    self->display = styling_add_display(wnd);
    ui_window_create(&self->display->base, (int)(wnd->width * 0.33), (int)(wnd->height * 0.6));
    ui_window_move_base(&self->display->base, 0, 0);
    ui_window_add_child(wnd, &self->display->base);

    info = mem_try_alloc(sizeof *info);
    if (info)
        info = ui_show_info_wnd_construct(info);
    self->info = info;
    ui_window_create(&info->base, (int)(wnd->width * 0.35), (int)(wnd->height * 0.4));
    kind = STYLE_TAB_ACCESSORY;
    ui_show_info_wnd_setup(self->info, &kind);
    ui_window_move_base(&self->info->base, 0, (int)(wnd->height * 0.6));
    self->info->display = self->display;
    ui_window_add_child(wnd, &self->info->base);

    show_case = mem_alloc(sizeof *show_case);
    if (show_case) {
        int case_width = (int)(wnd->width * 0.6);
        int case_height = wnd->height;

        ui_scroll_wnd_construct(&show_case->scroll, case_width, case_height, 0);
        show_case->scroll.base.ops = &ui_accessory_show_case_ops;
        show_case->shadows.first = NULL;
        show_case->shadows.last = NULL;
        show_case->shadows.end = NULL;
        ui_accessory_show_case_fill_headgear(show_case);
    }
    ui_window_move_base(&show_case->scroll.base, (int)(wnd->width * 0.38), 5);
    ui_window_add_child(wnd, &show_case->scroll.base);
}

void ui_accessory_cus_wnd_on_draw(struct UIWindow *window)
{
    struct UIStylingTab *self = (struct UIStylingTab *)window;
    ui_window_clear_canvas(&self->base, 0xffffffff);
    styling_draw_dotted_line(&self->base, 0x77);
}

int ui_accessory_cus_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1,
                                         int val2, int val3, int val4)
{
    struct UIStylingTab *self = (struct UIStylingTab *)window;
    if (msg != UI_MSG_COMMAND)
        return ui_window_send_msg_base(&self->base, sender, msg, val1, val2, val3, val4);
    ui_window_invalidate_base(&self->base);
    return 0;
}

const struct UIWindowOps ui_slot_for_select_wnd_ops = {
    .destroy = ui_slot_for_select_wnd_destroy,
    .resize = ui_window_resize_base,
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
    .on_create = ui_window_on_create_base,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_window_on_draw_base,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_slot_for_select_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_slot_for_select_wnd_on_l_btn_dbl_clk,
    .on_mouse_move = ui_window_on_mouse_move_base,
    .on_mouse_hover = ui_window_on_mouse_hover_base,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_window_on_l_btn_up_base,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_window_snap_base,
    .send_msg = ui_window_send_msg_base,
    .invalidate = ui_window_invalidate_base,
    .render = ui_window_render_base,
    .has_custom_backdrop = 0,
    .draw_backdrop = ui_window_draw_backdrop_base,
    .type = UI_TYPE_WINDOW,
    .on_hover_enter = ui_window_on_hover_enter_base,
    .on_hover_leave = ui_window_on_hover_leave_base,
    .make_quad = ui_window_make_quad_base,
    .hit_test = ui_window_hit_test_base,
    .render_overlay = ui_window_render_overlay_base,
};

#define ID_MY_STYLE_DELETE 0xc1
#define ID_MY_STYLE_SELECT 0xc4
#define ID_MY_STYLE_ROTATE_RIGHT 0xda
#define ID_MY_STYLE_ROTATE_LEFT 0xdb
#define MSG_MY_STYLE_DELETE 0xb00
#define UI_MSG_MY_STYLE_APPLY STYLE_MSG_MY_STYLE_APPLY

void ui_my_style_wnd_destroy(struct UIWindow *window)
{
    struct UIMyStyleWnd *self = (struct UIMyStyleWnd *)window;
    self->tab.base.ops = &ui_my_style_wnd_ops;
    ui_my_style_wnd_save_slots(self);
    self->tab.base.ops = &ui_stylingshop_tab_parent_wnd_ops;
    ui_window_destruct(&self->tab.base);
    mem_free(self);
}

static struct UIBitmapButton *my_style_add_button(struct UIMyStyleWnd *self, const char *out, const char *over,
                                                  const char *press, int x, int y, int id, int slot)
{
    struct UIBitmapButton *btn = mem_try_alloc(sizeof *btn);

    if (btn)
        btn = ui_bitmap_button_construct(btn);
    ui_bitmap_button_set_bitmap(btn, out, UI_BTN_UP, 0);
    ui_bitmap_button_set_bitmap(btn, over, UI_BTN_HOVER, 0);
    ui_bitmap_button_set_bitmap(btn, press, UI_BTN_DOWN, 0);
    ui_window_create(&btn->button.base, btn->bitmap_width, btn->bitmap_height);
    ui_window_move_base(&btn->button.base, x, y);
    btn->button.base.id = id;
    if (!self->slots[slot])
        ui_button_show((struct UIButton *)(&btn->button.base), 0);
    ui_window_add_child(&self->tab.base, &btn->button.base);
    return btn;
}

void ui_my_style_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UIMyStyleWnd *self = (struct UIMyStyleWnd *)window;
    struct UIWindow *wnd = &self->tab.base;
    struct UIWindow *select;
    int i;

    (void)width;
    (void)height;
    ui_my_style_wnd_load_slots(self);
    select = mem_alloc(sizeof(struct UIWindow));
    if (select) {
        ui_window_construct(select, 0);
        select->ops = &ui_slot_for_select_wnd_ops;
    }
    ui_window_create(select, wnd->width, wnd->height);
    ui_window_move(select, 0, 0);
    select->id = ID_MY_STYLE_SELECT;
    ui_window_add_child(wnd, select);

    for (i = 0; i < MY_STYLE_SLOTS; i++)
        self->delete_btn[i] = my_style_add_button(
            self, STYLE_SKIN "btn_delete_out.bmp", STYLE_SKIN "btn_delete_over.bmp", STYLE_SKIN "btn_delete_press.bmp",
            (int)((wnd->width * 0.33 - 1.0) * i + 7.0), 4, ID_MY_STYLE_DELETE, i);
    for (i = 0; i < MY_STYLE_SLOTS; i++) {
        int x = (wnd->width / 3 - 1) * i;

        self->rotate_left[i] = my_style_add_button(self, STYLE_SKIN "btn_rotatearrow_l_out.bmp",
                                                   STYLE_SKIN "btn_rotatearrow_l_over.bmp",
                                                   STYLE_SKIN "btn_rotatearrow_l_press.bmp", x + 0x28, 0x6f,
                                                   ID_MY_STYLE_ROTATE_LEFT, i);
        self->rotate_right[i] = my_style_add_button(self, STYLE_SKIN "btn_rotatearrow_r_out.bmp",
                                                    STYLE_SKIN "btn_rotatearrow_r_over.bmp",
                                                    STYLE_SKIN "btn_rotatearrow_r_press.bmp", x + 0x3d, 0x6f,
                                                    ID_MY_STYLE_ROTATE_RIGHT, i);
    }
}

void ui_my_style_wnd_on_draw(struct UIWindow *window)
{
    struct UIMyStyleWnd *self = (struct UIMyStyleWnd *)window;
    struct UIWindow *wnd = &self->tab.base;
    int i;

    ui_window_clear_canvas(wnd, 0xffffffff);
    for (i = 0; i < 2; i++) {
        struct BitmapRes *line = ui_skin_bitmap(STYLE_SKIN "line_dotted_2px.bmp");

        if (line)
            ui_window_draw_bitmap(wnd, (int)(wnd->width * 0.33 * (i + 1)), 0, line, 0);
    }
}

static void my_style_apply(struct UIMyStyleWnd *self)
{
    struct UIStylingShopWnd *shop = (struct UIStylingShopWnd *)ui_get_window(WND_STYLING_SHOP);
    struct UIStylingTab *slot = self->slots[self->selected];
    struct TreeNode *node;

    if (!slot)
        return;
    for (node = tree_first(&shop->tabs); node != shop->tabs.head; node = tree_next(node)) {
        struct StyleTab *entry = TREE_VALUE(node, struct StyleTab);

        if (entry->button->id == STYLE_TAB_MY_STYLE && entry->tab->base.visible == 1) {
            ui_display_character_copy_look(shop->display, slot->display);
            ui_window_send_msg(&shop->frame.base, NULL, UI_MSG_COMMAND, ID_SHOP_APPLY, 0, 0, 0);
        }
    }
    ui_window_invalidate(&shop->frame.base);
}

static void my_style_delete(struct UIMyStyleWnd *self, struct UIWindow *sender)
{
    int i;

    if (ui_window_mgr_message_box(&g_windowMgr, msg_str(MSG_MY_STYLE_DELETE), UI_MSGBOX_QUESTION, 0, 0, 0,
                               "메시지", 0x118, 0x78, 0)
        == 0xb4)
        return;
    for (i = 0; i < MY_STYLE_SLOTS; i++) {
        if (sender != &self->delete_btn[i]->button.base)
            continue;
        ui_window_remove_child(&self->tab.base, &self->slots[i]->base, 1);
        self->slots[i] = NULL;
        ui_button_show((struct UIButton *)(&self->delete_btn[i]->button.base), 0);
        ui_button_show((struct UIButton *)(&self->rotate_left[i]->button.base), 0);
        ui_button_show((struct UIButton *)(&self->rotate_right[i]->button.base), 0);
    }
    ui_window_invalidate_base(&self->tab.base);
}

static void my_style_select(struct UIMyStyleWnd *self, int x)
{
    struct UIWindow *wnd = &self->tab.base;
    int i;

    if (wnd->visible)
        self->selected = x / (wnd->width / 3);
    for (i = 0; i < MY_STYLE_SLOTS; i++) {
        struct UIDisplayCharacter *look;

        if (!self->slots[i])
            continue;
        look = self->slots[i]->display;
        if (look) {
            look->shadow_bitmap = i == self->selected ? STYLE_SKIN "shadow_chr_on.bmp"
                                                      : STYLE_SKIN "shadow_chr_off.bmp";
            ui_window_invalidate_base(&look->base);
        }
        ui_window_invalidate_children(&self->slots[i]->base);
    }
    ui_window_invalidate_base(wnd);
}

int ui_my_style_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1, int val2,
                                    int val3, int val4)
{
    struct UIMyStyleWnd *self = (struct UIMyStyleWnd *)window;
    int i;

    if (msg == UI_MSG_MY_STYLE_APPLY) {
        my_style_apply(self);
    } else if (msg == UI_MSG_COMMAND) {
        switch (val1) {
        case ID_MY_STYLE_DELETE:
            my_style_delete(self, sender);
            break;
        case ID_MY_STYLE_SELECT:
            my_style_select(self, val2);
            break;
        case ID_MY_STYLE_ROTATE_RIGHT:
        case ID_MY_STYLE_ROTATE_LEFT:
            for (i = 0; i < MY_STYLE_SLOTS; i++) {
                if (sender == &self->rotate_left[i]->button.base || sender == &self->rotate_right[i]->button.base)
                    ui_display_character_send_msg(&self->slots[i]->display->base, NULL, UI_MSG_COMMAND, val1, 0, 0, 0);
            }
            break;
        }
    } else {
        return ui_window_send_msg_base(&self->tab.base, sender, msg, val1, val2, val3, val4);
    }
    ui_window_invalidate_base(&self->tab.base);
    return 0;
}
