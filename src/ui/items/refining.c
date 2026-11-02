#include "ui/controls/buttons.h"
#include "contents/sound.h"
#include "session/state.h"
#include "core/object_type_tables.h"
#include "game/context.h"
#include "contents/connection_api.h"
#include "ui/items/refining_internal.h"
#include "ui/items/refine_controls.h"
#include "core/msg_str.h"
#include "diag/perf.h"
#include "session/session_items.h"

void ui_refining_wnd_destroy(struct UIWindow *window);
void ui_refining_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_refining_wnd_on_process(struct UIWindow *self);
int ui_refining_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_refining_wnd_ops = {
    .destroy = ui_refining_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_refining_wnd_drag_and_drop,
    .store_info = ui_window_store_info_base,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_refining_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_refining_wnd_on_process,
    .on_draw = ui_refining_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_refining_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_refining_wnd_on_l_btn_dbl_clk,
    .on_mouse_move = ui_frame_wnd_on_mouse_move,
    .on_mouse_hover = ui_refining_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_refining_wnd_on_l_btn_up,
    .on_r_btn_down = ui_refining_wnd_on_r_btn_down,
    .on_r_btn_up = ui_refining_wnd_on_r_btn_up,
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_refining_wnd_send_msg,
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

int refining_selected_zeny(struct UIRefiningWnd *self)
{
    unsigned n;

    if (!self->item.index)
        return 0;
    n = MATERIAL_COUNT(&self->materials);
    if (!n || self->selected < 0 || (unsigned)self->selected >= n)
        return 0;
    return MATERIAL(&self->materials, self->selected)->zeny;
}

BYTE refining_selected_chance(struct UIRefiningWnd *self)
{
    unsigned n;

    if (!self->item.index)
        return 0;
    n = MATERIAL_COUNT(&self->materials);
    if (!n || self->selected < 0 || (unsigned)self->selected >= n)
        return 0;
    return MATERIAL(&self->materials, self->selected)->chance;
}

int refining_carried(int item_id)
{
    struct ItemInfo inv;
    int count;

    session_get_inventory_item_by_id(&g_session1, &inv, item_id, 0);
    count = inv.count;
    item_info_destruct(&inv);
    return count;
}

static void send_packet(const void *packet, int id)
{
    int len = rag_connection_get_packet_length(rag_connection_instance(), id);

    rag_connection_send_packet(rag_connection_instance(), len, packet);
}

static void warn_again(struct UIRefiningWnd *self, int msg)
{
    ui_refining_notice_set_text(self->notice, msg_str(msg), NOTICE_TIME, 1);
    self->btn_again->button.button.disabled = 1;
    ui_window_invalidate_base(&self->btn_again->button.button.base);
}

static void show_ready_buttons(struct UIRefiningWnd *self)
{
    ui_refine_button_set_show((struct UIButton *)(self->btn_refine), 1);
    self->btn_refine->button.button.disabled = 1;
    ui_window_invalidate_base(&self->btn_refine->button.button.base);
    ui_button_set_show((struct UIButton *)(self->btn_return), 0);
    ui_refine_button_set_show((struct UIButton *)(self->btn_again), 0);
    ui_window_invalidate_base(&self->frame.base);
}

struct UIRefiningWnd *ui_refining_wnd_construct(struct UIRefiningWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_refining_wnd_ops;
    self->btn_refine = NULL;
    self->btn_return = NULL;
    self->state = REFINE_WAIT;
    self->result = REFINE_NO_RESULT;
    self->frame_time = 0x64;
    self->frame_start = 0;
    self->anim_frame = 0;
    self->selected = -1;
    self->materials.first = self->materials.last = self->materials.end = NULL;
    self->blessing = 0;
    self->blessing_item = 0;
    self->blessing_count = 0;
    item_info_construct_with_arg(&self->item);
    self->busy = 0;
    self->next_materials.first = self->next_materials.last = self->next_materials.end = NULL;
    self->next_blessing_count = 0;
    item_info_construct_with_arg(&self->next_item);
    self->blessing_item = BLACKSMITH_BLESSING;
    return self;
}

struct UIWindow *ui_refining_wnd_new(void)
{
    return (struct UIWindow *)ui_refining_wnd_construct(mem_alloc(sizeof(struct UIRefiningWnd)));
}

void ui_refining_wnd_destroy(struct UIWindow *window)
{
    struct UIRefiningWnd *self = (struct UIRefiningWnd *)window;
    self->frame.base.ops = &ui_refining_wnd_ops;
    item_info_destruct(&self->next_item);
    vec_free(&self->next_materials);
    item_info_destruct(&self->item);
    vec_free(&self->materials);
    ui_frame_wnd_destruct(&self->frame);
    mem_free(self);
}

void ui_refining_wnd_reset(struct UIRefiningWnd *self)
{
    struct ItemInfo empty;

    self->busy = 0;
    item_info_construct_with_arg(&empty);
    item_info_assign(&self->item, &empty);
    item_info_destruct(&empty);
    self->selected = -1;
    self->materials.last = self->materials.first;
    self->blessing = 0;
    self->blessing_count = 0;
    ui_refining_notice_set_text(self->notice, "", 0, 0);
    self->btn_refine->price = 0;
    ui_window_invalidate_base(&self->btn_refine->button.button.base);
    self->btn_refine->button.button.disabled = 1;
    ui_window_invalidate_base(&self->btn_refine->button.button.base);
    self->state = REFINE_WAIT;
    self->result = REFINE_NO_RESULT;
    self->frame_start = timeGetTime();
    self->anim_frame = 0;
    self->result = REFINE_NO_RESULT;
    show_ready_buttons(self);
    ui_window_invalidate_base(&self->frame.base);
}

void ui_refining_wnd_refresh(struct UIRefiningWnd *self)
{
    struct RefineMaterial *m, *n;
    unsigned next_count, index = 0;
    BYTE same;

    ui_refining_notice_set_text(self->notice, "", 0, 0);
    item_info_assign(&self->item, &self->next_item);
    ui_refine_button_set_show((struct UIButton *)(self->btn_refine), 0);
    ui_button_set_show((struct UIButton *)(self->btn_return), 1);
    ui_refine_button_set_show((struct UIButton *)(self->btn_again), 1);
    switch (self->result) {
    case REFINE_SUCCESS:
        self->btn_return->highlight = 1;
        ui_refining_btn_return_set_bitmaps(self->btn_return);
        self->btn_again->highlight = 1;
        ui_refining_btn_again_set_bitmaps(self->btn_again);
        break;
    case REFINE_FAILURE:
        self->btn_return->highlight = 0;
        ui_refining_btn_return_set_bitmaps(self->btn_return);
        self->btn_again->highlight = 0;
        ui_refining_btn_again_set_bitmaps(self->btn_again);
        self->btn_again->button.button.disabled = 1;
        ui_window_invalidate_base(&self->btn_again->button.button.base);
        break;
    case REFINE_DOWNGRADE:
    case REFINE_FAILURE_KEPT:
        self->btn_return->highlight = 0;
        ui_refining_btn_return_set_bitmaps(self->btn_return);
        self->btn_again->highlight = 0;
        ui_refining_btn_again_set_bitmaps(self->btn_again);
        break;
    }

    if (self->next_materials.first != self->next_materials.last) {
        int id = MATERIAL(&self->materials, self->selected)->item_id;

        for (n = self->next_materials.first; n != self->next_materials.last && n->item_id != id; n++)
            index++;
    }

    if (g_session1.zeny < refining_selected_zeny(self)) {
        ui_refining_notice_set_text(self->notice, msg_str(MSG_AGAIN_NO_ZENY), NOTICE_TIME, 1);
        self->btn_again->button.button.disabled = 1;
    } else {
        self->btn_again->price = refining_selected_zeny(self);
    }
    ui_window_invalidate_base(&self->btn_again->button.button.base);

    self->blessing_count = self->next_blessing_count;
    if (self->blessing == 1) {
        if (!self->blessing_count) {
            self->blessing = 0;
            warn_again(self, MSG_MATERIALS_CHANGED);
        } else {
            ui_refining_notice_set_text(self->notice, msg_str(MSG_BLESSING_ON), 0, 0);
            if (self->blessing_count > refining_carried(self->blessing_item)) {
                self->blessing = 0;
                warn_again(self, MSG_AGAIN_NO_BLESSINGS);
            }
        }
    }

    next_count = MATERIAL_COUNT(&self->next_materials);
    if (!next_count) {
        self->materials.last = self->materials.first;
        self->selected = -1;
        warn_again(self, MSG_CANNOT_REFINE);
        goto done;
    }
    same = 1;
    if (MATERIAL_COUNT(&self->materials) != next_count) {
        same = 0;
    } else {
        for (m = self->materials.first; m != self->materials.last; m++) {
            for (n = self->next_materials.first; n != self->next_materials.last; n++)
                if (n->item_id == m->item_id)
                    break;
            if (n == self->next_materials.last) {
                same = 0;
                break;
            }
        }
    }
    vec_assign(&self->materials, self->next_materials.first, self->next_materials.last,
               sizeof(struct RefineMaterial));
    self->btn_again->count = refining_selected_chance(self);
    ui_window_invalidate_base(&self->btn_again->button.button.base);
    if (!same) {
        self->selected = -1;
        warn_again(self, MSG_MATERIALS_CHANGED);
        goto done;
    }
    if (self->selected >= 0 && (unsigned)self->selected < MATERIAL_COUNT(&self->materials)) {
        struct ItemInfo material;
        const char *name;
        int id;

        self->selected = (int)index;
        id = MATERIAL(&self->materials, index)->item_id;
        item_info_construct(&material);

        item_info_set_item_id(&material, id);
        name = item_info_get_name(&material);
        ui_bitmap_button_set_tool_tip(&self->btn_again->button, name);
        if (!refining_carried(id))
            warn_again(self, MSG_MATERIAL_GONE);
        item_info_destruct(&material);
    }
done:
    self->busy = 0;
    ui_window_invalidate_base(&self->frame.base);
}

void ui_refining_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UIRefiningWnd *self = (struct UIRefiningWnd *)window;
    struct UIWindow *wnd = WND(self);
    struct UIBasicButton *close;
    struct UIRefining_BtnRefining *refine;
    struct UIRefining_BtnReturn *ret;
    struct UIRefining_BtnAgain *again;
    struct UIRefining_Notice *notice;

    ui_window_add_sys_button(wnd);
    ui_window_add_close_button(wnd, "", 0);

    close = mem_alloc(sizeof *close);
    if (close)
        close = ui_basic_button_construct(close);
    ui_basic_button_set_label(&close->button.base, 0x38, msg_str(MSG_CLOSE));
    ui_window_move_base(&close->button.base, 0xcc, 0x142);
    close->button.base.id = ID_CLOSE;
    ui_window_add_child(wnd, &close->button.base);

    refine = mem_alloc(sizeof *refine);
    if (refine) {
        ui_bitmap_button_construct(&refine->button);
        refine->button.button.base.ops = &ui_refining_btn_refining_ops.base;
        refine->price = 0;
    }
    self->btn_refine = refine;
    ui_refining_btn_refining_set_bitmaps(refine);
    ui_window_move_base(&self->btn_refine->button.button.base, 0x54, 0x106);
    self->btn_refine->button.button.base.id = ID_REFINE;
    self->btn_refine->button.button.disabled = 1;
    ui_window_invalidate_base(&self->btn_refine->button.button.base);
    ui_window_add_child(wnd, &self->btn_refine->button.button.base);

    ret = mem_alloc(sizeof *ret);
    if (ret) {
        ui_bitmap_button_construct(&ret->button);
        ret->button.button.base.ops = &ui_refining_btn_return_ops.base;
        ret->highlight = 1;
    }
    self->btn_return = ret;
    ui_refining_btn_return_set_bitmaps(ret);
    ui_window_move_base(&self->btn_return->button.button.base, 0x14, 0x10b);
    self->btn_return->button.button.base.id = ID_RETURN;
    ui_button_set_show((struct UIButton *)(self->btn_return), 0);
    ui_window_add_child(wnd, &self->btn_return->button.button.base);

    again = mem_alloc(sizeof *again);
    if (again) {
        ui_bitmap_button_construct(&again->button);
        again->button.button.base.ops = &ui_refining_btn_again_ops.base;
        again->price = 0;
    }
    self->btn_again = again;
    ui_refining_btn_again_set_bitmaps(again);
    ui_window_move_base(&self->btn_again->button.button.base, 0x8e, 0x10b);
    self->btn_again->button.button.base.id = ID_AGAIN;
    ui_refine_button_set_show((struct UIButton *)(self->btn_again), 0);
    ui_window_add_child(wnd, &self->btn_again->button.button.base);

    notice = mem_alloc(sizeof *notice);
    if (notice) {
        ui_static_text_construct(&notice->base.base);
        notice->base.base.ops = &ui_refining_notice_ops;
        string_init(&notice->next_text);
        notice->next_red = 0;
        notice->due = 0;
        notice->hide_when_due = 0;
        notice->red = 0;
    }
    self->notice = notice;
    ui_window_create(&notice->base.base, 0xc2, 0x16);
    ui_window_move_base(&self->notice->base.base, 6, 0x142);
    ui_window_add_child(wnd, &self->notice->base.base);
}

static void item_compose_name(struct UIRefiningWnd *self, struct ItemInfo *item)
{
    struct ItemNameRuns runs = {0};

    item_info_name_runs(WND(self), item, &runs, 1, 0);
    if (item->identified == 1 && item->option_count)
        string_format(&runs.text, msg_str(MSG_NAME_OPTIONS), string_cstr(&runs.text), item->option_count);
    item_info_set_name(item, string_cstr(&runs.text));
    string_free(&runs.text);
}

void ui_refining_wnd_set_item(struct UIRefiningWnd *self, int index, int count, const void *materials,
                           BYTE blessing_count)
{
    const struct RefineMaterial *list = materials;
    struct ItemInfo inv;

    switch (self->state) {
    case REFINE_WAIT:
    case REFINE_READY:
        if (index == -1 || !count) {
            ui_refining_notice_set_text(self->notice, msg_str(MSG_CANNOT_REFINE), NOTICE_TIME_SHORT, 1);
            return;
        }
        session_get_inventory_item(&g_session1, &inv, index);
        if (inv.index) {
            item_info_assign(&self->item, &inv);
            item_compose_name(self, &self->item);
            vec_assign(&self->materials, list, list + count, sizeof *list);
            self->selected = -1;
            self->blessing_count = blessing_count;
            self->blessing = 0;
            ui_refining_notice_set_text(self->notice, "", 0, 0);
            self->state = REFINE_READY;
            self->result = REFINE_NO_RESULT;
            self->frame_start = timeGetTime();
            self->anim_frame = 0;
            self->result = REFINE_NO_RESULT;
            show_ready_buttons(self);
            self->busy = 0;
        }
        item_info_destruct(&inv);
        break;
    case REFINE_PROCESS:
    case REFINE_SUCCESS:
    case REFINE_FAILURE:
    case REFINE_DOWNGRADE:
    case REFINE_FAILURE_KEPT:
        if (index == -1 || self->item.index != index)
            return;
        session_get_inventory_item(&g_session1, &inv, index);
        if (inv.index) {
            item_info_assign(&self->next_item, &inv);
            item_compose_name(self, &self->next_item);
            vec_assign(&self->next_materials, list, list + count, sizeof *list);
            self->next_blessing_count = blessing_count;
        }
        item_info_destruct(&inv);
        break;
    }
}

void ui_refining_wnd_set_state(struct UIRefiningWnd *self, int state, int result)
{
    int msg;

    self->state = state;
    self->result = result;
    self->frame_start = timeGetTime();
    self->anim_frame = 0;
    switch (state) {
    case REFINE_WAIT:
    case REFINE_READY:
        self->result = REFINE_NO_RESULT;
    case REFINE_PROCESS:
        show_ready_buttons(self);
        return;
    case REFINE_SUCCESS:
        msg = MSG_CHAT_SUCCESS;
        break;
    case REFINE_FAILURE:
    case REFINE_FAILURE_KEPT:
        msg = MSG_CHAT_FAILURE;
        break;
    case REFINE_DOWNGRADE:
        msg = MSG_CHAT_DOWNGRADE;
        break;
    default:
        ui_window_invalidate_base(&self->frame.base);
        return;
    }
    ui_window_mgr_send_msg_(&g_windowMgr, UIWM_MSG_CHAT_LINE, (int)msg_str(msg), COLOUR_CHAT, 0, 0);
    ui_window_invalidate_base(&self->frame.base);
}

void ui_refining_wnd_on_process(struct UIWindow *window)
{
    struct UIRefiningWnd *self = (struct UIRefiningWnd *)window;
    static const int last_frame[7] = { 4, 3, 0xe, 0x11, 0x14, 0x14, 0x14 };
    int state;

    if (self->notice)
        ui_refining_notice_on_process(&self->notice->base.base);
    state = self->state;
    if (state == REFINE_READY && self->btn_refine->button.button.disabled == 1 && self->item.index &&
        self->selected >= 0 && (unsigned)self->selected < MATERIAL_COUNT(&self->materials)) {
        self->btn_refine->button.button.disabled = 0;
        ui_window_invalidate_base(&self->btn_refine->button.button.base);
        self->btn_refine->price = refining_selected_zeny(self);
        ui_window_invalidate_base(&self->btn_refine->button.button.base);
        ui_window_invalidate_base(&self->frame.base);
        state = self->state;
    }
    if (last_frame[state] <= self->anim_frame || timeGetTime() <= (DWORD)(self->frame_time + self->frame_start))
        return;

    self->anim_frame++;
    if (self->state == REFINE_PROCESS) {
        if (self->anim_frame == 7)
            sound_play_wave("effect\\refine\\bs_refine_1.wav", 0.0f, 0.0f, 0.0f, 0xfa, 0x28, 1.0f);
        if (self->anim_frame == 0xc)
            sound_play_wave("effect\\refine\\bs_refine_2.wav", 0.0f, 0.0f, 0.0f, 0xfa, 0x28, 1.0f);
    }
    state = self->state;
    if (last_frame[state] <= self->anim_frame) {
        switch (state) {
        case REFINE_WAIT:
            self->anim_frame = 0;
            break;
        case REFINE_SUCCESS:
            self->anim_frame = 0xd;
            break;
        case REFINE_FAILURE:
        case REFINE_DOWNGRADE:
        case REFINE_FAILURE_KEPT:
            self->anim_frame = 0x12;
            break;
        default:
            self->anim_frame = last_frame[state] - 1;
            break;
        }
        if (state == REFINE_PROCESS) {
            if (self->result == REFINE_SUCCESS)
                sound_play_wave("effect\\refine\\bs_refine_success.wav", 0.0f, 0.0f, 0.0f, 0xfa,
                                0x28, 1.0f);
            else if (self->result == REFINE_FAILURE || self->result == REFINE_DOWNGRADE ||
                     self->result == REFINE_FAILURE_KEPT)
                sound_play_wave("effect\\refine\\bs_refine_failed.wav", 0.0f, 0.0f, 0.0f, 0xfa,
                                0x28, 1.0f);
            ui_refining_wnd_refresh(self);
            ui_refining_wnd_set_state(self, self->result, REFINE_NO_RESULT);
            return;
        }
    }
    self->frame_start = timeGetTime();
    ui_window_invalidate_base(&self->frame.base);
}

int ui_refining_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1, int val2,
                                     int val3, int val4)
{
    struct UIRefiningWnd *self = (struct UIRefiningWnd *)window;
    if (msg == UI_MSG_COMMAND) {
        switch (val1) {
        case ID_CLOSE:
        case UI_ID_CLOSE: {
            WORD packet = PACKET_REFINING_CLOSE;

            send_packet(&packet, PACKET_REFINING_CLOSE);
            ui_window_mgr_post_quit1(&g_windowMgr, WID_REFINE);
            break;
        }
        case ID_REFINE:
            if (self->busy == 1 || !self->item.index)
                break;
            if (g_session1.zeny < refining_selected_zeny(self)) {
                ui_refining_notice_set_text(self->notice, msg_str(MSG_NO_ZENY), NOTICE_TIME_SHORT, 1);
            } else {
                struct PacketReqRefining packet;

                self->busy = 1;
                packet.id = PACKET_REQ_REFINING;
                packet.index = (WORD)self->item.index;
                packet.material = MATERIAL(&self->materials, self->selected)->item_id;
                packet.blessing = self->blessing;
                send_packet(&packet, PACKET_REQ_REFINING);
            }
            break;
        case ID_RETURN:
            ui_refining_notice_set_text(self->notice, "", 0, 0);
            self->selected = -1;
            self->blessing = 0;
            if (self->state == REFINE_FAILURE) {
                ui_refining_wnd_reset(self);
                break;
            }
            self->state = REFINE_READY;
            self->result = REFINE_NO_RESULT;
            self->frame_start = timeGetTime();
            self->anim_frame = 0;
            self->result = REFINE_NO_RESULT;
            show_ready_buttons(self);
            break;
        case ID_AGAIN:
            ui_refining_wnd_send_msg(&self->frame.base, WND(self), UI_MSG_COMMAND, ID_REFINE, 0, 0, 0);
            break;
        }
    } else if (msg == MSG_ITEM_DROPPED && (self->state == REFINE_WAIT || self->state == REFINE_READY)) {
        struct ItemInfo inv;

        session_get_inventory_item(&g_session1, &inv, val1);
        if (inv.index) {
            if (inv.identified == 1) {
                struct PacketRefiningSelectItem packet;

                packet.id = PACKET_REFINING_SELECT_ITEM;
                packet.index = (WORD)val1;
                send_packet(&packet, PACKET_REFINING_SELECT_ITEM);
            } else {
                ui_refining_notice_set_text(self->notice, msg_str(MSG_CANNOT_REFINE), NOTICE_TIME_SHORT, 1);
            }
        }
        item_info_destruct(&inv);
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
}
