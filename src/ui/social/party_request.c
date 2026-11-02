#include "ui/controls/listbox.h"
#include "render/rp_pool.h"
#include "core/object_type_tables.h"
#include "core/memory.h"
#include "core/fatal.h"
#include "world/mode_manager.h"
#include "ui/window_access.h"
#include <string.h>
#include "ui/social/party_board.h"
#include "ui/ui_std.h"
#include "core/msg_str.h"
#include "diag/perf.h"
#include "session/session_names.h"
#include "ui/wndmgr/wndmgr.h"

#define REQUEST_JOIN_PARTY_WND_ID 0x146

#define REQUEST_ID_ACCEPT 0x215
#define REQUEST_ID_REFUSE 0x216
#define REQUEST_ID_CLOSE 0xc1

#define MSG_REQUEST_TITLE 0xdb1
#define MSG_REQUEST_ACCEPT 0xda1
#define MSG_REQUEST_REFUSE 0xda2
#define MSG_REQUEST_TEXT 0xd32
#define MSG_REQUEST_BLOCK_LABEL 0xdbe

#define MSG_REQUEST_SHOW 0x38
#define MODE_REQ_ANSWER_JOIN 0x108

#define RENEWAL_PARTY UI_SKIN "renewalparty\\"

struct PartyJoinRequest {
    char name[0x18];
    int aid;
    int char_id;
    short job;
    short level;
};
SIZE_ASSERT(PartyJoinRequest, 0x24);

struct UIRequestJoinPartyMgr {
    BYTE valid;
    struct List requests;
    struct List blocked;
};

struct UIRequestJoinPartyMgr *g_requestJoinPartyMgr;

static struct UIRequestJoinPartyMgr *request_mgr_construct(struct UIRequestJoinPartyMgr *self)
{
    list_init(&self->requests);
    list_init(&self->blocked);
    self->valid = 1;
    return self;
}

static void request_mgr_destruct(struct UIRequestJoinPartyMgr *self)
{
    list_free(&self->blocked, NULL);
    list_free(&self->requests, NULL);
}

static void request_mgr_delete(struct UIRequestJoinPartyMgr *mgr)
{
    request_mgr_destruct(mgr);
    mem_free(mgr);
}

char ui_request_join_party_mgr_create(void)
{
    struct UIRequestJoinPartyMgr *mgr;

    if (g_requestJoinPartyMgr)
        return 0;
    mgr = mem_try_alloc(sizeof *mgr);
    g_requestJoinPartyMgr = mgr ? request_mgr_construct(mgr) : NULL;
    if (!g_requestJoinPartyMgr)
        return 0;
    if (!g_requestJoinPartyMgr->valid) {
        request_mgr_delete(g_requestJoinPartyMgr);
        g_requestJoinPartyMgr = NULL;
        return 0;
    }
    return 1;
}

void ui_request_join_party_mgr_delete(void)
{
    if (!g_requestJoinPartyMgr)
        return;
    request_mgr_delete(g_requestJoinPartyMgr);
    g_requestJoinPartyMgr = NULL;
}

static void request_mgr_block(struct UIRequestJoinPartyMgr *self, int char_id)
{
    list_push_back(&self->blocked, &char_id, sizeof char_id);
}

static int request_mgr_is_blocked(struct UIRequestJoinPartyMgr *self, int char_id)
{
    struct ListNode *head = self->blocked.head, *node;

    for (node = head->next; node != head; node = node->next)
        if (*LIST_VALUE(node, int) == char_id)
            return 1;
    return 0;
}

static void request_mgr_pop_front(struct UIRequestJoinPartyMgr *self)
{
    list_erase(&self->requests, self->requests.head->next, NULL);
}

void ui_request_join_party_mgr_show_next(struct UIRequestJoinPartyMgr *self)
{
    struct ListNode *head;
    struct UIWindow *wnd;
    struct PartyJoinRequest request;

    if (!self->requests.size)
        ui_window_mgr_post_quit1(&g_windowMgr, REQUEST_JOIN_PARTY_WND_ID);
    head = self->requests.head;
    if (head->next == head)
        return;
    wnd = ui_window_find(REQUEST_JOIN_PARTY_WND_ID);
    if (!wnd)
        return;
    request = *LIST_VALUE(head->next, struct PartyJoinRequest);
    if (request_mgr_is_blocked(self, request.char_id)) {
        request_mgr_pop_front(self);
        ui_request_join_party_mgr_show_next(self);
        return;
    }
    ui_window_send_msg(wnd, NULL, MSG_REQUEST_SHOW, (intptr_t)&request, 0, 0, 0);
    request_mgr_pop_front(self);
}

struct UIRequestJoinPartyWnd {
    struct UIFrameWnd frame;
    struct UIWindow *text;
    struct UIWindow *block_text;
    struct UIWindow *block_box;
    int char_id;
    int aid;
    short job;
    short level;
    struct UIBitmapButton *job_icon;
    struct UIWindow *level_text;
};
void ui_request_join_party_wnd_destroy(struct UIWindow *window);
void ui_request_join_party_wnd_on_create(struct UIWindow *self, int width_unused, int height_unused);
void ui_request_join_party_wnd_on_draw(struct UIWindow *self);
intptr_t ui_request_join_party_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4);
const struct UIWindowOps ui_request_join_party_wnd_ops = {
    .destroy = ui_request_join_party_wnd_destroy,
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
    .on_create = ui_request_join_party_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_request_join_party_wnd_on_draw,
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
    .send_msg = ui_request_join_party_wnd_send_msg,
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

struct UIRequestJoinPartyWnd *ui_request_join_party_wnd_construct(struct UIRequestJoinPartyWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_request_join_party_wnd_ops;
    return self;
}

struct UIWindow *ui_request_join_party_wnd_new(void)
{
    return (struct UIWindow *)ui_request_join_party_wnd_construct(mem_alloc(sizeof(struct UIRequestJoinPartyWnd)));
}

static void ui_request_join_party_wnd_destruct(struct UIRequestJoinPartyWnd *self)
{
    self->frame.base.ops = &ui_request_join_party_wnd_ops;
    ui_frame_wnd_destruct(&self->frame);
}

void ui_request_join_party_wnd_destroy(struct UIWindow *window)
{
    struct UIRequestJoinPartyWnd *self = (struct UIRequestJoinPartyWnd *)window;
    ui_request_join_party_wnd_destruct(self);
    mem_free(self);
}

static struct UIWindow *add_text_viewer(struct UIWindow *wnd, int x, int y, int w, int h, DWORD colour)
{
    struct UIWindow *text = mem_alloc(sizeof(struct UITextViewer));

    if (text)
        text = ui_text_viewer_construct(text);
    ui_list_box_create_window((struct UIListBox *)text, x, y, w, h, 0);
    UI_TEXT_VIEWER_BACKGROUND(text) = colour;
    ui_window_invalidate(text);
    ui_window_add_child(wnd, text);
    return text;
}

void ui_request_join_party_wnd_on_create(struct UIWindow *window, int width_unused,
                                               int height_unused)
{
    struct UIRequestJoinPartyWnd *self = (struct UIRequestJoinPartyWnd *)window;
    static const struct {
        int x, y;
    } button_at[2] = { { 0x7a, 0x88 }, { 0xa9, 0x88 } };
    static const int button_id[2] = { REQUEST_ID_ACCEPT, REQUEST_ID_REFUSE };
    static const char icon[] = RENEWAL_PARTY "icon_jobs_4046.bmp";
    struct UIWindow *wnd = &self->frame.base;
    const char *label[2];
    int i;

    self->job_icon = mem_alloc(sizeof(struct UIBitmapButton));
    if (self->job_icon)
        self->job_icon = ui_bitmap_button_construct(self->job_icon);
    ui_window_create(&self->job_icon->button.base, 0x19, 0x19);
    ui_bitmap_button_set_bitmap(self->job_icon, icon, UI_BTN_UP, 0);
    ui_bitmap_button_set_bitmap(self->job_icon, icon, UI_BTN_HOVER, 0);
    ui_bitmap_button_set_bitmap(self->job_icon, icon, UI_BTN_DOWN, 0);
    ui_window_move_base(&self->job_icon->button.base, 0xc, 0x1d);
    ui_window_add_child(wnd, &self->job_icon->button.base);

    self->level_text = mem_alloc(sizeof(struct UIStaticText));
    if (self->level_text)
        self->level_text = ui_static_text_construct(self->level_text);
    ui_window_create(self->level_text, 0x19, 0xe);
    ui_window_move(self->level_text, 0x10, 0x38);
    ui_static_text_set_text(self->level_text, "1", 0);
    ui_static_text_set_text_colour(self->level_text, 0, 0, 0);
    ui_window_add_child(wnd, self->level_text);

    label[0] = msg_str(MSG_REQUEST_ACCEPT);
    label[1] = msg_str(MSG_REQUEST_REFUSE);
    for (i = 0; i < 2; i++) {
        struct UIWindow *button = mem_alloc(sizeof(struct UIBasicButton));

        if (button)
            button = (struct UIWindow *)ui_basic_button_construct((struct UIBasicButton *)button);
        ui_basic_button_set_label(button, ui_window_text_width(wnd, label[i], 0, 0, 0xc, 0, 0) + 0xa,
                               label[i]);
        ui_window_move_base(button, button_at[i].x, button_at[i].y);
        button->id = button_id[i];
        ui_window_add_child(wnd, button);
    }
    UI_FRAME_DEFAULT_ID(&self->frame) = REQUEST_ID_ACCEPT;

    self->text = add_text_viewer(wnd, 0x2b, 0x14, 0xa7, 0x3c, 0xffffffff);

    self->block_box = mem_alloc(sizeof(struct UICheckBox));
    if (self->block_box)
        self->block_box = (struct UIWindow *)ui_check_box_construct(self->block_box);
    ui_window_create(self->block_box, 0x18, 0x2f);
    ui_check_box_set_bitmaps(self->block_box, UI_SKIN "uiadventureragency\\btn_block_on.bmp",
                          UI_SKIN "uiadventureragency\\btn_block_off.bmp", "");
    ui_window_move_base(self->block_box, 7, 0x51);
    ui_check_box_set_check(self->block_box, 0);
    self->block_box->id = UI_ID_CHECKBOX;
    ui_window_add_child(wnd, self->block_box);

    self->block_text = add_text_viewer(wnd, 0x1f, 0x51, 0xb6, 0x2f, 0xffd7c6bc);
    ((struct UITextViewer *)self->block_text)->list.line_height = 0xf;
    ui_window_send_msg(self->block_text, NULL, UI_MSG_ADD_TEXT, (intptr_t)msg_str(MSG_REQUEST_BLOCK_LABEL), 0x1e, 0, 0);

    ui_window_add_big_close_button(wnd, "", 0);
}

void ui_request_join_party_wnd_on_draw(struct UIWindow *window)
{
    struct UIRequestJoinPartyWnd *self = (struct UIRequestJoinPartyWnd *)window;
    struct UIWindow *wnd = &self->frame.base;

    ui_window_draw_bitmap(wnd, 0, 0, res_mgr_get(g_resMgr(), ui_skin_path(AGENCY "bg_agency_admit.bmp")),
                        0);
    ui_window_draw_title_bar(wnd, 1, msg_str(MSG_REQUEST_TITLE), 0);
}

static void request_show(struct UIRequestJoinPartyWnd *self, const struct PartyJoinRequest *request)
{
    struct UIWindow *wnd = &self->frame.base;
    char icon[0x1f4];
    char level[5];
    char text[0x400];
    int i;

    self->char_id = request->aid;
    self->level = request->level;
    self->job = request->job;
    self->aid = request->char_id;

    memset(icon, 0, sizeof icon);
    _snprintf_s(icon, sizeof icon, (size_t)-1, (RENEWAL_PARTY "icon_jobs_%lu.bmp"),
                    (int)self->job);
    for (i = UI_BTN_UP; i <= UI_BTN_DOWN; i++)
        ui_bitmap_button_set_bitmap(self->job_icon, icon, i, 0);
    ui_bitmap_button_set_tool_tip(self->job_icon, session_get_job_name(&g_session1, self->job, -1));

    memset(level, 0, sizeof level);
    _snprintf_s(level, sizeof level, (size_t)-1, "%d", (int)self->level);
    ui_static_text_set_text(self->level_text, level, 0);
    ui_window_move(self->level_text, 0x19 - ui_window_text_width(wnd, level, 0, 0, 0xc, 0, 0) / 2, 0x38);

    ui_list_box_clear((struct UIListBox *)self->text);
    memset(text, 0, sizeof text);
    _snprintf_s(text, sizeof text, (size_t)-1, msg_str(MSG_REQUEST_TEXT),
                    request->name);
    ui_window_send_msg(self->text, NULL, UI_MSG_ADD_TEXT, (intptr_t)text, 0, 0, 1);
    ui_window_move(self->text, 0x2b, VEC_COUNT(&((struct UITextViewer *)self->text)->list.items, struct String) == 3 ? 0x14 : 0x1e);
    ui_window_invalidate_children(wnd);
}

intptr_t ui_request_join_party_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                             intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4)
{
    struct UIRequestJoinPartyWnd *self = (struct UIRequestJoinPartyWnd *)window;
    if (msg == MSG_REQUEST_SHOW) {
        request_show(self, (const struct PartyJoinRequest *)val1);
        return 0;
    }
    if (msg != UI_MSG_COMMAND)
        return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);

    switch (val1) {
    case REQUEST_ID_CLOSE:
        ui_request_join_party_wnd_send_msg(&self->frame.base, NULL, UI_MSG_COMMAND, REQUEST_ID_REFUSE, 0, 0, 0);
        return 0;
    case REQUEST_ID_ACCEPT:
        mode_send_message(client_current_mode(), MODE_REQ_ANSWER_JOIN, self->char_id, 1, (intptr_t)&self->aid, 0);
        break;
    case REQUEST_ID_REFUSE:
        if (self->block_box->state)
            request_mgr_block(g_requestJoinPartyMgr, self->aid);
        mode_send_message(client_current_mode(), MODE_REQ_ANSWER_JOIN, self->char_id, 0, (intptr_t)&self->aid, 0);
        break;
    default:
        return 0;
    }
    ui_request_join_party_mgr_show_next(g_requestJoinPartyMgr);
    ui_check_box_set_check(self->block_box, 0);
    return 0;
}

void ui_request_join_party_mgr_add_request(struct UIRequestJoinPartyMgr *self, int char_id, int aid, const char *name, int level,
                           int job)
{
    struct PartyJoinRequest request;
    struct UIWindow *wnd;

    if (request_mgr_is_blocked(self, char_id))
        return;
    request.aid = aid;
    _snprintf_s(request.name, sizeof request.name, (size_t)-1, "%s", name);
    request.char_id = char_id;
    request.level = (short)level;
    request.job = (short)job;
    if (!ui_window_find(REQUEST_JOIN_PARTY_WND_ID)) {
        wnd = ui_window_open(REQUEST_JOIN_PARTY_WND_ID);
        ui_window_send(wnd, NULL, MSG_REQUEST_SHOW, (intptr_t)&request, 0, 0, 0);
        return;
    }
    list_push_back(&self->requests, &request, sizeof request);
}
