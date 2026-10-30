#include "ui/controls/buttons.h"
#include "render/rp_pool.h"
#include "core/object_type_tables.h"
#include "world/mode_manager.h"
#include "ui/window_access.h"
#include "contents/connection_api.h"
#include <string.h>
#include <stdlib.h>
#include "ui/window_mgr.h"
#include "ui/game.h"
#include "ui/ui_items.h"
#include "ui/controls/controls.h"
#include "ui/draw.h"
#include "ui/skin.h"
#include <mmsystem.h>
#include "core/msg_str.h"
#include "diag/perf.h"
#include "ui/wndmgr/make_window_internal.h"

struct UIStorePasswordInputWnd {
    struct UIFrameWnd frame;
    int selected_row;
    int last_row;
    char pw[3][9];
    int typed;
    int star_x;
    int step;
    int star_y;
    int row_y[3];
    BYTE changing;
};

struct UISecondPasswordInputWnd {
    struct UIStorePasswordInputWnd base;
    int length;
    BYTE perm[10];
    int mode;
    int attempts;
    struct UIBitmapButton *keys[11];
    struct UIBitmapButton *big[5];
    struct UIWindow *notice;
};
#define BIG_OK 0
#define BIG_CHANGE 1
#define BIG_UNUSED 2
#define BIG_NEXT 3

#define PWD_MSG_BUTTON 6
#define PWD_ID_OK 0xb0
#define PWD_ID_CANCEL 0xb1
#define PWD_ID_NEXT 0xd2
#define PWD_ID_CHANGE 0x11c
#define PWD_ID_DIGIT0 0x11d
#define PWD_ID_RESET 0x127
#define PWD_ID_ENTER 0x128
#define PWD_ID_UNUSED 0x19b

#define WHITE 0xffffff
#define FIELD_BACK 0xfae6c8

static const char g_star[] = "*";

static void pwd_box(int msg)
{
    ui_window_mgr_message_box(&g_windowMgr, msg_str(msg), UI_MSGBOX_OK, 1, 0, 0, "메시지", 0x118, 0x78, 0);
}

static void button_disable(struct UIBitmapButton *button, BYTE disabled)
{
    button->button.disabled = disabled;
    ui_window_invalidate_base(&button->button.base);
}

void ui_store_password_input_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_store_password_input_wnd_on_draw(struct UIWindow *self);
int ui_store_password_input_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
void ui_store_password_input_wnd_destroy(struct UIWindow *window);
const struct UIWindowOps ui_store_password_input_wnd_ops = {
    .destroy = ui_store_password_input_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_window_store_info_base,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_store_password_input_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_store_password_input_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_frame_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_frame_wnd_on_mouse_move,
    .on_mouse_hover = ui_frame_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_frame_wnd_on_l_btn_up,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_store_password_input_wnd_send_msg,
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

static void store_password_construct(struct UIStorePasswordInputWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_store_password_input_wnd_ops;
    self->selected_row = -1;
    self->last_row = 2;
    memset(self->pw, 0, sizeof self->pw);
    self->star_x = 0;
    self->changing = 0;
    self->step = 0;
    self->typed = 0;
    self->star_y = 0x28;
    self->row_y[0] = 0x28;
    self->row_y[1] = 0x50;
    self->row_y[2] = 0x77;
    if (g_session1.pinChangeOnly) {
        self->step = 2;
        self->star_y = 0x50;
        self->typed = 0;
        self->star_x = 0;
        memset(self->pw, 0, sizeof self->pw);
    }
}

struct UISecondPasswordInputWnd *ui_second_password_input_wnd_construct(struct UISecondPasswordInputWnd *self)
{
    int i;

    store_password_construct(&self->base);
    self->base.frame.base.ops = &ui_second_password_input_wnd_ops;
    self->length = 4;
    self->mode = 0;
    self->attempts = 0;
    self->notice = NULL;
    for (i = 0; i < 11; i++)
        self->keys[i] = NULL;
    self->big[BIG_OK] = self->big[BIG_CHANGE] = self->big[BIG_UNUSED] = NULL;
    return self;
}

struct UIWindow *ui_second_password_input_wnd_new(void)
{
    return (struct UIWindow *)ui_second_password_input_wnd_construct(mem_alloc(sizeof(struct UISecondPasswordInputWnd)));
}

struct PwdRandom {
    DWORD state;
    DWORD mul;
    DWORD add;
};

static void pwd_shuffle(BYTE *first, BYTE *last, struct PwdRandom *rng)
{
    BYTE *it;
    unsigned i;

    if (first == last)
        return;
    for (it = first + 1, i = 1; it != last; it++, i++) {
        unsigned j;

        rng->state = rng->state * rng->mul + rng->add;
        j = rng->state % (i + 1);
        if (j != i) {
            BYTE t = *it;
            *it = first[j];
            first[j] = t;
        }
    }
}

static const int g_keyPos[11][2] = {
    { 0xdb, 0x4e }, { 0x117, 0x4e }, { 0x153, 0x4e }, { 0xdb, 0x8a }, { 0x117, 0x8a }, { 0x153, 0x8a },
    { 0xdb, 0xc6 }, { 0x117, 0xc6 }, { 0x153, 0xc6 }, { 0xdb, 0x102 }, { 0x117, 0x102 },
};

void ui_second_password_wnd_shuffle_keys(struct UISecondPasswordInputWnd *self)
{
    BYTE order[10];
    struct PwdRandom rng;
    int i;

    for (i = 0; i < 10; i++)
        order[i] = (BYTE)i;
    rng.state = timeGetTime();
    rng.mul = 0x3498;
    rng.add = 0x881234;
    pwd_shuffle(order, order + 10, &rng);
    for (i = 0; i < 10; i++)
        ui_window_move_base(&self->keys[i]->button.base, g_keyPos[order[i]][0], g_keyPos[order[i]][1]);
    ui_window_invalidate_base(&self->base.frame.base);
}

int ui_second_password_wnd_key_of(struct UISecondPasswordInputWnd *self, int digit)
{
    int i;

    for (i = 0; i < 10; i++)
        if (self->perm[i] == digit)
            return i;
    return -1;
}

void ui_second_password_wnd_disable_all(struct UISecondPasswordInputWnd *self)
{
    int i;

    for (i = 0; i < 11; i++)
        button_disable(self->keys[i], 1);
    for (i = 0; i < 5; i++)
        button_disable(self->big[i], 1);
}

void ui_second_password_wnd_set_mode(struct UISecondPasswordInputWnd *self, int mode)
{
    self->mode = mode;
    if (!mode)
        ui_static_text_set_text(self->notice, msg_str(0xa06), 0);
}

void ui_second_password_wnd_setup(struct UISecondPasswordInputWnd *self, DWORD seed)
{
    static const char *const big_names[5][4] = {
        { "btn_big_ok", "btn_big_ok_a", "btn_big_ok_b", "btn_big_ok_b" },
        { "btn_big_change", "btn_big_change_a", "btn_big_change_b", "btn_big_change_c" },
        { "btn_big_unused", "btn_big_unused_a", "btn_big_unused_b", "btn_big_unused_c" },
        { "btn_big_next", "btn_big_next_a", "btn_big_next_b", "btn_big_next_b" },
        { "btn_big_cancel", "btn_big_cancel_a", "btn_big_cancel_b", "btn_big_cancel_b" },
    };
    static const int big_pos[5][2] = { { 0x19e, 0xc0 }, { 0x19e, 0xdc }, { 0x19e, 0xf8 }, { 0x19e, 0xc0 },
                                       { 0x19e, 0x114 } };
    static const int big_ids[5] = { PWD_ID_OK, PWD_ID_CHANGE, PWD_ID_UNUSED, PWD_ID_NEXT, PWD_ID_CANCEL };
    static const char *const key_names[11][3] = {
        { "0-a", "0-b", "0-c" }, { "1-a", "1-b", "1-c" }, { "2-a", "2-b", "2-c" }, { "3-a", "3-b", "3-c" },
        { "4-a", "4-b", "4-c" }, { "5-a", "5-b", "5-c" }, { "6-a", "6-b", "6-c" }, { "7-a", "7-b", "7-c" },
        { "8-a", "8-b", "8-c" }, { "9-a", "9-b", "9-c" }, { "reset-a", "reset-b", "reset-c" },
    };
    static const int key_ids[11] = { 0x11d, 0x11e, 0x11f, 0x120, 0x121, 0x122, 0x123, 0x124, 0x125, 0x126,
                                     PWD_ID_RESET };
    struct UIStorePasswordInputWnd *s = &self->base;
    struct PwdRandom rng;
    struct String dir;
    struct UIWindow *text;
    void *mem;
    int i, state;

    s->star_y = 0x5b;
    s->row_y[0] = 0x5b;
    s->row_y[1] = 0x82;
    s->row_y[2] = 0xa8;
    for (i = 0; i < 10; i++)
        self->perm[i] = (BYTE)i;
    rng.state = seed;
    rng.mul = 0x3498;
    rng.add = 0x881234;
    pwd_shuffle(self->perm, self->perm + 10, &rng);

    ui_skin_dir(&dir);
    for (i = 0; i < 5; i++) {
        struct UIBitmapButton *button;

        mem = mem_alloc(sizeof(struct UIBitmapButton));
        button = mem ? ui_bitmap_button_construct(mem) : NULL;
        self->big[i] = button;
        for (state = 0; state < 4; state++)
            ui_button_bitmap(self->big[i], &dir, big_names[i][state], state);
        ui_window_create(&self->big[i]->button.base, self->big[i]->bitmap_width,
                         self->big[i]->bitmap_height);
        ui_window_move_base(&self->big[i]->button.base, big_pos[i][0], big_pos[i][1]);
        self->big[i]->button.base.id = big_ids[i];
        ui_window_add_child(&s->frame.base, &self->big[i]->button.base);
    }

    string_assign(&dir, UI_SKIN "basic_interface\\", 0x1f);
    for (i = 0; i < 10; i++) {
        int digit = self->perm[i];
        struct UIBitmapButton *button;

        mem = mem_alloc(sizeof(struct UIBitmapButton));
        button = mem ? ui_bitmap_button_construct(mem) : NULL;
        self->keys[i] = button;
        for (state = 0; state < 3; state++)
            ui_button_bitmap(self->keys[i], &dir, key_names[digit][state], state);
        ui_window_create(&self->keys[i]->button.base, self->keys[i]->bitmap_width,
                         self->keys[i]->bitmap_height);
        ui_window_move_base(&self->keys[i]->button.base, g_keyPos[i][0], g_keyPos[i][1]);
        self->keys[i]->button.base.id = key_ids[digit];
        ui_window_add_child(&s->frame.base, &self->keys[i]->button.base);
    }
    mem = mem_alloc(sizeof(struct UIBitmapButton));
    self->keys[10] = mem ? ui_bitmap_button_construct(mem) : NULL;
    for (state = 0; state < 3; state++)
        ui_button_bitmap(self->keys[10], &dir, key_names[10][state], state);
    ui_window_create(&self->keys[10]->button.base, self->keys[10]->bitmap_width,
                     self->keys[10]->bitmap_height);
    ui_window_move_base(&self->keys[10]->button.base, g_keyPos[10][0], g_keyPos[10][1]);
    self->keys[10]->button.base.id = PWD_ID_RESET;
    ui_window_add_child(&s->frame.base, &self->keys[10]->button.base);

    ui_button_show((struct UIButton *)(self->big[BIG_OK]), 0);
    if (g_session1.pinState == 2) {
        ui_button_show((struct UIButton *)(self->big[BIG_OK]), 1);
        ui_button_show((struct UIButton *)(self->big[BIG_NEXT]), 0);
    } else if ((unsigned)g_session1.pinState <= 1) {
        button_disable(self->big[BIG_CHANGE], 1);
        button_disable(self->big[BIG_UNUSED], 1);
    }
    ui_button_show((struct UIButton *)(self->big[BIG_UNUSED]), 0);

    mem = mem_alloc(sizeof(struct UIStaticText));
    self->notice = mem ? ui_static_text_construct(mem) : NULL;
    ui_window_create(self->notice, 0x12c, 0xf);
    ui_static_text_set_text(self->notice, msg_str(0x771), 0);
    ui_window_move(self->notice, 0xc8, 0x1e);
    ui_window_add_child(&s->frame.base, self->notice);
    mem = mem_alloc(sizeof(struct UIStaticText));
    text = mem ? ui_static_text_construct(mem) : NULL;
    ui_window_create(text, 0x140, 0xf);
    ui_static_text_set_text(text, msg_str(0x772), 0);
    ui_window_move(text, 0xc8, 0x32);
    ui_window_add_child(&s->frame.base, text);
    string_free(&dir);
}

#pragma pack(push, 1)
struct PwdPacket {
    WORD id;
    DWORD aid;
    DWORD pin;
};
struct PwdChangePacket {
    WORD id;
    DWORD aid;
    DWORD old_pin;
    DWORD new_pin;
};
#pragma pack(pop)

static void pwd_send(int id, const void *packet)
{
    int len = rag_connection_get_packet_length(rag_connection_instance(), id);

    rag_connection_send_packet(rag_connection_instance(), len, packet);
}

static DWORD pin_dword(const char *pw)
{
    DWORD v;

    memcpy(&v, pw, 4);
    return v;
}

static void send_pin(struct UISecondPasswordInputWnd *self, int id, const char *pw)
{
    struct PwdPacket packet;

    (void)atoi(pw);
    packet.id = (WORD)id;
    packet.aid = g_session1.pinAid;
    packet.pin = pin_dword(pw);
    pwd_send(id, &packet);
    ui_second_password_wnd_disable_all(self);
}

static void pwd_restart(struct UIStorePasswordInputWnd *s, int step, int row)
{
    s->step = step;
    s->star_y = s->row_y[row];
    s->typed = 0;
    s->star_x = 0;
}

static void second_pwd_confirm(struct UISecondPasswordInputWnd *self, int input)
{
    struct UIStorePasswordInputWnd *s = &self->base;
    struct UIWindow *wnd = &s->frame.base;

    switch (s->step) {
    case 0:
        if (strlen(s->pw[0]) == (size_t)self->length) {
            send_pin(self, input ? 0x8b8 : 0x8bc, s->pw[0]);
            return;
        }
        pwd_box(0x75f);
        pwd_restart(s, 0, 0);
        memset(s->pw[0], 0, 9);
        memset(s->pw[1], 0, 18);
        ui_button_show((struct UIButton *)(self->big[BIG_OK]), 0);
        ui_button_show((struct UIButton *)(self->big[BIG_NEXT]), 1);
        if (input)
            ui_window_invalidate_base(wnd);
        return;
    case 1:
        if (input && strlen(s->pw[0]) != (size_t)self->length) {
            pwd_box(0x75f);
            pwd_restart(s, 1, 0);
            memset(s->pw[0], 0, 9);
            memset(s->pw[1], 0, 18);
            ui_window_invalidate_base(wnd);
            return;
        }
        pwd_restart(s, 2, 1);
        memset(s->pw[1], 0, 18);
        ui_window_fill_rect(wnd, 0x19f, 0x80, 0x62, 0xe, WHITE);
        return;
    case 2:
        if (strlen(s->pw[1]) == (size_t)self->length) {
            pwd_restart(s, 3, 2);
            memset(s->pw[2], 0, 9);
            ui_window_fill_rect(wnd, 0x19f, 0xa7, 0x62, 0xe, WHITE);
            ui_button_show((struct UIButton *)(self->big[BIG_OK]), 1);
            ui_button_show((struct UIButton *)(self->big[BIG_NEXT]), 0);
            return;
        }
        pwd_box(0x75f);
        pwd_restart(s, 2, 1);
        memset(s->pw[1], 0, 9);
        memset(s->pw[2], 0, 9);
        ui_window_fill_rect(wnd, 0x19f, 0x80, 0x62, 0xe, WHITE);
        if (input)
            ui_window_invalidate_base(wnd);
        return;
    case 3:
        if (strlen(s->pw[2]) != (size_t)self->length) {
            pwd_box(0x75f);
            pwd_restart(s, 3, 2);
            memset(s->pw[2], 0, 9);
            ui_window_fill_rect(wnd, 0x19f, 0xa7, 0x62, 0xe, WHITE);
            if (input)
                ui_window_invalidate_base(wnd);
            return;
        }
        if (strcmp(s->pw[1], s->pw[2])) {
            pwd_box(0x3f8);
            pwd_restart(s, 2, 1);
            memset(s->pw[1], 0, 9);
            memset(s->pw[2], 0, 9);
            ui_window_fill_rect(wnd, 0x19f, 0x80, 0x62, 0xe, WHITE);
            ui_window_fill_rect(wnd, 0x19e, 0xa6, 0x64, 0x10, FIELD_BACK);
            ui_button_show((struct UIButton *)(self->big[BIG_OK]), 0);
            ui_button_show((struct UIButton *)(self->big[BIG_NEXT]), 1);
            return;
        }
        (void)atoi(s->pw[0]);
        (void)atoi(s->pw[2]);
        if (input) {
            struct PwdChangePacket packet;

            packet.id = 0x8be;
            packet.aid = g_session1.pinAid;
            packet.old_pin = pin_dword(s->pw[0]);
            packet.new_pin = pin_dword(s->pw[2]);
            pwd_send(0x8be, &packet);
        } else {
            struct PwdPacket packet;

            packet.id = 0x8ba;
            packet.aid = g_session1.pinAid;
            packet.pin = pin_dword(s->pw[2]);
            pwd_send(0x8ba, &packet);
        }
        ui_second_password_wnd_disable_all(self);
        return;
    }
}

static void second_pwd_digit(struct UISecondPasswordInputWnd *self, int key)
{
    struct UIStorePasswordInputWnd *s = &self->base;
    char *pw;

    if (++s->typed > self->length) {
        pwd_box(0x75f);
        return;
    }
    switch (s->step) {
    case 0:
    case 1:
        pw = s->pw[0];
        break;
    case 2:
        pw = s->pw[1];
        break;
    case 3:
        pw = s->pw[2];
        break;
    default:
        pw = NULL;
        break;
    }
    if (pw) {
        struct String text;
        const char *digits;

        string_init(&text);
        string_format(&text, "%d", ui_second_password_wnd_key_of(self, key));
        digits = string_cstr(&text);
        memcpy(pw + strlen(pw), digits, strlen(digits));
        string_free(&text);
    }
    ui_window_text_out(&s->frame.base, s->star_x + 0x1a0, s->star_y, g_star, 0, 0, 0xc, 0, 0, 0);
    s->star_x += 5;
}

static int second_pwd_send_msg(struct UISecondPasswordInputWnd *self, struct UIWindow *sender, int msg, int val1,
                               int val2, int val3, int val4, int input)
{
    struct UIStorePasswordInputWnd *s = &self->base;
    struct UIWindow *wnd = &s->frame.base;

    if (msg == PWD_MSG_BUTTON) {
        switch (val1) {
        case PWD_ID_OK:
        case PWD_ID_NEXT:
            second_pwd_confirm(self, input);
            break;
        case PWD_ID_CANCEL:
            if (ui_window_mgr_message_box(&g_windowMgr, msg_str(0x11), UI_MSGBOX_QUESTION, 1, 0, 0, "메시지",
                                       0x118, 0x78, 0) == UI_MSGBOX_ANSWER_YES) {
                mode_send_message(client_current_mode(), 2, 0, 0, 0, 0);
                ui_window_mgr_post_quit2(&g_windowMgr, wnd);
            }
            break;
        case PWD_ID_CHANGE:
            g_session1.pinRequest = 2;
            s->changing = 1;
            pwd_restart(s, 1, 0);
            memset(s->pw, 0, sizeof s->pw);
            ui_button_show((struct UIButton *)(self->big[BIG_OK]), 0);
            ui_button_show((struct UIButton *)(self->big[BIG_NEXT]), 1);
            button_disable(self->big[BIG_CHANGE], 1);
            button_disable(self->big[BIG_UNUSED], 1);
            ui_window_invalidate_base(wnd);
            break;
        case PWD_ID_DIGIT0:
        case PWD_ID_DIGIT0 + 1:
        case PWD_ID_DIGIT0 + 2:
        case PWD_ID_DIGIT0 + 3:
        case PWD_ID_DIGIT0 + 4:
        case PWD_ID_DIGIT0 + 5:
        case PWD_ID_DIGIT0 + 6:
        case PWD_ID_DIGIT0 + 7:
        case PWD_ID_DIGIT0 + 8:
        case PWD_ID_DIGIT0 + 9:
            second_pwd_digit(self, val1 - PWD_ID_DIGIT0);
            break;
        case PWD_ID_RESET:
            if (s->step == 0) {
                pwd_restart(s, 0, 0);
                memset(s->pw, 0, sizeof s->pw);
                ui_button_show((struct UIButton *)(self->big[BIG_OK]), 1);
                ui_button_show((struct UIButton *)(self->big[BIG_NEXT]), 0);
            } else if (g_session1.pinChangeOnly) {
                pwd_restart(s, 2, 1);
                memset(s->pw, 0, sizeof s->pw);
                ui_button_show((struct UIButton *)(self->big[BIG_OK]), 1);
                ui_button_show((struct UIButton *)(self->big[BIG_NEXT]), 0);
            } else {
                pwd_restart(s, 1, 0);
                memset(s->pw, 0, sizeof s->pw);
                ui_button_show((struct UIButton *)(self->big[BIG_OK]), 0);
                ui_button_show((struct UIButton *)(self->big[BIG_NEXT]), 1);
            }
            ui_second_password_wnd_shuffle_keys(self);
            break;
        case PWD_ID_ENTER:
            ui_window_send_msg(wnd, wnd, PWD_MSG_BUTTON, PWD_ID_OK, 0, 0, 0);
            break;
        case PWD_ID_UNUSED:
            if (strlen(s->pw[0]) == (size_t)self->length) {
                send_pin(self, 0x8c3, s->pw[0]);
            } else {
                pwd_box(0x75f);
                pwd_restart(s, 0, 0);
                memset(s->pw[0], 0, 9);
                memset(s->pw[1], 0, 18);
            }
            break;
        }
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&s->frame), sender, msg, val1, val2, val3, val4);
}

int ui_second_password_edit_wnd_send_msg(struct UIWindow *window,
                                               struct UIWindow *sender, int msg, int val1, int val2, int val3,
                                               int val4)
{
    struct UISecondPasswordInputWnd *self = (struct UISecondPasswordInputWnd *)window;

    return second_pwd_send_msg(self, sender, msg, val1, val2, val3, val4, 0);
}

int ui_second_password_input_wnd_send_msg(struct UIWindow *window,
                                                struct UIWindow *sender, int msg, int val1, int val2, int val3,
                                                int val4)
{
    struct UISecondPasswordInputWnd *self = (struct UISecondPasswordInputWnd *)window;

    return second_pwd_send_msg(self, sender, msg, val1, val2, val3, val4, 1);
}

static void draw_rows(struct UIStorePasswordInputWnd *s, int x, int y_text, int y_box, int y_text2, int y_box2,
                      int y_text3, int y_box3)
{
    struct UIWindow *wnd = &s->frame.base;
    COLORREF colour, colour2, back;

    if (g_session1.pinChangeOnly) {
        colour = 0xf0f0f0;
        colour2 = 0xe6e6e6;
        back = 0xe6e6e6;
    } else {
        colour = 0xe6e6e6;
        colour2 = 0;
        back = 0xfac8aa;
    }
    ui_window_text_out_doubled(wnd, x, y_text, msg_str(0x3f4), colour, colour2, 0, 0, 0xc);
    ui_window_fill_rect(wnd, x, y_box, 0x64, 0x10, back);
    ui_window_fill_rect(wnd, x + 1, y_box + 1, 0x62, 0xe, WHITE);
    if (!g_session1.pinChangeOnly && !s->changing) {
        ui_window_text_out_doubled(wnd, x, y_text2, msg_str(0x3f5), 0xf0f0f0, 0xe6e6e6, 0, 0, 0xc);
        ui_window_fill_rect(wnd, x, y_box2, 0x64, 0x10, 0xe6e6e6);
        ui_window_text_out_doubled(wnd, x, y_text3, msg_str(0x3f6), 0xf0f0f0, 0xe6e6e6, 0, 0, 0xc);
        ui_window_fill_rect(wnd, x, y_box3, 0x64, 0x10, 0xe6e6e6);
        return;
    }
    ui_window_text_out_doubled(wnd, x, y_text2, msg_str(0x3f5), 0xe6e6e6, 0, 0, 0, 0xc);
    ui_window_fill_rect(wnd, x, y_box2, 0x64, 0x10, FIELD_BACK);
    ui_window_text_out_doubled(wnd, x, y_text3, msg_str(0x3f6), 0xe6e6e6, 0, 0, 0, 0xc);
    ui_window_fill_rect(wnd, x, y_box3, 0x64, 0x10, FIELD_BACK);
    if (g_session1.pinChangeOnly)
        ui_window_fill_rect(wnd, x + 1, y_box2 + 1, 0x62, 0xe, WHITE);
}

void ui_second_password_input_wnd_on_draw(struct UIWindow *window)
{
    struct UISecondPasswordInputWnd *self = (struct UISecondPasswordInputWnd *)window;
    struct UIStorePasswordInputWnd *s = &self->base;
    struct UIWindow *wnd = &s->frame.base;
    int i, x;

    ui_window_draw_body(wnd, ui_window_draw_title_bar(wnd, 1, msg_str(0x76b), 0), 0x21c,
                      UI_SKIN "basic_interface\\2ndpassword_bg.bmp");
    ui_window_draw_button_bar(wnd, 2, 0, 1);
    draw_rows(s, 0x19e, 0x4c, 0x59, 0x70, 0x7f, 0x98, 0xa6);
    if (s->step == 3)
        ui_window_fill_rect(wnd, 0x19f, 0xa7, 0x62, 0xe, WHITE);
    if (s->step == 2 || s->step == 3)
        ui_window_fill_rect(wnd, 0x19f, 0x80, 0x62, 0xe, WHITE);
    for (i = 0, x = 0x1a0; (size_t)i < strlen(s->pw[0]); i++, x += 5)
        ui_window_text_out(wnd, x, s->row_y[0], g_star, 0, 0, 0xc, 0, 0, 0);
    for (i = 0, x = 0x1a0; (size_t)i < strlen(s->pw[1]); i++, x += 5)
        ui_window_text_out(wnd, x, s->row_y[1], g_star, 0, 0, 0xc, 0, 0, 0);
    for (i = 0, x = 0x1a0; (size_t)i < strlen(s->pw[2]); i++, x += 5)
        ui_window_text_out(wnd, x, s->row_y[2], g_star, 0, 0, 0xc, 0, 0, 0);
}

void ui_store_password_input_wnd_on_create(struct UIWindow *self, int width,
                                                 int height)
{
    (void)self;

    (void)width;
    (void)height;
}

void ui_store_password_input_wnd_on_draw(struct UIWindow *window)
{
    struct UIStorePasswordInputWnd *self = (struct UIStorePasswordInputWnd *)window;
    static const char *const names[3][3] = {
        { "btn_ok", "btn_ok_a", "btn_ok_b" },
        { "btn_cancel", "btn_cancel_a", "btn_cancel_b" },
        { "btn_change", "btn_change_a", "btn_change_b" },
    };
    static const int ids[3] = { PWD_ID_OK, PWD_ID_CANCEL, PWD_ID_CHANGE };
    static const char *const key_names[11][3] = {
        { "btn_num1", "btn_num1_a", "btn_num1_b" }, { "btn_num2", "btn_num2_a", "btn_num2_b" },
        { "btn_num3", "btn_num3_a", "btn_num3_b" }, { "btn_num4", "btn_num4_a", "btn_num4_b" },
        { "btn_num5", "btn_num5_a", "btn_num5_b" }, { "btn_num6", "btn_num6_a", "btn_num6_b" },
        { "btn_num7", "btn_num7_a", "btn_num7_b" }, { "btn_num8", "btn_num8_a", "btn_num8_b" },
        { "btn_num9", "btn_num9_a", "btn_num9_b" }, { "btn_num0", "btn_num0_a", "btn_num0_b" },
        { "btn_numR", "btn_numR_a", "btn_numR_b" },
    };
    static const int key_ids[11] = { 0x11e, 0x11f, 0x120, 0x121, 0x122, 0x123, 0x124, 0x125, 0x126, 0x11d,
                                     PWD_ID_RESET };
    static const int key_pos[10][2] = {
        { 0xa, 0x19 }, { 0x28, 0x19 }, { 0x46, 0x19 }, { 0xa, 0x37 }, { 0x28, 0x37 },
        { 0x46, 0x37 }, { 0xa, 0x55 }, { 0x28, 0x55 }, { 0x46, 0x55 }, { 0xa, 0x73 },
    };
    struct UIWindow *wnd = &self->frame.base;
    struct String dir;
    int used[10];
    int i, count, placed;

    ui_window_draw_body(wnd, ui_window_draw_title_bar(wnd, 1, msg_str(0x3f9), 0), 0, "");
    ui_window_draw_button_bar(wnd, 2, 0, 1);
    ui_skin_dir(&dir);
    count = 2 + (g_session1.pinChangeOnly == 0);
    for (i = 0; i < count; i++) {
        static const int dx[3] = { 0x5b, 0x2e, 0xd2 };

        ui_add_named_skin_button(wnd, &dir, names[i], wnd->width - dx[i], 0x92, ids[i]);
    }

    string_assign(&dir, UI_SKIN "basic_interface\\", 0x1f);
    memset(used, 0, sizeof used);
    placed = 0;
    for (i = 0; i < 11; i++) {
        void *mem = mem_alloc(sizeof(struct UIBitmapButton));
        struct UIBitmapButton *button = mem ? ui_bitmap_button_construct(mem) : NULL;

        ui_button_bitmap(button, &dir, key_names[i][0], 0);
        ui_button_bitmap(button, &dir, key_names[i][1], 1);
        ui_button_bitmap(button, &dir, key_names[i][2], 2);
        ui_window_create(&button->button.base, button->bitmap_width, button->bitmap_height);
        if (i == 10) {
            ui_window_move_base(&button->button.base, 0x28, 0x73);
        } else {
            int r = rand() % 10;

            if (placed != 10) {
                while (used[r])
                    r = rand() % 10;
                used[r] = 1;
                ui_window_move_base(&button->button.base, key_pos[r][0], key_pos[r][1]);
                placed++;
            }
        }
        button->button.base.id = key_ids[i];
        ui_window_add_child(wnd, &button->button.base);
    }
    draw_rows(self, 0x6e, 0x1b, 0x28, 0x3f, 0x4e, 0x67, 0x75);
    string_free(&dir);
}

static int store_pin_value(const char *pw)
{
    int value = atoi(pw);
    size_t i;

    for (i = 0; i < strlen(pw); i++)
        value += 100000000;
    return value;
}

static int store_pin_ok(const char *pw)
{
    return strlen(pw) < 9 && strlen(pw) > 3;
}

int ui_store_password_input_wnd_send_msg(struct UIWindow *window,
                                               struct UIWindow *sender, int msg, int val1, int val2, int val3,
                                               int val4)
{
    struct UIStorePasswordInputWnd *self = (struct UIStorePasswordInputWnd *)window;
    struct Mode *mode = client_current_mode();
    struct UIWindow *wnd = &self->frame.base;

    if (msg != PWD_MSG_BUTTON)
        goto base;
    switch (val1) {
    case PWD_ID_OK:
        switch (self->step) {
        case 0:
            if (store_pin_ok(self->pw[0])) {
                int value = store_pin_value(self->pw[0]);

                mode_send_message(mode, 0xbc, value, g_session1.pinRequest, 0, 0);
                ui_window_mgr_post_quit1(&g_windowMgr, 0x73);
                break;
            }
            pwd_box(0x3f3);
            pwd_restart(self, 0, 0);
            memset(self->pw[0], 0, 9);
            memset(self->pw[1], 0, 18);
            ui_window_on_draw(wnd);
            break;
        case 1:
            if (store_pin_ok(self->pw[0])) {
                pwd_restart(self, 2, 1);
                memset(self->pw[1], 0, 18);
                ui_window_fill_rect(wnd, 0x6f, 0x4f, 0x62, 0xe, WHITE);
                break;
            }
            pwd_box(0x3f3);
            pwd_restart(self, 1, 0);
            memset(self->pw[0], 0, 9);
            memset(self->pw[1], 0, 18);
            ui_window_on_draw(wnd);
            break;
        case 2:
            if (store_pin_ok(self->pw[1])) {
                pwd_restart(self, 3, 2);
                memset(self->pw[2], 0, 9);
                ui_window_fill_rect(wnd, 0x6f, 0x76, 0x62, 0xe, WHITE);
                break;
            }
            pwd_box(0x3f3);
            pwd_restart(self, 2, 1);
            memset(self->pw, 0, sizeof self->pw);
            ui_window_fill_rect(wnd, 0x6f, 0x4f, 0x62, 0xe, WHITE);
            break;
        case 3:
            if (!store_pin_ok(self->pw[2])) {
                pwd_box(0x3f3);
                pwd_restart(self, 3, 2);
                memset(self->pw, 0, sizeof self->pw);
                ui_window_fill_rect(wnd, 0x6f, 0x76, 0x62, 0xe, WHITE);
                break;
            }
            if (strcmp(self->pw[1], self->pw[2])) {
                pwd_box(0x3f8);
                pwd_restart(self, 2, 1);
                memset(self->pw[1], 0, 9);
                memset(self->pw[2], 0, 9);
                ui_window_fill_rect(wnd, 0x6f, 0x4f, 0x62, 0xe, WHITE);
                ui_window_fill_rect(wnd, 0x6e, 0x75, 0x64, 0x10, FIELD_BACK);
                break;
            } else {
                int old_value = store_pin_value(self->pw[0]);
                int new_value = store_pin_value(self->pw[2]);

                mode_send_message(mode, 0xbc, old_value, g_session1.pinRequest, new_value, 0);
                ui_window_mgr_post_quit1(&g_windowMgr, 0x73);
            }
            break;
        }
        break;
    case PWD_ID_CANCEL:
        ui_window_mgr_post_quit1(&g_windowMgr, 0x73);
        if (mode->sub_mode == 9)
            ui_window_open(0x115);
        break;
    case PWD_ID_CHANGE:
        g_session1.pinRequest = 2;
        self->changing = 1;
        pwd_restart(self, 1, 0);
        memset(self->pw, 0, sizeof self->pw);
        ui_window_on_draw(wnd);
        break;
    case PWD_ID_DIGIT0:
    case PWD_ID_DIGIT0 + 1:
    case PWD_ID_DIGIT0 + 2:
    case PWD_ID_DIGIT0 + 3:
    case PWD_ID_DIGIT0 + 4:
    case PWD_ID_DIGIT0 + 5:
    case PWD_ID_DIGIT0 + 6:
    case PWD_ID_DIGIT0 + 7:
    case PWD_ID_DIGIT0 + 8:
    case PWD_ID_DIGIT0 + 9:
        if (++self->typed > 8) {
            pwd_box(0x3f3);
            break;
        }
        if ((unsigned)self->step <= 3) {
            char *pw = self->pw[self->step <= 1 ? 0 : self->step - 1];
            char *end = pw + strlen(pw);

            end[0] = (char)('0' + val1 - PWD_ID_DIGIT0);
            end[1] = 0;
        }
        ui_window_text_out(wnd, self->star_x + 0x70, self->star_y, g_star, 0, 0, 0xc, 0, 0, 0);
        self->star_x += 5;
        break;
    case PWD_ID_RESET:
        if (self->step == 0)
            pwd_restart(self, 0, 0);
        else if (g_session1.pinChangeOnly)
            pwd_restart(self, 2, 1);
        else
            pwd_restart(self, 1, 0);
        memset(self->pw, 0, sizeof self->pw);
        ui_window_on_draw(wnd);
        break;
    case PWD_ID_ENTER:
        ui_window_send_msg(wnd, wnd, PWD_MSG_BUTTON, PWD_ID_OK, 0, 0, 0);
        break;
    }
base:
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
}

void ui_store_password_input_wnd_destruct(struct UIFrameWnd *self)
{
    self->base.ops = &ui_store_password_input_wnd_ops;
    ui_frame_wnd_destruct(self);
}

void ui_store_password_input_wnd_destroy(struct UIWindow *window)
{
    struct UIFrameWnd *self = (struct UIFrameWnd *)window;
    self->base.ops = &ui_store_password_input_wnd_ops;
    ui_frame_wnd_destruct(self);
    mem_free(self);
}

void ui_second_password_input_wnd_destroy(struct UIWindow *window)
{
    struct UIFrameWnd *self = (struct UIFrameWnd *)window;
    self->base.ops = &ui_second_password_input_wnd_ops;
    self->base.ops = &ui_store_password_input_wnd_ops;
    ui_frame_wnd_destruct(self);
    mem_free(self);
}

struct UISecondPasswordInputWnd;
struct UIStorePasswordInputWnd;
void ui_second_password_edit_wnd_destroy(struct UIWindow *window);
void ui_second_password_input_wnd_on_draw(struct UIWindow *self);
int ui_second_password_edit_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_second_password_edit_wnd_ops = {
    .destroy = ui_second_password_edit_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_window_store_info_base,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_store_password_input_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_second_password_input_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_frame_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_frame_wnd_on_mouse_move,
    .on_mouse_hover = ui_frame_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_frame_wnd_on_l_btn_up,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_second_password_edit_wnd_send_msg,
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

struct UIFrameWnd;
struct UISecondPasswordInputWnd;
struct UIStorePasswordInputWnd;
struct UIWindow;
int ui_second_password_input_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_second_password_input_wnd_ops = {
    .destroy = ui_second_password_input_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_window_store_info_base,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_store_password_input_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_second_password_input_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_frame_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_frame_wnd_on_mouse_move,
    .on_mouse_hover = ui_frame_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_frame_wnd_on_l_btn_up,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_second_password_input_wnd_send_msg,
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

void ui_second_password_input_wnd_destruct(struct UIFrameWnd *self)
{
    self->base.ops = &ui_second_password_input_wnd_ops;
    ui_store_password_input_wnd_destruct(self);
}

void ui_second_password_edit_wnd_destroy(struct UIWindow *window)
{
    struct UIFrameWnd *self = (struct UIFrameWnd *)window;
    self->base.ops = &ui_second_password_edit_wnd_ops;
    ui_second_password_input_wnd_destruct(self);
    mem_free(self);
}
