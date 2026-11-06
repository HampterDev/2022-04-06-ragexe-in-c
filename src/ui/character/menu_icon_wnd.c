#include "ui/controls/buttons.h"
#include "render/rp_pool.h"
#include "ui/social/entry_queue_state.h"
#include "contents/guild_info.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "core/object_type_tables.h"
#include "ui/window_access.h"
#include "game/context.h"
#include "contents/connection_api.h"
#include <string.h>
#include "ui/character/character.h"
#include "ui/ui_std.h"
#include "ui/window_mgr.h"
#include "ui/game.h"
#include "ui/ui_items.h"
#include "ui/controls/controls.h"

#include "replay/replay.h"
#include "core/msg_str.h"
#include "core/text_util.h"
#include "diag/perf.h"
#include "session/session_jobs.h"
#include "std/tree.h"
#include "world/world.h"

enum MenuIconId {
    MENU_ID_STATUS = 0xb8,
    MENU_ID_OPTION = 0xb9,
    MENU_ID_ITEM = 0xba,
    MENU_ID_EQUIP = 0xbb,
    MENU_ID_SKILL = 0xbc,
    MENU_ID_PARTY = 0xbf,
    MENU_ID_MAP = 0xd3,
    MENU_ID_MARK_NEW = 0x116,
    MENU_ID_QUEST = 0x161,
    MENU_ID_KEYBOARD = 0x16a,
    MENU_ID_GUILD = 0x16d,
    MENU_ID_BATTLE = 0x170,
    MENU_ID_BOOKING = 0x173,
    MENU_ID_REC = 0x187,
    MENU_ID_NAVIGATION = 0x1a6,
    MENU_ID_BANK = 0x1c5,
    MENU_ID_ACHIEVEMENT = 0x1d1,
    MENU_ID_MAIL = 0x1d4,
    MENU_ID_MENU = 0x1f6,
    MENU_ID_TIP = 0x1f7,
    MENU_ID_SHOP = 0x1f8,
    MENU_ID_SNS = 0x1fe,
    MENU_ID_ATTENDANCE = 0x214,
    MENU_ID_AGENCY = 0x218,
    MENU_ID_REPUTE = 0x22f,
};

#define MENU_MSG_TOGGLE 0x3c

#define WND_STATUS 0xb
#define WND_ESC_MENU 0x9b
#define WND_ITEM 0x8
#define WND_EQUIP 0xa
#define WND_SKILL 0x25
#define WND_PARTY 0x45
#define WND_MAP 0x8c
#define WND_QUEST 0x141
#define WND_KEYBOARD 0x12f
#define WND_GUILD 0x3b
#define WND_GUILD_NONE 0xd4
#define WND_BATTLE 0x9d
#define WND_REPLAY_REC 0xc6
#define WND_NAVIGATION 0xcb
#define WND_ACHIEVEMENT 0x10e
#define WND_MAIL 0x107
#define WND_TIP 0x13b
#define WND_ATTENDANCE 0x148
#define WND_AGENCY 0x144
#define WND_REPUTE 0x15a
#define WND_MAIL_BLOCK_1 0x127
#define WND_MAIL_BLOCK_2 0x157
#define WND_MAIL_BLOCK_3 0x15b
#define MSG_MAIL_BLOCK_1 0xbb0
#define MSG_MAIL_BLOCK_2 0xe83
#define MSG_MAIL_BLOCK_3 0xf03
#define MAIL_BLOCK_COLOUR 0xff

#define MODE_REQ_SHOP 0x143
#define PACKET_BANK_OPEN 0x9ab
#define PACKET_ATTENDANCE 0xa68
#define ATTENDANCE_ARG 5

#define MENU_COLUMNS 5
#define MENU_ICON_DX 0x2a
#define MENU_ICON_DY 0x2b
#define MENU_ICON_X 9
#define MENU_ICON_Y 0x17
#define MENU_ICON_NEW_Y 0x11
#define MENU_BUTTON_Y 4
#define MENU_ICON_COUNT 23
#define MENU_BIG_COUNT 0x14
#define MENU_BIG_WIDTH 0xdc
#define MENU_BIG_HEIGHT 0xee

struct MenuIcon {
    int id;
    struct UIBitmapButton *button;
};

struct UIMenuIconWnd {
    struct UIFrameWnd frame;
    struct Tree icons;
    struct Tree new_icons;
    struct List marks;
    struct UIBitmapButton *menu;
    struct UIBitmapButton *menu_new;
    struct UIBitmapButton *close;
};
OFFSET_ASSERT(UIMenuIconWnd, icons, 0x94);
OFFSET_ASSERT(UIMenuIconWnd, marks, 0xa4);
OFFSET_ASSERT(UIMenuIconWnd, close, 0xb4);
SIZE_ASSERT(UIMenuIconWnd, 0xb8);

struct UIWindow;
void ui_menu_icon_wnd_destroy(struct UIWindow *window);
void ui_menu_icon_wnd_move(struct UIWindow *self, int x, int y);
BYTE ui_menu_icon_wnd_is_click_through(struct UIWindow *self);
void ui_menu_icon_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_menu_icon_wnd_on_draw(struct UIWindow *self);
void ui_menu_icon_wnd_on_l_btn_down(struct UIWindow *self, int x, int y);
void ui_menu_icon_wnd_on_mouse_move(struct UIWindow *self, int x, int y);
void ui_menu_icon_wnd_on_l_btn_up(struct UIWindow *self, int x, int y);
int ui_menu_icon_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
struct UIWindow *ui_menu_icon_wnd_hit_test(struct UIWindow *self, int x, int y);
const struct UIWindowOps ui_menu_icon_wnd_ops = {
    .destroy = ui_menu_icon_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_menu_icon_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_menu_icon_wnd_is_click_through,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_window_store_info_base,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_menu_icon_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_menu_icon_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_menu_icon_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_menu_icon_wnd_on_mouse_move,
    .on_mouse_hover = ui_frame_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_menu_icon_wnd_on_l_btn_up,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_menu_icon_wnd_send_msg,
    .invalidate = ui_window_invalidate_base,
    .render = ui_window_render_base,
    .has_custom_backdrop = 0,
    .draw_backdrop = ui_window_draw_backdrop_base,
    .get_records = ui_frame_wnd_get_records,
    .type = UI_TYPE_FRAME,
    .on_hover_enter = ui_window_on_hover_enter_base,
    .on_hover_leave = ui_window_on_hover_leave_base,
    .make_quad = ui_window_make_quad_base,
    .hit_test = ui_menu_icon_wnd_hit_test,
    .render_overlay = ui_window_render_overlay_base,
};

static struct ListNode *find_mark(struct UIMenuIconWnd *self, int id)
{
    struct ListNode *node;

    for (node = self->marks.head->next; node != self->marks.head; node = node->next)
        if (*LIST_VALUE(node, int) == id)
            break;
    return node;
}

struct UIMenuIconWnd *ui_menu_icon_wnd_construct(struct UIMenuIconWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 1);
    self->frame.base.ops = &ui_menu_icon_wnd_ops;
    tree_init(&self->icons);
    tree_init(&self->new_icons);
    list_init(&self->marks);
    self->menu = NULL;
    self->menu_new = NULL;
    self->close = NULL;
    return self;
}

struct UIWindow *ui_menu_icon_wnd_new(void)
{
    return (struct UIWindow *)ui_menu_icon_wnd_construct(mem_alloc(sizeof(struct UIMenuIconWnd)));
}

void ui_menu_icon_wnd_destruct(struct UIMenuIconWnd *self)
{
    self->frame.base.ops = &ui_menu_icon_wnd_ops;
    list_free(&self->marks, NULL);
    tree_free(&self->new_icons, NULL);
    tree_free(&self->icons, NULL);
    ui_frame_wnd_destruct(&self->frame);
}

void ui_menu_icon_wnd_destroy(struct UIWindow *window)
{
    struct UIMenuIconWnd *self = (struct UIMenuIconWnd *)window;
    ui_menu_icon_wnd_destruct(self);
    mem_free(self);
}

BYTE ui_menu_icon_wnd_toggle(int window_id)
{
    BYTE closed = ui_window_mgr_post_quit1(&g_windowMgr, window_id);

    if (!closed)
        ui_window_open(window_id);
    return closed == 0;
}

BYTE ui_menu_icon_wnd_is_click_through(struct UIWindow *self)
{
    return 1;
}

void ui_menu_icon_wnd_move(struct UIWindow *window, int x, int y)
{
    struct UIMenuIconWnd *self = (struct UIMenuIconWnd *)window;
    self->frame.base.x = x;
    self->frame.base.y = y - 4;
    ui_window_mgr_update_snap_rect(&g_windowMgr, &self->frame.base);
}

void ui_menu_icon_wnd_on_l_btn_down(struct UIWindow *self, int x, int y)
{
}

void ui_menu_icon_wnd_on_l_btn_up(struct UIWindow *self, int x, int y)
{
}

void ui_menu_icon_wnd_on_mouse_move(struct UIWindow *self, int x, int y)
{
}

struct UIWindow *ui_menu_icon_wnd_hit_test(struct UIWindow *base_window, int x, int y)
{
    struct UIMenuIconWnd *self = (struct UIMenuIconWnd *)base_window;
    struct UIWindow *w = &self->frame.base;
    struct List copy;
    struct ListNode *node;
    struct UIWindow *hit = w;

    if (!ui_window_is_hit_testable_base(w) || !w->visible)
        return NULL;
    if (!g_session1.showMenuIcon) {
        if (x < w->x || x >= w->x + w->width)
            return NULL;
        if (y < w->y || y >= w->y + self->menu_new->button.base.height)
            return NULL;
        return self->marks.size ? &self->menu_new->button.base : &self->menu->button.base;
    }
    if (x < w->x || x >= w->x + w->width || y < w->y || y >= w->y + w->height)
        return NULL;
    list_init(&copy);
    for (node = w->children.head->next; node != w->children.head; node = node->next)
        list_push_back(&copy, LIST_VALUE(node, struct UIWindow *), sizeof(struct UIWindow *));
    node = copy.head;
    if (node != node->next) {
        do {
            node = node->prev;
            hit = ui_window_hit_test(*LIST_VALUE(node, struct UIWindow *), x - w->x, y - w->y);
            if (hit)
                break;
            hit = w;
        } while (node != copy.head->next);
    }
    list_free(&copy, NULL);
    return hit;
}

static struct UIBitmapButton *menu_icon_button(struct UIMenuIconWnd *self, const char *name, const char *normal_fmt,
                                               const char *press_fmt, int x, int y, int id)
{
    struct UIBitmapButton *button = mem_alloc(sizeof *button);
    struct String path;

    if (button) {
        ui_bitmap_button_construct(button);
        button->button.base.ops = &ui_menu_icon_ops.base;
    }
    string_init(&path);
    string_format(&path, normal_fmt, name);
    ui_bitmap_button_set_bitmap(button, string_cstr(&path), UI_BTN_UP, 0);
    string_free(&path);
    string_format(&path, normal_fmt, name);
    ui_bitmap_button_set_bitmap(button, string_cstr(&path), UI_BTN_HOVER, 0);
    string_free(&path);
    string_format(&path, press_fmt, name);
    ui_bitmap_button_set_bitmap(button, string_cstr(&path), UI_BTN_DOWN, 0);
    string_free(&path);
    ui_window_create(&button->button.base, button->bitmap_width, button->bitmap_height);
    ui_window_move_base(&button->button.base, x, y);
    button->button.base.id = id;
    ui_button_show((struct UIButton *)(button), 0);
    ui_window_add_child(&self->frame.base, &button->button.base);
    return button;
}

static struct UIBitmapButton *menu_button(struct UIMenuIconWnd *self, const char *normal, const char *over,
                                          const char *press, int y, int id)
{
    struct UIBitmapButton *button = mem_alloc(sizeof *button);

    if (button)
        button = ui_bitmap_button_construct(button);
    ui_bitmap_button_set_bitmap(button, normal, UI_BTN_UP, 0);
    ui_bitmap_button_set_bitmap(button, over, UI_BTN_HOVER, 0);
    ui_bitmap_button_set_bitmap(button, press, UI_BTN_DOWN, 0);
    ui_window_create(&button->button.base, button->bitmap_width, button->bitmap_height);
    ui_window_move_base(&button->button.base, 0, y);
    button->button.base.id = id;
    ui_button_show((struct UIButton *)(button), 0);
    ui_window_add_child(&self->frame.base, &button->button.base);
    return button;
}

static const int menu_icon_ids[MENU_ICON_COUNT] = {
    MENU_ID_STATUS,  MENU_ID_EQUIP,  MENU_ID_ITEM,       MENU_ID_SKILL,
    MENU_ID_BOOKING, MENU_ID_PARTY,  MENU_ID_GUILD,      MENU_ID_BATTLE,
    MENU_ID_QUEST,   MENU_ID_MAP,    MENU_ID_NAVIGATION, MENU_ID_OPTION,
    MENU_ID_BANK,    MENU_ID_REC,    MENU_ID_MAIL,       MENU_ID_ACHIEVEMENT,
    MENU_ID_TIP,     MENU_ID_SHOP,   MENU_ID_KEYBOARD,   MENU_ID_SNS,
    MENU_ID_ATTENDANCE, MENU_ID_AGENCY, MENU_ID_REPUTE,
};
static const char *const menu_icon_names[MENU_ICON_COUNT] = {
    "status",  "equip", "item",       "skill",
    "booking", "party", "guild",      "battle",
    "quest",   "map",   "navigation", "option",
    "bank",    "rec",   "mail",       "achievement",
    "tip",     "shop",  "keyboard",   "sns",
    "attendance", "adventurerAgency", "repute",
};

#define MENU_ICON_DIR UI_SKIN "menu_icon\\"

void ui_menu_icon_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UIMenuIconWnd *self = (struct UIMenuIconWnd *)window;
    struct UIBitmapButton *button;
    struct MenuIcon *icon;
    const char *name;
    int i, id, x, y;
    int placed = 0;

    for (i = 0; i < MENU_ICON_COUNT; i++) {
        id = menu_icon_ids[i];
        name = menu_icon_names[i];
        if (id == MENU_ID_STATUS && session_is_doram_job(&g_session1, g_session1.job) == 1)
            name = "status_doram";
        if (id == MENU_ID_BOOKING || id == MENU_ID_SNS || id == 0x239 || id == MENU_ID_GUILD || id == MENU_ID_MAP ||
            id == MENU_ID_NAVIGATION || id == MENU_ID_BANK || id == MENU_ID_REC || id == MENU_ID_TIP ||
            id == MENU_ID_AGENCY || id == MENU_ID_REPUTE)
            continue;
        x = (unsigned)placed % MENU_COLUMNS * MENU_ICON_DX + MENU_ICON_X;
        y = (unsigned)placed / MENU_COLUMNS * MENU_ICON_DY;
        button = menu_icon_button(self, name, MENU_ICON_DIR "bt_%s.bmp", MENU_ICON_DIR "bt_%s_press.bmp", x,
                                  y + MENU_ICON_Y, id);
        icon = tree_get(&self->icons, &id, sizeof id, sizeof *icon, tree_less_int);
        icon->button = button;
        switch (id) {
        case MENU_ID_STATUS:
        case MENU_ID_ITEM:
        case MENU_ID_SKILL:
        case MENU_ID_ACHIEVEMENT:
        case MENU_ID_MAIL:
            button = menu_icon_button(self, name, MENU_ICON_DIR "bt_%s_new.bmp", MENU_ICON_DIR "bt_%s_new_press.bmp",
                                      x, y + MENU_ICON_NEW_Y, id);
            icon = tree_get(&self->new_icons, &id, sizeof id, sizeof *icon, tree_less_int);
            icon->button = button;
            break;
        }
        placed++;
    }
    self->menu = menu_button(self, MENU_ICON_DIR "bt_menu_normal.bmp", MENU_ICON_DIR "bt_menu_over.bmp",
                             MENU_ICON_DIR "bt_menu_press.bmp", MENU_BUTTON_Y, MENU_ID_MENU);
    self->menu_new = menu_button(self, MENU_ICON_DIR "bt_menu_new_normal.bmp", MENU_ICON_DIR "bt_menu_new_over.bmp",
                                 MENU_ICON_DIR "bt_menu_new_press.bmp", 0, MENU_ID_MENU);
    self->close = menu_button(self, MENU_ICON_DIR "bt_menu_close_normal.bmp", MENU_ICON_DIR "bt_menu_close_over.bmp",
                              MENU_ICON_DIR "bt_menu_close_press.bmp", MENU_BUTTON_Y, UI_ID_CLOSE);
    ui_menu_icon_wnd_send_msg(&self->frame.base, &self->frame.base, UI_MSG_COMMAND, g_session1.showMenuIcon == 1 ? MENU_ID_MENU : UI_ID_CLOSE, 0, 0,
             0);
    if (self->icons.size > MENU_BIG_COUNT)
        ui_window_resize_base(&self->frame.base, MENU_BIG_WIDTH, MENU_BIG_HEIGHT);
}

static int menu_marked(struct UIMenuIconWnd *self, int id)
{
    return find_mark(self, id) != self->marks.head;
}

void ui_menu_icon_wnd_on_draw(struct UIWindow *window)
{
    struct UIMenuIconWnd *self = (struct UIMenuIconWnd *)window;
    struct TreeNode *it;
    struct MenuIcon *variant;
    struct String bg;
    int marked;

    list_clear(&self->frame.base.draws, NULL);

    ui_button_show((struct UIButton *)(self->menu), 0);
    ui_button_show((struct UIButton *)(self->menu_new), 0);
    ui_button_show((struct UIButton *)(self->close), 0);
    if (g_session1.showMenuIcon)
        ui_button_show((struct UIButton *)(self->close), 1);
    else if (self->marks.size)
        ui_button_show((struct UIButton *)(self->menu_new), 1);
    else
        ui_button_show((struct UIButton *)(self->menu), 1);

    for (it = tree_first(&self->icons); it != self->icons.head; it = tree_next(it)) {
        const struct MenuIcon *icon = TREE_VALUE(it, struct MenuIcon);

        if (!g_session1.showMenuIcon) {
            ui_button_show((struct UIButton *)(icon->button), 0);
            variant = tree_find(&self->new_icons, &icon->id, tree_less_int);
            if (variant && variant->button)
                ui_button_show((struct UIButton *)(variant->button), 0);
            continue;
        }
        string_init(&bg);
        string_assign_cstr(&bg, MENU_ICON_DIR "bg_menu.tga");
        ui_window_queue_bitmap(&self->frame.base, 0, 0xd, bg, 0xffffffff, 0, 0);
        marked = menu_marked(self, icon->id);
        ui_button_show((struct UIButton *)(icon->button), !marked);
        variant = tree_find(&self->new_icons, &icon->id, tree_less_int);
        if (variant && variant->button)
            ui_button_show((struct UIButton *)(variant->button), marked);
    }
}

static void menu_mark(struct UIMenuIconWnd *self, int id, int on)
{
    struct ListNode *found = find_mark(self, id);

    if (on > 0) {
        if (found == self->marks.head)
            list_push_back(&self->marks, &id, sizeof id);
    } else if (found != self->marks.head) {
        list_erase(&self->marks, found, NULL);
    }
    ui_window_invalidate_base(&self->frame.base);
}

int ui_menu_icon_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1, int val2,
                                     int val3, int val4)
{
    struct UIMenuIconWnd *self = (struct UIMenuIconWnd *)window;
    if (msg == MENU_MSG_TOGGLE) {
        ui_menu_icon_wnd_send_msg(&self->frame.base, NULL, UI_MSG_COMMAND, g_session1.showMenuIcon == 1 ? UI_ID_CLOSE : MENU_ID_MENU, 0, 0, 0);
    } else if (msg == UI_MSG_COMMAND && !replay_recorder_blocks_action(replay_recorder_instance(), val1)) {
        switch (val1) {
        case MENU_ID_STATUS:
            ui_menu_icon_wnd_toggle(WND_STATUS);
            break;
        case MENU_ID_OPTION:
            ui_menu_icon_wnd_toggle(WND_ESC_MENU);
            break;
        case MENU_ID_ITEM:
            ui_menu_icon_wnd_toggle(WND_ITEM);
            break;
        case MENU_ID_EQUIP:
            ui_menu_icon_wnd_toggle(WND_EQUIP);
            break;
        case MENU_ID_SKILL:
            ui_menu_icon_wnd_toggle(WND_SKILL);
            break;
        case MENU_ID_PARTY:
            ui_menu_icon_wnd_toggle(WND_PARTY);
            break;
        case UI_ID_CLOSE:
            g_session1.showMenuIcon = 0;
            ui_window_invalidate_base(&self->frame.base);
            break;
        case MENU_ID_MAP:
            ui_menu_icon_wnd_toggle(WND_MAP);
            break;
        case MENU_ID_MARK_NEW:
            menu_mark(self, val2, val3);
            break;
        case MENU_ID_QUEST:
            ui_menu_icon_wnd_toggle(WND_QUEST);
            break;
        case MENU_ID_KEYBOARD:
            ui_menu_icon_wnd_toggle(WND_KEYBOARD);
            break;
        case MENU_ID_GUILD:
            ui_menu_icon_wnd_toggle(g_guildInfo.guild_id < 1 ? WND_GUILD_NONE : WND_GUILD);
            break;
        case MENU_ID_BATTLE:
            if (g_entryQueueMgr && g_entryQueueMgr->in_queue == 1)
                entry_queue_mgr_request_ranking(g_entryQueueMgr);
            else
                ui_menu_icon_wnd_toggle(WND_BATTLE);
            break;
        case MENU_ID_REC:
            if (replay_recorder_instance()->state != 1)
                ui_message_box("REC End please");
            else
                ui_menu_icon_wnd_toggle(WND_REPLAY_REC);
            break;
        case MENU_ID_NAVIGATION:
            if (g_windowMgr.navigation_v4_wnd)
                ui_window_send_msg(g_windowMgr.navigation_v4_wnd, NULL, UI_MSG_COMMAND, UI_ID_CLOSE, 0, 0, 0);
            else
                ui_window_open(WND_NAVIGATION);
            break;
        case MENU_ID_BANK: {
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
        case MENU_ID_ACHIEVEMENT:
            ui_menu_icon_wnd_toggle(WND_ACHIEVEMENT);
            break;
        case MENU_ID_MAIL:
            if (ui_get_window(WND_MAIL_BLOCK_1))
                ui_window_mgr_send_msg_(&g_windowMgr, UIWM_MSG_CHAT_LINE, (int)msg_str(MSG_MAIL_BLOCK_1),
                                      MAIL_BLOCK_COLOUR, 0, 0);
            else if (ui_get_window(WND_MAIL_BLOCK_2))
                ui_window_mgr_send_msg_(&g_windowMgr, UIWM_MSG_CHAT_LINE, (int)msg_str(MSG_MAIL_BLOCK_2),
                                      MAIL_BLOCK_COLOUR, 0, 0);
            else if (ui_get_window(WND_MAIL_BLOCK_3))
                ui_window_mgr_send_msg_(&g_windowMgr, UIWM_MSG_CHAT_LINE, (int)msg_str(MSG_MAIL_BLOCK_3),
                                      MAIL_BLOCK_COLOUR, 0, 0);
            else
                ui_menu_icon_wnd_toggle(WND_MAIL);
            break;
        case MENU_ID_MENU:
            g_session1.showMenuIcon = 1;
            ui_window_invalidate_base(&self->frame.base);
            break;
        case MENU_ID_TIP:
            ui_menu_icon_wnd_toggle(WND_TIP);
            break;
        case MENU_ID_SHOP:
            game_send_message(MODE_REQ_SHOP, 0, 0, 0, 0);
            break;
        case MENU_ID_ATTENDANCE:
            if (ui_get_window(WND_ATTENDANCE)) {
                ui_window_mgr_post_quit1(&g_windowMgr, WND_ATTENDANCE);
            } else {
#pragma pack(push, 1)
                struct { WORD id; BYTE arg; } packet;
#pragma pack(pop)
                int len;

                packet.id = PACKET_ATTENDANCE;
                packet.arg = ATTENDANCE_ARG;
                len = rag_connection_get_packet_length(rag_connection_instance(), PACKET_ATTENDANCE);
                rag_connection_send_packet(rag_connection_instance(), len, &packet);
            }
            break;
        case MENU_ID_AGENCY:
            ui_menu_icon_wnd_toggle(WND_AGENCY);
            break;
        case MENU_ID_REPUTE:
            ui_menu_icon_wnd_toggle(WND_REPUTE);
            break;
        }
    }
    return ui_window_send_msg_base(&self->frame.base, sender, msg, val1, val2, val3, val4);
}
