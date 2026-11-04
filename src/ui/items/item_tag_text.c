#include "ui/window_access.h"
#include "ui/items/item_tags_internal.h"
#include "ui/window_manager_state.h"
#include "core/markup_tags.h"
#include "ui/items/items.h"
#include "ui/ui_std.h"
#include <string.h>
#include <stdlib.h>

#include "platform/clientinfo.h"
#include "ui/mail/rodex_compose.h"
#include "core/cstr.h"

#define TAG_FONT_HEIGHT 0xc
#define TAG_BUTTON_ID 0x1c8
#define TAG_OFF_SIDE 1000
#define EDIT_ID_FIRST 0x1e2
#define EDIT_ID_NO_SCROLL 0x1e3
#define EDIT_ID_LAST 0x1e6

static void fit_name(struct UIItemTagCtrl *self, struct String *out, const char *name)
{
    ui_window_fit_plain_text(&self->edit.base, out, text_view_cstr(name), self->width / 2);
}

static void bracket_fit(struct UIItemTagCtrl *self, struct String *out, struct TextView name)
{
    struct String fit = {0};
    ui_window_fit_plain_text(&self->edit.base, &fit, name, self->width / 2);
    string_assign_cstr(out, "<");
    string_append(out, string_cstr(&fit), fit.size);
    string_append_cstr(out, ">");
    string_free(&fit);
}

static struct String *to_base62(struct String *out, int value)
{
    static const char digits[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    char rev[16];
    int n = 0;

    do {
        rev[n++] = digits[value % 62];
        value /= 62;
    } while (value >= 62);
    rev[n++] = digits[value];
    string_clear(out);
    while (n)
        string_append_char(out, rev[--n]);
    return out;
}

static void append_base62(struct String *s, int value)
{
    struct String digits = {0};

    to_base62(&digits, value);
    string_append(s, string_cstr(&digits), digits.size);
    string_free(&digits);
}

struct String *ui_item_tag_ctrl_append_tag(struct UIItemTagCtrl *self, struct String *out,
                                                     struct TextView name)
{
    struct String tag = {0};

    bracket_fit(self, &tag, name);
    string_append(&self->text, string_cstr(&tag), tag.size);
    string_assign(out, string_cstr(&self->text), self->text.size);
    string_free(&tag);
    return out;
}

void ui_item_tag_ctrl_set_text(struct UIItemTagCtrl *self, struct TextView text, int width)
{
    string_assign(&self->text, text.data, text.size);
    self->width = width;
}

void ui_item_tag_ctrl_add_item(struct UIItemTagCtrl *self, const struct ItemInfo *info, int colour)
{
    static const char sep[14] = "!#$%&'()*+,-/";
    struct String tip = {0}, label = {0}, code = {0}, digits = {0};
    struct String name = {0};
    struct UIItemTagButton *btn;
    struct ItemTagPayload tag = {0};
    char eq[8] = { 0 };
    char id_code[0x40];
    int cards, k;
    unsigned i;

    string_init(&tip);
    string_assign_cstr(&tip, "<");
    item_info_get_display_name(info, &name);
    string_append(&tip, string_cstr(&name), name.size);
    string_free(&name);
    string_append_cstr(&tip, ">");
    btn = mem_alloc(sizeof *btn);
    if (!btn) {
        string_free(&tip);
        return;
    }
    btn = ui_item_tag_button_construct(btn);
    item_info_get_display_name(info, &name);
    bracket_fit(self, &label, string_view(&name));
    string_free(&name);
    ui_text_button_set_text(&btn->text.button.base, string_cstr(&label), colour, 0, TAG_FONT_HEIGHT, 0, 0);
    ui_bitmap_button_set_tool_tip((struct UIBitmapButton *)btn, string_cstr(&tip));
    btn->text.button.base.id = TAG_BUTTON_ID;
    ui_window_move_base(&btn->text.button.base, 0, 0);
    item_info_assign(&btn->item, info);

    to_base62(&digits, info->equip_location);
    for (i = digits.size; i < 5; ++i) string_append_char(&code, '0');
    string_append(&code, string_cstr(&digits), digits.size);
    string_clear(&digits);
    sprintf(eq, "%d", item_info_is_equipment_type(info));
    string_append_cstr(&code, eq);
    append_base62(&code, atoi(string_cstr(&info->id_text)));
    if (info->refine) {
        string_append_char(&code, sep[3]);
        append_base62(&code, info->refine);
    }
    string_append_char(&code, sep[4]);
    append_base62(&code, info->view);
    string_append_char(&code, sep[5]);
    append_base62(&code, 0); /* grade field kept in the link format; items have no grade */
    cards = item_info_get_slot_count(info);
    if (cards == 0)
        cards = 4;
    for (k = 0; k < 4; k++)
        if (info->slot[k])
            cards = 4;
    for (k = 0; k < cards; k++) {
        string_append_char(&code, sep[7]);
        append_base62(&code, k < 4 ? info->slot[k] : 0);
    }
    for (k = 0; k < info->option_count; k++) {
        const struct ItemOption *opt = &info->options[k];

        string_append_char(&code, sep[9]);
        append_base62(&code, (short)opt->id);
        string_append_char(&code, sep[10]);
        append_base62(&code, opt->param);
        string_append_char(&code, sep[11]);
        append_base62(&code, opt->value);
    }

    memset(id_code, 0, sizeof id_code);
    tag.button = &btn->text.button.base;
    to_base62(&digits, atoi(string_cstr(&info->id_text)));
    cstr_copy(id_code, sizeof id_code, string_cstr(&digits));
    cstr_copy(tag.code, sizeof tag.code, id_code);
    cstr_copy(tag.name, sizeof tag.name, string_cstr(item_info_get_display_name(info, &name)));
    string_free(&name);
    cstr_copy(tag.link, sizeof tag.link, string_cstr(&code));
    if (item_tag_list_append(&self->tags, &tag))
        self->actions->attach(self, tag.button);
    else
        ui_window_destroy(tag.button);
    string_free(&digits);
    string_free(&code);
    string_free(&label);
    string_free(&tip);
}

static BYTE reorder_tags(struct UIItemTagCtrl *self)
{
    struct String text = {0};
    struct ItemTagList ordered = {0};

    string_assign(&text, string_cstr(&self->text), self->text.size);
    while (string_find(&text, "<", 0) != STRING_NPOS) {
        struct String t = {0};

        string_substring(&t, &text, string_find(&text, "<", 0), STRING_NPOS);
        if (string_find(&t, ">", 0) != STRING_NPOS) {
            struct ItemTagNode *node;

            string_substring(&text, &text, string_find(&text, ">", 0) + 1, STRING_NPOS);
            string_truncate(&t, string_find(&t, ">", 0) + 1);
            for (node = self->tags.head; node; node = node->next) {
                struct String fit = {0};

                fit_name(self, &fit, node->tag.name);
                if (text_view_find(string_view(&t), string_view(&fit), 0) != STRING_NPOS) {
                    node->tag.order = (int)ordered.size;
                    item_tag_list_move(&ordered, &self->tags, node);
                    string_free(&fit);
                    break;
                }
                string_free(&fit);
            }
        } else {
            string_substring(&text, &text, string_find(&text, "<", 0) + 1, STRING_NPOS);
        }
        string_free(&t);
    }
    ui_item_tag_ctrl_clear_tags(self);
    self->tags = ordered;
    string_free(&text);
    return 1;
}

static int caret_in_this_text(void)
{
    return !g_windowMgr.rodex_compose_wnd || (struct UIWindow *)ui_rodex_compose_body(g_windowMgr.rodex_compose_wnd) == ui_window_mgr_get_focus_edit(&g_windowMgr);
}

static BYTE check_tag(struct UIItemTagCtrl *self, struct ItemTagNode **it)
{
    struct ItemTagNode *node = *it;
    struct String fit = {0};
    struct TextView text = string_view(&self->text);
    size_t pos, length;
    int found = 0, saw_name = 0;

    fit_name(self, &fit, node->tag.name);
    length = strlen(string_cstr(&fit));
    if (length && fit.size <= text.size) {
        for (pos = 0; pos <= text.size - fit.size;) {
            int open, close, snap;
            size_t end;
            if (memcmp(text.data + pos, string_cstr(&fit), fit.size)) {
                ++pos;
                continue;
            }
            saw_name = 1;
            end = pos + length;
            open = pos > 0 && text.data[pos - 1] == '<';
            close = end < text.size && text.data[end] == '>';
            if (open && close && ++found > node->tag.order) break;
            snap = open != close && caret_in_this_text() && g_language->caret >= 0 &&
                   (size_t)g_language->caret == (open ? end : pos);
            if (snap) {
                struct String replacement = {0};
                string_assign(&replacement, text.data, pos - open);
                string_append(&replacement, text.data + end + close, text.size - end - close);
                language_cancel_input(g_language);
                language_replace_selection(g_language, string_cstr(&replacement));
                string_free(&replacement);
                saw_name = 0;
                break;
            }
            if ((open || close) && !node->tag.order) break;
            pos = end;
        }
    }
    string_free(&fit);
    if (saw_name) return 0;
    self->actions->detach(self, node->tag.button);
    *it = item_tag_list_erase(&self->tags, node);
    return 1;
}

void ui_item_tag_ctrl_update(struct UIItemTagCtrl *self)
{
    struct String before = {0}, rest = {0};
    struct ItemTagNode *it;

    string_clear(&self->linked_text);
    if (self->ordered_count != self->tags.size && reorder_tags(self))
        self->ordered_count = self->tags.size;
    string_assign(&rest, string_cstr(&self->text), self->text.size);
    if ((it = self->tags.head) != NULL) {
        do {
            struct ItemTagNode *node;
            struct String fit = {0}, label = {0}, pre = {0}, cur = {0};
            struct UIWindow *edit;
            unsigned scroll = 0;
            size_t p;
            int len, x;

            if (check_tag(self, &it))
                break;
            node = it;
            fit_name(self, &fit, node->tag.name);
            string_init(&label);
            string_assign_cstr(&label, "<");
            string_append(&label, string_cstr(&fit), fit.size);
            string_append_cstr(&label, ">");
            p = text_view_find(string_view(&rest), string_view(&label), 0);
            if (p == STRING_NPOS) {
                it = node->next;
                string_free(&label);
                string_free(&fit);
                continue;
            }
            string_substring(&pre, &rest, 0, p);
            string_append(&before, string_cstr(&pre), pre.size);
            string_assign(&cur, string_cstr(&pre), pre.size);
            edit = ui_window_mgr_get_focus_edit(&g_windowMgr);
            x = 0;
            if (edit) {
                int id = edit->id;

                if (id >= EDIT_ID_FIRST && id <= EDIT_ID_LAST && id != EDIT_ID_NO_SCROLL)
                    scroll = ((struct UIEditCtrl *)edit)->scroll;
                if (before.size >= scroll)
                    string_substring(&cur, &before, scroll, STRING_NPOS);
                if (scroll && before.size <= scroll)
                    x = TAG_OFF_SIDE;
            }
            self->actions->place(self, node->tag.button, string_view(&cur), x);
            len = (int)strlen(string_cstr(&fit));
            if (caret_in_this_text()) {
                int lo = (int)before.size, hi = lo + 2 + len, caret = g_language->caret;

                if (caret > lo && caret < hi) {
                    if (len / 2 + 1 + lo > caret)
                        language_set_selection(g_language, hi, hi);
                    else
                        language_set_selection(g_language, lo, lo);
                }
            }
            string_append(&self->linked_text, string_cstr(&pre), pre.size);
            string_append_cstr(&self->linked_text, "<ITEML>");
            string_append_cstr(&self->linked_text, node->tag.link);
            string_append_cstr(&self->linked_text, "</ITEML>");
            string_substring(&rest, &rest, label.size + p, STRING_NPOS);
            string_append(&before, string_cstr(&label), label.size);
            it = node->next;
            string_free(&cur);
            string_free(&pre);
            string_free(&label);
            string_free(&fit);
        } while (it != NULL);
    }
    string_append(&self->linked_text, string_cstr(&rest), rest.size);
    string_free(&rest);
    string_free(&before);
}
