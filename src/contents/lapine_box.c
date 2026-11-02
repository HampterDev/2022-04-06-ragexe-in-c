#include "game/drag_info.h"
#include "world/tooltip.h"
#include "contents/client_lua_calls.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "core/memory.h"
#include "world/mode_manager.h"
#include "game/context.h"
#include "ui/window_access.h"
#include "contents/connection_api.h"
#include <stdlib.h>
#include <string.h>
#include "contents/client_res.h"
#include "contents/connection.h"
#include "contents/game_session.h"
#include "iteminfo/item_api.h"
#include "std/vec.h"
#include "contents/lapine_box.h"
#include "std/std.h"
#include "ui/controls/buttons.h"
#include "ui/controls/combo_box.h"
#include "ui/controls/scroll.h"
#include "ui/controls/line_info.h"
#include "ui/quests/tag_text_viewer.h"
#include "base.h"
#include "ui/window_types.h"
#include "ui/controls/edit_types.h"
#include "contents/ui_window.h"
#include "contents/ui_window_mgr.h"
#include "core/object_type_tables.h"
#include "core/msg_str.h"
#include "core/text_util.h"
#include "session/session_items.h"
#include "ui/ui_items.h"
#include "platform/clock.h"

#define LAPINE_TABLE     "tblLapineDdukddakBox"

#define MSG_NEEDS_LABEL         0xb50
#define MSG_NAME_WITH_OPTIONS   0xa28

#define SLOT_SIZE               24

#pragma pack(push, 1)
struct LapineMakeHeader {
    unsigned short id;
    short len;
    int box_item;
};
struct LapineMakeEntry {
    unsigned short index;
    unsigned short count;
};
#pragma pack(pop)
SIZE_ASSERT(LapineMakeHeader, 8);
SIZE_ASSERT(LapineMakeEntry, 4);

struct LapinePoint { int x, y; };

static size_t chosen_count(const struct LapineBox *self)
{
    return VEC_COUNT(&self->chosen, struct ItemInfo);
}

static struct LapinePoint slot_pos(const struct LapineBox *self, int i)
{
    struct LapinePoint pos = { 0, 0 };
    size_t n = chosen_count(self);

    if (i < 0 || n < (size_t)i)
        return pos;
    pos.x = n > 5 ? 0x19 : 0x21;
    pos.y = n > 5 ? 0x98 : 0xa2;
    if (i >= 5) {
        pos.x = 0x2a;
        pos.y = 0xb7;
    }
    pos.x += i % 5 * 34;
    return pos;
}

static struct ItemInfo *chosen_at(const struct LapineBox *self, struct ItemInfo *out, int x, int y)
{
    const struct ItemInfo *chosen = (const struct ItemInfo *)self->chosen.first;
    size_t n = chosen_count(self), i;

    for (i = 0; i < n; i++) {
        struct LapinePoint pos = slot_pos(self, (int)i);

        if (x >= pos.x && x <= pos.x + SLOT_SIZE && y >= pos.y && y <= pos.y + SLOT_SIZE)
            return item_info_copy(out, &chosen[i]);
    }
    return item_info_construct_with_arg(out);
}

static void send_close_packet(void)
{
    unsigned short id = PACKET_LAPINE_CLOSE;
    int len = rag_connection_get_packet_length(rag_connection_instance(), PACKET_LAPINE_CLOSE);

    rag_connection_send_packet(rag_connection_instance(), len, &id);
}

static void send_make_packet(struct LapineBox *self)
{
    const struct ItemInfo *chosen;
    struct LapineMakeHeader *head = (struct LapineMakeHeader *)mem_try_alloc(sizeof(struct LapineMakeHeader));
    unsigned char *buf;
    size_t i;

    head->id = PACKET_LAPINE_MAKE;
    head->len = (short)(chosen_count(self) * sizeof(struct LapineMakeEntry) + sizeof *head);
    head->box_item = self->box_item;
    buf = (unsigned char *)mem_try_alloc(head->len);
    mem_copy_checked(buf, head->len, head, sizeof *head);
    for (i = 0; i < chosen_count(self); i++) {
        struct LapineMakeEntry entry;

        chosen = (const struct ItemInfo *)self->chosen.first;
        entry.index = (unsigned short)chosen[i].index;
        entry.count = (unsigned short)chosen[i].count;
        mem_copy_checked(buf + sizeof *head + i * sizeof entry, head->len, &entry, sizeof entry);
    }
    rag_connection_send_packet(rag_connection_instance(), head->len, buf);
    self->waiting = 1;
    ((struct UIButton *)self->make_button)->disabled = 1;
    ui_window_invalidate(self->make_button);
    ui_window_invalidate_base(&self->frame.base);
    mem_free(head);
    if (buf)
        mem_free(buf);
}

static void remove_chosen(struct LapineBox *self, int index)
{
    struct ItemInfo *it = (struct ItemInfo *)self->chosen.first;
    struct ItemInfo *last = (struct ItemInfo *)self->chosen.last;

    while (it != last && it->index != index)
        it++;
    if (it == last)
        return;
    for (it++; it != last; it++)
        item_info_assign(it - 1, it);
    last = (struct ItemInfo *)self->chosen.last - 1;
    item_info_destruct(last);
    self->chosen.last = last;
    if ((unsigned)self->need_count > chosen_count(self)) {
        ((struct UIButton *)self->move_button)->disabled = 0;
        ui_window_invalidate(self->move_button);
    }
    ui_window_invalidate_base(&self->frame.base);
}

static void move_selected(struct LapineBox *self)
{
    struct LapineIngredientList *list = self->list;
    struct LapineIngredient **it = (struct LapineIngredient **)list->rows.first;
    struct LapineIngredient **last = (struct LapineIngredient **)list->rows.last;
    struct LapineIngredient *row;

    while (it != last && (*it)->selected != 1)
        it++;
    if (it == last)
        return;
    row = *it;
    if (row && row->unusable != 1
        && ui_window_send(&list->scroll.base, &self->frame.base, LAPINE_MSG_REMOVE, row->item.index, 0, 0, 0) >= 0)
        lapine_box_add(self, row->item.index, row->count);
}

static void new_button(struct UIWindow *wnd, struct UIWindow **slot, const char *const bitmaps[4],
                       int x, int y, unsigned id)
{
    struct UIWindow *button = (struct UIWindow *)mem_alloc(sizeof(struct UIBitmapButton));
    int state;

    if (button)
        ui_bitmap_button_construct((struct UIBitmapButton *)button);
    *slot = button;
    for (state = 0; state < 4; state++)
        ui_bitmap_button_set_bitmap((struct UIBitmapButton *)*slot, bitmaps[state], state, 0);
    ui_window_create(*slot, ((struct UIBitmapButton *)*slot)->bitmap_width,
                     ((struct UIBitmapButton *)*slot)->bitmap_height);
    ui_window_move(*slot, x, y);
    (*slot)->id = id;
    ui_window_add_child(wnd, *slot);
}

void lapine_add_buttons(struct UIWindow *wnd, struct UIWindow **cancel, struct UIWindow **make,
                        struct UIWindow **move, unsigned make_id)
{
    static const char *const cancel_bitmaps[4] = {
        LAPINE_BMP("btn_cancel_out.bmp"), LAPINE_BMP("btn_cancel_over.bmp"),
        LAPINE_BMP("btn_cancel_press.bmp"), LAPINE_BMP("btn_cancel_dis.bmp"),
    };
    static const char *const make_bitmaps[4] = {
        LAPINE_BMP("btn_make_out.bmp"), LAPINE_BMP("btn_make_over.bmp"),
        LAPINE_BMP("btn_make_press.bmp"), LAPINE_BMP("btn_make_dis.bmp"),
    };
    static const char *const move_bitmaps[4] = {
        LAPINE_BMP("btn_moveitem_out.bmp"), LAPINE_BMP("btn_moveitem_over.bmp"),
        LAPINE_BMP("btn_moveitem_press.bmp"), LAPINE_BMP("btn_moveitem_dis.bmp"),
    };

    new_button(wnd, cancel, cancel_bitmaps, 0x118, 0xe2, LAPINE_CANCEL_BUTTON_ID);
    new_button(wnd, make, make_bitmaps, 0x154, 0xe2, make_id);
    new_button(wnd, move, move_bitmaps, 0xd5, 0x6e, LAPINE_MOVE_BUTTON_ID);
}

struct LapineBox *lapine_box_construct(struct LapineBox *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &lapine_box_ops;
    self->cancel_button = NULL;
    self->make_button = NULL;
    self->move_button = NULL;
    self->list = NULL;
    self->box_item = 0;
    string_init(&self->title);
    self->chosen.first = self->chosen.last = self->chosen.end = NULL;
    string_init(&self->need_source);
    string_init(&self->notice);
    self->notice_time = 0;
    self->notice_duration = 3000;
    self->waiting = 0;
    return self;
}

struct UIWindow *lapine_box_new(void)
{
    return (struct UIWindow *)lapine_box_construct(mem_alloc(sizeof(struct LapineBox)));
}

void lapine_box_destruct(struct LapineBox *self)
{
    self->frame.base.ops = &lapine_box_ops;
    item_info_vec_clear(&self->chosen);
    send_close_packet();
    string_free(&self->notice);
    string_free(&self->need_source);
    item_info_vec_free(&self->chosen);
    string_free(&self->title);
    ui_frame_wnd_destruct(&self->frame);
}

void lapine_box_delete(struct UIWindow *window)
{
    struct LapineBox *self = (struct LapineBox *)window;
    lapine_box_destruct(self);
    mem_free(self);
}

void lapine_box_set_item(struct LapineBox *self, int box_item)
{
    struct ItemInfo item;
    const char *name;
    int min_refine, max_refine;
    const char *need_source;

    self->box_item = box_item;
    item_info_construct(&item);
    item_info_set_item_id(&item, box_item);

    name = item_info_get_name(&item);
    string_assign_cstr(&self->title, name);
    item_info_destruct(&item);

    client_lua_call_table(g_session1.luaState, LAPINE_TABLE, "set_resource_item_list", "d", box_item);

    min_refine = 0;

    client_lua_call_table(g_session1.luaState, LAPINE_TABLE, "get_min_need_refine", "d>d", box_item, &min_refine);
    self->list->min_refine = min_refine;

    max_refine = 0;

    client_lua_call_table(g_session1.luaState, LAPINE_TABLE, "get_max_need_refine", "d>d", box_item, &max_refine);
    self->list->max_refine = max_refine;
    ui_window_send(&self->list->scroll.base, &self->frame.base, LAPINE_MSG_FILL, 0, 0, 0, 0);

    client_lua_call_table(g_session1.luaState, LAPINE_TABLE, "get_need_item_count", "d>d", box_item, &self->need_count);

    need_source = NULL;

    client_lua_call_table(g_session1.luaState, LAPINE_TABLE, "get_need_source_string", "d>s", box_item, &need_source);
    string_assign_cstr(&self->need_source, need_source);
}

void lapine_box_add_ingredient(struct LapineBox *self, int item_id, int count)
{
    struct Vec *resources = &self->list->resources;
    struct ItemInfo *it, item;

    for (it = (struct ItemInfo *)resources->first; it != (struct ItemInfo *)resources->last; it++)
        if (item_id == atoi(string_cstr(&it->id_text)))
            break;
    if (it != (struct ItemInfo *)resources->last)
        return;
    item_info_construct(&item);
    item_info_set_item_id(&item, item_id);
    item.count = count;
    vec_push(resources, &item, sizeof item);
}

void lapine_box_set_waiting(struct LapineBox *self, char on)
{
    self->waiting = on;
    ((struct UIButton *)self->make_button)->disabled = on;
    ui_window_invalidate(self->make_button);
    ui_window_invalidate_base(&self->frame.base);
}

char lapine_box_add(struct LapineBox *self, int index, int count)
{
    const struct ItemInfo *it, *last;
    struct ItemInfo item;
    struct ItemNameRuns runs = {0};

    if (chosen_count(self) >= LAPINE_MAX_CHOSEN)
        return 0;
    it = (const struct ItemInfo *)self->chosen.first;
    last = (const struct ItemInfo *)self->chosen.last;
    while (it != last && it->index != index)
        it++;
    if (it != (const struct ItemInfo *)self->chosen.last)
        return 0;

    session_get_inventory_item(&g_session1, &item, index);
    item.count = count;

    item_info_name_runs(&self->frame.base, &item, &runs, 1, 0);
    if (item.identified == 1 && item.option_count)
        string_format(&runs.text, msg_str(MSG_NAME_WITH_OPTIONS), string_cstr(&runs.text), item.option_count);
    item_info_set_name(&item, string_cstr(&runs.text));
    string_free(&runs.text);
    vec_push(&self->chosen, &item, sizeof item);
    if ((size_t)self->need_count == chosen_count(self)) {
        ((struct UIButton *)self->move_button)->disabled = 1;
        ui_window_invalidate(self->move_button);
    }
    ui_window_invalidate_base(&self->frame.base);
    return 1;
}

void lapine_box_drop(struct UIWindow *window, int x, int y, const struct DragInfo *drop)
{
    struct LapineBox *self = (struct LapineBox *)window;

    if (chosen_count(self) < (unsigned)self->need_count && drop->source == LAPINE_DRAG_CANDIDATE
        && ui_window_send(&self->list->scroll.base, &self->frame.base, LAPINE_MSG_REMOVE, drop->index, 0, 0, 0) >= 0)
        lapine_box_add(self, drop->index, drop->drop_count);
    game_send_message(MODE_MSG_DROPPED, 0, 0, 0, 0);
}

void lapine_box_on_create(struct UIWindow *window, int w, int h)
{
    struct LapineBox *self = (struct LapineBox *)window;
    ui_window_add_sys_button(&self->frame.base);
    ui_window_add_close_button(&self->frame.base, "", 0);
    self->list = lapine_list_new();
    ui_window_move_base(&self->list->scroll.base, 0xf1, 0x11);
    ui_window_add_child(&self->frame.base, &self->list->scroll.base);
    lapine_add_buttons(&self->frame.base, &self->cancel_button, &self->make_button, &self->move_button,
                       LAPINE_MAKE_BUTTON_ID);
}

void lapine_box_on_frame(struct UIWindow *window)
{
    struct LapineBox *self = (struct LapineBox *)window;
    if (!self->waiting) {
        struct UIWindow *make = self->make_button;
        char disabled = ((struct UIButton *)make)->disabled;
        int changed = 0;

        if ((size_t)self->need_count == chosen_count(self)) {
            if (disabled == 1) {
                ((struct UIButton *)make)->disabled = 0;
                changed = 1;
            }
        } else if (disabled == 0) {
            ((struct UIButton *)make)->disabled = 1;
            changed = 1;
        }
        if (changed) {
            ui_window_invalidate(make);
            ui_window_invalidate_base(&self->frame.base);
        }
    }
    if (self->notice_time + self->notice_duration < platform_ticks_ms())
        string_clear(&self->notice);
}

static void draw_number(struct LapineBox *self, int x, int value)
{
    struct String text;

    string_init(&text);
    string_format(&text, "%d", value);
    ui_window_text_out_centred(&self->frame.base, x, 0x4c, string_cstr(&text), 0xe, 0, 0xb, 0xffffff, 0);
    string_free(&text);
}

void lapine_box_on_draw(struct UIWindow *window)
{
    struct LapineBox *self = (struct LapineBox *)window;
    const struct ItemInfo *chosen;
    size_t i;

    ui_window_draw_bitmap(&self->frame.base, 0, 0, res_mgr_get(g_resMgr(), LAPINE_BMP("bg_lapine.bmp")), 1);
    if (self->title.size > 0)
        ui_window_draw_title_bar(&self->frame.base, 0, string_cstr(&self->title), 0);
    ui_window_text_out_centred(&self->frame.base, 0x21, 0x2f, msg_str(MSG_NEEDS_LABEL), 0xa0, 0, 0xb, 0xcd7a51, 0);
    ui_window_text_out_centred(&self->frame.base, 0x21, 0x3f, string_cstr(&self->need_source), 0xa0, 0, 0xb, 0x844a42, 0);
    draw_number(self, 0x62, (int)chosen_count(self));
    draw_number(self, 0x73, self->need_count);

    for (i = 0; i < chosen_count(self); i++) {
        struct LapinePoint pos = slot_pos(self, (int)i);
        void *shadow = res_mgr_get(g_resMgr(), LAPINE_BMP("shadow_item_off.bmp"));

        ui_window_draw_bitmap(&self->frame.base, pos.x - 4, pos.y + 0xb, shadow, 1);
        chosen = (const struct ItemInfo *)self->chosen.first;
        ui_window_draw_item_icon_count(&self->frame.base, pos.x, pos.y, (struct ItemInfo *)&chosen[i], 0, 0);
    }

    if (self->notice_time + self->notice_duration > platform_ticks_ms()) {
        ui_window_draw_bitmap(&self->frame.base, 6, 0xe5, res_mgr_get(g_resMgr(), LAPINE_BMP("icon_notify.bmp")), 1);
        ui_window_text_out(&self->frame.base, 0x19, 0xe6, string_cstr(&self->notice), 0, 0, 0xc, 0xff0000, 0, 0);
    }
}

void lapine_box_press(struct UIWindow *window, int x, int y)
{
    struct LapineBox *self = (struct LapineBox *)window;
    struct ItemInfo item;

    if (y < 0x11)
        ui_frame_wnd_on_l_btn_down((struct UIWindow *)(&self->frame), x, y);
    chosen_at(self, &item, x, y);
    if ((unsigned)item.index > 0)
        game_mode_drag_item(game_current_mode(), LAPINE_DRAG_CHOSEN, &item, item.count, -1);
    item_info_destruct(&item);
}

void lapine_box_double_click(struct UIWindow *window, int x, int y)
{
    struct LapineBox *self = (struct LapineBox *)window;
    struct ItemInfo item;

    chosen_at(self, &item, x, y);
    if (item.index && lapine_list_add(self->list, item.index) == 1)
        ui_window_send(&self->frame.base, &self->frame.base, LAPINE_MSG_REMOVE, item.index, 0, 0, 0);
    item_info_destruct(&item);
}

void lapine_box_hover(struct UIWindow *window, int x, int y)
{
    struct LapineBox *self = (struct LapineBox *)window;
    struct ItemInfo item;
    int index;

    ui_frame_wnd_on_mouse_move(&self->frame.base, x, y);
    chosen_at(self, &item, x, y);
    index = item.index;
    if (index) {
        const struct ItemInfo *chosen = (const struct ItemInfo *)self->chosen.first;
        size_t n = chosen_count(self), i;
        int bx = 0, by = -20;

        for (i = 0; i < n; i++) {
            if (index == chosen[i].index) {
                struct LapinePoint pos = slot_pos(self, (int)i);

                bx = pos.x;
                by = pos.y - 20;
                break;
            }
        }
        ui_window_to_screen(&self->frame.base, &bx, &by);
        mode_show_tooltip(client_current_mode(), string_cstr(&item.name), bx, by, -1, 0);
    }
    item_info_destruct(&item);
}

void lapine_box_show_info(struct UIWindow *window, int x, int y)
{
    struct LapineBox *self = (struct LapineBox *)window;
    struct ItemInfo item;

    chosen_at(self, &item, x, y);
    if ((unsigned)item.index > 0) {
        struct UIWindow *desc = ui_window_open(ITEM_DESC_WND_ID);

        ui_window_send(desc, NULL, UI_MSG_SHOW_ITEM, (intptr_t)&item, 0, 0, 0);
    }
    item_info_destruct(&item);
}

intptr_t lapine_box_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                   intptr_t a, intptr_t b, intptr_t c, intptr_t d)
{
    struct LapineBox *self = (struct LapineBox *)window;
    switch (msg) {
    case UI_MSG_BUTTON:
        if (a == LAPINE_CANCEL_BUTTON_ID)
            ui_window_mgr_post_quit1(&g_windowMgr, LAPINE_BOX_WND_ID);
        else if (a == LAPINE_MAKE_BUTTON_ID)
            send_make_packet(self);
        else if (a == LAPINE_MOVE_BUTTON_ID)
            move_selected(self);
        return 0;
    case LAPINE_MSG_REMOVE:
        remove_chosen(self, a);
        return 0;
    case LAPINE_MSG_NOTICE:
        self->notice_time = platform_ticks_ms();
        string_assign_cstr(&self->notice, (const char *)a);
        ui_window_invalidate_base(&self->frame.base);
        return 0;
    default:
        return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, a, b, c, d);
    }
}
