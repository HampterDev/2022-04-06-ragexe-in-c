#include "ui/chat/readable_links.h"
#include "ui/chat/navi_text.h"
#include "platform/code_page.h"
#include "core/text_format.h"
#include "core/text_markup.h"
#include "render/rp_pool.h"
#include "ui/window_manager_state.h"
#include "contents/mouse.h"
#include "core/object_type_tables.h"
#include "ui/window_access.h"
#include "contents/connection_api.h"
#include "ui/quests/quests.h"
#include "core/msg_str.h"
#include "core/text_util.h"
#include "diag/perf.h"

#define MENU_ENTRIES(v) ((struct UIMenuEntry *)(v)->first)
#define MENU_ENTRY_COUNT(v) ((unsigned)VEC_COUNT(v, struct UIMenuEntry))

struct UIQuestDisplay;
void ui_quest_display_destroy(struct UIWindow *window);
BYTE ui_quest_display_is_click_through(struct UIWindow *self);
void ui_quest_display_store_info(struct UIWindow *self);
void ui_quest_display_on_draw(struct UIWindow *self);
void ui_quest_display_on_r_btn_up(struct UIWindow *self, int x, int y);
intptr_t ui_quest_display_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4);
const struct UIWindowOps ui_quest_display_ops = {
    .destroy = ui_quest_display_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 0,
    .refreshes_children = 0,
    .move = ui_window_move_base,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_quest_display_is_click_through,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_quest_display_store_info,
    .save_pos = ui_window_save_pos_base,
    .move_delta = ui_window_move_delta_base,
    .set_show = ui_window_set_show_base,
    .on_create = ui_window_on_create_base,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_quest_display_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_window_on_l_btn_down_base,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_window_on_mouse_move_base,
    .on_mouse_hover = ui_window_on_mouse_hover_base,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_window_on_l_btn_up_base,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_quest_display_on_r_btn_up,
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_window_snap_base,
    .send_msg = ui_quest_display_send_msg,
    .invalidate = ui_window_invalidate_base,
    .render = ui_window_render_base,
    .has_custom_backdrop = 0,
    .draw_backdrop = ui_window_draw_backdrop_base,
    .type = UI_TYPE_WINDOW,
    .on_hover_enter = ui_window_on_hover_enter_base,
    .on_hover_leave = ui_window_on_hover_leave_base,
    .make_quad = ui_window_make_quad_base,
    .hit_test = ui_window_hit_test_base,
    .render_overlay = ui_window_render_overlay_base,
};

#define QUEST_DISPLAY_WND_ID 0xcc
#define QUEST_DISPLAY_MAX 5
#define QUEST_DISPLAY_TITLE_HEIGHT 10
#define QUEST_DISPLAY_SUMMARY_MAX 0x1e
#define QUEST_DISPLAY_TITLE_COLOUR 0xd6e1
#define QUEST_DISPLAY_MSG_SETTING 0x22
#define QUEST_DISPLAY_MSG_CLICKED 0x31
#define QUEST_MENU_TOGGLE 0x30
#define QUEST_MENU_DETAILS 0x31
#define MSG_QUEST_MENU_TOGGLE 0x15f
#define MSG_QUEST_MENU_DETAILS 0x656
#define MSG_QUEST_HUNT_DONE 0x7ee
#define WND_ID_MENU 0x12
#define MENU_MSG_FIT 0x1e
#define MENU_MSG_ADD_ITEM 0x1f
#define MENU_MSG_CHOICE 0x30
#define MENU_MSG_SET_OWNER 0x53
#define PACKET_QUEST_ACTIVE 0x2b6

struct UIQuestDisplay {
    struct UIWindow base;
    int selected;
    int hover;
    struct Vec menu;
    int *setting;
};

#pragma pack(push, 1)
struct PacketQuestActive {
    WORD id;
    int quest_id;
    BYTE active;
};
#pragma pack(pop)

struct UIQuestDisplay *ui_quest_display_construct(struct UIQuestDisplay *self)
{
    ui_window_construct(&self->base, 0);
    self->base.ops = &ui_quest_display_ops;
    self->menu.first = self->menu.last = self->menu.end = NULL;
    return self;
}

struct UIWindow *ui_quest_display_new(void)
{
    return (struct UIWindow *)ui_quest_display_construct(mem_alloc(sizeof(struct UIQuestDisplay)));
}

void ui_quest_display_create(struct UIQuestDisplay *self, int width, int height)
{
    ui_window_create(&self->base, width, height);
}

void ui_quest_display_destroy(struct UIWindow *window)
{
    struct UIQuestDisplay *self = (struct UIQuestDisplay *)window;
    self->base.ops = &ui_quest_display_ops;
    ui_menu_entry_vec_free(&self->menu);
    ui_window_destruct(&self->base);
    mem_free(self);
}

BYTE ui_quest_display_is_click_through(struct UIWindow *self)
{
    return 1;
}

void ui_quest_display_store_info(struct UIWindow *window)
{
    struct UIQuestDisplay *self = (struct UIQuestDisplay *)window;
    if (self->setting)
        *self->setting = self->base.visible;
}

static void filter(struct String *s, const char *open, const char *close)
{
    if (s->size)
        s->size = text_unwrap_tag(s->data, s->size, open, close);
}

static void clean_text(struct String *s)
{
    struct String out = {0};

    filter(s, "<URL>", "</URL>");
    filter(s, "<NAVI>", "</NAVI>");
    filter(s, "<TIPBOX>", "</TIPBOX>");
    filter(s, "<ITEM>", "</ITEM>");
    if (s->size)
        s->size = text_strip_info(s->data, s->size);
    if (g_windowMgr.new_chat_wnd) {
        ui_new_chat_wnd_readable_links(g_windowMgr.new_chat_wnd, &out, string_view(s));
        string_assign(s, string_cstr(&out), out.size);
        string_free(&out);
    }
    {
        struct String readable = {0};

        navi_links_readable_links(&readable, string_view(s));
        string_assign(s, string_cstr(&readable), readable.size);
        string_free(&readable);
    }
    filter(s, "<QUEST>", "</QUEST>");
}

static void strip_colours(struct String *s)
{
    struct OwnedText out = {0};

    if (text_copy_without_colours(&out, text_view(string_cstr(s), s->size),
                                  g_codePage, g_useCharsetTag))
        string_assign(s, owned_text_cstr(&out), out.size);
    owned_text_clear(&out);
}

static void hunt_label(struct String *label, const struct RenewQuestB *info, int i)
{
    struct String text;
    short min = info->plain.hunt_min_level[i], max = info->plain.hunt_max_level[i];
    const char *name = info->plain.hunt_name[i];

    if (info->plain.hunt_mob[i] == QUEST_HUNT_ANY_MOB) {
        renew_quest_condition_text(g_questTable, &text, info->plain.hunt_flags[i], min, max);
        string_assign(label, string_cstr(&text), text.size);
    } else if (min < 0 || max < 0 || (min == 0 && max == 0)) {
        string_init(&text);
        string_format(&text, "%s", name);
        string_assign(label, string_cstr(&text), text.size);
    } else if (min == 0) {
        string_init(&text);
        string_format(&text, "%s Lv.%d▼", name, max);
        string_assign(label, string_cstr(&text), text.size);
    } else if (max == 0) {
        string_init(&text);
        string_format(&text, "%s Lv.%d▲", name, min);
        string_assign(label, string_cstr(&text), text.size);
    } else {
        string_init(&text);
        string_format(&text, "%s Lv.%d~Lv.%d", name, min, max);
        string_assign(label, string_cstr(&text), text.size);
    }
    string_free(&text);
}

void ui_quest_display_on_draw(struct UIWindow *window)
{
    struct UIQuestDisplay *self = (struct UIQuestDisplay *)window;
    struct UIWindow *wnd = &self->base;
    struct Tree *quests = &g_questTable->quests_b;
    struct TreeNode *node;
    int plain = 0, double_rows = 0, single_rows = 0;

    ui_window_clear_canvas(wnd, 0xff00ff);
    for (node = tree_first(quests); node != quests->head; node = tree_next(node)) {
        struct OngoingQuestEntry *entry = TREE_VALUE(node, struct OngoingQuestEntry);
        struct RenewQuestB *info = &entry->quest;
        int quest_id = (int)entry->id;
        int y = plain * 0x28 + double_rows * 0x50 + single_rows * 0x32;
        struct String title, text, out;
        int i;

        if (info->base.active != 1 || info->shown != 1)
            continue;

        string_init(&out);
        if (!info->base.text[RENEW_QUEST_TEXT_TITLE].size)
            string_format(&out, "Unknown Quest, ID = %d", quest_id);
        else
            string_copy(&out, &info->base.text[RENEW_QUEST_TEXT_TITLE]);
        title = out;
        if (quest_id == self->selected)
            ui_window_fill_rect(wnd, -0xc, y - 2, wnd->width - 10, 0x28, 0);
        ui_window_draw_shadow_text(wnd, 0, y, string_cstr(&title), 0, QUEST_DISPLAY_TITLE_COLOUR, 0, 0, 0xc, 0);

        if (info->description.first == info->description.last) {
            string_init(&text);
        } else {
            string_copy(&text, &info->base.text[RENEW_QUEST_TEXT_SUMMARY]);
        }
        strip_colours(&text);
        clean_text(&text);
        if (strlen(string_cstr(&text)) > QUEST_DISPLAY_SUMMARY_MAX) {
            string_truncate(&text, text_prefix_length(string_cstr(&text), strlen(string_cstr(&text)),
                                           QUEST_DISPLAY_SUMMARY_MAX, g_codePage));
            string_append_cstr(&text, "...");
        }
        if (strlen(string_cstr(&text)))
            ui_window_draw_shadow_text(wnd, 0, y + 0xf, string_cstr(&text), 0, 0xffffff, 0, 0, 0xc, 0);

        if (info->plain.hunt_count == 0 || info->plain.hunt_count > QUEST_HUNT_MAX) {
            plain++;
        } else {
            for (i = 0; i < info->plain.hunt_count; i++) {
                struct String label;

                string_init(&label);
                hunt_label(&label, info, i);
                strip_colours(&label);
                clean_text(&text);
                if (info->plain.hunt_goal[i] > 0) {
                    string_init(&out);
                    if (info->plain.hunt_done[i] != info->plain.hunt_goal[i])
                        string_format(&out, "%s ( %d / %d )", string_cstr(&label), info->plain.hunt_done[i],
                                      info->plain.hunt_goal[i]);
                    else
                        string_format(&out, "%s ( %s )", string_cstr(&label), msg_str(MSG_QUEST_HUNT_DONE));
                    string_assign(&label, string_cstr(&out), out.size);
                    string_free(&out);
                    ui_window_text_out(wnd, 0xb, y + i * 0xd + 0x1d, string_cstr(&label), 0, 0, 0xc, 0, 0, 0);
                    ui_window_text_out(wnd, 0xa, y + i * 0xd + 0x1c, string_cstr(&label), 0, 0, 0xc, 0xffffff,
                                     0, 0);
                }
                string_free(&label);
            }
            if (info->plain.hunt_count > 1)
                double_rows++;
            else
                single_rows++;
        }
        string_free(&text);
        string_free(&title);
        if (plain + double_rows + single_rows >= QUEST_DISPLAY_MAX)
            return;
    }
}

void ui_quest_display_on_r_btn_up(struct UIWindow *window, int x, int y)
{
    struct UIQuestDisplay *self = (struct UIQuestDisplay *)window;
    struct Tree *quests = &g_questTable->quests_b;
    struct TreeNode *node;
    int old = self->hover, index = 0, plain = 0, double_rows = 0, single_rows = 0;
    int hit = -1;

    for (node = tree_first(quests); node != quests->head; node = tree_next(node), index++) {
        struct RenewQuestB *info = &TREE_VALUE(node, struct OngoingQuestEntry)->quest;
        int top = (double_rows + plain) * 0x28 + single_rows;

        if (!info->base.active || !info->shown)
            continue;
        if (y >= top && y <= top + QUEST_DISPLAY_TITLE_HEIGHT) {
            hit = index;
            break;
        }
        if (!info->plain.hunt_count)
            plain++;
        else if (info->plain.hunt_count > 1)
            double_rows += 2;
        else
            single_rows += 0x32;
    }
    self->hover = hit;
    if (old != hit)
        ui_window_invalidate_base(&self->base);
    if (self->hover != -1)
        ui_quest_display_send_msg(&self->base, NULL, QUEST_DISPLAY_MSG_CLICKED, 0, 0, 0, 0);
}

static void add_entry(struct UIQuestDisplay *self, int msg, int id)
{
    struct String text;
    struct UIMenuEntry entry;
    const char *s = msg_str(msg);

    string_init(&text);
    string_assign_cstr(&text, s);
    ui_menu_entry_construct(&entry, string_cstr(&text), id);
    ui_menu_entry_vec_push_move(&self->menu, &entry);
    string_free(&entry.text);
    string_free(&text);
}

static void open_menu(struct UIQuestDisplay *self)
{
    struct UIMenuEntry *entry;
    struct UIWindow *menu;
    unsigned i;
    int x, y;

    for (entry = MENU_ENTRIES(&self->menu); entry != (struct UIMenuEntry *)self->menu.last; entry++)
        string_free(&entry->text);
    self->menu.last = self->menu.first;
    if (g_windowMgr.menu_wnd)
        ui_window_set_show_base(&self->base, 0);
    if (self->hover != -1) {
        add_entry(self, MSG_QUEST_MENU_DETAILS, QUEST_MENU_DETAILS);
        add_entry(self, MSG_QUEST_MENU_TOGGLE, QUEST_MENU_TOGGLE);
    }
    if (!MENU_ENTRY_COUNT(&self->menu))
        return;

    menu = ui_window_open(WND_ID_MENU);
    for (i = 0; i < MENU_ENTRY_COUNT(&self->menu); i++)
        ui_window_send_msg(menu, NULL, MENU_MSG_ADD_ITEM, (intptr_t)string_cstr(&MENU_ENTRIES(&self->menu)[i].text), 0,
                 0, 0);
    ui_window_send_msg(menu, NULL, MENU_MSG_SET_OWNER, QUEST_DISPLAY_WND_ID, 0, 0, 0);
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
}

static void menu_choice(struct UIQuestDisplay *self, int item)
{
    struct Tree *quests = &g_questTable->quests_b;
    struct TreeNode *node;
    int index = 0;

    for (node = tree_first(quests); node != quests->head; node = tree_next(node), index++) {
        struct OngoingQuestEntry *entry = TREE_VALUE(node, struct OngoingQuestEntry);
        int quest_id;

        if (index != self->hover)
            continue;
        quest_id = (int)entry->id;
        switch (MENU_ENTRIES(&self->menu)[item].id) {
        case QUEST_MENU_TOGGLE: {
            struct PacketQuestActive packet;
            void *conn;
            int len;

            packet.id = PACKET_QUEST_ACTIVE;
            packet.quest_id = quest_id;
            packet.active = entry->quest.base.active == 0;
            len = rag_connection_get_packet_length(rag_connection_instance(), PACKET_QUEST_ACTIVE);
            conn = rag_connection_instance();
            rag_connection_send_packet(conn, len, &packet);
            ui_window_invalidate_base(&self->base);
            break;
        }
        case QUEST_MENU_DETAILS: {
            struct UIWindow *detail = ui_window_open(QUEST_DETAIL_WND_ID);

            if (detail)
                ui_renew_detail_quest_info_wnd_show_quest((struct UIRenewDetailQuestInfoWnd *)detail, quest_id);
            break;
        }
        }
        return;
    }
}

intptr_t ui_quest_display_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2,
                                      intptr_t val3, intptr_t val4)
{
    struct UIQuestDisplay *self = (struct UIQuestDisplay *)window;
    switch (msg) {
    case QUEST_DISPLAY_MSG_CLICKED:
        open_menu(self);
        return 0;
    case MENU_MSG_CHOICE:
        menu_choice(self, val1);
        return 0;
    case QUEST_DISPLAY_MSG_SETTING:
        self->setting = (int *)val1;
        ui_window_set_show_base(&self->base, *self->setting);
        return 0;
    }
    return ui_window_send_msg_base(&self->base, sender, msg, val1, val2, val3, val4);
}
