#include "ui/controls/buttons.h"
#include "world/tooltip.h"
#include <stdlib.h>
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "contents/mouse.h"
#include "core/object_type_tables.h"
#include "world/mode_manager.h"
#include "ui/window_access.h"
#include "contents/connection_api.h"
#include <string.h>
#include "contents/client_res.h"
#include "contents/connection.h"
#include "contents/game_session.h"
#include "iteminfo/item_api.h"
#include "std/vec.h"
#include "contents/item_reform.h"
#include "contents/item_reform_wnd.h"
#include "std/std.h"
#include "ui/controls/combo_box.h"
#include "ui/controls/scroll.h"
#include "ui/controls/line_info.h"
#include "ui/quests/tag_text_viewer.h"
#include "base.h"
#include "ui/window_types.h"
#include "ui/controls/edit_types.h"
#include "contents/ui_window.h"
#include "contents/ui_window_mgr.h"
#include "core/msg_str.h"
#include "core/text_util.h"
#include "diag/perf.h"
#include "session/session_items.h"
#include "ui/ui_items.h"

const struct UIWindowOps item_reform_item_ops = {
    .destroy = item_reform_item_delete,
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
    .on_draw = item_reform_item_on_draw,    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_window_on_l_btn_down_base,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_window_on_mouse_move_base,
    .on_mouse_hover = item_reform_item_show_name,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_window_on_l_btn_up_base,
    .on_r_btn_down = item_reform_item_show_info,
    .on_r_btn_up = ui_window_on_r_btn_up_base,    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_window_snap_base,
    .send_msg = ui_window_send_msg_base,
    .invalidate = ui_window_invalidate_base,    .render = ui_window_render_base,
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

const struct UIButtonOps item_reform_detail_btn_ops = {
    .base = {
        .destroy = item_reform_detail_btn_delete,
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
        .on_draw = item_reform_detail_btn_on_draw,        .on_focus = ui_window_on_focus_base,
        .on_focus_lost = ui_window_on_focus_lost_base,
        .on_l_btn_down = ui_button_on_l_btn_down,
        .on_l_btn_dbl_clk = ui_button_on_l_btn_dbl_clk,
        .on_mouse_move = ui_button_on_mouse_move,
        .on_mouse_hover = ui_button_on_mouse_hover,
        .track_mouse = ui_button_track_mouse,
        .on_mouse_shape = ui_button_on_mouse_shape,
        .on_l_btn_up = ui_button_on_l_btn_up,
        .on_r_btn_down = ui_window_on_r_btn_down_base,
        .on_r_btn_up = ui_window_on_r_btn_up_base,        .on_wheel = ui_window_on_wheel_base,
        .snap = ui_window_snap_base,
        .send_msg = ui_window_send_msg_base,
        .invalidate = ui_window_invalidate_base,        .render = ui_window_render_base,
        .has_custom_backdrop = 0,
        .draw_backdrop = ui_window_draw_backdrop_base,
        .get_records = ui_window_get_records_base,
        .type = UI_TYPE_WINDOW,
        .on_hover_enter = item_reform_detail_btn_enter,
        .on_hover_leave = item_reform_detail_btn_leave,
        .make_quad = ui_window_make_quad_base,
        .hit_test = ui_window_hit_test_base,
        .render_overlay = ui_window_render_overlay_base,
    },
    .set_show = ui_button_set_show,
};

const struct UIWindowOps item_reform_detail_msg_ops = {
    .destroy = item_reform_detail_msg_delete,
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
    .on_draw = item_reform_detail_msg_on_draw,    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_window_on_l_btn_down_base,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_window_on_mouse_move_base,
    .on_mouse_hover = ui_window_on_mouse_hover_base,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_window_on_l_btn_up_base,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_window_on_r_btn_up_base,    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_window_snap_base,
    .send_msg = ui_window_send_msg_base,
    .invalidate = ui_window_invalidate_base,    .render = ui_window_render_base,
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

#define REFORM_BG_BMP   "유저인터페이스\\ItemReform\\bg_reform.bmp"
#define ITEM_ICON_FMT   "유저인터페이스\\item\\%s.bmp"

#define MSG_CLOSE               0xda
#define MSG_REFORM              0xf07
#define MSG_MATERIAL_MISSING    0xf08
#define MSG_DETAILS             0xf0e
#define MSG_SEE_DETAILS         0xf0f
#define MSG_NOTHING_TO_REFORM   0xf10

#define COLOR_SHORT     0x321ec8
#define COLOR_ENOUGH    0x3c3c3c
#define COLOR_NOTICE    0x0a0aff
#define COLOR_KEY       0xffff00ff

#pragma pack(push, 1)
struct ReformItemPacket {
    unsigned short id;
    int reform_item_id;
    unsigned short base_index;
};
#pragma pack(pop)
SIZE_ASSERT(ReformItemPacket, 8);

static int inventory_count_of(const struct ItemInfo *item)
{
    return session_inventory_count(&g_session1, atoi(string_cstr(&item->id_text)));
}

static void send_close_packet(void)
{
    unsigned short id = PACKET_REFORM_CLOSE;
    int len = rag_connection_get_packet_length(rag_connection_instance(), PACKET_REFORM_CLOSE);

    rag_connection_send_packet(rag_connection_instance(), len, &id);
}

static struct UIItemReformWnd_Item *new_item_slot(struct UIItemReformWnd *self, int x, int y)
{
    struct UIItemReformWnd_Item *slot = (struct UIItemReformWnd_Item *)mem_alloc(sizeof *slot);

    if (slot) {
        ui_window_construct(&slot->wnd, 0);
        slot->wnd.ops = &item_reform_item_ops;
        slot->item = NULL;
    }
    ui_window_create(&slot->wnd, 0x18, 0x18);
    ui_window_move_base(&slot->wnd, x, y);
    ui_window_add_child(&self->frame.base, &slot->wnd);
    return slot;
}

static void set_notice(struct UIItemReformWnd *self, int msg_id)
{
    const char *text = msg_str(msg_id);

    string_assign_cstr(&self->notice, text);
}

struct UIItemReformWnd *item_reform_wnd_construct(struct UIItemReformWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &item_reform_wnd_ops;
    self->reform_button = NULL;
    self->list = NULL;
    self->base_slot = NULL;
    self->result_slot = NULL;
    self->material_slots.first = self->material_slots.last = self->material_slots.end = NULL;
    self->detail_button = NULL;
    self->detail_popup = NULL;
    item_info_construct(&self->reform_item);
    self->base_item = NULL;
    item_info_construct(&self->result);
    self->materials.first = self->materials.last = self->materials.end = NULL;
    string_init(&self->information);
    string_init(&self->notice);
    return self;
}

struct UIWindow *item_reform_wnd_new(void)
{
    return (struct UIWindow *)item_reform_wnd_construct(mem_alloc(sizeof(struct UIItemReformWnd)));
}

void item_reform_wnd_destruct(struct UIItemReformWnd *self)
{
    self->frame.base.ops = &item_reform_wnd_ops;
    if (self->detail_popup)
        ui_window_mgr_post_quit2(&g_windowMgr, self->detail_popup);
    send_close_packet();
    string_free(&self->notice);
    string_free(&self->information);
    item_info_vec_free(&self->materials);
    item_info_destruct(&self->result);
    item_info_destruct(&self->reform_item);
    vec_free(&self->material_slots);
    ui_frame_wnd_destruct(&self->frame);
}

void item_reform_wnd_delete(struct UIWindow *window)
{
    struct UIItemReformWnd *self = (struct UIItemReformWnd *)window;
    item_reform_wnd_destruct(self);
    mem_free(self);
}

void item_reform_wnd_on_create(struct UIWindow *window, int w, int h)
{
    struct UIItemReformWnd *self = (struct UIItemReformWnd *)window;
    struct UIWindow *button;
    unsigned i;

    (void)w; (void)h;
    ui_window_add_sys_button(&self->frame.base);
    ui_window_add_close_button(&self->frame.base, "", 0);

    button = (struct UIWindow *)mem_alloc(sizeof(struct UIBasicButton));
    if (button)
        button = (struct UIWindow *)ui_basic_button_construct((struct UIBasicButton *)button);
    ui_basic_button_set_label(button, 0x3c, msg_str(MSG_CLOSE));
    ui_window_move(button, 0x12c, 0xec);
    button->id = REFORM_CLOSE_BUTTON_ID;
    ui_window_add_child(&self->frame.base, button);

    button = (struct UIWindow *)mem_alloc(sizeof(struct UIBasicButton));
    if (button)
        button = (struct UIWindow *)ui_basic_button_construct((struct UIBasicButton *)button);
    self->reform_button = button;
    ui_basic_button_set_label(self->reform_button, 0x3c, msg_str(MSG_REFORM));
    ui_window_move(self->reform_button, 0x16d, 0xec);
    self->reform_button->id = REFORM_BUTTON_ID;
    ((struct UIButton *)self->reform_button)->disabled = 1;
    ui_window_invalidate(self->reform_button);
    ui_window_add_child(&self->frame.base, self->reform_button);

    self->list = item_reform_list_new();
    ui_window_create(&self->list->frame.base, 0xde, 0xc0);
    item_reform_list_move(&self->list->frame.base, 0xd7, 0x15);
    ui_window_add_child(&self->frame.base, &self->list->frame.base);

    self->base_slot = new_item_slot(self, 0x90, 0x32);
    self->result_slot = new_item_slot(self, 0x36, 0x32);
    for (i = 0; i < item_reform_mgr()->max_materials; i++) {
        struct UIItemReformWnd_Item *slot = new_item_slot(self, (i % 3) * 0x45 + 0x1e, (i / 3) * 0x32 + 0x8c);

        vec_push(&self->material_slots, &slot, sizeof slot);
    }

    button = (struct UIWindow *)mem_alloc(sizeof(struct UIButton));
    if (button) {
        ui_button_construct((struct UIButton *)button);
        button->ops = &item_reform_detail_btn_ops.base;
    }
    self->detail_button = button;
    ui_window_create(button, 0x95, 0xe);
    ui_window_move(self->detail_button, 0x24, 0x7c);
    ui_button_show((struct UIButton *)(self->detail_button), 0);
    ui_window_add_child(&self->frame.base, self->detail_button);
}

void item_reform_wnd_place_popup(struct UIWindow *window)
{
    struct UIItemReformWnd *self = (struct UIItemReformWnd *)window;
    if (!self->detail_popup)
        return;
    ui_window_mgr_raise_window(&g_windowMgr, self->detail_popup);
    ui_window_move(self->detail_popup, g_mouse.x + 5, g_mouse.y + 5);
}

void item_reform_wnd_on_draw(struct UIWindow *window)
{
    struct UIItemReformWnd *self = (struct UIItemReformWnd *)window;
    struct String text = {0};

    struct String name = {0};
    unsigned i;

    ui_window_draw_bitmap(&self->frame.base, 0, 0, res_mgr_get(g_resMgr(), REFORM_BG_BMP), 1);

    if (atoi(string_cstr(&self->reform_item.id_text)) > 0) {
        item_info_get_display_name(&self->reform_item, &name);
        ui_window_draw_title_bar(&self->frame.base, 0, string_cstr(&name), 0);
        string_free(&name);
    }
    if (atoi(string_cstr(&self->result.id_text)) != 0) {
        item_info_get_display_name(&self->result, &name);
        ui_window_fit_text(&self->frame.base, &text, 0xc8, string_cstr(&name), 0, 0xe, 1);
        ui_window_text_out_centred(&self->frame.base, 6, 0x68, string_cstr(&text), 0xd1, 0, 0xe, 0, 1);
        string_free(&text);
        string_free(&name);
    }

    for (i = 0; i < (unsigned)VEC_COUNT(&self->materials, struct ItemInfo); i++) {
        int half = (int)(ui_window_text_width(&self->frame.base, "/", 0, 0, 0xc, 0, 0) * 0.5);
        int x = (i % 3) * 0x45 + 0x2a;
        int y = (i / 3) * 0x32 + 0xa7;
        struct ItemInfo *material = (struct ItemInfo *)self->materials.first + i;
        int have = inventory_count_of(material);
        int need = material->count;
        unsigned color = have < need ? COLOR_SHORT : COLOR_ENOUGH;

        ui_window_text_out(&self->frame.base, x - half, y, "/", 0, 0, 0xc, 0, 0, 0);
        string_format(&text, "%d", have);
        ui_window_text_out_right(&self->frame.base, x - half, y, string_cstr(&text), 0, 0, 0xc, color, 0);
        string_free(&text);
        string_format(&text, "%d", need);
        ui_window_text_out(&self->frame.base, x + half, y, string_cstr(&text), 0, 0, 0xc, color, 0, 0);
        string_free(&text);
    }

    if (self->notice.size != 0) {
        ui_window_fit_text(&self->frame.base, &text, 0x118, string_cstr(&self->notice), 0, 0xc, 0);
        ui_window_text_out(&self->frame.base, 8, 0xf0, string_cstr(&text), 0, 0, 0xc, COLOR_NOTICE, 0, 0);
        string_free(&text);
    }
}

static void set_base_item(struct UIItemReformWnd *self, struct ItemInfo *base)
{
    struct UIItemReformWnd_Item **slot;
    struct ReformInfo *info;
    struct TreeNode *head, *node;
    struct ItemInfo *material, *last;
    unsigned i, slots;

    self->base_slot->item = NULL;
    ui_window_invalidate_base(&self->base_slot->wnd);
    self->result_slot->item = NULL;
    ui_window_invalidate_base(&self->result_slot->wnd);
    slots = (unsigned)VEC_COUNT(&self->material_slots, struct UIItemReformWnd_Item *);
    for (i = 0, slot = (struct UIItemReformWnd_Item **)self->material_slots.first; i < slots; i++, slot++) {
        if (*slot) {
            (*slot)->item = NULL;
            ui_window_invalidate_base(&(*slot)->wnd);
        }
    }
    string_clear(&self->notice);
    item_info_vec_clear(&self->materials);

    self->base_item = base;
    if (!base)
        return;
    {
        int base_id = atoi(string_cstr(&base->id_text));
        int reform_id = atoi(string_cstr(&self->reform_item.id_text));

        info = item_reform_find(item_reform_mgr(), reform_id, base_id);
    }
    if (!info)
        return;

    item_info_set_item_id(&self->result, info->result_item);
    self->result.type = 5;
    self->result.identified = 1;
    self->result.refine = self->base_item->refine + (int)info->change_refine_value;
    memset(self->result.options, 0, sizeof self->result.options);
    if (info->random_option_code.size == 0) {
        self->result.option_count = self->base_item->option_count;
        memcpy_s(self->result.options, sizeof self->result.options,
                 self->base_item->options, self->base_item->option_count * 5);
    } else {
        const char *code = string_cstr(&info->random_option_code);

        if (info->random_option_code.size != 2 || *(const unsigned short *)code != 0x3047) {
            self->result.option_count = 1;
            self->result.options[0].id = (unsigned short)ITEM_OPTION_TITLE;
        }
    }
    if (info->preserve_socket_item == 1) {
        self->result.slot[0] = self->base_item->slot[0];
        self->result.slot[1] = self->base_item->slot[1];
        self->result.slot[2] = self->base_item->slot[2];
        self->result.slot[3] = self->base_item->slot[3];
    }
    if (info->preserve_grade == 1)
        self->result.grade = self->base_item->grade;

    head = info->materials.head;
    for (node = tree_first(&info->materials); node != head; node = tree_next(node)) {
        const struct ReformMaterial *m = TREE_VALUE(node, const struct ReformMaterial);
        int need = (int)m->count;
        struct ItemInfo item;

        item_info_construct(&item);
        item_info_set_item_id(&item, m->item);
        item.count = need;
        item.identified = 1;
        vec_push(&self->materials, &item, sizeof item);
    }

    ((struct UIButton *)self->reform_button)->disabled = 1;
    ui_window_invalidate(self->reform_button);
    ui_button_show((struct UIButton *)(self->detail_button), 0);
    last = (struct ItemInfo *)self->materials.last;
    for (material = (struct ItemInfo *)self->materials.first; material != last; material++)
        if (inventory_count_of(material) < material->count)
            break;
    if (material == last) {
        ((struct UIButton *)self->reform_button)->disabled = 0;
        ui_window_invalidate(self->reform_button);
    } else {
        set_notice(self, MSG_MATERIAL_MISSING);
    }

    string_assign(&self->information, string_cstr(&info->information), info->information.size);
    if (self->information.size != 0) {
        ui_button_show((struct UIButton *)(self->detail_button), 1);
        set_notice(self, MSG_SEE_DETAILS);
    }

    self->base_slot->item = self->base_item;
    ui_window_invalidate_base(&self->base_slot->wnd);
    self->result_slot->item = &self->result;
    ui_window_invalidate_base(&self->result_slot->wnd);
    for (i = 0; i < (unsigned)VEC_COUNT(&self->materials, struct ItemInfo); i++) {
        struct UIItemReformWnd_Item *s;

        if (i >= (unsigned)VEC_COUNT(&self->material_slots, struct UIItemReformWnd_Item *))
            break;
        s = ((struct UIItemReformWnd_Item **)self->material_slots.first)[i];
        s->item = (struct ItemInfo *)self->materials.first + i;
        ui_window_invalidate_base(&s->wnd);
    }
    ui_window_invalidate_base(&self->frame.base);
}

static void detail_popup_set_text(struct UIWindow *popup, const struct String *text)
{
    struct CreateLineInfo lines;
    struct LineStyle style = {0, 12, 0, 0};
    size_t i;
    unsigned bottom;

    ui_window_resize(popup, 0x101, 0x7f);
    bottom = popup->height;
    create_line_info_construct(&lines, popup, string_cstr(text), popup->width - 0x14, 4, style, 0, 0, 0);
    for (i = 0; i < lines.count; ++i) {
        struct UIWindow *child = lines.windows[i];
        ui_window_move(child, child->x + 0xc, child->y + 0xc);
        ui_window_add_child(popup, child);
        if ((unsigned)(child->y + 0xc) > bottom)
            bottom = child->y + 0xc;
    }
    if (bottom > (unsigned)popup->height) ui_window_resize(popup, popup->width, bottom);
    create_line_info_destruct(&lines);
}

int item_reform_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                        int a, int b, int c, int d)
{
    struct UIItemReformWnd *self = (struct UIItemReformWnd *)window;
    switch (msg) {
    case UI_MSG_BUTTON:
        if (a == REFORM_BUTTON_ID) {
            if (self->base_item && item_info_get_item_id(&self->reform_item) != 0) {
                struct ReformItemPacket packet;
                int len;

                packet.id = PACKET_REFORM_ITEM;
                packet.reform_item_id = item_info_get_item_id(&self->reform_item);
                packet.base_index = self->base_item->index;
                len = rag_connection_get_packet_length(rag_connection_instance(), (short)packet.id);
                rag_connection_send_packet(rag_connection_instance(), len, &packet);
            }
        } else if (a == REFORM_CLOSE_BUTTON_ID) {
            ui_window_mgr_post_quit1(&g_windowMgr, REFORM_WND_ID);
        } else if (a == REFORM_LIST_PRESSED) {
            set_base_item(self, (struct ItemInfo *)b);
        }
        break;
    case UI_MSG_REFORM_ITEM:
        item_info_set_item_id(&self->reform_item, a);
        if (ui_window_send(&self->list->frame.base, &self->frame.base, UI_MSG_REFORM_ITEM, a, 0, 0, 0) == -1)
            set_notice(self, MSG_NOTHING_TO_REFORM);
        ui_window_invalidate_base(&self->frame.base);
        break;
    case UI_MSG_DETAIL_ENTER:
        if (sender == self->detail_button && !self->detail_popup) {
            struct UIWindow *popup = mem_alloc(sizeof *popup);

            if (popup) {
                ui_window_construct(popup, 0);
                popup->ops = &item_reform_detail_msg_ops;
            }
            self->detail_popup = popup;
            detail_popup_set_text(popup, &self->information);
            ui_window_mgr_add_window(&g_windowMgr, self->detail_popup);
        }
        break;
    case UI_MSG_DETAIL_LEAVE:
        if (sender == self->detail_button && self->detail_popup) {
            ui_window_mgr_post_quit2(&g_windowMgr, self->detail_popup);
            self->detail_popup = NULL;
        }
        break;
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, a, b, c, d);
}

void item_reform_item_delete(struct UIWindow *window)
{
    struct UIItemReformWnd_Item *self = (struct UIItemReformWnd_Item *)window;
    self->wnd.ops = &item_reform_item_ops;
    ui_window_destruct(&self->wnd);
    mem_free(self);
}

void item_reform_item_on_draw(struct UIWindow *window)
{
    struct UIItemReformWnd_Item *self = (struct UIItemReformWnd_Item *)window;
    struct String path;

    ui_window_clear_canvas(&self->wnd, COLOR_KEY);
    if (!self->item)
        return;
    string_init(&path);
    string_format(&path, ITEM_ICON_FMT, item_info_get_res_name(self->item));
    ui_window_draw_bitmap(&self->wnd, 0, 0, res_mgr_get(g_resMgr(), string_cstr(&path)), 1);
    string_free(&path);
}

void item_reform_item_show_name(struct UIWindow *window, int x, int y)
{
    struct UIItemReformWnd_Item *self = (struct UIItemReformWnd_Item *)window;
    struct String name = {0};

    if (!self->item)
        return;
    ui_window_to_screen(&self->wnd, &x, &y);
    item_info_get_display_name(self->item, &name);
    mode_show_tooltip(client_current_mode(), string_cstr(&name), x + 5, y - 5, -1, 0);
    string_free(&name);
}

void item_reform_item_show_info(struct UIWindow *window, int x, int y)
{
    struct UIItemReformWnd_Item *self = (struct UIItemReformWnd_Item *)window;
    struct UIWindow *desc;

    (void)x; (void)y;
    if (!self->item)
        return;
    desc = ui_window_open(ITEM_DESC_WND_ID);
    if (desc)
        ui_window_send(desc, NULL, UI_MSG_SHOW_ITEM, (int)self->item, 0, 0, 0);
}

void item_reform_detail_btn_delete(struct UIWindow *self)
{
    self->ops = &item_reform_detail_btn_ops.base;
    ui_button_destruct((struct UIButton *)self);
    mem_free(self);
}

void item_reform_detail_btn_on_draw(struct UIWindow *self)
{
    struct String caption;
    const char *text = msg_str(MSG_DETAILS);
    int width;

    ui_window_clear_canvas(self, COLOR_KEY);
    string_init(&caption);
    string_assign_cstr(&caption, text);
    width = ui_window_text_width(self, string_cstr(&caption), 0, 0, 0xc, 0, 0);
    ui_window_text_out_centred(self, 0, 0, string_cstr(&caption), self->width, 0, 0xc, 0, 0);
    ui_window_fill_rect(self, (int)((double)(unsigned)(self->width - width) * 0.5), 0xc, width, 1, 0);
    string_free(&caption);
}

void item_reform_detail_btn_enter(struct UIWindow *self)
{
    if (self->parent)
        ui_window_send(self->parent, self, UI_MSG_DETAIL_ENTER, 0, 0, 0, 0);
}

void item_reform_detail_btn_leave(struct UIWindow *self)
{
    if (self->parent)
        ui_window_send(self->parent, self, UI_MSG_DETAIL_LEAVE, 0, 0, 0, 0);
}

void item_reform_detail_msg_delete(struct UIWindow *self)
{
    self->ops = &item_reform_detail_msg_ops;
    ui_window_destruct(self);
    mem_free(self);
}

void item_reform_detail_msg_on_draw(struct UIWindow *self)
{
    ui_window_clear_canvas(self, COLOR_KEY);
    ui_window_fill_round_rect(self, 0, 0, self->width, self->height, 0xffffff);
    ui_window_draw_round_frame(self, 2, 2, self->width - 4, self->height - 4, 0xc8c8c8);
    ui_window_fill_rect(self, 8, 8, self->width - 0x10, self->height - 0x10, 0xf7f7f7);
}
