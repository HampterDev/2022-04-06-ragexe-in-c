#include "ui/controls/buttons.h"
#include "ui/chat/friends.h"
#include "session/character_name.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "contents/mouse.h"
#include "world/mode_manager.h"
#include "game/context.h"
#include "ui/window_access.h"
#include "core/markup_tags.h"
#include "ui/social/party_windows.h"
#include "ui/ui_std.h"
#include "core/msg_str.h"
#include "session/csession.h"
#include "session/session_items.h"
#include "session/session_party.h"
#include "session/session_state.h"
#include "ui/character/gauge.h"

#define POPUP_MENU_WND 0x12
#define MENU_MSG_ADD 0x1f
#define MENU_MSG_OWNER 0x53
#define MENU_MSG_FIT 0x1e

enum PartyMenuValue {
    PARTY_MENU_EXPEL = 0,
    PARTY_MENU_INVITE = 5,
    PARTY_MENU_WHISPER = 7,
    PARTY_MENU_LEAVE = 9,
    PARTY_MENU_FRIEND_DELETE = 0x13,
    PARTY_MENU_DELEGATE = 0x2b,
    PARTY_MENU_EQUIPMENT = 0x3a,
    PARTY_MENU_PLATE = 0x3c,
    PARTY_MENU_REMOVE_PLATE = 0x3d,
};

static int party_box(int msg_id, int type, int arg)
{
    return ui_window_mgr_message_box(&g_windowMgr, msg_str(msg_id), type, arg, 0, 0, UI_MSGBOX_TITLE, 0x118, 0x78,
                                  0);
}
#define party_ask(msg_id) (party_box((msg_id), UI_MSGBOX_QUESTION, 0) == UI_MSGBOX_ANSWER_YES)

static int is_own_name(const struct PartyMember *m)
{
    return string_equal_cstr(&m->name, session_character_name(&g_session1));
}

static void destroy_member(void *value)
{
    party_member_destroy(value);
}

static void rows_push_back(struct UIMessengerGroupWnd *self, const struct PartyMember *m)
{
    struct PartyMember copy;

    party_member_info_copy(&copy, m);
    list_push_back(&self->rows, &copy, sizeof copy);
}

static int info_usable(const struct PartyWndInfo *info)
{
    if (info->mode > 2 || info->width < 0xf3 || info->height < 0xa6)
        return 0;
    if (info->x < 0 || info->x > g_renderer->width - info->width)
        return 0;
    if (info->y < 0 || info->y > g_renderer->height - info->height)
        return 0;
    return info->tab == 0 || info->tab == 1;
}

static void info_default(struct PartyWndInfo *info)
{
    info->width = 0x111;
    info->height = 0x146;
    info->mode = 0;
    info->x = 0x12c;
    info->y = 0xc8;
    info->show = 0;
    info->tab = 1;
}

static int refresh(struct UIMessengerGroupWnd *self)
{
    struct UIWindow *wnd = &self->frame.base;
    struct ListNode *node;
    struct PartyMember m;
    struct Mode *mode;
    char title[0x40];
    int count, i;

    list_clear(&self->rows, destroy_member);
    for (i = 0; i < PARTY_WND_ROWS; i++)
        ui_window_move_base(&self->gauges[i]->gage.base, -100, -100);
    if (self->tab == 0) {
        count = session_get_friend_count(&g_session1);
        for (i = 0; i < count; i++) {
            session_get_friend_by_index(&g_session1, &m, i);
            if (m.found)
                rows_push_back(self, &m);
            party_member_destroy(&m);
        }
        self->row_height = 0x17;
    } else if (self->tab == 1) {
        count = session_get_party_member_count(&g_session1);
        for (i = 0; i < count; i++) {
            party_member_at(&m, i);
            if (m.found)
                rows_push_back(self, &m);
            self->row_height = PARTY_LAYOUT(&self->layouts, self->mode).row_height;
            party_member_destroy(&m);
        }
    }

    mode = game_current_base_mode();
    if (mode && self->tab != 0) {
        i = 0;
        for (node = self->rows.head->next; node != self->rows.head; node = node->next, i++) {
            struct PartyMember *n = LIST_VALUE(node, struct PartyMember);
            struct UIJobIconButton *icon = self->job_icons[i];
            struct String base;
            char path[0x100];
            int die;

            if (n->found && n->state == 0) {
                void *actor;
                int hp, max_hp;

                if (n->aid == g_session1.ownAid) {
                    actor = game_current_player();
                    hp = g_session1.hp;
                    max_hp = g_session1.maxHp;
                } else {
                    actor = game_find_actor(n->aid);
                    if (actor && party_actor_gauge(actor)) {
                        hp = party_actor_gauge(actor)->hp;
                        max_hp = party_actor_gauge(actor)->max_hp;
                    } else {
                        hp = max_hp = 0;
                    }
                }
                if (actor)
                    ui_pc_gage_set_hp(self->gauges[i], hp, max_hp);
                icon->draw_mode = 0;
            } else {
                icon->draw_mode = 1;
            }
            memset(path, 0, sizeof(path));
            die = 0;
            if (n->dead == 1)
                die = 1;
            else if (g_session1.ownAid == n->aid && game_player_is_dead())
                die = 1;
            string_init(&base);
            string_assign(&base, RENEWAL_PARTY, 0x1c);
            sprintf(path, die ? "%sicon_jobs_%d_die.bmp" : "%sicon_jobs_%d.bmp", string_cstr(&base),
                        n->job);
            ui_bitmap_button_set_bitmap(&icon->bmp, path, 0, 0);
            ui_bitmap_button_set_bitmap(&icon->bmp, path, 1, 0);
            ui_bitmap_button_set_bitmap(&icon->bmp, path, 2, 0);
            ui_window_invalidate_base(&icon->bmp.button.base);
            string_free(&base);
        }
    }

    ui_messenger_group_wnd_update_scroll(self);
    ui_window_invalidate_base(wnd);
    party_wnd_title(self, title);
    ui_static_text_set_text_ellipsis(self->title, title);
    if (g_windowMgr.kafra_move_map_service_info_wnd)
        ui_window_send_msg(g_windowMgr.kafra_move_map_service_info_wnd, NULL, 0x3c, 0, 0, 0, 0);
    {
        struct UIBitmapButton *agency = self->buttons[0];

        if (self->tab == 0) {
            agency->button.disabled = 1;
            ui_window_invalidate_base(&agency->button.base);
            for (i = 0; i < 4; i++)
                ui_bitmap_button_set_bitmap(agency, UI_SKIN "basic_interface\\mesbtn_01_disable.bmp", i, 0);
            ui_bitmap_button_set_tool_tip(agency, msg_str(0x62));
        } else {
            agency->button.disabled = 0;
            ui_window_invalidate_base(&agency->button.base);
            ui_bitmap_button_set_bitmap(agency, UI_SKIN "basic_interface\\mesbtn_partymaster_01.bmp", 0, 0);
            ui_bitmap_button_set_bitmap(agency, UI_SKIN "basic_interface\\mesbtn_partymaster_01_a.bmp", 1, 0);
            ui_bitmap_button_set_bitmap(agency, UI_SKIN "basic_interface\\mesbtn_partymaster_01_b.bmp", 2, 0);
            ui_bitmap_button_set_tool_tip(agency, msg_str(0xdba));
        }
    }
    ui_messenger_group_wnd_layout_buttons(self);
    return 0;
}

static int resize(struct UIMessengerGroupWnd *self, int width, int height)
{
    struct UIWindow *wnd = &self->frame.base;
    struct UIWindow *docked;
    char title[0x40];
    int rh;

    if (self->mode == 0) {
        width = 0x111;
        if (height < 0x146)
            height = 0x146;
    } else if (self->mode == 1) {
        width = 0xf3;
        if (height < 0xb3)
            height = 0xb3;
    }
    if (self->tab != 0)
        self->row_height = PARTY_LAYOUT(&self->layouts, self->mode).row_height;
    else
        self->row_height = 0x17;
    rh = self->row_height;
    if ((height - 0x3a) / rh > 0xc)
        height = rh * 12 + 0x3a;
    else
        height -= (height - 0x3a) % rh;
    if (width != wnd->width || height != wnd->height) {
        self->first = 0;
        ui_window_resize_base(wnd, width, height);
    }
    ui_scroll_bar_resize(&self->scrollbar->base, ((struct UIWindow *)self->scrollbar)->width, wnd->height - 0x3a);
    if (self->tab != 0)
        self->visible = ((struct UIWindow *)self->scrollbar)->height / self->row_height;
    else
        self->visible = (wnd->height - 0xb4) / self->row_height + 5;
    ui_window_move_base(self->close_button, wnd->width - 0xe, 3);
    ui_window_move_base(&self->scrollbar->base, wnd->width - 0xf, 0x11);
    ui_window_move_base(&self->resizer->base, wnd->width - 0xf, wnd->height - 0xf);
    ui_window_move_base(self->mini_button, wnd->width - 0x1c, 3);
    ui_window_move_base(&self->max_button->button.base, wnd->width - 0x1c, 3);
    if (self->mode == 0) {
        ui_button_show((struct UIButton *)(self->mini_button), 1);
        ui_button_show((struct UIButton *)(self->max_button), 0);
    } else {
        ui_button_show((struct UIButton *)(self->max_button), 1);
        ui_button_show((struct UIButton *)(self->mini_button), 0);
    }
    ui_window_move_base(&self->tabs[0]->base, 4, wnd->height - 0x10);
    ui_window_move_base(&self->tabs[1]->base, 0x40, wnd->height - 0x10);
    ui_messenger_group_wnd_layout_buttons(self);
    memset(title, 0, sizeof(title));
    party_wnd_title(self, title);
    ui_window_resize(self->title, wnd->width - 0x39, 0xc);
    ui_static_text_set_text_ellipsis(self->title, title);
    ui_window_move(self->title, 0x10, 3);
    ui_messenger_group_wnd_update_scroll(self);
    docked = self->tab != 0 ? g_windowMgr.party_setting_wnd : g_windowMgr.friend_option_wnd;
    if (docked)
        party_wnd_dock(self, docked);
    ui_window_mgr_update_snap_rect(&g_windowMgr, wnd);
    return 0;
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

static int open_menu(struct UIMessengerGroupWnd *self)
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

    if (self->hover != -1) {
        if (self->tab == 0) {
            menu_add(&self->menu, 0x168, PARTY_MENU_WHISPER);
            menu_add(&self->menu, 0xaf8, PARTY_MENU_EQUIPMENT);
            menu_add(&self->menu, 0x15f, PARTY_MENU_FRIEND_DELETE);
            ui_messenger_group_wnd_get_member(self, &m, self->hover);
            if (m.found && m.state == 0) {
                int invite = 0;

                if (g_session1.partyLeader) {
                    struct PartyMember member;

                    if (!session_get_party_member_by_name(&g_session1, &member, string_cstr(&m.name))->found)
                        invite = 1;
                    party_member_destroy(&member);
                }
                if (invite) {
                    char text[0x100];
                    struct UIMenuEntry entry;

                    sprintf(text, msg_str(0x532), string_cstr(&m.name));
                    string_init(&entry.text);
                    string_assign_cstr(&entry.text, text);
                    entry.id = PARTY_MENU_INVITE;
                    ui_menu_entry_vec_push_move(&self->menu, &entry);
                    string_free(&entry.text);
                }
            }
            party_member_destroy(&m);
        } else {
            ui_messenger_group_wnd_get_member(self, &m, self->hover);
            if (m.found) {
                menu_add(&self->menu, 0xaf8, PARTY_MENU_EQUIPMENT);
                if (is_own_name(&m)) {
                    menu_add(&self->menu, 0x60, PARTY_MENU_LEAVE);
                } else {
                    menu_add(&self->menu, 0x168, PARTY_MENU_WHISPER);
                    if (g_session1.partyLeader) {
                        menu_add(&self->menu, 0x61, PARTY_MENU_EXPEL);
                        menu_add(&self->menu, 0x5fc, PARTY_MENU_DELEGATE);
                    }
                }
                if (m.state == 0) {
                    menu_add(&self->menu, 0xc1d, PARTY_MENU_PLATE);
                    menu_add(&self->menu, 0xc1e, PARTY_MENU_REMOVE_PLATE);
                }
            }
            party_member_destroy(&m);
        }
    }
    if (VEC_COUNT(&self->menu, struct UIMenuEntry) == 0)
        return 0;

    menu = ui_window_open(POPUP_MENU_WND);
    for (i = 0; i < VEC_COUNT(&self->menu, struct UIMenuEntry); i++)
        ui_window_send_msg(menu, NULL, MENU_MSG_ADD,
                 (intptr_t)string_cstr(&VEC_AT(&self->menu, struct UIMenuEntry, i).text), 0, 0, 0);
    ui_window_send_msg(menu, NULL, MENU_MSG_OWNER, 0x22, 0, 0, 0);
    ui_window_send_msg(menu, NULL, MENU_MSG_FIT, 0, 0, 0, 0);
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
    return 0;
}

static int menu_choice(struct UIMessengerGroupWnd *self, int index)
{
    struct UIWindow *wnd = &self->frame.base;
    struct PartyMember m, m2;
    struct PlateRecord rec;

    if (self->menu.first == self->menu.last)
        return 0;
    ui_messenger_group_wnd_get_member(self, &m, self->hover);
    if (text_view_find(string_view(&m.name), string_view(&text_tag_NAMELESS), 0)
        != STRING_NPOS) {
        ui_message_box(msg_str(0xbe1));
        party_member_destroy(&m);
        return 0;
    }
    if (!m.found)
        goto out;
    switch (VEC_AT(&self->menu, struct UIMenuEntry, index).id) {
    case PARTY_MENU_EXPEL:
    case PARTY_MENU_FRIEND_DELETE:
        ui_messenger_group_wnd_send_msg(wnd, NULL, UI_MSG_COMMAND, 0xe3, 0, 0, 0);
        break;
    case 3:
        ui_messenger_group_wnd_send_msg(wnd, NULL, UI_MSG_COMMAND, 0xe1, 0, 0, 0);
        break;
    case PARTY_MENU_INVITE:
        if (self->hover == -1)
            break;
        ui_messenger_group_wnd_get_member(self, &m2, self->hover);
        if (m2.found)
            mode_send_message(client_current_mode(), 0x3b, (intptr_t)string_cstr(&m2.name), 0, 0, 0);
        party_member_destroy(&m2);
        break;
    case 6:
        ui_messenger_group_wnd_send_msg(wnd, NULL, UI_MSG_COMMAND, 0xdf, 0, 0, 0);
        break;
    case PARTY_MENU_WHISPER:
        ui_messenger_group_wnd_send_msg(wnd, NULL, UI_MSG_COMMAND, 0xe0, 0, 0, 0);
        break;
    case PARTY_MENU_LEAVE:
        ui_messenger_group_wnd_send_msg(wnd, NULL, UI_MSG_COMMAND, 0xe4, 0, 0, 0);
        break;
    case 0x14:
        ui_messenger_group_wnd_send_msg(wnd, NULL, 0x47, 0, 0, 0, 0);
        break;
    case PARTY_MENU_DELEGATE:
        if (self->hover == -1)
            break;
        ui_messenger_group_wnd_get_member(self, &m2, self->hover);
        if (m2.found && m2.state == 0) {
            if (party_ask(0x5fd))
                mode_send_message(client_current_mode(), 0x104, m2.aid, 0, 0, 0);
        } else {
            ui_window_mgr_send_msg_(&g_windowMgr, 3, 0, 0, 0, 0);
            ui_window_mgr_send_msg_(&g_windowMgr, UIWM_MSG_CHAT_LINE, (intptr_t)msg_str(0x5fe), 0xff, 3, 0);
        }
        party_member_destroy(&m2);
        break;
    case PARTY_MENU_EQUIPMENT:
        ui_messenger_group_wnd_get_member(self, &m2, self->hover);
        if (m2.found)
            mode_send_message(client_current_mode(), 0x10c, (intptr_t)string_cstr(&m2.name), 0, 0, 0);
        party_member_destroy(&m2);
        break;
    case PARTY_MENU_PLATE:
        ui_messenger_group_wnd_get_member(self, &m2, self->hover);
        if (m2.found) {
            session_get_plate_record(&g_session1, &rec, m2.aid);
            if (rec.found != 1)
                ui_messenger_group_wnd_create_plate(ui_window_mgr_find_free_plate_cell(&g_windowMgr), &m2);
        }
        party_member_destroy(&m2);
        break;
    case PARTY_MENU_REMOVE_PLATE:
        ui_messenger_group_wnd_get_member(self, &m2, self->hover);
        if (m2.found)
            ui_window_mgr_remove_plate(&g_windowMgr, m2.aid);
        party_member_destroy(&m2);
        break;
    }
out:
    party_member_destroy(&m);
    return 0;
}

static int choose_tab(struct UIMessengerGroupWnd *self, struct UIWindow *sender, int which)
{
    struct UIWindow *wnd = &self->frame.base;
    int old = self->tab, i;

    if (which)
        sender = (struct UIWindow *)self->tabs[which - 1];
    for (i = 0; i < 2; i++) {
        ui_window_invalidate_base(&self->tabs[i]->base);
        if (sender == (struct UIWindow *)self->tabs[i]) {
            self->tab = i;
            ui_radio_btn_send_msg(&self->tabs[i]->base, wnd, PARTY_MSG_SET_STATE, 1, 0, 0, 0);
        } else {
            ui_radio_btn_send_msg(&self->tabs[i]->base, wnd, PARTY_MSG_SET_STATE, 0, 0, 0, 0);
        }
    }
    if (self->tab == 0) {
        ui_bitmap_button_set_tool_tip(self->buttons[3], msg_str(0x163));
        ui_bitmap_button_set_tool_tip(self->buttons[4], msg_str(0x15f));
    } else {
        ui_bitmap_button_set_tool_tip(self->buttons[3], msg_str(0x65));
        ui_bitmap_button_set_tool_tip(self->buttons[4], msg_str(0x61));
    }
    if (self->tab != old)
        ui_messenger_group_wnd_send_msg(wnd, NULL, PARTY_MSG_REFRESH, 0, 0, 0, 0);
    ui_messenger_group_wnd_send_msg(wnd, NULL, PARTY_MSG_RESIZE, PARTY_RESIZE_SIZE, wnd->width, wnd->height, 0);
    return 0;
}

static int member_hp(struct UIMessengerGroupWnd *self, int aid, int hp, int max_hp)
{
    struct Mode *mode = game_current_base_mode();
    struct ListNode *node;
    int i = 0;

    if (!mode || self->tab == 0)
        return 0;
    for (node = self->rows.head->next; node != self->rows.head; node = node->next, i++) {
        const struct PartyMember *member = LIST_VALUE(node, struct PartyMember);
        void *actor;

        if (!member->found || member->state != 0 || member->aid != aid)
            continue;
        if (member->aid == g_session1.ownAid) {
            actor = game_current_player();
            hp = g_session1.hp;
            max_hp = g_session1.maxHp;
        } else {
            actor = game_find_actor(aid);
        }
        if (actor)
            ui_pc_gage_set_hp(self->gauges[i], hp, max_hp);
        ui_messenger_group_wnd_update_gauges(self);
        ui_window_invalidate_base(&self->frame.base);
        return 1;
    }
    return 0;
}

static void toggle_window(int id)
{
    if (!ui_window_mgr_post_quit1(&g_windowMgr, id))
        ui_window_open(id);
}

static int command(struct UIMessengerGroupWnd *self, struct UIWindow *sender, int id, int val2, int val3, int val4)
{
    struct UIWindow *wnd = &self->frame.base;
    struct PartyMember m;
    struct UIWindow *docked;

    switch (id) {
    case 0x22:
        return member_hp(self, val2, val3, val4);
    case UI_ID_CLOSE:
        ui_window_mgr_post_quit1(&g_windowMgr, 0x45);
        return 0;
    case UI_ID_HELP:
        self->mode = 1;
        ui_button_show((struct UIButton *)(self->max_button), 1);
        ui_button_show((struct UIButton *)(self->mini_button), 0);
        ui_messenger_group_wnd_send_msg(wnd, sender, PARTY_MSG_RESIZE, PARTY_RESIZE_SIZE, 0xf3, 0xb3, 0);
        return 0;
    case UI_ID_RADIO:
        return choose_tab(self, sender, val2);
    case 0xdf:
        if (g_session1.lockMouse || self->hover == -1)
            return 0;
        ui_messenger_group_wnd_get_member(self, &m, self->hover);
        party_member_destroy(&m);
        return 0;
    case 0xe0: {
        const char *note;
        struct String number;

        if (g_session1.lockMouse || self->hover == -1)
            return 0;
        ui_messenger_group_wnd_get_member(self, &m, self->hover);
        if (m.found && !is_own_name(&m)) {
            if (ui_window_mgr_is_friend(&g_windowMgr, string_cstr(&m.name)))
                note = msg_str(0x66);
            else
                note = session_is_guild_member_char_id(&g_session1, m.char_id) ? "GuildMember" : NULL;
            session_scramble_number(&g_session1, &number, m.char_id);
            ui_window_mgr_send_msg_(&g_windowMgr, 0xe, (intptr_t)string_cstr(&m.name), (intptr_t)note,
                                  (intptr_t)string_cstr(&number), 0);
            string_free(&number);
        }
        party_member_destroy(&m);
        return 0;
    }
    case 0xe1:
        if (g_session1.lockMouse || self->hover == -1)
            return 0;
        ui_messenger_group_wnd_get_member(self, &m, self->hover);
        if (m.found)
            ui_message_box(msg_str(0xbf));
        party_member_destroy(&m);
        return 0;
    case 0xe2:
        if (g_session1.lockMouse)
            return 0;
        if (g_windowMgr.party_invitation_to_wnd)
            ui_window_mgr_post_quit1(&g_windowMgr, 0xd0);
        if (self->tab == 0)
            toggle_window(0x44);
        else if (!ui_window_mgr_post_quit1(&g_windowMgr, 0x35) && session_get_party_member_count(&g_session1)
                 && self->tab == 1)
            ui_window_open(0x35);
        if (self->tab == 0) {
            ui_window_mgr_post_quit1(&g_windowMgr, 0x35);
            docked = g_windowMgr.friend_option_wnd;
        } else {
            ui_window_mgr_post_quit1(&g_windowMgr, 0x44);
            docked = g_windowMgr.party_setting_wnd;
        }
        if (docked)
            party_wnd_dock(self, docked);
        return 0;
    case 0xe3:
        if (g_session1.lockMouse || self->hover == -1)
            return 0;
        ui_messenger_group_wnd_get_member(self, &m, self->hover);
        if (m.found) {
            if (self->tab == 0) {
                if (party_ask(0x164))
                    mode_send_message(client_current_mode(), 0xb0, m.aid, m.char_id, 0, 0);
            } else if (!is_own_name(&m) && party_ask(0x16b)) {
                mode_send_message(client_current_mode(), 0x3e, (intptr_t)string_cstr(&m.name), 0, 0, 0);
            }
        }
        party_member_destroy(&m);
        return 0;
    case 0xe4:
        if (g_session1.lockMouse || self->hover == -1)
            return 0;
        ui_messenger_group_wnd_get_member(self, &m, self->hover);
        if (m.found && is_own_name(&m) && party_ask(0x165))
            mode_send_message(client_current_mode(), 0x3d, 0, 0, 0, 0);
        party_member_destroy(&m);
        return 0;
    case 0x164:
        if (g_session1.lockMouse != 1)
            ui_window_mgr_sort_plates(&g_windowMgr);
        return 0;
    case 0x171: {
        struct UIWindow *other;

        if (sender == (struct UIWindow *)self->buttons[5]) {
            g_session1.lockMouse = 0;
            other = (struct UIWindow *)self->buttons[6];
        } else if (sender == (struct UIWindow *)self->buttons[6]) {
            g_session1.lockMouse = 1;
            other = (struct UIWindow *)self->buttons[5];
        } else {
            return 0;
        }
        ui_window_move_base(other, sender->x, sender->y);
        ui_window_move(sender, -100, -100);
        return 0;
    }
    case 0x1b5:
        if (g_session1.lockMouse)
            return 0;
        if (g_windowMgr.friend_option_wnd)
            ui_window_mgr_post_quit1(&g_windowMgr, 0x44);
        if (!ui_window_mgr_post_quit1(&g_windowMgr, 0x35) && !session_get_party_member_count(&g_session1)) {
            if (self->tab != 1)
                return 0;
            ui_window_open(0xcf);
        }
        if (self->tab != 1)
            return 0;
        if (g_windowMgr.party_setting_wnd)
            party_wnd_dock(self, g_windowMgr.party_setting_wnd);
        return 0;
    case 0x1b6:
        if (g_session1.lockMouse)
            return 0;
        if (!party_ask(0x165))
            return 0;
        mode_send_message(client_current_mode(), 0x3d, 0, 0, 0, 0);
        if (g_windowMgr.party_invitation_to_wnd)
            ui_window_mgr_post_quit1(&g_windowMgr, 0xd0);
        return 0;
    case 0x1b7:
        if (g_session1.lockMouse)
            return 0;
        if (g_windowMgr.friend_option_wnd)
            ui_window_mgr_post_quit1(&g_windowMgr, 0x44);
        if (g_windowMgr.party_setting_wnd)
            ui_window_mgr_post_quit1(&g_windowMgr, 0x35);
        if (!ui_window_mgr_post_quit1(&g_windowMgr, 0xd0)) {
            if (g_session1.partyLeader)
                ui_window_open(0xd0);
            else
                party_box(0x816, 4, 1);
        }
        if (self->tab != 1)
            return 0;
        if (g_windowMgr.party_invitation_to_wnd)
            party_wnd_dock(self, g_windowMgr.party_invitation_to_wnd);
        return 0;
    case 0x203:
        self->mode = 0;
        ui_button_show((struct UIButton *)(self->max_button), 0);
        ui_button_show((struct UIButton *)(self->mini_button), 1);
        ui_messenger_group_wnd_send_msg(wnd, sender, PARTY_MSG_RESIZE, PARTY_RESIZE_SIZE, 0x111, 0x146, 0);
        return 0;
    case 0x218:
        toggle_window(0x144);
        return 0;
    }
    return 0;
}

intptr_t ui_messenger_group_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, intptr_t val1,
                                           intptr_t val2, intptr_t val3, intptr_t val4)
{
    struct UIMessengerGroupWnd *self = (struct UIMessengerGroupWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    int old;

    switch (msg) {
    case UI_MSG_COMMAND:
        return command(self, sender, val1, val2, val3, val4);
    case PARTY_MSG_SCROLL:
    case PARTY_MSG_PAGE_UP:
    case PARTY_MSG_PAGE_DOWN:
        old = self->first;
        if (msg == PARTY_MSG_PAGE_UP)
            self->first = old - self->visible + 1;
        else if (msg == PARTY_MSG_PAGE_DOWN)
            self->first = self->visible - 1 + old;
        else
            self->first = old + val1;
        if (old != self->first) {
            ui_messenger_group_wnd_update_scroll(self);
            ui_window_invalidate_base(wnd);
        }
        return self->first - old;
    case PARTY_MSG_RESIZE:
        if (val1 != PARTY_RESIZE_SIZE)
            return 0;
        return resize(self, val2, val3);
    case PARTY_MSG_REFRESH:
        return refresh(self);
    case UI_MSG_SET_INFO:
        self->info = (struct PartyWndInfo *)val1;
        if (!info_usable(self->info))
            info_default(self->info);
        self->mode = self->info->mode;
        ui_messenger_group_wnd_send_msg(wnd, NULL, PARTY_MSG_RESIZE, PARTY_RESIZE_SIZE, self->info->width, self->info->height, 0);
        ui_frame_wnd_move(wnd, self->info->x, self->info->y);
        return 0;
    case PARTY_MSG_MENU_CHOICE:
        return menu_choice(self, val1);
    case PARTY_MSG_OPEN_MENU:
        return open_menu(self);
    default:
        return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
    }
}
