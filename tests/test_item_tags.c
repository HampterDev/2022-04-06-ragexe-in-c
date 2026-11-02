#include "ui/controls/edit.h"
#include "ui/window_access.h"
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ui/items/item_tags_internal.h"

static int fail_node, fail_button;
static unsigned node_live, heap_live, detached, attached, destroyed, placed;
static struct UIWindow *detach_order[8], *place_order[8];
static char place_before[8][256], replacement[256];
static int place_x[8], cancelled;
static struct UIEditCtrl *focus;

static void *node_alloc(size_t bytes)
{
    void *p;
    if (fail_node)
        return NULL;
    p = malloc(bytes);
    assert(p);
    ++node_live;
    return p;
}
static void node_free(void *p)
{
    assert(p && node_live);
    --node_live;
    free(p);
}
#define malloc node_alloc
#define free node_free
#include "../src/ui/items/item_tag_list.c"
#undef malloc
#undef free
#include "ui/window_manager_state.h"
#include "platform/language.h"
#include "core/markup_tags.h"
#include "ui/window.h"

struct UIWindowMgr g_windowMgr;
static struct Language language;
struct Language *g_language = &language;
const struct String text_tag_ITEML = STRING_LITERAL("<ITEML>");
const struct String text_tag_ITEML_END = STRING_LITERAL("</ITEML>");

void *mem_alloc(size_t bytes)
{
    void *p;
    if (fail_button && bytes == sizeof(struct UIItemTagButton))
        return NULL;
    p = malloc(bytes);
    assert(p);
    ++heap_live;
    return p;
}
void mem_free(void *p)
{
    if (p) {
        assert(heap_live);
        --heap_live;
        free(p);
    }
}
void fatal_error(const char *message) { (void)message; abort(); }

const struct UIEditCtrlOps ui_item_tag_on_chat_ops;
const struct UIEditCtrlOps ui_item_tag_on_mega_phone_ops;
void ui_item_tag_ctrl_destruct(struct UIItemTagCtrl *self) { (void)self; abort(); }
void ui_window_add_child(struct UIWindow *self, struct UIWindow *child) { (void)self; (void)child; abort(); }
void ui_window_remove_child(struct UIWindow *self, struct UIWindow *child, BYTE destroy)
{ (void)self; (void)child; (void)destroy; abort(); }
int ui_window_text_width(struct UIWindow *self, const char *text, int len, int face, int height, int a, int b)
{ (void)self; (void)text; (void)len; (void)face; (void)height; (void)a; (void)b; abort(); }

struct ItemInfo *item_info_copy(struct ItemInfo *self, const struct ItemInfo *from)
{ (void)self; (void)from; abort(); }

struct UIWindow *ui_window_mgr_get_focus_edit(struct UIWindowMgr *self)
{ assert(self == &g_windowMgr); return focus ? &focus->base : NULL; }
struct UIWindow *ui_chat_window_edit(struct UIWindow *chat) { (void)chat; abort(); }
struct UIEditCtrl *ui_npc_text_dialog_edit(struct UIWindow *dialog) { (void)dialog; abort(); }
struct UIEditCtrl3 *ui_rodex_compose_body(struct UIWindow *compose) { (void)compose; abort(); }
void language_cancel_input(struct Language *self)
{ assert(self == g_language); ++cancelled; }
int language_replace_selection(struct Language *self, const char *text)
{ assert(self == g_language); strcpy_s(replacement, sizeof replacement, text); return 1; }
void language_set_selection(struct Language *self, int anchor, int caret)
{ assert(self == g_language); self->anchor = anchor; self->caret = caret; }
struct String *ui_window_fit_plain_text(struct UIWindow *self, struct String *out, struct TextView text, int width)
{
    (void)self;
    assert(width >= 0);

    string_assign(out, text.data, text.size < (size_t)width ? text.size : (size_t)width);
    return out;
}
int crt_sprintf(char *buffer, const char *format, ...)
{
    int result;
    va_list args;
    va_start(args, format);
    result = vsprintf(buffer, format, args);
    va_end(args);
    return result;
}
struct String *item_info_get_display_name(const struct ItemInfo *info, struct String *out)
{ string_assign(out, string_cstr(&info->name), info->name.size); return out; }
struct ItemInfo *item_info_assign(struct ItemInfo *to, const struct ItemInfo *from)
{ (void)from; return to; }
unsigned char item_info_is_equipment_type(const struct ItemInfo *info) { return info->type == 4; }
int item_info_get_slot_count(const struct ItemInfo *info) { (void)info; return 0; }

static void button_destroy(struct UIWindow *button)
{
    ++destroyed;
    mem_free(button);
}
void ui_window_move_base(struct UIWindow *button, int x, int y)
{ button->x = x; button->y = y; }
static struct UIWindowOps button_ops = { .destroy = button_destroy, .move = ui_window_move_base };
struct UIItemTagButton *ui_item_tag_button_construct(struct UIItemTagButton *self)
{ memset(self, 0, sizeof *self); self->text.button.base.ops = &button_ops; return self; }
void ui_text_button_set_text(struct UIWindow *self, const char *text, DWORD colour,
                         int face, int height, BYTE outlined, BYTE underlined)
{ assert(self && text && colour == 0x123456 && !face && height == 12 && !outlined && !underlined); }
void ui_bitmap_button_set_tool_tip(struct UIBitmapButton *self, const char *text)
{ assert(self && text); }

static void detach_tag(struct UIItemTagCtrl *self, struct UIWindow *button)
{
    assert(self->tags.size && detached < 8);
    detach_order[detached++] = button;
}
static void attach_tag(struct UIItemTagCtrl *self, struct UIWindow *button)
{
    assert(self->tags.size && self->tags.head->tag.button == button);
    ++attached;
}
static void place_tag(struct UIItemTagCtrl *self, struct UIWindow *button, struct TextView before, int x)
{
    assert(self && placed < 8);
    place_order[placed] = button;
    place_x[placed] = x;
    strcpy_s(place_before[placed], sizeof place_before[placed], before.data);
    ++placed;
}
static const struct ItemTagActions actions = { NULL, attach_tag, detach_tag, place_tag };
static void init_control(struct UIItemTagCtrl *ctrl, const char *text)
{
    memset(ctrl, 0, sizeof *ctrl);
    string_init(&ctrl->text);
    string_init(&ctrl->linked_text);
    string_assign_cstr(&ctrl->text, text);
    ctrl->width = 400;
    ctrl->actions = &actions;
    detached = attached = placed = destroyed = 0;
    memset(&language, 0, sizeof language);
    cancelled = 0; replacement[0] = 0; focus = NULL;
}
static void finish_control(struct UIItemTagCtrl *ctrl)
{
    ui_item_tag_ctrl_clear_tags(ctrl);
    ui_item_tag_ctrl_clear_tags(ctrl);
    string_free(&ctrl->text);
    string_free(&ctrl->linked_text);
    assert(!node_live && !heap_live);
}
static struct ItemTagNode *add_tag(struct UIItemTagCtrl *ctrl, const char *name,
                                 const char *link, struct UIWindow *button)
{
    struct ItemTagPayload tag = {0};
    tag.button = button;
    strcpy_s(tag.name, sizeof tag.name, name);
    strcpy_s(tag.link, sizeof tag.link, link);
    return item_tag_list_append(&ctrl->tags, &tag);
}
static void test_update(void)
{
    struct UIItemTagCtrl ctrl;
    struct UIWindow a = {0}, b = {0}, unused = {0};
    struct ItemTagNode *first, *second;
    struct UIEditCtrl edit = {0};
    init_control(&ctrl, "x<한국> y<Sword>!");
    second = add_tag(&ctrl, "Sword", "sword-link", &a);
    add_tag(&ctrl, "Gone", "unused-link", &unused);
    first = add_tag(&ctrl, "한국", "korean-link", &b);
    fail_node = 1;
    ui_item_tag_ctrl_update(&ctrl);
    fail_node = 0;
    assert(ctrl.tags.head == first && first->next == second && !second->next);
    assert(ctrl.tags.size == 2 && ctrl.ordered_count == 2);
    assert(first->tag.order == 0 && second->tag.order == 1);
    assert(detached == 1 && detach_order[0] == &unused);
    assert(placed == 2 && place_order[0] == &b && place_order[1] == &a);
    assert(!strcmp(place_before[0], "x") && !strcmp(place_before[1], " y"));
    assert(!strcmp(string_cstr(&ctrl.linked_text), "x<ITEML>korean-link</ITEML> y<ITEML>sword-link</ITEML>!"));
    edit.base.id = 0x1e2; edit.scroll = 1; focus = &edit;
    language.caret = 2; placed = 0;
    ui_item_tag_ctrl_update(&ctrl);
    assert(place_x[0] == 1000 && language.caret == (int)strlen("x<한국>"));
    finish_control(&ctrl);
    assert(detached == 3 && detach_order[1] == &b && detach_order[2] == &a);

    init_control(&ctrl, "<Same><Same>");
    first = add_tag(&ctrl, "Same", "first", &a);
    second = add_tag(&ctrl, "Same", "second", &b);
    ui_item_tag_ctrl_update(&ctrl);
    assert(ctrl.tags.head == first && first->next == second);
    assert(!strcmp(string_cstr(&ctrl.linked_text), "<ITEML>first</ITEML><ITEML>second</ITEML>"));
    finish_control(&ctrl);

    init_control(&ctrl, "prefix <unfinished");
    add_tag(&ctrl, "Sword", "unused", &a);
    ui_item_tag_ctrl_update(&ctrl);
    assert(!ctrl.tags.size && !strcmp(string_cstr(&ctrl.linked_text), "prefix <unfinished"));
    finish_control(&ctrl);
}
static void test_deleted_bracket(void)
{
    struct UIItemTagCtrl ctrl;
    struct UIWindow button = {0};
    init_control(&ctrl, "hi <Sword!");
    add_tag(&ctrl, "Sword", "sword-link", &button);
    ctrl.ordered_count = 1; language.caret = 9;
    ui_item_tag_ctrl_update(&ctrl);
    assert(!ctrl.tags.size && detached == 1 && cancelled == 1 && !strcmp(replacement, "hi !"));
    finish_control(&ctrl);
    init_control(&ctrl, "hi Sword>!");
    add_tag(&ctrl, "Sword", "sword-link", &button);
    ctrl.ordered_count = 1; language.caret = 3;
    ui_item_tag_ctrl_update(&ctrl);
    assert(!ctrl.tags.size && detached == 1 && cancelled == 1 && !strcmp(replacement, "hi !"));
    finish_control(&ctrl);
}
static void test_add_failure(void)
{
    struct UIItemTagCtrl ctrl;
    struct ItemInfo item = {0};
    struct UIWindow *button;
    unsigned item_heap;
    init_control(&ctrl, "");
    string_init(&item.id_text); string_init(&item.name);
    string_assign_cstr(&item.id_text, "62"); string_assign_cstr(&item.name, "한국 Sword");
    item_heap = heap_live;
    item.type = 4; item.equip_location = 1; item.refine = 2; item.view = 3;
    item.slot[0] = 5; item.option_count = 1;
    item.options[0].id = 6; item.options[0].param = 7; item.options[0].value = 8;
    fail_button = 1;
    ui_item_tag_ctrl_add_item(&ctrl, &item, 0x123456);
    fail_button = 0;
    assert(!ctrl.tags.size && !attached && !destroyed && heap_live == item_heap);
    fail_node = 1;
    ui_item_tag_ctrl_add_item(&ctrl, &item, 0x123456);
    fail_node = 0;
    assert(!ctrl.tags.size && !attached && destroyed == 1 && heap_live == item_heap);
    ui_item_tag_ctrl_add_item(&ctrl, &item, 0x123456);
    assert(ctrl.tags.size == 1 && attached == 1 && heap_live == item_heap + 1);
    assert(!strcmp(ctrl.tags.head->tag.name, "한국 Sword"));
    assert(!strcmp(ctrl.tags.head->tag.code, "10"));
    assert(!strcmp(ctrl.tags.head->tag.link, "00001110%02&03'00)05)00)00)00+06,07-08"));
    button = ctrl.tags.head->tag.button;
    ui_item_tag_ctrl_clear_tags(&ctrl);
    assert(detached == 1 && detach_order[0] == button && destroyed == 1);
    button_destroy(button);
    string_free(&item.name); string_free(&item.id_text);
    finish_control(&ctrl);
}

static void test_counted_text(void)
{
    struct UIItemTagCtrl ctrl;
    struct String out = {0};
    struct UIEditCtrl edit = {0};
    struct UIWindow button = {0};
    const char name[] = "한국x";
    unsigned i;

    init_control(&ctrl, "prefix");
    string_assign_cstr(&out, "old result with storage");
    ui_item_tag_ctrl_append_tag(&ctrl, &out, text_view(name, strlen(name) - 1));
    assert(!strcmp(string_cstr(&out), "prefix<한국>"));
    ui_item_tag_ctrl_set_text(&ctrl, text_view(string_cstr(&out) + 6, out.size - 6), 400);
    assert(!strcmp(string_cstr(&ctrl.text), "<한국>"));
    ui_item_tag_ctrl_append_tag(&ctrl, &ctrl.text, string_view(&ctrl.text));
    assert(!strcmp(string_cstr(&ctrl.text), "<한국><<한국>>"));
    edit.base.id = 0x1e2; edit.max_chars = 8; focus = &edit;
    string_assign_cstr(&edit.text, "abcd");
    assert(ui_item_tag_on_chat_can_tag(&ctrl, text_view("xy!", 2), 400));
    assert(!strcmp(string_cstr(&ctrl.text), "abcd<xy>"));
    --edit.max_chars;
    assert(!ui_item_tag_on_chat_can_tag(&ctrl, text_view("xy!", 2), 400));
    string_free(&edit.text); string_free(&out);
    finish_control(&ctrl);

    init_control(&ctrl, "Sword xx Sword yy <Sword!");
    add_tag(&ctrl, "Sword", "link", &button);
    ctrl.ordered_count = 1; language.caret = (int)strlen("Sword xx Sword yy <Sword");
    ui_item_tag_ctrl_update(&ctrl);
    assert(!ctrl.tags.size && cancelled == 1 && !strcmp(replacement, "Sword xx Sword yy !"));
    finish_control(&ctrl);

    init_control(&ctrl, "<Sword!");
    add_tag(&ctrl, "Sword", "link", &button);
    ctrl.ordered_count = 1;
    for (i = 0; i < 100; ++i) {
        ui_item_tag_ctrl_update(&ctrl);
        assert(ctrl.tags.size == 1 && !cancelled && !strcmp(string_cstr(&ctrl.linked_text), "<Sword!"));
    }
    finish_control(&ctrl);

    init_control(&ctrl, "text");
    add_tag(&ctrl, "", "invalid empty name", &button); ctrl.ordered_count = 1;
    ui_item_tag_ctrl_update(&ctrl);
    assert(!ctrl.tags.size && detached == 1);
    finish_control(&ctrl);
}
int main(void)
{
    test_update();
    test_deleted_bracket();
    test_add_failure();
    test_counted_text();
    puts("Item tags: production update/clear/add, Korean links, order, caret, deletion and allocation failure passed.");
    return 0;
}
