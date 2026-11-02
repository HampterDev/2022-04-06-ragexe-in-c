#include "session/state.h"
#include "core/memory.h"
#include "ui/window_access.h"
#include "ui/wndmgr/wndmgr.h"
#include "ui/social/party_windows.h"
#include "ui/character/popup_menu.h"
#include "ui/social/mini_party_area.h"
#include "session/session_party.h"
#include "session/session_state.h"
#include "session/session_windows.h"
#include "platform/clock.h"

#define PLATE_MEMBER_ID(plate) ((plate)->member.aid)
#define NODE_PLATE(node) (*LIST_VALUE(node, struct UIMiniPartyWnd *))

static struct MiniPartyWndAreaMgr *area_mgr(void)
{
    struct MiniPartyWndAreaMgr *mgr = g_miniPartyAreaMgr;

    if (mgr == NULL) {
        mgr = (struct MiniPartyWndAreaMgr *)mem_try_alloc(WM_MINI_PARTY_AREA_MGR_SIZE);
        mgr = mgr ? mini_party_area_mgr_construct(mgr) : NULL;
        g_miniPartyAreaMgr = mgr;
    }
    return mgr;
}

int ui_window_mgr_find_free_plate_cell(struct UIWindowMgr *self)
{
    (void)self;
    return mini_party_area_mgr_find_free_cell(area_mgr());
}

struct UIMiniPartyWnd *ui_window_mgr_get_plate(struct UIWindowMgr *self, int id)
{
    struct ListNode *node;

    for (node = self->plates.head->next; node != self->plates.head; node = node->next)
        if (PLATE_MEMBER_ID(NODE_PLATE(node)) == id)
            return NODE_PLATE(node);
    return NULL;
}

BYTE ui_window_mgr_has_plate(struct UIWindowMgr *self, int id)
{
    struct ListNode *node;

    for (node = self->plates.head->next; node != self->plates.head; node = node->next)
        if (PLATE_MEMBER_ID(NODE_PLATE(node)) == id)
            return 1;
    return 0;
}

struct UIWindow *ui_window_mgr_find_by_key(struct UIWindowMgr *self, int key)
{
    struct ListNode *node;

    for (node = self->plates.head->next; node != self->plates.head; node = node->next)
        if (NODE_PLATE(node)->cell == key)
            return *LIST_VALUE(node, struct UIWindow *);
    return NULL;
}

void ui_window_mgr_remove_plate(struct UIWindowMgr *self, int id)
{
    session_erase_plate_record(&g_session1, id);
    list_push_back(&self->plate_ids, &id, sizeof id);
    session_notify_messenger_group_and_plates(&g_session1);
}

void ui_window_mgr_close_all_plates(struct UIWindowMgr *self)
{
    struct ListNode *node, *next;

    for (node = self->plates.head->next; node != self->plates.head; node = next) {
        next = node->next;
        ui_window_mgr_post_quit2(self, *LIST_VALUE(node, struct UIWindow *));
        list_erase(&self->plates, node, NULL);
    }
}

void ui_window_mgr_close_drag_plate(struct UIWindowMgr *self)
{
    struct UIMiniPartyWnd *plate = (struct UIMiniPartyWnd *)self->drag_plate;

    if (plate && plate->member.found) {
        plate->frame.base.state = 1;
        ui_window_mgr_post_quit2(self, self->drag_plate);
        self->drag_plate = NULL;
    }
}

void ui_window_mgr_plates_changed(struct UIWindowMgr *self)
{
    struct ListNode *node;
    POINT pos;

    for (node = self->plates.head->next; node != self->plates.head; node = node->next) {
        int cell = NODE_PLATE(node)->cell;

        mini_party_area_mgr_cell_pos(area_mgr(), &pos, cell);
        ui_window_move(*LIST_VALUE(node, struct UIWindow *), pos.x, pos.y);
    }
}

void ui_window_mgr_add_plate(struct UIWindowMgr *self, struct UIMiniPartyWnd *plate)
{
    list_push_back(&self->windows, &plate, sizeof plate);
    list_push_back(&self->plates, &plate, sizeof plate);
    ui_window_mgr_plates_changed(self);
}

void ui_window_mgr_set_drag_plate(struct UIWindowMgr *self, struct UIWindow *plate)
{
    if (self->drag_plate)
        ui_window_mgr_post_quit2(self, self->drag_plate);
    self->drag_plate = plate;
    plate->alpha_target = 0x80;
    plate->alpha_time = platform_ticks_ms();
    list_push_back(&self->windows, &self->drag_plate, sizeof self->drag_plate);
}

RECT *ui_window_mgr_mini_party_area(struct UIWindowMgr *self, RECT *out)
{
    int height = self->screen_height;
    int top = (int)((float)height * 0.3f);
    struct UIWindow *anchor = ui_window_find(0x133);

    if (anchor)
        top = anchor->height;
    out->left = 0;
    out->top = top;
    out->right = self->screen_width;
    out->bottom = height - g_miniPartyCellHeight * 3;
    return out;
}

void mini_party_sort_area_get_cells(struct MiniPartySortArea *self, struct List *out)
{
    struct ListNode *node;

    list_init(out);
    for (node = self->cells.head->next; node != self->cells.head; node = node->next)
        list_push_back(out, LIST_VALUE(node, int), sizeof(int));
}

void ui_window_mgr_sort_plates(struct UIWindowMgr *self)
{
    struct MiniPartySortArea sort;
    struct List cells;
    struct ListNode *cell, *node;
    RECT area;
    POINT pos;
    int height = self->screen_height;
    int top = (int)((float)height * 0.3f);
    struct UIWindow *anchor = ui_window_find(0x133);

    if (anchor)
        top = anchor->height;
    area.left = 0;
    area.top = top;
    area.right = self->screen_width;
    area.bottom = height - g_miniPartyCellHeight * 3;
    mini_party_sort_area_construct(&sort, &area);
    mini_party_sort_area_get_cells(&sort, &cells);
    for (cell = cells.head->next, node = self->plates.head->next;
         cell != cells.head && node != self->plates.head; cell = cell->next, node = node->next) {
        struct UIMiniPartyWnd *plate = NODE_PLATE(node);
        int value = *LIST_VALUE(cell, int);

        mini_party_sort_area_cell_pos(&sort, &pos, value);
        ui_window_move(*LIST_VALUE(node, struct UIWindow *), pos.x, pos.y);
        plate->cell = value;
        session_set_plate_cell(&g_session1, PLATE_MEMBER_ID(plate), value);
    }
    list_free(&cells, NULL);
    list_free(&sort.cells, NULL);
}

#define WM_PLATE_WIDTH 0xae
#define WM_PLATE_HEIGHT 0x34
#define WM_PLATE_MSG_MEMBER 0x17
#define WM_PLATE_MSG_MENU 0x31

static struct UIMiniPartyWnd *new_plate(struct UIWindowMgr *self, struct PartyMember *member, int cell)
{
    struct UIMiniPartyWnd *plate = (struct UIMiniPartyWnd *)mem_alloc(sizeof *plate);

    plate = plate ? ui_mini_party_wnd_construct(plate) : NULL;
    ui_window_create(&plate->frame.base, WM_PLATE_WIDTH, WM_PLATE_HEIGHT);
    plate->cell = cell;
    ui_window_mgr_add_plate(self, plate);
    ui_mini_party_wnd_send_msg(&plate->frame.base, NULL, WM_PLATE_MSG_MEMBER, (intptr_t)member, 0, 0, 0);
    return plate;
}

void ui_window_mgr_create_party_plates(struct UIWindowMgr *self)
{
    struct PartyMember member;
    struct PlateRecord record;
    int i, count = ((int)g_session1.plateRecords.size);

    for (i = 0; i < count; i++) {
        party_member_at(&member, i);
        if (member.found) {
            session_get_plate_record(&g_session1, &record, member.aid);
            new_plate(self, &member, record.cell);
        }
        party_member_destroy(&member);
    }
}

void ui_window_mgr_restore_plates(struct UIWindowMgr *self)
{
    struct PartyMember member;
    struct PlateRecord *records;
    int i, kept = 0, count;

    ui_window_mgr_close_all_plates(self);
    count = session_get_party_member_count(&g_session1);
    records = count > 0 ? (struct PlateRecord *)mem_alloc(count * sizeof *records) : NULL;
    for (i = 0; i < count; i++) {
        party_member_at(&member, i);
        if (member.found) {
            session_get_plate_record(&g_session1, &records[kept], member.aid);
            if (records[kept].found)
                kept++;
        }
        party_member_destroy(&member);
    }
    session_clear_plate_records(&g_session1);

    for (i = 0; i < kept; i++) {
        struct UIMiniPartyWnd *plate;
        struct UIWindow *menu;

        session_get_party_member(&g_session1, &member, records[i].id);
        if (member.found) {
            plate = new_plate(self, &member, records[i].cell);
            session_add_plate_record(&g_session1, records[i].id, records[i].cell);
            menu = self->menu_wnd;
            if (menu && ui_menu_owner_id(menu) == MENU_OWNER_BY_KEY && ui_menu_owner_key(menu) == records[i].cell) {
                int x = menu->x, y = menu->y;

                ui_mini_party_wnd_send_msg(&plate->frame.base, NULL, WM_PLATE_MSG_MENU, 0, 0, 0, 0);
                ui_window_move(self->menu_wnd, x, y);
            }
        }
        party_member_destroy(&member);
    }

    if (ui_window_mgr_is_dragging_member_plate(self))
        ui_window_mgr_raise_window(self, self->drag_plate);
    if (self->menu_wnd)
        ui_window_mgr_raise_window(self, self->menu_wnd);
    if (records)
        mem_free(records);
}

int ui_window_mgr_is_dragging_member_plate(struct UIWindowMgr *self)
{
    struct UIMiniPartyWnd *plate = (struct UIMiniPartyWnd *)self->drag_plate;

    return plate && plate->member.found && plate->frame.base.state == 0;
}

void ui_window_mgr_move_dragged_plate(struct UIWindowMgr *self, int x, int y)
{
    struct UIDragMiniPartyWnd *plate = (struct UIDragMiniPartyWnd *)self->drag_plate;

    if (plate && plate->member.found)
        ui_frame_wnd_move(&plate->frame.base, x - plate->grab_x, y - plate->grab_y);
}

void ui_window_mgr_set_screen_size(struct UIWindowMgr *self, int width, int height)
{
    self->screen_width = width;
    self->screen_height = height;
    self->chat_wnd_x = 0;
    self->chat_wnd_y = height - self->chat_wnd_height;
    mini_party_area_mgr_rebuild(area_mgr());
}

void ui_window_mgr_command_plate_of(struct UIWindowMgr *self, int aid, int a, int b)
{
    struct ListNode *node;

    for (node = self->plates.head->next; node != self->plates.head; node = node->next) {
        if (NODE_PLATE(node)->member.aid == aid) {
            ui_window_send_msg(*LIST_VALUE(node, struct UIWindow *), NULL, UI_MSG_COMMAND, 0x22, aid, a, b);
            return;
        }
    }
}
