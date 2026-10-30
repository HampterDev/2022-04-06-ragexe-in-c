#include "ui/controls/buttons.h"
#include "web/party_booking_json.h"
#include "ui/social/party_board.h"
#include "web/service_manager_state.h"
#include "web/agency.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "core/memory.h"
#include "world/mode_dispatch.h"
#include "world/mode_manager.h"
#include "ui/window_access.h"
#include <string.h>
#include "json/service_json.h"
#include "contents/game_session.h"
#include "core/msg_str.h"
#include "session/session_items.h"

#define AGENCY_WND_ID          0x144
#define APPLY_FOR_PARTY_WND_ID 0x159
#define AGENCY_MSG_LIST_LOADED 0x3c
#define UI_MSG_BUTTON          6
#define AGENCY_BUTTON_REFRESH  0x197

#define CHAT_RED    0xff
#define CHAT_YELLOW 0xffff64

#define MODE_MSG_APPLY_FOR_PARTY 0x109

static void window_send(struct UIWindow *w, int msg, int arg)
{
    ui_window_send_msg(w, NULL, msg, arg, 0, 0, 0);
}

static void chat_line(int msg_id, int colour)
{
    ui_window_mgr_send_msg_(&g_windowMgr, 1, (int)msg_str(msg_id), colour, 0, 0);
}

static char parse_answer(struct JsonTree *root, const char *response)
{
    json_tree_init(root);
    return json_parse_text(root, response, strlen(response)) && json_value_is_object(root);
}

static void parse_done(struct JsonTree *root)
{
    json_tree_free(root);
}

static int answer_type(struct JsonTree *root)
{
    return json_value_as_int(json_value_member(root, "Type"));
}

static int get_int(const struct JsonTree *v, const char *key, int fallback)
{
    struct JsonTree def, got;
    int value;

    json_value_from_int(&def, fallback);
    json_value_get(v, &got, key, &def);
    value = json_value_as_int(&got);
    json_tree_free(&got);
    json_tree_free(&def);
    return value;
}

void adventurer_agency_change_master_answered(struct AdventurerAgencyMgr *self, int ok,
                                                         const char *response)
{
    struct JsonTree root;

    struct UIWindow *w;

    self->pending--;
    if (ok) {
        if (parse_answer(&root, response)) {
            if ((unsigned)(answer_type(&root) - 2) > 1) {
                w = ui_window_find(AGENCY_WND_ID);
                if (w)
                    window_send(w, UI_MSG_BUTTON, AGENCY_BUTTON_REFRESH);
            }
        }
        parse_done(&root);
    }
}

void adventurer_agency_my_party_loaded(struct AdventurerAgencyMgr *self, int ok,
                                                  const char *response)
{
    struct JsonTree root, data;

    struct AdvenPartyInfo info;
    struct UIWindow *w;

    self->pending--;
    if (ok) {
        if (parse_answer(&root, response) && answer_type(&root) != 2 && answer_type(&root) != 3) {
            if (json_value_is_null(json_value_member(&root, "data"))) {
                self->recruiting = 0;
                w = ui_window_open(AGENCY_WND_ID);
                if (w)
                    ui_adven_party_board_wnd_set_has_ad((struct UIAdvenPartyBoardWnd *)w, 0);
            } else {
                json_value_copy(&data, json_value_member(&root, "data"));
                party_booking_init(&info);
                if (!party_booking_read(&info, &data)) {
                    party_booking_clear(&info);
                    json_tree_free(&data);
                    parse_done(&root);
                    return;
                }
                w = ui_window_open(AGENCY_WND_ID);
                if (w) {
                    ui_adven_party_board_wnd_set_own_ad((struct UIAdvenPartyBoardWnd *)w, &info);
                }
                self->recruiting = 1;
                party_booking_clear(&info);
                json_tree_free(&data);
            }
        }
        parse_done(&root);
    }
}

void adventurer_agency_party_list_loaded(struct AdventurerAgencyMgr *self, int ok,
                                                    const char *response)
{
    struct JsonTree root;

    struct UIWindow *w;
    int total;

    self->pending--;
    if (ok) {
        if (parse_answer(&root, response) && answer_type(&root) != 2 && answer_type(&root) != 3) {
            if (!party_booking_list_read(&self->parties, json_value_member(&root, "data"))) {
                parse_done(&root);
                return;
            }
            w = ui_window_open(AGENCY_WND_ID);
            if (w) {
                total = get_int(&root, "totalPage", 0);
                if (total)
                    ((struct UIAdvenPartyBoardWnd *)w)->pages = total;
                window_send(w, AGENCY_MSG_LIST_LOADED, 0);
            }
        }
        parse_done(&root);
    }
}

void adventurer_agency_party_info_loaded(struct AdventurerAgencyMgr *self, int ok,
                                                    const char *response)
{
    struct JsonTree root;

    size_t it;
    struct AdvenPartyInfo info, *p;
    struct UIWindow *w;

    self->pending--;
    if (ok) {
        if (parse_answer(&root, response) && answer_type(&root) != 2 && answer_type(&root) != 3) {
            it = 0;
            if ((it == json_value_size(json_value_member(&root, "data")))) {
                chat_line(0xd73, CHAT_RED);
                ui_window_mgr_post_quit1(&g_windowMgr, APPLY_FOR_PARTY_WND_ID);
            } else {
                party_booking_init(&info);
                if (!party_booking_read(&info, json_value_child(json_value_member(&root, "data"), it))) {
                    party_booking_clear(&info);
                    parse_done(&root);
                    return;
                }

                w = ui_window_find(AGENCY_WND_ID);
                if (w) {
                    for (it = 0; it < self->parties.count; ++it) {
                        p = &self->parties.items[it];
                        if (p->aid == info.aid) {
                            if (!party_booking_assign(p, &info)) {
                                party_booking_clear(&info);
                                parse_done(&root);
                                return;
                            }
                            break;
                        }
                    }
                    window_send(w, AGENCY_MSG_LIST_LOADED, 0);
                }
                w = ui_window_find(APPLY_FOR_PARTY_WND_ID);
                if (w) {
                    ui_apply_for_party_wnd_set_ad((struct UIApplyForPartyWnd *)w, &info);
                    ui_window_mgr_raise_window(&g_windowMgr, w);
                }

                if (info.min_level > g_session1.charBaseLevel || info.max_level < g_session1.charBaseLevel) {
                    chat_line(0xd58, CHAT_RED);
                    party_booking_clear(&info);
                } else if (session_get_party_member_count(&g_session1) > 0) {
                    chat_line(0x4f, CHAT_RED);
                    party_booking_clear(&info);
                } else {
                    mode_send_message(client_current_mode(), MODE_MSG_APPLY_FOR_PARTY, (int)&info.aid,
                                                         (int)&info.gid, 0, 0);
                    ui_window_mgr_post_quit1(&g_windowMgr, APPLY_FOR_PARTY_WND_ID);
                    party_booking_clear(&info);
                }
            }
        }
        parse_done(&root);
    }
}

void adventurer_agency_register_answered(struct AdventurerAgencyMgr *self, int ok,
                                                    const char *response)
{
    struct JsonTree root;

    struct UIWindow *w;

    self->pending--;
    if (ok) {
        if (parse_answer(&root, response)) {
            if ((unsigned)(answer_type(&root) - 2) <= 1) {
                chat_line(0xd59, CHAT_RED);
            } else {
                self->recruiting = 1;
                chat_line(0xd5a, CHAT_YELLOW);
                w = ui_window_find(AGENCY_WND_ID);
                if (!w)
                    ui_window_open(AGENCY_WND_ID);
                else
                    window_send(w, UI_MSG_BUTTON, AGENCY_BUTTON_REFRESH);
            }
        }
        parse_done(&root);
    }
}

void adventurer_agency_stop_recruiting_answered(struct AdventurerAgencyMgr *self,
                                                           int ok,
                                                           const char *response)
{
    struct JsonTree root;

    struct UIWindow *w;
    struct UIButton *button;

    self->pending--;
    if (!ok) {
        if (self->stop_master_aid == g_session1.ownAid)
            chat_line(0xdb3, CHAT_RED);
    } else {
        if (parse_answer(&root, response)) {
            if ((unsigned)(answer_type(&root) - 2) <= 1) {
                if (self->stop_master_aid == g_session1.ownAid)
                    chat_line(0xdb3, CHAT_RED);
            } else {
                if (self->stop_master_aid == g_session1.ownAid)
                    chat_line(0xd83, CHAT_YELLOW);
                self->recruiting = 0;
                w = ui_window_find(AGENCY_WND_ID);
                if (w) {
                    button = (struct UIButton *)((struct UIAdvenPartyBoardWnd *)w)->booking_off;
                    ui_button_set_show(button, 0);
                    window_send(w, UI_MSG_BUTTON, AGENCY_BUTTON_REFRESH);
                }
            }
        }
        parse_done(&root);
    }
}
