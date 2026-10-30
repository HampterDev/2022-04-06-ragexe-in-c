#include "world/tooltip.h"
#include "render/rp_pool.h"
#include "session/character_name.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "contents/mouse.h"
#include "core/object_type_tables.h"
#include "world/mode_manager.h"
#include "ui/window_access.h"
#include "game/context.h"
#include "ui/social/party_windows.h"
#include "ui/ui_std.h"
#include "core/msg_str.h"
#include "diag/perf.h"
#include "session/session_items.h"
#include "session/session_party.h"
#include "ui/character/gauge.h"

struct Vec;
struct UIMiniPartyInfoWnd;
struct UIWindow;
void ui_mini_party_info_wnd_destroy(struct UIWindow *window);
void ui_mini_party_info_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_mini_party_info_wnd_on_draw(struct UIWindow *self);
void ui_mini_party_info_wnd_on_l_btn_down(struct UIWindow *self, int x, int y);
void ui_mini_party_info_wnd_on_mouse_hover(struct UIWindow *self, int x, int y);
void ui_mini_party_info_wnd_on_l_btn_up(struct UIWindow *self, int x, int y);
void ui_mini_party_info_wnd_on_r_btn_up(struct UIWindow *self, int x, int y);
int ui_mini_party_info_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_mini_party_info_wnd_ops = {
    .destroy = ui_mini_party_info_wnd_destroy,
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
    .on_create = ui_mini_party_info_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_mini_party_info_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_mini_party_info_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_frame_wnd_on_mouse_move,
    .on_mouse_hover = ui_mini_party_info_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_mini_party_info_wnd_on_l_btn_up,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_mini_party_info_wnd_on_r_btn_up,
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_mini_party_info_wnd_send_msg,
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

#define POPUP_MENU_WND 0x12
#define MENU_MSG_RESET 0x4b
#define MENU_MSG_ADD 0x1f
#define MENU_MSG_OWNER 0x53
#define MENU_MSG_FIT 0x1e
#define MENU_OWNER_PLATE 0x12d

enum PlateMenuValue {
    PLATE_MENU_EXPEL = 0,
    PLATE_MENU_WHISPER = 7,
    PLATE_MENU_LEAVE = 9,
    PLATE_MENU_DELEGATE = 0x2b,
    PLATE_MENU_EQUIPMENT = 0x3a,
    PLATE_MENU_REMOVE = 0x3d,
};

struct UIMiniPartyInfoWnd *ui_mini_party_info_wnd_construct(struct UIMiniPartyInfoWnd *self)
{
    struct UIMenuEntry *e;

    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_mini_party_info_wnd_ops;
    self->menu.first = self->menu.last = self->menu.end = NULL;
    self->gauge = NULL;
    self->owner = NULL;
    for (e = self->menu.first; e != self->menu.last; e++)
        string_free(&e->text);
    self->menu.last = self->menu.first;
    return self;
}

void ui_mini_party_info_wnd_destruct(struct UIMiniPartyInfoWnd *self)
{
    self->frame.base.ops = &ui_mini_party_info_wnd_ops;
    ui_menu_entry_vec_free(&self->menu);
    ui_frame_wnd_destruct(&self->frame);
}

void ui_mini_party_info_wnd_destroy(struct UIWindow *window)
{
    struct UIMiniPartyInfoWnd *self = (struct UIMiniPartyInfoWnd *)window;
    ui_mini_party_info_wnd_destruct(self);
    mem_free(self);
}

void ui_mini_party_info_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UIMiniPartyInfoWnd *self = (struct UIMiniPartyInfoWnd *)window;

    if (self->gauge)
        return;
    self->gauge = ui_pc_gage_new();
    ui_window_create(&self->gauge->gage.base, (int)((float)width * 0.38f), 5);
    ui_window_move_base(&self->gauge->gage.base, 0x23, 0x17);
    ui_window_add_child(&self->frame.base, &self->gauge->gage.base);
}

static void owner_member(struct UIMiniPartyInfoWnd *self, struct PartyMember *out)
{
    ui_mini_party_wnd_get_member(self->owner, out);
}

static void member_line(struct UIMiniPartyInfoWnd *self, const struct PartyMember *m, char *text,
                        const struct String *map, struct String *line)
{
    struct UIWindow *wnd = &self->frame.base;
    struct String fitted = {0};

    if (self->owner->mode == 1) {
        sprintf(text, "%s", string_cstr(&m->name));
        ui_window_fit_text(wnd, &fitted, (int)((float)wnd->width * 0.532f), text, 0, 0xc, 1);
    } else {
        sprintf(text, "Lv.%d %s(%s)", m->level, string_cstr(&m->name), string_cstr(map));
        ui_window_fit_text(wnd, &fitted, (int)((float)wnd->width * 0.6f), text, 0, 0xc, 1);
    }
    string_assign(line, string_cstr(&fitted), fitted.size);
    string_free(&fitted);
}

static void member_map(const struct PartyMember *m, struct String *out)
{
    const char *name = session_map_display_name(string_cstr(&m->map_name));

    string_init(out);
    string_assign_cstr(out, name);
}

static struct BitmapRes *skin_bitmap(const char *path)
{
    return (struct BitmapRes *)res_mgr_get(g_resMgr(), ui_skin_path(path));
}

void ui_mini_party_info_wnd_on_draw(struct UIWindow *window)
{
    struct UIMiniPartyInfoWnd *self = (struct UIMiniPartyInfoWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct String base, map, line;
    struct PartyMember m;
    char text[0x100], path[0x100];
    struct BitmapRes *bmp;
    int die;

    ui_window_clear_canvas(wnd, UI_TRANSPARENT);
    string_init(&base);
    string_assign(&base, RENEWAL_PARTY, 0x1c);
    owner_member(self, &m);
    if (!m.found)
        goto out;

    die = 0;
    if (m.dead == 1) {
        die = 1;
    }
    else if (g_session1.ownAid == m.aid) {
        die = game_player_is_dead();
    }
    sprintf(path, die ? "%sicon_jobs_%d_die.bmp" : "%sicon_jobs_%d.bmp", string_cstr(&base), m.job);
    bmp = skin_bitmap(path);
    if (m.state == 1)
        ui_window_draw_bitmap_tinted(wnd, 0, 7, m.role == 0 ? 8 : 6, bmp, 1, 0);
    else
        ui_window_draw_bitmap(wnd, 7, m.role == 0 ? 8 : 6, bmp, 1);
    if (m.role == 0) {
        sprintf(path, "%sico_partyCrown.bmp", string_cstr(&base));
        bmp = skin_bitmap(path);
        if (m.state == 1)
            ui_window_draw_bitmap_tinted(wnd, 0, 9, 0, bmp, 1, 0);
        else
            ui_window_draw_bitmap(wnd, 9, 0, bmp, 1);
    }

    if (self->owner->mode == 0)
        ui_window_move_base(&self->gauge->gage.base, 0x23, 0x17);
    else
        ui_window_move_base(&self->gauge->gage.base, -1000, -1000);
    if (self->owner->mode == 2)
        goto out;

    memset(text, 0, sizeof(text));
    member_map(&m, &map);
    string_init(&line);
    string_assign_cstr(&line, text);
    member_line(self, &m, text, &map, &line);
    ui_window_text_out(wnd, 0x23, 9, string_cstr(&line), 0, 0, 0xc, m.state == 1 ? 0xaaaaaa : 0xffffff, 1,
                     0);
    string_free(&line);
    string_free(&map);
out:
    party_member_destroy(&m);
    string_free(&base);
}

void ui_mini_party_info_wnd_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIMiniPartyInfoWnd *self = (struct UIMiniPartyInfoWnd *)window;
    struct PartyMember m;

    if (!g_session1.lockMouse)
        ui_mini_party_wnd_on_l_btn_down(&self->owner->frame.base, x, y);
    owner_member(self, &m);
    if (m.found && m.state == 0) {
        struct GameMode *mode = game_current_mode();

        if (mode)
            game_mode_click_actor(mode, m.aid);
    }
    party_member_destroy(&m);
}

void ui_mini_party_info_wnd_on_l_btn_up(struct UIWindow *window, int x, int y)
{
    struct UIMiniPartyInfoWnd *self = (struct UIMiniPartyInfoWnd *)window;
    if (g_session1.lockMouse != 1 && ui_window_mgr_get_capture(&g_windowMgr) == &self->frame.base)
        ui_mini_party_wnd_on_l_btn_up(&self->owner->frame.base, x, y);
}

void ui_mini_party_info_wnd_on_mouse_hover(struct UIWindow *window, int x, int y)
{
    struct UIMiniPartyInfoWnd *self = (struct UIMiniPartyInfoWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct Mode *mode = game_current_base_mode();
    struct String line, map;
    struct PartyMember m;
    char text[0x100];
    int tx, ty;

    if (!mode)
        return;
    owner_member(self, &m);
    if (!m.found) {
        party_member_destroy(&m);
        return;
    }
    memset(text, 0, sizeof(text));
    string_init(&line);
    member_map(&m, &map);
    member_line(self, &m, text, &map, &line);
    if (!(string_find(&line, text, 0) != STRING_NPOS)) {
        tx = wnd->x + 0xf;
        ty = wnd->y - 0xc;
        ui_window_to_screen(wnd, &tx, &ty);
        mode_show_tooltip(mode, text, tx, ty, 0xffffffff, 0);
    }
    string_free(&map);
    string_free(&line);
    party_member_destroy(&m);
}

void ui_mini_party_info_wnd_on_r_btn_up(struct UIWindow *window, int x, int y)
{
    struct UIMiniPartyInfoWnd *self = (struct UIMiniPartyInfoWnd *)window;
    if (!g_session1.lockMouse)
        ui_mini_party_info_wnd_send_msg(&self->frame.base, NULL, 0x31, 0, 0, 0, 0);
}

static void menu_add(struct Vec *menu, int msg_id, int value)
{
    struct String text;
    struct UIMenuEntry entry;

    string_init(&text);
    string_assign_cstr(&text, msg_str(msg_id));
    ui_menu_entry_construct(&entry, string_cstr(&text), value);
    ui_menu_entry_vec_push_move(menu, &entry);
    string_free(&entry.text);
    string_free(&text);
}

static int is_own_name(const struct PartyMember *m)
{
    return string_equal_cstr(&m->name, session_character_name(&g_session1));
}

static int ask(int msg_id)
{
    return ui_window_mgr_message_box(&g_windowMgr, msg_str(msg_id), UI_MSGBOX_QUESTION, 0, 0, 0, "메시지",
                                  0x118, 0x78, 0);
}

static int show_hp(struct UIMiniPartyInfoWnd *self)
{
    struct PartyMember m;
    struct Mode *mode;
    void *actor;
    int hp, max_hp, found;

    owner_member(self, &m);
    found = m.found;
    party_member_destroy(&m);
    if (found) {
        owner_member(self, &m);
        found = m.found;
        party_member_destroy(&m);
        if (!found)
            return 0;
    }
    mode = game_current_base_mode();
    owner_member(self, &m);
    if (m.state != 0) {
        party_member_destroy(&m);
        goto hide;
    }
    if (m.aid == g_session1.ownAid) {
        hp = g_session1.hp;
        actor = game_current_player();
        max_hp = g_session1.maxHp;
    } else {
        actor = game_find_actor(m.aid);
        if (!actor) {
            party_member_destroy(&m);
            goto hide;
        }
        if (party_actor_gauge(actor)) {
            hp = party_actor_gauge(actor)->hp;
            max_hp = party_actor_gauge(actor)->max_hp;
        } else {
            hp = max_hp = 0;
        }
    }
    party_member_destroy(&m);
    if (actor) {
        ui_window_move_base(&self->gauge->gage.base, 0x34, 0x20);
        ui_pc_gage_set_hp(self->gauge, hp, max_hp);
        return 0;
    }
hide:
    ui_window_move_base(&self->gauge->gage.base, -100, -100);
    return 0;
}

static void open_menu(struct UIMiniPartyInfoWnd *self)
{
    struct PartyMember m;
    struct UIMenuEntry *e;
    struct UIWindow *menu;
    size_t i;
    int x, y;

    for (e = self->menu.first; e != self->menu.last; e++)
        string_free(&e->text);
    self->menu.last = self->menu.first;
    if (g_windowMgr.menu_wnd)
        ui_window_mgr_post_quit1(&g_windowMgr, POPUP_MENU_WND);

    owner_member(self, &m);
    if (m.found) {
        menu_add(&self->menu, 0xaf8, PLATE_MENU_EQUIPMENT);
        if (is_own_name(&m)) {
            menu_add(&self->menu, 0x60, PLATE_MENU_LEAVE);
        } else {
            menu_add(&self->menu, 0x168, PLATE_MENU_WHISPER);
            if (g_session1.partyLeader) {
                menu_add(&self->menu, 0x61, PLATE_MENU_EXPEL);
                menu_add(&self->menu, 0x5fc, PLATE_MENU_DELEGATE);
            }
        }
        menu_add(&self->menu, 0xc1e, PLATE_MENU_REMOVE);
    }
    party_member_destroy(&m);
    if (VEC_COUNT(&self->menu, struct UIMenuEntry) == 0)
        return;

    menu = ui_window_open(POPUP_MENU_WND);
    if (!menu)
        return;
    ui_window_send(menu, NULL, MENU_MSG_RESET, 0, 0, 0, 0);
    for (i = 0; i < VEC_COUNT(&self->menu, struct UIMenuEntry); i++)
        ui_window_send(menu, NULL, MENU_MSG_ADD,
                 (int)string_cstr(&VEC_AT(&self->menu, struct UIMenuEntry, i).text), 0, 0, 0);
    ui_window_send(menu, NULL, MENU_MSG_OWNER, MENU_OWNER_PLATE, self->owner->cell, 0, 0);
    ui_window_send(menu, NULL, MENU_MSG_FIT, 0, 0, 0, 0);
    x = g_mouse.x + 1;
    if (x < 1)
        x = 1;
    if (x > g_renderer->width - menu->width)
        x = g_renderer->width - menu->width;
    y = g_mouse.y + 1;
    if (y < 1)
        y = 1;
    if (y > g_renderer->height - menu->height)
        y = g_renderer->height - menu->height;
    ui_window_move(menu, x, y);
}

static void menu_choice(struct UIMiniPartyInfoWnd *self, int index)
{
    struct UIWindow *wnd = &self->frame.base;
    struct PartyMember m;

    owner_member(self, &m);
    if (!m.found)
        goto out;
    if (self->menu.first == self->menu.last) {
        ui_window_mgr_message_box(&g_windowMgr, msg_str(0xca4), UI_MSGBOX_OK, 0, 0, 0, "메시지", 0x118, 0x78,
                               0);
        goto out;
    }
    switch (VEC_AT(&self->menu, struct UIMenuEntry, index).id) {
    case PLATE_MENU_EXPEL:
    case 0x13:
        ui_mini_party_info_wnd_send_msg(wnd, NULL, UI_MSG_COMMAND, 0xe3, 0, 0, 0);
        break;
    case 3:
        ui_mini_party_info_wnd_send_msg(wnd, NULL, UI_MSG_COMMAND, 0xe1, 0, 0, 0);
        break;
    case 5:
        if (self->owner->cell != -1 && m.found)
            mode_send_message(client_current_mode(), 0x3b, (int)string_cstr(&m.name), 0, 0, 0);
        break;
    case 6:
        ui_mini_party_info_wnd_send_msg(wnd, NULL, UI_MSG_COMMAND, 0xdf, 0, 0, 0);
        break;
    case PLATE_MENU_WHISPER:
        ui_mini_party_info_wnd_send_msg(wnd, NULL, UI_MSG_COMMAND, 0xe0, 0, 0, 0);
        break;
    case PLATE_MENU_LEAVE:
        ui_mini_party_info_wnd_send_msg(wnd, NULL, UI_MSG_COMMAND, 0xe4, 0, 0, 0);
        break;
    case 0x14:
        ui_mini_party_info_wnd_send_msg(wnd, NULL, 0x47, 0, 0, 0, 0);
        break;
    case PLATE_MENU_DELEGATE:
        if (self->owner->cell == -1)
            break;
        if (m.found && m.state == 0) {
            if (ask(0x5fd) == UI_MSGBOX_ANSWER_YES)
                mode_send_message(client_current_mode(), 0x104, m.aid, 0, 0, 0);
        } else {
            ui_window_mgr_send_msg_(&g_windowMgr, 3, 0, 0, 0, 0);
            ui_window_mgr_send_msg_(&g_windowMgr, UIWM_MSG_CHAT_LINE, (int)msg_str(0x5fe), 0xff, 3, 0);
        }
        break;
    case PLATE_MENU_EQUIPMENT:
        mode_send_message(client_current_mode(), 0x10c, (int)string_cstr(&m.name), 0, 0, 0);
        break;
    case PLATE_MENU_REMOVE:
        ui_window_mgr_remove_plate(&g_windowMgr, m.aid);
        break;
    }
out:
    party_member_destroy(&m);
}

static void command(struct UIMiniPartyInfoWnd *self, int id, int val2, int val3, int val4)
{
    struct UIWindow *wnd = &self->frame.base;
    struct PartyMember m;

    switch (id) {
    case 0x22: {
        void *actor = val2 == g_session1.ownAid ? (struct GameActor *)game_current_player()
                                       : game_find_actor(val2);

        if (actor) {
            ui_window_move_base(&self->gauge->gage.base, 0x34, 0x20);
            ui_pc_gage_set_hp(self->gauge, val3, val4);
        } else {
            ui_window_move_base(&self->gauge->gage.base, -100, -100);
        }
        ui_window_invalidate_base(wnd);
        return;
    }
    case 0xdf:
        if (g_session1.lockMouse)
            return;
        owner_member(self, &m);
        party_member_destroy(&m);
        return;
    case 0xe0: {
        const char *note;
        struct String number;

        if (g_session1.lockMouse)
            return;
        owner_member(self, &m);
        if (!m.found || is_own_name(&m)) {
            party_member_destroy(&m);
            return;
        }
        if (ui_window_mgr_is_friend(&g_windowMgr, string_cstr(&m.name)))
            note = msg_str(0x66);
        else
            note = session_is_guild_member_char_id(&g_session1, m.char_id) ? "GuildMember" : NULL;
        session_scramble_number(&g_session1, &number, m.char_id);
        ui_window_mgr_send_msg_(&g_windowMgr, 0xe, (int)string_cstr(&m.name), (int)note,
                              (int)string_cstr(&number), 0);
        string_free(&number);
        party_member_destroy(&m);
        return;
    }
    case 0xe1:
        if (g_session1.lockMouse)
            return;
        owner_member(self, &m);
        if (m.found)
            ui_message_box(msg_str(0xbf));
        party_member_destroy(&m);
        return;
    case 0xe2: {
        int x = 0, y = 0;

        if (g_session1.lockMouse)
            return;
        if (g_windowMgr.party_invitation_to_wnd)
            ui_window_mgr_post_quit1(&g_windowMgr, 0xd0);
        if (!ui_window_mgr_post_quit1(&g_windowMgr, 0x35) && session_get_party_member_count(&g_session1))
            ui_window_open(0x35);
        ui_window_mgr_post_quit1(&g_windowMgr, 0x44);
        if (!g_windowMgr.party_setting_wnd)
            return;
        ui_window_to_screen(wnd, &x, &y);
        ui_window_move(g_windowMgr.party_setting_wnd, x + 2 + wnd->width, y);
        return;
    }
    case 0xe3:
        if (g_session1.lockMouse)
            return;
        owner_member(self, &m);
        if (m.found && !is_own_name(&m) && ask(0x16b) == UI_MSGBOX_ANSWER_YES)
            mode_send_message(client_current_mode(), 0x3e, (int)string_cstr(&m.name), 0, 0, 0);
        party_member_destroy(&m);
        return;
    case 0xe4:
        if (g_session1.lockMouse)
            return;
        owner_member(self, &m);
        if (m.found && is_own_name(&m)) {
            party_member_destroy(&m);
            if (ask(0x165) == UI_MSGBOX_ANSWER_YES)
                mode_send_message(client_current_mode(), 0x3d, 0, 0, 0, 0);
            return;
        }
        party_member_destroy(&m);
        return;
    }
}

int ui_mini_party_info_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1,
                                          int val2, int val3, int val4)
{
    struct UIMiniPartyInfoWnd *self = (struct UIMiniPartyInfoWnd *)window;
    switch (msg) {
    case UI_MSG_COMMAND:
        command(self, val1, val2, val3, val4);
        return 0;
    case 0x17:
        return show_hp(self);
    case 0x30:
        menu_choice(self, val1);
        return 0;
    case 0x31:
        open_menu(self);
        return 0;
    default:
        return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
    }
}
