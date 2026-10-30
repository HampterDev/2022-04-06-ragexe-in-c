#include "ui/chat/chat_history.h"
#include "ui/chat/readable_links.h"
#include "contents/player_names.h"
#include "iteminfo/item_api.h"
#include "session/session_party.h"
#include "ui/items/item_tags.h"
#include "ui/ui_items.h"
#include "ui/ui_std.h"
#include "core/memory.h"
#include <stdint.h>
#include <string.h>
#include "core/msg_str.h"

#define TAG_FONT_HEIGHT 12
#define TAG_COLOUR 0x82f2e1
#define TAG_BUTTON_ID 0x1c8
#define MSG_NAME_PENDING 0x245

static int text_width(struct UIChatHisBox *self, const char *text)
{
    return ui_window_text_width(&self->list.base, text, 0, 0, TAG_FONT_HEIGHT, 0, 0);
}

struct ChatItemLine {
    struct UIChatHisBox *box;
    struct Vec tags;
};

static void add_item_tag(void *context, const struct ItemInfo *item,
                         const char *name, const char *caption, const char *prefix)
{
    struct ChatItemLine *line = context;
    struct UIChatHisBox *self = line->box;
    struct UIItemTagButton *tag = mem_try_alloc(sizeof *tag);
    void *value;

    if (!tag) return;
    tag = ui_item_tag_button_construct(tag);
    item_info_assign(&tag->item, item);
    ui_text_button_set_text(&tag->text.button.base, caption, TAG_COLOUR, 0, TAG_FONT_HEIGHT, 0, 0);
    ui_bitmap_button_set_tool_tip((struct UIBitmapButton *)tag, name);
    tag->text.hover_colour = TAG_COLOUR;
    tag->text.button.base.id = TAG_BUTTON_ID;
    ui_text_button_fit_width(&tag->text, text_width(self, caption) + 1, tag->text.button.base.height);
    if (strstr(caption, msg_str(MSG_NAME_PENDING))) {
        tag->aid = (int)(((unsigned)item->slot[3] << 16) + (unsigned)item->slot[2]);
        session_add_pending_name(&g_session1, tag->aid);
    }
    ui_window_add_child(&self->list.base, &tag->text.button.base);
    value = tag;
    vec_push(&line->tags, &value, sizeof(void *));
    tag->line = (int)VEC_COUNT(&self->list.items, struct String);
    tag->line_x = text_width(self, prefix) - self->list.scroll_x;
}

struct String *ui_chat_his_box_build_item_line(struct UIChatHisBox *self, struct String *out,
                                            const char *line, struct String *rest)
{
    struct ChatItemLine context = {0};
    struct Vec tags;
    context.box = self;
    if (!item_link_build_line(&self->list.base, out, rest, line, add_item_tag, &context)) return out;
    vec_copy(&tags, &context.tags, sizeof(void *));
    list_push_back(&self->item_tags, &tags, sizeof tags);
    vec_free(&context.tags);
    return out;
}

void ui_chat_his_box_resolve_pending_name(struct UIChatHisBox *self, const char *text)
{
    struct ListNode *node;
    struct String line_text = {0}, display = {0}, shown = {0}, prefix = {0}, replaced = {0};
    const char *pending = msg_str(MSG_NAME_PENDING);
    size_t pending_size = strlen(pending);

    for (node = self->item_tags.head->next; node != self->item_tags.head; node = node->next) {
        struct Vec *windows = LIST_VALUE(node, struct Vec);
        size_t count = VEC_COUNT(windows, struct UIItemTagButton *), scan = 0, i;
        for (i = 0; i < count; ++i) {
            struct UIItemTagButton *tag = ((struct UIItemTagButton **)windows->first)[i];
            struct String *line;
            size_t at;
            if (tag->line < 0 || (size_t)tag->line >= VEC_COUNT(&self->list.items, struct String)) continue;
            line = (struct String *)self->list.items.first + tag->line;
            string_assign(&line_text, string_cstr(line), line->size);
            if (strstr(string_cstr(&tag->text.button.text), pending)) {
                struct TextView name = player_name_lookup(tag->aid);
                if (strcmp(name.data, text)) continue;
                at = text_view_find(string_view(&line_text), text_view(pending, pending_size), 0);
                if (at == SIZE_MAX) continue;
                string_assign(&replaced, string_cstr(&line_text), at);
                string_append_cstr(&replaced, text);
                string_append(&replaced, string_cstr(&line_text) + at + pending_size, line_text.size - at - pending_size);
                string_assign(&line_text, string_cstr(&replaced), replaced.size);
                string_assign_cstr(&prefix, "<");
                string_append_cstr(&prefix, text);
                at = text_view_find(string_view(&line_text), string_view(&prefix), scan);
                if (at != SIZE_MAX) {
                    scan = at + prefix.size + 1;
                    if (scan > line_text.size) scan = line_text.size;
                    string_assign(&prefix, string_cstr(&line_text), at);
                    tag->line_x = text_width(self, string_cstr(&prefix)) - self->list.scroll_x;
                }
                item_info_get_display_name(&tag->item, &display);
                item_link_caption(&self->list.base, &shown, string_view(&display));
                ui_bitmap_button_set_tool_tip((struct UIBitmapButton *)tag, string_cstr(&shown));
                string_assign(&tag->text.button.text, string_cstr(&shown), shown.size);
                ui_text_button_fit_width(&tag->text, text_width(self, string_cstr(&shown)) + 1, tag->text.button.base.height);
                string_assign(line, string_cstr(&line_text), line_text.size);
            } else {
                item_info_get_display_name(&tag->item, &display);
                item_link_caption(&self->list.base, &shown, string_view(&display));
                at = text_view_find(string_view(&line_text), string_view(&shown), scan);
                if (at == SIZE_MAX) continue;
                scan = at + shown.size;
                string_assign(&prefix, string_cstr(&line_text), at);
                tag->line_x = text_width(self, string_cstr(&prefix)) - self->list.scroll_x;
            }
        }
    }
    string_free(&replaced);
    string_free(&prefix);
    string_free(&shown);
    string_free(&display);
    string_free(&line_text);
    ui_chat_his_box_rebuild(self);
}
