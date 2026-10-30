#include "ui/controls/edit.h"
#include "ui/window_access.h"
#include "ui/items/item_tags_internal.h"
#include "ui/window_list.h"
#include "ui/window_manager_state.h"
#include "ui/window_mgr.h"
#include "ui/game.h"
#include "ui/ui_items.h"
#include "ui/controls/controls.h"
#include "ui/ui_std.h"
#include <mmsystem.h>
#include <stdlib.h>
#include <string.h>
#include "ui/chat/chat_window_parts.h"
#include "ui/quests/npc_input.h"
#include "core/object_type_tables.h"

#define TAG_OPEN "<"
#define TAG_CLOSE ">"
#define TAG_FONT_HEIGHT 0xc

#define TEXT_DIALOG_EDIT_ID 0x1e4
#define CHAT_EDIT_ID_A 0x1e2
#define CHAT_EDIT_ID_B 0x1e6
#define MAX_CHAT_TAGS 3
#define OFF_SCREEN 1000

void ui_item_tag_ctrl_clear_tags(struct UIItemTagCtrl *self)
{
    while (self->tags.head) {
        struct ItemTagNode *node = self->tags.head;
        self->actions->detach(self, node->tag.button);
        item_tag_list_erase(&self->tags, node);
    }
}

static void make_tag(struct UIItemTagCtrl *self, struct String *tag, struct TextView text, int width)
{
    struct String cut = {0};

    string_init(tag);
    string_assign(tag, TAG_OPEN, 1);
    self->width = width;
    ui_window_fit_plain_text(&self->edit.base, &cut, text, width / 2);
    string_append(tag, string_cstr(&cut), cut.size);
    string_free(&cut);
    string_append(tag, TAG_CLOSE, 1);
}

static BYTE tag_fits(struct UIItemTagCtrl *self, struct UIEditCtrl *edit, const struct String *tag,
                     unsigned max_chars)
{
    BYTE fits;

    string_assign(&self->text, string_cstr(&edit->text), edit->text.size);
    string_append(&self->text, string_cstr(tag), tag->size);
    fits = self->text.size <= max_chars;
    return fits;
}

static int tag_text_width(struct UIWindow *wnd, const struct String *text)
{
    return ui_window_text_width(wnd, string_cstr(text), 0, 0, TAG_FONT_HEIGHT, 0, 0);
}

void ui_item_tag_on_chat_destroy(struct UIWindow *window)
{
    struct UIItemTagCtrl *self = (struct UIItemTagCtrl *)window;
    self->edit.base.ops = &ui_item_tag_on_chat_ops.base;
    ui_item_tag_ctrl_destruct(self);
    mem_free(self);
}

BYTE ui_item_tag_on_chat_can_tag(struct UIItemTagCtrl *self, struct TextView text, int width)
{
    struct UIEditCtrl *edit;
    struct String tag = {0};
    BYTE fits;

    if (ui_window_mgr_get_focus_edit(&g_windowMgr)->id != CHAT_EDIT_ID_A
        && ui_window_mgr_get_focus_edit(&g_windowMgr)->id != CHAT_EDIT_ID_B && !g_windowMgr.chat_bar_shown) {
        ui_item_tag_ctrl_clear_tags(self);
        return 0;
    }
    if (self->tags.size >= MAX_CHAT_TAGS)
        return 0;
    make_tag(self, &tag, text, width);
    edit = (struct UIEditCtrl *)ui_window_mgr_get_focus_edit(&g_windowMgr);
    fits = tag_fits(self, edit, &tag, (unsigned)edit->max_chars);
    string_free(&tag);
    return fits;
}

void ui_item_tag_on_chat_attach_tag(struct UIItemTagCtrl *self, struct UIWindow *button)
{
    struct UIWindow *edit = ui_window_mgr_get_focus_edit(&g_windowMgr);

    if (edit)
        ui_window_add_child(edit, button);
}

static int has_child(const struct UIWindow *parent, const struct UIWindow *wnd)
{
    struct ListNode *node;

    for (node = parent->children.head->next; node != parent->children.head; node = node->next)
        if (*LIST_VALUE(node, struct UIWindow *) == wnd)
            return 1;
    return 0;
}

void ui_item_tag_on_chat_detach_tag(struct UIItemTagCtrl *self, struct UIWindow *button)
{
    struct UIWindow *edit;

    if (g_windowMgr.chat_bar_shown == 1 && ui_chat_window_edit(g_windowMgr.new_chat_wnd)) {
        if (has_child(ui_chat_window_edit(g_windowMgr.new_chat_wnd), button)) {
            ui_window_remove_child(ui_chat_window_edit(g_windowMgr.new_chat_wnd), button, 1);
            return;
        }
    }
    edit = ui_window_mgr_get_focus_edit(&g_windowMgr);
    if (!edit)
        return;
    if (has_child(edit, button))
        ui_window_remove_child(edit, button, 1);
}

void ui_item_tag_on_chat_place_tag(struct UIItemTagCtrl *self, struct UIWindow *button,
                                         struct TextView before, int x)
{
    struct UIEditCtrl *edit = (struct UIEditCtrl *)ui_window_mgr_get_focus_edit(&g_windowMgr);
    struct String text = {0};
    int y;

    if (!edit)
        return;
    y = edit->text_y - 1;
    if (x) {
        ui_window_move(button, x, y);
        return;
    }
    string_init(&text);
    string_assign(&text, before.data, before.size);
    ui_window_move(button, tag_text_width(&edit->base, &text) + 2, y);
    string_free(&text);
}

void ui_item_tag_on_mega_phone_destroy(struct UIWindow *window)
{
    struct UIItemTagOnMegaPhone *self = (struct UIItemTagOnMegaPhone *)window;
    self->ctrl.edit.base.ops = &ui_item_tag_on_mega_phone_ops.base;
    ui_item_tag_ctrl_clear_tags(&self->ctrl);
    ui_item_tag_ctrl_destruct(&self->ctrl);
    mem_free(self);
}

int ui_mega_phone_tag_width(struct UIWindow *self, const struct String *text,
                                    const int *len)
{
    return ui_window_text_width(self, string_cstr(text), *len, 0, TAG_FONT_HEIGHT, 0, 0);
}

BYTE ui_item_tag_on_mega_phone_can_tag(struct UIItemTagCtrl *ctrl, struct TextView text, int width)
{
    struct UIItemTagOnMegaPhone *self = (struct UIItemTagOnMegaPhone *)ctrl;
    struct UIEditCtrl *edit;
    struct String tag = {0};
    unsigned max_chars;
    int all = 0;
    BYTE fits;

    if (self->ctrl.tags.size > MAX_CHAT_TAGS - 1)
        return 0;
    make_tag(&self->ctrl, &tag, text, width);
    self->tag_width = ui_mega_phone_tag_width(g_windowMgr.npc_text_edit_dialog_wnd, &tag, &all);
    edit = ui_npc_text_dialog_edit(g_windowMgr.npc_text_edit_dialog_wnd);
    max_chars = (unsigned)edit->max_chars;
    fits = tag_fits(&self->ctrl, edit, &tag, max_chars);
    string_free(&tag);
    return fits;
}

void ui_item_tag_on_mega_phone_attach_tag(struct UIItemTagCtrl *ctrl, struct UIWindow *button)
{
    if (g_windowMgr.npc_text_edit_dialog_wnd)
        ui_window_add_child(g_windowMgr.npc_text_edit_dialog_wnd, button);
}

void ui_item_tag_on_mega_phone_detach_tag(struct UIItemTagCtrl *ctrl, struct UIWindow *button)
{
    if (g_windowMgr.npc_text_edit_dialog_wnd)
        ui_window_remove_child(g_windowMgr.npc_text_edit_dialog_wnd, button, 1);
}

void ui_item_tag_on_mega_phone_place_tag(struct UIItemTagCtrl *ctrl, struct UIWindow *button,
                                              struct TextView before, int x)
{
    struct UIItemTagOnMegaPhone *self = (struct UIItemTagOnMegaPhone *)ctrl;
    struct UIEditCtrl *edit;
    struct String text = {0};
    int edit_x, edit_y, at, width, all = 0;

    if (!g_windowMgr.npc_text_edit_dialog_wnd)
        return;
    edit = ui_npc_text_dialog_edit(g_windowMgr.npc_text_edit_dialog_wnd);
    if (edit->base.id != TEXT_DIALOG_EDIT_ID)
        return;
    edit_x = edit->base.x;
    at = x;
    edit_y = edit->base.y;
    string_init(&text);
    string_assign(&text, before.data, before.size);
    width = ui_mega_phone_tag_width(g_windowMgr.npc_text_edit_dialog_wnd, &text, &all);
    string_free(&text);
    if (self->tag_width + width > ui_npc_text_dialog_edit(g_windowMgr.npc_text_edit_dialog_wnd)->base.width)
        ui_window_move(button, OFF_SCREEN, edit_y + 1);
    else if (at)
        ui_window_move(button, at, edit_y + 1);
    else
        ui_window_move(button, width + edit_x + 2, edit_y + 1);
}
