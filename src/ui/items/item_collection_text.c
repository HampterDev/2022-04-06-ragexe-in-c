#include "ui/controls/listbox.h"
#include "ui/window_access.h"
#include "contents/client_lua_calls.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "ui/items/item_collection_internal.h"
#include "core/msg_str.h"
#include "core/text_util.h"
#include "session/session_items.h"
#include "session/session_talk.h"
#include "std/tree.h"

static void destroy_string(void *value) { string_free(value); }

void option_lines_destroy(void *value)
{
    struct OptionLines *entry = value;

    list_free(&entry->lines, destroy_string);
}

static void push_line(struct List *lines, const struct String *line)
{
    struct String copy;

    string_copy(&copy, line);
    list_push_back(lines, &copy, sizeof copy);
}

static void option_lines_make(struct OptionLines *out, int index, const struct List *lines)
{
    struct ListNode *node;

    out->index = index;
    list_init(&out->lines);
    for (node = lines->head->next; node != lines->head; node = node->next)
        push_line(&out->lines, LIST_VALUE(node, struct String));
}

static BYTE collection_wrap_text(struct UIItemCollectionWnd *self, const struct String *text, int width,
                                 struct List *lines)
{
    struct UIWindow *wnd = COLLECTION_WND(self);
    const char *data = string_cstr(text);
    const char *end = data + text->size;
    const char *start = data, *cur = data;
    struct String piece;

    if (width >= ui_window_text_width(wnd, data, 0, 0, 0xc, 0, 0)) {
        push_line(lines, text);
        return 0;
    }
    while (cur != end) {
        if (*cur == '\n') {
            string_init(&piece);
            if (start != cur)
                string_assign(&piece, start, cur - start);
            list_push_back(lines, &piece, sizeof piece);
            start = cur + 1;
            cur++;
        } else if ((signed char)cur[0] < 0 && (signed char)cur[1] < 0) {
            int w;

            string_init(&piece);
            if (start != cur + 2)
                string_assign(&piece, start, cur + 2 - start);
            w = ui_window_text_width(wnd, string_cstr(&piece), 0, 0, 0xc, 0, 0);
            string_free(&piece);
            if (width < w) {
                string_init(&piece);
                if (start != cur)
                    string_assign(&piece, start, cur - start);
                list_push_back(lines, &piece, sizeof piece);
                start = cur;
            }
            cur += 2;
        } else {
            int w;

            string_init(&piece);
            if (start != cur + 1)
                string_assign(&piece, start, cur + 1 - start);
            w = ui_window_text_width(wnd, string_cstr(&piece), 0, 0, 0xc, 0, 0);
            string_free(&piece);
            if (width < w) {
                string_init(&piece);
                if (start != cur)
                    string_assign(&piece, start, cur - start);
                list_push_back(lines, &piece, sizeof piece);
                start = cur;
            }
            cur++;
        }
    }
    string_init(&piece);
    if (start != end)
        string_assign(&piece, start, end - start);
    if (piece.size != 0)
        push_line(lines, &piece);
    string_free(&piece);
    return 1;
}

static void collection_build_options(struct UIItemCollectionWnd *self)
{
    struct TreeNode *node;
    int i;

    for (i = 0; i < self->item.option_count; i++) {
        struct ItemOption *option = ITEM_OPTION(&self->item, i);
        struct List lines;
        struct OptionLines entry;
        int added;

        if (((short)option->id) == 0)
            continue;
        if (((short)option->id) == ITEM_OPTION_TITLE) {
            struct String title;

            string_init(&title);
            string_assign_cstr(&title, msg_str(MSG_OPTION_TITLE));
            list_init(&lines);
            push_line(&lines, &title);
            string_free(&title);
            option_lines_make(&entry, i, &lines);
            tree_add(&self->options, &entry, sizeof entry, tree_less_int, &added);
            if (!added)
                option_lines_destroy(&entry);
        } else {
            const char *format = NULL;
            struct String line;

            if (!client_lua_call(g_session1.luaState, "GetVarOptionName", "d>s", (int)((short)option->id), &format) || !format)
                continue;
            list_init(&lines);
            string_init(&line);
            string_format(&line, format, (int)option->value, (int)option->param);
            collection_wrap_text(self, &line, 0xfe, &lines);
            option_lines_make(&entry, i, &lines);
            tree_add(&self->options, &entry, sizeof entry, tree_less_int, &added);
            if (!added)
                option_lines_destroy(&entry);
            string_free(&line);
        }
        list_free(&lines, destroy_string);
    }
    for (node = tree_first(&self->options); node != self->options.head; node = tree_next(node)) {
        unsigned count = TREE_VALUE(node, struct OptionLines)->lines.size;

        self->options_height += (int)count * 11 + (int)count * 4 + 0xe;
    }
}

static void collection_add_line(struct UIItemCollectionWnd *self, const char *line, BYTE tag_test_is_one)
{
    char buf[0x104];

    if (tag_test_is_one ? g_useCharsetTag == 1 : g_useCharsetTag != 0) {
        if (tag_test_is_one)
            memset(buf, 0, sizeof buf);
        sprintf(buf, "%s%s", language_charset_tag(g_language, 1), line);
        ui_window_send_msg(self->text, NULL, UI_MSG_ADD_TEXT, (int)buf, TEXT_LAYOUT, 0, 0);
    } else {
        ui_window_send_msg(self->text, NULL, UI_MSG_ADD_TEXT, (int)line, TEXT_LAYOUT, 0, 0);
    }
}

static void collection_place_card_button(struct UIItemCollectionWnd *self, int hide_view)
{
    int y = card_affix_has(COLLECTION_ITEM_ID(self)) * 100 - 0x5e;

    if (hide_view)
        y = -100;
    ui_window_move_base(&self->view_btn->button.base, 6, y);
}

static void collection_add_expiry(struct UIItemCollectionWnd *self)
{
    char time_text[0x80], buf[0x104];
    int expire = self->item.expire_time;
    int layout = TEXT_LAYOUT_EXPIRE;
    int hours_per_day;

    strftime(time_text, sizeof time_text, msg_str(MSG_EXPIRE_TIME_FORMAT), _localtime32((__time32_t *)&expire));
    memset(buf, 0, sizeof buf);
    if (g_useCharsetTag) {
        _snprintf_s(buf, sizeof buf, (size_t)-1, "%s%s", language_charset_tag(g_language, 1), time_text);
        _snprintf_s(buf, sizeof buf, (size_t)-1, msg_str(MSG_EXPIRE_LINE), buf);
        hours_per_day = 24;
    } else {
        sprintf(buf, msg_str(MSG_EXPIRE_LINE), time_text);
        hours_per_day = 24;
    }
    if (self->item.expire_in_hours)
        _snprintf_s(buf, sizeof buf, (size_t)-1, msg_str(MSG_EXPIRE_DAYS_HOURS), expire / hours_per_day,
                       expire % hours_per_day);
    ui_window_send_msg(self->text, NULL, UI_MSG_ADD_TEXT, (int)buf, layout, 0, 0);
}

static void collection_fit_text(struct UIItemCollectionWnd *self)
{
    struct UIWindow *wnd = COLLECTION_WND(self);
    int lines = (int)VEC_COUNT(&((struct UIListBox *)self->text)->items, struct String);
    int move_id = item_move_info_get(COLLECTION_ITEM_ID(self))->item_id;
    int move_row = self->move_info->button.button.base.height + 3;

    if (lines > 4) {
        if (move_id == 0 || wnd->height == 0x78)
            item_collection_set_height(self, wnd->height + (lines - 4) * 18);
        else
            item_collection_set_height(self, (lines - 4) * 18 - move_row + wnd->height);
    } else if (move_id != 0) {
        if (wnd->height == 0x78) {
            ui_window_resize_base(wnd, wnd->width, move_row + 0x78);
            self->move_info->item_id = COLLECTION_ITEM_ID(self);
            ui_window_move_base(&self->resizer->base, self->resizer->base.x, self->resizer->base.y + move_row);
        } else {
            item_collection_set_height(self, wnd->height + (lines - 4) * 18);
        }
    }
}

static int collection_is_headgear(int location)
{
    switch (location) {
    case 0x1: case 0x4: case 0x100: case 0x101: case 0x200: case 0x201: case 0x300: case 0x301: case 0x400:
    case 0x800: case 0xc00: case 0x1000: case 0x1400: case 0x1800: case 0x1c00: case 0x2000:
        return 1;
    }
    return 0;
}

int item_collection_set_item(struct UIItemCollectionWnd *self, struct ItemInfo *info, int toggle, int no_compare)
{
    struct UIWindow *wnd = COLLECTION_WND(self);
    const struct ItemTextLines *desc;
    unsigned i;

    if (string_equal(&info->id_text, &self->item.id_text)
        && info->refine == self->item.refine && info->slot[0] == self->item.slot[0]
        && info->slot[1] == self->item.slot[1] && info->slot[2] == self->item.slot[2]
        && info->slot[3] == self->item.slot[3] && toggle) {
        ui_window_mgr_post_quit1(&g_windowMgr, self->wnd_id);
        return 0;
    }
    self->pet_level = -1;
    session_is_pet_egg_id(&g_session1, atoi(string_cstr(&info->id_text)));
    item_info_assign(&self->item, info);
    tree_clear(&self->options, option_lines_destroy);
    self->options_height = 0;
    if (self->item.identified == 1)
        collection_build_options(self);
    self->item.expire_time = info->expire_time;
    self->show_cards = self->item.identified == 1 && !item_info_is_pet_range_id(&self->item)
                       && item_info_is_equipment_type(&self->item) == 1;
    self->option_count = self->item.option_count;
    if (self->show_cards) {
        if (wnd->height != 0x90) {
            item_collection_set_height(self, self->options_height + 0x95);
            ui_frame_wnd_snap(wnd);
        }
    } else if (wnd->height != 0x78) {
        item_collection_set_height(self, self->options_height + 0x78);
        ui_frame_wnd_snap(wnd);
    }

    {
        const char *res = item_info_get_res_name(&self->item);
        struct String path;

        string_init(&path);
        string_assign_cstr(&path, UI_SKIN "collection\\");
        string_append_cstr(&path, res);
        string_append_cstr(&path, ".bmp");
        string_assign(&self->picture, string_cstr(&path), path.size);
        string_free(&path);
    }

    desc = item_info_get_description(&self->item);
    collection_place_card_button(self, toggle);
    ui_list_box_clear((struct UIListBox *)self->text);
    if (self->item.expire_time)
        collection_add_expiry(self);
    for (i = 0; i < desc->count; i++)
        collection_add_line(self, desc->data[i], 0);

    if (session_is_pet_egg_id(&g_session1, atoi(string_cstr(&info->id_text))) == 1) {
        static const int egg_states[5] = { 0x2a0, 0x2a1, 0x29d, 0x2a2, 0x2a3 };
        struct String line;
        int state = ITEM_EGG_STATE(info);

        string_init(&line);
        string_format(&line, msg_str(MSG_EGG_LINE),
                      msg_str(state >= 1 && state <= 5 ? egg_states[state - 1] : MSG_EGG_STATE_OTHER));
        collection_add_line(self, string_cstr(&line), 1);
        string_free(&line);
    }
    ui_window_invalidate_base(wnd);
    ui_window_mgr_raise_window(&g_windowMgr, wnd);

    if (self->wnd_id == WND_ITEM_COLLECTION) {
        struct ItemInfo *worn = session_get_worn_counterpart(&g_session1, &self->item);

        if (g_session1.itemCompare && worn && !no_compare) {
            struct UIWindow *cmp = ui_make_window(WND_ITEM_COLLECTION_CMP);

            if (cmp)
                ui_window_send_msg(cmp, NULL, UI_MSG_COLLECTION_ITEM, (int)worn, 1, 0, 0);
        } else {
            ui_window_mgr_post_quit1(&g_windowMgr, WND_ITEM_COLLECTION_CMP);
        }
    }
    collection_fit_text(self);

    if (self->preview_btn) {
        ui_window_remove_child(wnd, self->preview_btn, 1);
        self->preview_btn = NULL;
    }
    if (collection_is_headgear(info->equip_location) && info->identified) {
        BYTE effect_hat = 0;

        struct UIBasicButton *btn;

        client_lua_call(g_session1.luaState, "IsEffectHatItem", "d>b", atoi(string_cstr(&info->id_text)), &effect_hat);
        if (effect_hat == 1)
            return 0;
        btn = mem_alloc(sizeof *btn);
        if (btn)
            btn = ui_basic_button_construct(btn);
        self->preview_btn = &btn->button.base;
        ui_basic_button_set_label(self->preview_btn, 0x4d, msg_str(MSG_PREVIEW_BUTTON));
        ui_window_move_base(self->preview_btn, 9, 0x75);
        self->preview_btn->id = ID_PREVIEW_BUTTON;
        ((struct UIBasicButton *)self->preview_btn)->redraw_parent = 1;
        ui_window_add_child(wnd, self->preview_btn);
        if (wnd->height < 0xac)
            item_collection_set_height(self, 0xac);
        ui_window_invalidate_base(wnd);
    }
    return 0;
}

void item_collection_refresh(struct UIItemCollectionWnd *self)
{
    struct UIWindow *wnd = COLLECTION_WND(self);
    const struct ItemTextLines *desc = item_info_get_description(&self->item);
    struct Vec lines;
    struct String line;
    char number[0x24];
    unsigned i;

    lines.first = NULL;
    lines.last = NULL;
    lines.end = NULL;
    string_init(&line);
    if (self->pet_level != -1) {
        string_assign_cstr(&line, self->pet_name);
        string_vec_push_copy(&lines, &line);
        if (self->pet_renamed)
            string_assign(&line, "이름 변경됨", 0xb);
        else
            string_assign(&line, "이름 변경되지않음",
                              0x11);
        string_vec_push_copy(&lines, &line);
        _itoa(self->pet_level, number, 10);
        string_assign(&line, "레벨 : ", 7);
        string_append_cstr(&line, number);
        string_vec_push_copy(&lines, &line);
        session_get_pet_hunger_text(self->pet_hunger, number);
        string_assign(&line, "만복도 : ", 9);
        string_append_cstr(&line, number);
        string_vec_push_copy(&lines, &line);
        session_get_pet_intimacy_text(self->pet_intimacy, number);
        string_assign(&line, "친밀도 : ", 9);
        string_append_cstr(&line, number);
        string_vec_push_copy(&lines, &line);
        if (self->pet_accessory)
            string_assign(&line, "악세사리 : 장착됨",
                              0x11);
        else
            string_assign(&line, "악세사리 : 장착안됨",
                              0x13);
        string_vec_push_copy(&lines, &line);
    }
    for (i = 0; i < desc->count; i++) {
        struct String text;
        const char *src = desc->data[i];

        string_init(&text);
        string_assign_cstr(&text, src);
        string_vec_push_move(&lines, &text);
        string_free(&text);
    }
    collection_place_card_button(self, 0);
    ui_list_box_clear((struct UIListBox *)self->text);
    for (i = 0; i < VEC_COUNT(&lines, struct String); i++)
        ui_window_send_msg(self->text, NULL, UI_MSG_ADD_TEXT,
                 (int)string_cstr(&VEC_AT(&lines, struct String, i)), TEXT_LAYOUT, 0, 0);
    ui_window_invalidate_base(wnd);
    ui_window_mgr_raise_window(&g_windowMgr, wnd);
    string_free(&line);
    string_vec_free(&lines);
}
