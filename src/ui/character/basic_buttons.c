#include "render/rp_pool.h"
#include "contents/guild_info.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "core/object_type_tables.h"
#include "contents/connection_api.h"
#include <stdio.h>
#include "ui/character/character.h"
#include "ui/controls/controls.h"
#include "ui/window_mgr.h"
#include "ui/game.h"

#include "core/msg_str.h"
#include "diag/perf.h"
#include "platform/clock.h"

#define UI_MSG_RELAYOUT 0x3c

enum BasicButtonId {
    BASIC_ID_STATUS = 0xb8,
    BASIC_ID_OPTION = 0xb9,
    BASIC_ID_ITEM = 0xba,
    BASIC_ID_INFO = 0xbb,
    BASIC_ID_SKILL = 0xbc,
    BASIC_ID_PARTY = 0xbf,
    BASIC_ID_MAP = 0xd3,
    BASIC_ID_QUEST = 0x161,
    BASIC_ID_GUILD = 0x16d,
    BASIC_ID_BOOKING = 0x173,
    BASIC_ID_NOT_BUILT_WINDOW = 0x194,
    BASIC_ID_NAVIGATION = 0x1a6,
    BASIC_ID_BANK = 0x1c5,
    BASIC_ID_ACHIEVEMENT = 0x1d1,
    BASIC_ID_MAIL = 0x1d4,
};

#define WND_STATUS 0xb
#define WND_ESC_MENU 0x9b
#define WND_ITEM 0x8
#define WND_INFO 0xa
#define WND_SKILL 0x25
#define WND_PARTY 0x45
#define WND_MAP 0x8c
#define WND_QUEST 0x141
#define WND_GUILD 0x3b
#define WND_GUILD_NONE 0xd4
#define WND_BOOKING 0xa9
#define WND_NOT_BUILT 0xbf
#define WND_NAVIGATION 0xcb
#define WND_ACHIEVEMENT 0x10e
#define WND_MAIL 0x107
#define WND_MAIL_BLOCK_1 0x127
#define WND_MAIL_BLOCK_2 0x157
#define MSG_MAIL_BLOCK_1 0xbb0
#define MSG_MAIL_BLOCK_2 0xe83

#define PACKET_BANK_OPEN 0x9ab
#define BASIC_FADE_ALPHA 0x80

struct UIButtonsOfBasicWnd {
    struct UIFrameWnd frame;
    DWORD colour;
    struct UIBitmapButton *info;
    struct UIBitmapButton *skill;
    struct UIBitmapButton *item;
    struct UIBitmapButton *map;
    struct UIBitmapButton *party;
    struct UIBitmapButton *guild;
    struct UIBitmapButton *quest;
    struct UIBitmapButton *option;
    struct UIBitmapButton *booking;
    struct UIBitmapButton *navigation;
    struct UIBitmapButton *bank;
    struct UIBitmapButton *status;
    struct UIBitmapButton *mail;
    struct UIBitmapButton *achievement;
    int extra_button;
};

void ui_buttons_of_basic_wnd_destruct(struct UIButtonsOfBasicWnd *self)
{
    self->frame.base.ops = &ui_buttons_of_basic_wnd_ops;
    ui_frame_wnd_destruct(&self->frame);
}

void ui_buttons_of_basic_wnd_destroy(struct UIWindow *window)
{
    struct UIButtonsOfBasicWnd *self = (struct UIButtonsOfBasicWnd *)window;
    ui_buttons_of_basic_wnd_destruct(self);
    mem_free(self);
}

static struct UIBitmapButton *basic_button(struct UIButtonsOfBasicWnd *self, const char *stem,
                                           const char *const suffixes[3], int id)
{
    struct UIBitmapButton *button = mem_alloc(sizeof *button);
    char path[MAX_PATH];
    int state;

    if (button)
        button = ui_bitmap_button_construct(button);
    for (state = 0; state < 3; state++) {
        sprintf(path, "%s%s", stem, suffixes[state]);
        ui_bitmap_button_set_bitmap(button, path, state, 0);
    }
    ui_window_create(&button->button.base, button->bitmap_width, button->bitmap_height);
    button->button.base.alpha_target = BASIC_FADE_ALPHA;
    button->button.base.alpha_time = platform_ticks_ms();
    button->button.base.id = id;
    ui_window_add_child(&self->frame.base, &button->button.base);
    return button;
}

static void basic_flow(struct UIButtonsOfBasicWnd *self, struct UIBitmapButton *button, int *x, int *y)
{
    struct UIWindow *wnd = &self->frame.base;
    int w, h;

    ui_window_move_base(&button->button.base, *x, *y);
    w = button->bitmap_width;
    h = button->bitmap_height;
    if (*x + 2 * w <= wnd->width) {
        *x += w;
    } else {
        *x = 0;
        *y += h;
    }
    if (h + *y > wnd->height)
        ui_window_resize_base(wnd, wnd->width, h + *y);
}

static void ui_buttons_of_basic_wnd_layout(struct UIButtonsOfBasicWnd *self)
{
    int x = 0, y = 0;

    basic_flow(self, self->status, &x, &y);
    basic_flow(self, self->info, &x, &y);
    basic_flow(self, self->skill, &x, &y);
    basic_flow(self, self->item, &x, &y);
    basic_flow(self, self->booking, &x, &y);
    basic_flow(self, self->party, &x, &y);
    basic_flow(self, self->guild, &x, &y);
    basic_flow(self, self->quest, &x, &y);
    basic_flow(self, self->map, &x, &y);
    basic_flow(self, self->navigation, &x, &y);
    basic_flow(self, self->option, &x, &y);
    basic_flow(self, self->bank, &x, &y);
    basic_flow(self, self->mail, &x, &y);
    basic_flow(self, self->achievement, &x, &y);
}

void ui_buttons_of_basic_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UIButtonsOfBasicWnd *self = (struct UIButtonsOfBasicWnd *)window;
    static const char *const basic_names[8] = {
        "info", "skill", "item", "map", "party", "guild", "quest", "option",
    };
    static const int basic_ids[8] = {
        BASIC_ID_INFO, BASIC_ID_SKILL, BASIC_ID_ITEM, BASIC_ID_MAP,
        BASIC_ID_PARTY, BASIC_ID_GUILD, BASIC_ID_QUEST, BASIC_ID_OPTION,
    };
    static const char *const digits[3] = { "1.bmp", "2.bmp", "3.bmp" };
    static const char *const letters[3] = { "_a.bmp", "_b.bmp", "_c.bmp" };
    static const char *const numbered[3] = { "_1.bmp", "_2.bmp", "_3.bmp" };
    struct UIBitmapButton **row = &self->info;
    char stem[MAX_PATH];
    int i;

    for (i = 0; i < 8; i++) {
        sprintf(stem, UI_SKIN "basic_interface\\%s", basic_names[i]);
        row[i] = basic_button(self, stem, digits, basic_ids[i]);
    }
    self->booking = basic_button(self, UI_SKIN "basic_interface\\booking", digits, BASIC_ID_BOOKING);
    self->navigation = basic_button(self, UI_SKIN "navigation_interface\\btn_Navigation", letters,
                                    BASIC_ID_NAVIGATION);
    self->bank = basic_button(self, UI_SKIN "basic_interface\\btn_bank", letters, BASIC_ID_BANK);
    self->status = basic_button(self, UI_SKIN "basic_interface\\btn_status", letters, BASIC_ID_STATUS);
    self->mail = basic_button(self, UI_SKIN "basic_interface\\rodexsystem\\renewal\\mail", numbered,
                              BASIC_ID_MAIL);
    self->achievement = basic_button(self, UI_SKIN "RO_menu_icon\\achievement", numbered, BASIC_ID_ACHIEVEMENT);
    ui_buttons_of_basic_wnd_layout(self);
    self->frame.base.alpha_target = BASIC_FADE_ALPHA;
    self->frame.base.alpha_time = platform_ticks_ms();
}

static void basic_toggle(int window_id)
{
    if (!ui_close_window(window_id))
        ui_make_window(window_id);
}

int ui_buttons_of_basic_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                           int val1, int val2, int val3, int val4)
{
    struct UIButtonsOfBasicWnd *self = (struct UIButtonsOfBasicWnd *)window;
    if (msg == UI_MSG_RELAYOUT) {
        ui_buttons_of_basic_wnd_layout(self);
        return 0;
    }
    if (msg != UI_MSG_COMMAND)
        return 0;

    switch (val1) {
    case BASIC_ID_STATUS:
        basic_toggle(WND_STATUS);
        break;
    case BASIC_ID_OPTION:
        basic_toggle(WND_ESC_MENU);
        break;
    case BASIC_ID_ITEM:
        basic_toggle(WND_ITEM);
        break;
    case BASIC_ID_INFO:
        basic_toggle(WND_INFO);
        break;
    case BASIC_ID_SKILL:
        basic_toggle(WND_SKILL);
        break;
    case BASIC_ID_PARTY:
        basic_toggle(WND_PARTY);
        break;
    case BASIC_ID_MAP:
        basic_toggle(WND_MAP);
        break;
    case BASIC_ID_QUEST:
        basic_toggle(WND_QUEST);
        break;
    case BASIC_ID_GUILD:
        basic_toggle(g_guildInfo.guild_id < 1 ? WND_GUILD_NONE : WND_GUILD);
        break;
    case BASIC_ID_BOOKING:
        basic_toggle(WND_BOOKING);
        break;

    case BASIC_ID_NOT_BUILT_WINDOW:
        basic_toggle(WND_NOT_BUILT);
        break;
    case BASIC_ID_NAVIGATION:
        if (g_windowMgr.navigation_v4_wnd)
            ui_window_send_msg(g_windowMgr.navigation_v4_wnd, NULL, UI_MSG_COMMAND, UI_ID_CLOSE, 0, 0, 0);
        else
            ui_make_window(WND_NAVIGATION);
        break;
    case BASIC_ID_BANK: {
#pragma pack(push, 1)
        struct { WORD id; int aid; } packet;
#pragma pack(pop)
        int len;

        packet.id = PACKET_BANK_OPEN;
        packet.aid = g_session1.ownAid;
        len = rag_connection_get_packet_length(rag_connection_instance(), PACKET_BANK_OPEN);
        rag_connection_send_packet(rag_connection_instance(), len, &packet);
        break;
    }
    case BASIC_ID_ACHIEVEMENT:
        basic_toggle(WND_ACHIEVEMENT);
        break;
    case BASIC_ID_MAIL:
        if (ui_get_window(WND_MAIL_BLOCK_1))
            ui_window_mgr_send_msg_(&g_windowMgr, UIWM_MSG_CHAT_LINE, (int)msg_str(MSG_MAIL_BLOCK_1), 0xff, 0, 0);
        else if (ui_get_window(WND_MAIL_BLOCK_2))
            ui_window_mgr_send_msg_(&g_windowMgr, UIWM_MSG_CHAT_LINE, (int)msg_str(MSG_MAIL_BLOCK_2), 0xff, 0, 0);
        else
            basic_toggle(WND_MAIL);
        break;
    }
    return 0;
}

static void basic_blit(struct UIButtonsOfBasicWnd *self, struct UIBitmapButton *button, int x, int y)
{
    struct UIWindow *wnd = &button->button.base;

    dc_bitmap_draw(wnd->canvas, wnd->x + x, wnd->y + y, wnd->width, wnd->height, self->colour);
}

void ui_buttons_of_basic_wnd_render_buttons(struct UIButtonsOfBasicWnd *self)
{
    struct UIBitmapButton **first = &self->info;
    int x = 0, y = 0;
    int i;

    ui_window_to_screen(&self->frame.base, &x, &y);
    for (i = 0; i < 8; i++)
        basic_blit(self, first[i], x, y);
    basic_blit(self, self->booking, x, y);
    basic_blit(self, self->navigation, x, y);
    basic_blit(self, self->bank, x, y);
    basic_blit(self, self->status, x, y);
    basic_blit(self, self->mail, x, y);
    basic_blit(self, self->achievement, x, y);
}

struct UIButtonsOfBasicWnd;
struct UIWindow;
void ui_buttons_of_basic_wnd_destroy(struct UIWindow *window);
void ui_buttons_of_basic_wnd_on_create(struct UIWindow *self, int width, int height);
int ui_buttons_of_basic_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_buttons_of_basic_wnd_ops = {
    .destroy = ui_buttons_of_basic_wnd_destroy,
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
    .on_create = ui_buttons_of_basic_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_window_on_draw_base,
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
    .send_msg = ui_buttons_of_basic_wnd_send_msg,
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

struct UIButtonsOfBasicWnd *ui_buttons_of_basic_wnd_construct(struct UIButtonsOfBasicWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_buttons_of_basic_wnd_ops;
    self->colour = 0x90ffffff;
    self->extra_button = 0;
    return self;
}

struct UIWindow *ui_buttons_of_basic_wnd_new(void)
{
    return (struct UIWindow *)ui_buttons_of_basic_wnd_construct(mem_alloc(sizeof(struct UIButtonsOfBasicWnd)));
}
